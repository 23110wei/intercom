/***************************************************
 * key2_wav_record.c
 *
 * 1）按键触发的在线录音：R_RECORD_AUDIO 流 → WAV 文件
 * 2）WeChat 语音缓存：g_pcm_record_buf[][] → WAV 文件
 *
 * 说明：
 *  - WAV 文件目录统一为 0:/DCIM
 *  - 文件名统一为 PREFIX_0001.wav ~ PREFIX_9999.wav，自动找空号
 ***************************************************/
#include "sys_config.h"
#include "tx_platform.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "osal/string.h"
#include "osal_file.h"
#include "custom_mem/custom_mem.h"
#include "key2_wav_record.h"
#include "keyWork.h"
#include "keyScan.h"

// 复用录像那套盘符，保持一致 
#define WAV_DIR   "0:/DCIM"      // 直接写到 0:/DCIM 目录下

extern int16_t g_pcm_record_buf[PCM_MAX_FRAMES][PCM_SAMPLE_LEN];
extern volatile uint32_t g_pcm_wr_index;

void key_save_audio_thread(void *d);

// WAV文件头部结构
typedef struct
{
    char  chRIFF[4];        // "RIFF"
    int   total_Len;        // 文件长度 - 8
    char  chWAVE[4];        // "WAVE"
    char  chFMT[4];         // "fmt "
    int   dwFMTLen;         // 过渡字节，一般为16
    short fmt_pcm;          // 格式类别(PCM=1)
    short channels;         // 声道数
    int   fmt_samplehz;     // 采样率
    int   fmt_bytepsec;     // 每秒字节数 = samplehz * block_align
    short fmt_bytesample;   // block_align = channels * bits_per_sample / 8
    short fmt_bitpsample;   // bits_per_sample
    char  chDATA[4];        // "data"
    int   dwDATALen;        // 数据长度(字节)
} WaveHeader;

typedef enum {
    WEICHAT_MSG_FROM_PEER = 0,
    WEICHAT_MSG_FROM_ME   = 1,
} wechat_msg_from_t;

/* wechat UI 那边提供的绑定气泡接口 */
extern void wechat_add_voice_message_bound(wechat_msg_from_t from,
                                           uint8_t           sec,
                                           uint16_t          msg_id,
                                           const char       *wav_path);
extern void wechat_set_pending_voice_bubble(uint16_t msg_id,
                                            uint8_t  sec,
                                            const char *wav_path);
extern const unsigned char wav_header[] ;

struct KEY_AUDIO
{
    uint32_t       frq;              // 采样率
    struct os_task task;             // 录音任务
    uint8_t        filename_prefix[4];
    uint8_t        running;          // 录音标志
    uint32_t       minute;           // 最大录制分钟数
};

static struct KEY_AUDIO *audio_s = NULL;

// 返回 1~9999 的循环序号（第一次调用保证从 0001 开始）
// 按前缀在 0:/DCIM 下寻找一个没被占用的编号（1~9999）
static uint32_t find_free_wav_index(const char *prefix)
{
    char path[64];
    void *fp;
    uint32_t i;

    if (!prefix || !prefix[0]) {
        prefix = "def";
    }

    for (i = 1; i <= 9999; i++) {
        os_sprintf(path,
                   WAV_DIR "/%s_%04u.wav",
                   prefix,
                   (unsigned)i);

        fp = osal_fopen(path, "rb");   // 尝试只读打开
        if (!fp) {
            // 打不开，说明这个文件不存在，可以用这个编号
            return i;
        }
        // 文件已经存在，用完记得关掉
        osal_fclose(fp);
    }

    // 如果 1~9999 都被占了，就退回 1（极端情况，基本不会发生）
    return 1;
}

extern uint8_t s_wechat_focus_idx;  
extern struct {
    uint8_t page_cur;
    uint8_t page_back;
    uint8_t pagebtn_index;
} camera_gvar; 
#define PAGE_WECHAT 1
#define SAMPLERATE             8000
static uint8_t s_key2_recording = 0;

uint8_t g_video_intercom_mode = 0;  
extern void audio_adc_mute(void);
extern void audio_adc_unmute(void);
extern void audio_dac_set_filter_type(int filter_type);
extern int get_audio_dac_set_filter_type(void);


