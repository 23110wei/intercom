/***************************************************
    Key按键触发的WAV音频录制功能
    由 AT_save_audio.c 改造而来：去掉AT命令，用API控制
***************************************************/
#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "osal/task.h"
#include "osal_file.h"

void key_save_audio_thread(void *d);

/* WAV文件头部结构 */
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


extern const unsigned char wav_header[] ;

struct KEY_AUDIO
{
    uint32_t       frq;              // 采样率
    struct os_task task;             // 录音任务
    uint8_t        filename_prefix[4];
    uint8_t        running;          // 录音标志
    uint32_t       minute;           // 最大录制分钟数（我们按键版可以设置为 ~0）
};

static struct KEY_AUDIO *audio_s = NULL;


/* 按键版：开始录音
 * frq    : 采样率，比如 8000
 * prefix : 文件名前缀，例如 "wct"，文件名形如 0:audio/wct_xxxx.wav
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

    /* 时长限制：0 表示无限制（依靠按键 stop） */
    if (max_minute == 0) {
        audio_s->minute = ~0;    // 原代码就是这么干的
    } else {
        audio_s->minute = max_minute;
    }

    /* 文件名前缀 */
    if (prefix && prefix[0]) {
        int len = os_strlen(prefix);
        if (len > 3) len = 3;
        os_memcpy(audio_s->filename_prefix, prefix, len);
    } else {
        os_memcpy(audio_s->filename_prefix, "def", 3);
    }

    /* 创建录音任务 */
    OS_TASK_INIT("key_audio", &audio_s->task,
                 key_save_audio_thread, (uint32)audio_s,
                 OS_TASK_PRIORITY_NORMAL, 1024);

    return 0;
}

/* 停止录音 */
void key_wav_record_stop(void)
{
    if (!audio_s) {
        os_printf("%s not running\n", __FUNCTION__);
        return;
    }

    /* 设置标志位即可，线程里的 while 会退出 */
    audio_s->running = 0;
}

/*======================= 原有 stream 回调和录音线程保留 =======================*/

static int opcode_func(stream *s, void *priv, int opcode)
{
    int res = 0;
    switch (opcode)
    {
        case STREAM_OPEN_ENTER:
            break;
        case STREAM_OPEN_EXIT:
            enable_stream(s, 1);
            break;
        case STREAM_OPEN_FAIL:
            break;
        default:
            break;
    }
    return res;
}