static uint32_t wechat_ptt_key_cb(struct key_callback_list_s *callback_list,
                                  uint32_t keyvalue,
                                  uint32_t extern_value)
{
    // 打一条总入口日志
    os_printf("[PTT] key2_cb in: key=0x%08X, page_cur=%d, focus=%d, video_mode=%d\n",
              keyvalue, camera_gvar.page_cur, s_wechat_focus_idx, g_video_intercom_mode);

    /* 1) 视频对讲模式下，实体语音键无效 */
    if (g_video_intercom_mode) {
        os_printf("[PTT] ignore: in video_intercom_mode=%d\n", g_video_intercom_mode);
        return 0;
    }

    /* 2) 只在微聊页面生效 */
    if (camera_gvar.page_cur != PAGE_WECHAT) {
        os_printf("[PTT] ignore: not in PAGE_WECHAT (cur=%d)\n", camera_gvar.page_cur);
        return 0;
    }

    /* 3) 只在底部“语音”按钮被选中时生效（focus_idx = 1） */
    if (s_wechat_focus_idx != 1) {
        os_printf("[PTT] ignore: focus_idx=%d (not voice btn)\n", s_wechat_focus_idx);
        return 0;
    }

    /* 4) 只响应该物理键位：KEY_CALL */
    if ((keyvalue >> 8) != KEY_CALL) {
        os_printf("[PTT] ignore: not KEY_CALL, key=0x%08X\n", keyvalue);
        return 0;
    }

    uint32 key_val = (keyvalue & 0xff);
    os_printf("[PTT] KEY_CALL event = 0x%02X\n", key_val);

    /* 5) 长按开始说话：KEY_EVENT_LDOWN */
    if (key_val == KEY_EVENT_LDOWN) {

        if (!s_key2_recording) {
            s_key2_recording = 1;
			
			pcm_record_start();

            // 硬件音频路径切换：关喇叭，停播放，开 MIC
            mute_speaker(1);                       // 喇叭关闭
            audio_dac_set_filter_type(SOUND_NONE); // DAC 不再播放对讲通路

            audio_adc_unmute();                    // 麦克风打开

            printf("[wechat] record start\r\n");

            // 开始本地 WAV 录音（写 SD 卡）
//            if (key_wav_record_start(SAMPLERATE, "wct", 0) != 0) {
//                printf("[wechat] key_wav_record_start fail\n");
//                // 失败的话：可以根据需求决定是否继续仅发送对讲，不录文件
//            }
        }
    }
    /* 6) 松开结束说话：KEY_EVENT_LUP */
    else if (key_val == KEY_EVENT_LUP) {

        if (s_key2_recording) {
            s_key2_recording = 0;

            // 停止 WAV 录音
//            key_wav_record_stop();
			pcm_record_stop();
			  
			char    wav_path[64] = {0};
			uint16_t msg_id      = 0;
			uint8_t  sec         = 0;  
   
			int wav_ret = wechat_save_record_buf_to_wav_ex("wct",SAMPLERATE,wav_path,sizeof(wav_path));
			
			/* 2) 通过 UDP 发送语音，拿到 msg_id + 时长 */
			int send_ret = wechat_send_record_buf_as_voice_msg(SAMPLERATE, &msg_id, &sec);
			if (wav_ret == 0 && send_ret == 0) {
				/* 3) 发送成功：在 UI 里插入“绑定好的语音气泡” */
				wechat_set_pending_voice_bubble(msg_id, sec, wav_path);
			} else {
				os_printf("[wechat] wav_ret=%d send_ret=%d, skip bubble bind\n",wav_ret, send_ret);
			}    
                                  
            // 音频路径复原：关 MIC、开喇叭、回到对讲播放滤波
            audio_adc_mute();                         // 麦克风关闭
            mute_speaker(0);                          // 喇叭打开
            audio_dac_set_filter_type(SOUND_INTERCOM);// 继续用对讲滤波播放

            printf("[wechat] record stop\n");
        }
    }

    return 0;
}

void wechat_ptt_key_init(void)
{
    add_keycallback(wechat_ptt_key_cb, NULL);
}

/* 开始录音
 * frq    : 采样率，比如 8000
 * prefix : 文件名前缀，例如 "wct"，文件名形如 0:/DCIM/wct_xxxx.wav
 * max_minute : 最长录制时间（分钟），0 表示不限时，只受按键控制
 */
int key_wav_record_start(uint32_t frq, const char *prefix, uint32_t max_minute)
{
    if (audio_s) {
        os_printf("%s already running\n", __FUNCTION__);
        return -1;
    }

    audio_s = custom_malloc(sizeof(struct KEY_AUDIO));
    if (!audio_s) {
        os_printf("%s malloc fail\n", __FUNCTION__);
        return -2;
    }

    os_memset(audio_s, 0, sizeof(struct KEY_AUDIO));

    audio_s->frq = frq;
    os_printf("frq:%d\n", audio_s->frq);

    //时长限制：0 表示无限制，依靠按键 stop
    if (max_minute == 0) {
        audio_s->minute = ~0;  
    } else {
        audio_s->minute = max_minute;
    }

    // 文件名前缀
    if (prefix && prefix[0]) {
        int len = os_strlen(prefix);
        if (len > 3) len = 3;
        os_memcpy(audio_s->filename_prefix, prefix, len);
    } else {
        os_memcpy(audio_s->filename_prefix, "def", 3);
    }

    // 创建录音任务：直接复用你系统里通用的 OS_TASK_INIT 
    OS_TASK_INIT("key_audio", &audio_s->task,
                 key_save_audio_thread, (uint32)audio_s,
                 OS_TASK_PRIORITY_NORMAL, 1024);

    return 0;
}

//停止录音
void key_wav_record_stop(void)
{
    if (!audio_s) {
        os_printf("%s not running\n", __FUNCTION__);
        return;
    }

    /* 设置标志位，线程里的 while 会退出 */
    audio_s->running = 0;
}

//stream 回调

static int opcode_func(stream *s, void *priv, int opcode)
{
    int res = 0;
    switch (opcode)
    {
        case STREAM_OPEN_ENTER:
            break;
        case STREAM_OPEN_EXIT:
            /* 打开后立即使能 */
            enable_stream(s, 1);
            break;
        case STREAM_OPEN_FAIL:
            break;
        default:
            break;
    }
    return res;
}

// WAV 录音线程：从 R_RECORD_AUDIO 流读裸 PCM，写成 WAV 到 SD 
void key_save_audio_thread(void *d)
{
    struct KEY_AUDIO *a_s = (struct KEY_AUDIO *)d;
    WaveHeader header;

    os_memset(&header, 0, sizeof(WaveHeader));
    os_memcpy(&header, wav_header, sizeof(WaveHeader));

    header.fmt_pcm        = 1;          // PCM
    header.channels       = 1;          // 单声道
    header.fmt_samplehz   = a_s->frq;   // 采样率
    header.fmt_bitpsample = 16;         // 16bit

    header.fmt_bytesample =
        header.channels * (header.fmt_bitpsample / 8);   // block_align = 2
    header.fmt_bytepsec =
        header.fmt_samplehz * header.fmt_bytesample;     // 字节率 = frq * 2

    char filename[64] = {0};
    a_s->running = 1;
    uint32_t count   = 0;
    int      w_len   = 0;
    uint32_t w_count = 0;       // 累计写入的字节数
    uint8_t *buf     = NULL;
    void    *fp      = NULL;
    struct data_structure *get_f = NULL;
    stream *s = NULL;
    uint32_t start_time = 0;

    // 打开音频流：R_RECORD_AUDIO 
    s = open_stream_available(R_RECORD_AUDIO, 0, 8, opcode_func, NULL);
    if (!s) {
        os_printf("[wav] open_stream R_RECORD_AUDIO fail\n");
        goto at_save_audio_thread_end;
    }

	os_printf("prefix:%s\n", a_s->filename_prefix);
	uint32_t sn = find_free_wav_index((const char *)a_s->filename_prefix);

	// 路径改为 0:/DCIM
	os_sprintf(filename,
			   WAV_DIR "/%s_%04u.wav",   //无符号
			   a_s->filename_prefix,
			   (unsigned)sn);

	os_printf("[wav] record name:%s\n", filename);


    fp = osal_fopen(filename, "wb+");
    if (!fp) {
        os_printf("[wav] fopen fail\n");
        goto at_save_audio_thread_end;
    }

    // 先写一个占位 WAV 头，后面回填长度
    osal_fwrite(&header, sizeof(WaveHeader), 1, fp);
    start_time = os_jiffies();

    while (a_s->running &&
           (os_jiffies() - start_time) / 1000 < a_s->minute * 60)
    {
        count++;
        if (count % 1000 == 0) {
            os_printf("%s:%d bytes=%u time=%dms\n",
                      __FUNCTION__, __LINE__,
                      w_count, os_jiffies() - start_time);
        }

        get_f = recv_real_data(s);
        if (get_f) {
            uint32_t flen = get_stream_real_data_len(get_f);
            buf  = get_stream_real_data(get_f);


            // 约定：osal_fwrite(ptr, size, 1, fp) 返回实际写入的**字节数** 
            w_len = osal_fwrite(buf, flen, 1, fp);
            free_data(get_f);
            get_f = NULL;

            if (w_len != (int)flen) {
                os_printf("%s:%d write error: expected %u bytes, got %d bytes\n",
                          __FUNCTION__, __LINE__, flen, w_len);
                if (w_len > 0) {
                    w_count += (uint32_t)w_len; //累计总PCM数据字节数
                }
                goto at_save_audio_thread_end;
            }
            w_count += (uint32_t)w_len;//累计总PCM数据字节数
        } else {
            os_sleep_ms(1);
        }
    }

at_save_audio_thread_end:

    // 回填 WAV 头长度信息 
    header.dwDATALen = w_count;
    header.total_Len = w_count + sizeof(WaveHeader) - 8;

    os_printf("%s end!!!!!!!!!!!!!\n", __FUNCTION__);
    os_printf("start_time:%d\tend_time:%d\n", start_time, os_jiffies());
    os_printf("[wav] total bytes=%u\n", w_count);

    if (fp) {
        // 回到文件头，把真正的长度写回去 
        osal_fseek(fp, 0);
        osal_fwrite(&header, sizeof(header), 1, fp);
        osal_fclose(fp);
    }

    if (s) {
        close_stream(s);
    }

    a_s->running = 0;
    custom_free((void *)a_s);
    audio_s = NULL;
}