void key_save_audio_thread(void *d)
{
    struct KEY_AUDIO *a_s = (struct KEY_AUDIO *)d;
    WaveHeader header;

    /* 用模板初始化 WAV 头 */
    os_memset(&header, 0, sizeof(WaveHeader));
    os_memcpy(&header, wav_header, sizeof(WaveHeader));

    /* 显式设置关键字段，避免模板坑 */
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
    uint32_t flen    = 0;
    uint32_t w_count = 0;
    uint8_t *buf     = NULL;
    void    *fp      = NULL;
    struct data_structure *get_f = NULL;
    stream *s = NULL;
    uint32_t start_time = 0;

    s = open_stream_available(R_AT_SAVE_AUDIO, 0, 8, opcode_func, NULL);
    if (!s) {
        goto at_save_audio_thread_end;
    }

    os_printf("prefix:%s\n", a_s->filename_prefix);
    os_sprintf(filename,
               "0:audio/%s_%04d.wav",
               a_s->filename_prefix,
               (uint32_t)os_jiffies() % 9999);
    os_printf("record name:%s\n", filename);

    fp = osal_fopen(filename, "wb+");
    if (!fp) {
        goto at_save_audio_thread_end;
    }

    /* 先跳过 WAV 头位置，后面再回填 */
    osal_fseek(fp, sizeof(WaveHeader));
    start_time = os_jiffies();

    while (a_s->running &&
           (os_jiffies() - start_time) / 1000 < a_s->minute * 60)
    {
        count++;
        if (count % 1000 == 0) {
            os_printf("%s:%d\t%d\trecord time:%d\n",
                      __FUNCTION__, __LINE__,
                      w_count, os_jiffies() - start_time);
        }

        get_f = recv_real_data(s);
        if (get_f) {
            buf  = get_stream_real_data(get_f);
            flen = get_stream_real_data_len(get_f);

            w_len = osal_fwrite(buf, flen, 1, fp);
            free_data(get_f);
            get_f = NULL;

            /* osal_fwrite 返回实际写入的字节数（按你原工程的注释） */
            if (w_len != (int)flen) {
                os_printf("%s:%d write error: expected %d bytes, got %d bytes\n",
                          __FUNCTION__, __LINE__, flen, w_len);
                if (w_len > 0) {
                    w_count += w_len;   // 部分写入也记进去
                }
                goto at_save_audio_thread_end;
            }
            w_count += w_len;
        } else {
            os_sleep_ms(1);
        }
    }

at_save_audio_thread_end:

    /* 回填 WAV 头长度信息 */
    header.dwDATALen = w_count;
    header.total_Len = w_count + sizeof(WaveHeader) - 8;

    os_printf("%s end!!!!!!!!!!!!!\n", __FUNCTION__);
    os_printf("start_time:%d\tend_time:%d\n", start_time, os_jiffies());

    if (fp) {
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
    return;
}




/********************************************************************
 * 将内存中的 PCM 数组保存为 WAV 文件
 *  - prefix      : 文件名前缀，例如 "wct" → 0:/audio/wct_xxxx.wav
 *  - frq         : 采样率，例如 8000
 *  - pcm         : PCM 数据指针（int16_t）
 *  - sample_cnt  : 样本总数（不是字节，是 int16 个数）
 ********************************************************************/
int wechat_save_pcm_to_wav(const char *prefix,
                           uint32_t frq,
                           const int16_t *pcm,
                           uint32_t sample_cnt)
{
    if (!pcm || sample_cnt == 0) {
        os_printf("%s invalid pcm or sample_cnt=0\n", __FUNCTION__);
        return -1;
    }

    WaveHeader header;
    os_memset(&header, 0, sizeof(WaveHeader));
    os_memcpy(&header, wav_header, sizeof(WaveHeader));

    /* 显式配置 WAV 头 */
    header.fmt_pcm        = 1;          // PCM
    header.channels       = 1;          // 单声道
    header.fmt_samplehz   = frq;        // 采样率
    header.fmt_bitpsample = 16;         // 每样本 16 bit

    header.fmt_bytesample =
        header.channels * (header.fmt_bitpsample / 8);   // block_align = 2

    header.fmt_bytepsec =
        header.fmt_samplehz * header.fmt_bytesample;     // 字节率 = frq * 2

    /* 计算数据长度 */
    uint32_t data_bytes = sample_cnt * sizeof(int16_t);
    header.dwDATALen    = data_bytes;
    header.total_Len    = data_bytes + sizeof(WaveHeader) - 8;

    /* 生成文件名 */
    char filename[64] = {0};
    const char *pf = prefix && prefix[0] ? prefix : "def";

    os_sprintf(filename,
               "0:audio/%s_%04d.wav",
               pf,
               (uint32_t)os_jiffies() % 9999);

    os_printf("[wechat] save pcm to wav: %s, samples=%u, bytes=%u\n",
              filename, sample_cnt, data_bytes);

    void *fp = osal_fopen(filename, "wb+");
    if (!fp) {
        os_printf("%s fopen fail\n", __FUNCTION__);
        return -2;
    }

    /* 写 WAV 头 */
    osal_fwrite(&header, sizeof(WaveHeader), 1, fp);

    /* 写 PCM 数据主体
     * 保持和你工程里其它地方一致，用 flen,1 的风格，
     * 且假设 osal_fwrite 返回写入的字节数。
     */
    int w_len = osal_fwrite((uint8_t *)pcm, data_bytes, 1, fp);
    if (w_len != (int)data_bytes) {
        os_printf("%s write pcm error: expect=%u, got=%d\n",
                  __FUNCTION__, data_bytes, w_len);
        osal_fclose(fp);
        return -3;
    }

    osal_fclose(fp);
    os_printf("[wechat] save wav done\n");
    return 0;
}