/********************************************************************
 * 将内存中的 PCM 数组保存为 WAV 文件
 *  - prefix      : 文件名前缀，例如 "wct" → 0:/DCIM/wct_xxxx.wav
 *  - frq         : 采样率，例如 8000
 *  - pcm         : PCM 数据指针（int16_t）
 *  - sample_cnt  : 样本总数（不是字节，是 int16 的个数）
 ********************************************************************/
int wechat_save_pcm_to_wav_ex(const char *prefix,
                              uint32_t    frq,
                              const int16_t *pcm,
                              uint32_t    sample_cnt,
                              char       *out_path,
                              uint32_t    out_path_len)
{
    if (!pcm || sample_cnt == 0) {
        os_printf("%s invalid pcm or sample_cnt=0\n", __FUNCTION__);
        return -1;
    }
//    /* --- 调试：打印第 20、30、70 帧的前 20 个样本（如果存在） --- */
//    const uint32_t interesting_frames[] = {20, 30, 70}; // 以 1 为基的帧号（和你 UI/逻辑一致）
//    const uint32_t dump_per_frame = 20;                // 每帧打印的样本数上限
//    for (size_t fi = 0; fi < sizeof(interesting_frames)/sizeof(interesting_frames[0]); fi++) {
//        uint32_t frame_no = interesting_frames[fi];
//        if (frame_no == 0) continue; // 防止误用
//        /* 计算该帧的起始样本索引（0-based） */
//        uint32_t start_sample = (frame_no - 1) * PCM_SAMPLE_LEN;
//        if (start_sample >= sample_cnt) {
//            printf("[wechat debug] frame %u out of range (start_sample=%u >= total_samples=%u), skip\r\n",
//                      (unsigned)frame_no, (unsigned)start_sample, (unsigned)sample_cnt);
//            continue;
//        }
//        uint32_t avail = sample_cnt - start_sample;
//        uint32_t to_dump = (avail < dump_per_frame) ? avail : dump_per_frame;
//        const int16_t *p = pcm + start_sample;
//        printf("[wechat debug] frame %u start_sample=%u avail=%u dump=%u: \r\n",
//                  (unsigned)frame_no, (unsigned)start_sample, (unsigned)avail, (unsigned)to_dump);
//        for (uint32_t i = 0; i < to_dump; i++) {
//            printf("%d ", (int)p[i]);
//        }
//        printf("\r\n");
//    }
//    /* --- end debug print --- */
    WaveHeader header;
    os_memset(&header, 0, sizeof(WaveHeader));
    os_memcpy(&header, wav_header, sizeof(WaveHeader));

    // 显式配置 WAV 头 
    header.fmt_pcm        = 1;          // PCM
    header.channels       = 1;          // 单声道
    header.fmt_samplehz   = frq;        // 采样率
    header.fmt_bitpsample = 16;         // 每样本 16 bit

    header.fmt_bytesample =
        header.channels * (header.fmt_bitpsample / 8);   // block_align = 2

    header.fmt_bytepsec =
        header.fmt_samplehz * header.fmt_bytesample;     // 字节率 = frq * 2

    // 计算数据长度 
    uint32_t data_bytes = sample_cnt * sizeof(int16_t);
    header.dwDATALen    = data_bytes;
    header.total_Len    = data_bytes + sizeof(WaveHeader) - 8;

    // 生成文件名：也统一到 0:/DCIM 下
	char filename[64] = {0};
	const char *pf = (prefix && prefix[0]) ? prefix : "def";
	uint32_t sn = find_free_wav_index(pf);

	os_sprintf(filename,
			   WAV_DIR "/%s_%04u.wav",
			   pf,
			   (unsigned)sn);


    os_printf("[wechat] save pcm to wav: %s, samples=%u, bytes=%u\n",
              filename, sample_cnt, data_bytes);

    if (out_path && out_path_len > 0) {
        os_strncpy(out_path, filename, out_path_len - 1);
        out_path[out_path_len - 1] = '\0';
    }

    void *fp = osal_fopen(filename, "wb+");
    if (!fp) {
        os_printf("%s fopen fail\n", __FUNCTION__);
        return -2;
    }

    //写 WAV 头 
    int w_len = osal_fwrite(&header, sizeof(WaveHeader), 1, fp);
    if (w_len != (int)sizeof(WaveHeader)) {
        os_printf("%s write header error: expect=%u, got=%d\n",
                  __FUNCTION__, (unsigned)sizeof(WaveHeader), w_len);
        osal_fclose(fp);
        return -3;
    }

    // 写 PCM 数据主体 
    w_len = osal_fwrite((uint8_t *)pcm, data_bytes, 1, fp);
    if (w_len != (int)data_bytes) {
        os_printf("%s write pcm error: expect=%u, got=%d\n",
                  __FUNCTION__, data_bytes, w_len);
        osal_fclose(fp);
        return -4;
    }

    osal_fclose(fp);
    os_printf("[wechat] save wav done\n");
    return 0;
}
int wechat_save_pcm_to_wav(const char *prefix,
                           uint32_t    frq,
                           const int16_t *pcm,
                           uint32_t    sample_cnt)
{
    return wechat_save_pcm_to_wav_ex(prefix, frq, pcm, sample_cnt,
                                     NULL, 0);
}
/***************************************************
 * 语音录音控制：控制是否往 g_pcm_record_buf 写
 ***************************************************/

// 3000帧 × 160samples × 2bytes ≈ 960KB，放 PSRAM
int16_t g_pcm_record_buf[PCM_MAX_FRAMES][PCM_SAMPLE_LEN]__attribute__ ((aligned(4),section(".psram.src")));

volatile uint32_t g_pcm_wr_index   = 0;   // 已写入的帧数
volatile uint8_t  g_pcm_recording  = 0;   // 是否正在录制到数组
extern uint8_t           rec_open  ;   // 给音频链路用的“录音开关”（静音判断会用到）


void pcm_record_start(void)
{
    g_pcm_wr_index  = 0;
    g_pcm_recording = 1;
    rec_open        = 1;
    printf("[wechat] start PCM record\r\n");
}

void pcm_record_stop(void)
{
    g_pcm_recording = 0;
    rec_open        = 0;
    printf("[wechat] stop PCM record, frames=%u\r\n", g_pcm_wr_index);
}

uint32_t pcm_record_get_frames(void)
{
    return g_pcm_wr_index;
}

int wechat_save_record_buf_to_wav_ex(const char *prefix,
                                     uint32_t    frq,
                                     char       *out_path,
                                     uint32_t    out_path_len)
{
    if (g_pcm_wr_index == 0) {
        os_printf("%s no frames, skip\r\n", __FUNCTION__);
        return -1;
    }

    uint32_t frames     = g_pcm_wr_index;
    uint32_t sample_cnt = frames * PCM_SAMPLE_LEN;

    os_printf("[wechat] save_record_buf_to_wav: frames=%u, samples=%u\r\n",
              frames, sample_cnt);

    return wechat_save_pcm_to_wav_ex(prefix,
                                     frq,
                                     &g_pcm_record_buf[0][0],
                                     sample_cnt,
                                     out_path,
                                     out_path_len);
}

/* 旧接口保留 */
int wechat_save_record_buf_to_wav(const char *prefix, uint32_t frq)
{
    return wechat_save_record_buf_to_wav_ex(prefix, frq, NULL, 0);
}

void pcm_record_dump(void)
{
    uint32_t frames = pcm_record_get_frames();

    printf("Recorded frames = %u\r\n", frames);

    for (uint32_t i = 0; i < frames; i++) {
//        printf("[Frame %u]: ", i);

        for (uint32_t j = 0; j < PCM_SAMPLE_LEN; j++) {
            printf("%d ", g_pcm_record_buf[i][j]);
        }
//		printf("\r\n");

    }        
	printf("\n");
}