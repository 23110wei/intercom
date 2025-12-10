#include "sys_config.h"
#include "osal/string.h"
#include "custom_mem/custom_mem.h"
#include "stream_frame.h"
#include "stream_define.h"
#include "osal/task.h"
#include "osal_file.h"

#include "play_pcmtone.h"      // 里面有 SOUND_PCM / get_audio_dac_set_filter_type 等
#include "key2_wav_record.h"   // 里面已经有 WaveHeader 定义（你上面贴的）

extern int get_audio_dac_set_filter_type(void);
extern void audio_dac_set_filter_type(int filter_type);
extern void audio_adc_mute(void);
extern void mute_speaker(uint8 enable);

/* ===== 复用你 tone 的参数 ===== */
#define WAV_AUDIO_LEN    (480)   // 每个节点装 480 字节 PCM（和 tone 一样）
#define WAV_AUDIO_COUNT  4       // 4 个节点循环

/* 和 tone 一样的 get_data_len 回调 */
static uint32_t wechat_get_sound_data_len(void *data)
{
    return WAV_AUDIO_LEN;
}

static stream_ops_func wechat_stream_sound_ops =
{
    .get_data_len = wechat_get_sound_data_len,
};

/* 播放线程句柄 */
static struct os_task s_wechat_wav_task;

/* 播放线程用的上下文：只保存路径 */
typedef struct {
    char path[64];
} wechat_wav_play_ctx_t;

/* ========= 流的 opcode 回调，基本拷贝你 tone 的 ========= */

static int wechat_wav_opcode_func(stream *s, void *priv, int opcode)
{
    static uint8 *audio_buf = NULL;
    int res = 0;

    switch (opcode)
    {
        case STREAM_OPEN_ENTER:
            break;

        case STREAM_OPEN_EXIT:
        {
            audio_buf = os_malloc(WAV_AUDIO_COUNT * WAV_AUDIO_LEN);
            if (audio_buf)
            {
                stream_data_dis_mem(s, WAV_AUDIO_COUNT);
            }
            /* 绑定到 R_SPEAKER，跟 pcmtone 一样 */
            streamSrc_bind_streamDest(s, R_SPEAKER);
        }
        break;

        case STREAM_OPEN_FAIL:
            break;

        case STREAM_FILTER_DATA:
            break;

        case STREAM_DATA_DIS:
        {
            struct data_structure *data = (struct data_structure *)priv;
            int data_num = (int)data->priv;
            data->ops  = &wechat_stream_sound_ops;
            data->data = audio_buf + (data_num) * WAV_AUDIO_LEN;
        }
        break;

        case STREAM_DATA_FREE:
            break;

        case STREAM_RECV_DATA_FINISH:
            break;

        case STREAM_CLOSE_EXIT:
        {
            if (audio_buf)
            {
                os_free(audio_buf);
                audio_buf = NULL;
            }
        }
        break;

        default:
            break;
    }
    return res;
}

/* ========= 真正从 SD 卡读 WAV + 喂 stream 的线程 ========= */

static int wechat_wav_play_thread(void *d)
{
    wechat_wav_play_ctx_t *ctx = (wechat_wav_play_ctx_t *)d;
    const char *wav_path = ctx ? ctx->path : NULL;

    if (!wav_path || !wav_path[0]) {
        printf("[wechat] wav_play_thread: invalid path\n");
        goto exit_thread;
    }

    printf("[wechat] wav_play_thread start: %s\n", wav_path);

    /* 1. 打开文件 */
    void *fp = osal_fopen(wav_path, "rb");
    if (!fp) {
        printf("[wechat] fopen fail: %s\n", wav_path);
        goto exit_thread;
    }

    /* 2. 读取 WAV 头并简单校验 */
    WaveHeader hdr;
    int rlen = osal_fread(&hdr, sizeof(WaveHeader), 1, fp);
    if (rlen != (int)sizeof(WaveHeader)) {
        printf("[wechat] read header fail, rlen=%d\n", rlen);
        osal_fclose(fp);
        goto exit_thread;
    }

    if (os_memcmp(hdr.chRIFF, "RIFF", 4) != 0 ||
        os_memcmp(hdr.chWAVE, "WAVE", 4) != 0 ||
        hdr.fmt_pcm        != 1 ||          /* PCM */
        hdr.channels       != 1 ||          /* 单声道 */
        hdr.fmt_bitpsample != 16) {         /* 16bit */
        printf("[wechat] invalid wav fmt: ch=%d bits=%d fmt=%d\n",
               hdr.channels, hdr.fmt_bitpsample, hdr.fmt_pcm);
        osal_fclose(fp);
        goto exit_thread;
    }

    printf("[wechat] wav: %d Hz, %d ch, %d bits, data=%d bytes\n",
           hdr.fmt_samplehz,
           hdr.channels,
           hdr.fmt_bitpsample,
           hdr.dwDATALen);

    /* 3. 打开 audio 流：复用类似 pcmtone 的办法 */
    stream *src = open_stream_available("wechat_wav",
                                        WAV_AUDIO_COUNT,
                                        0,
                                        wechat_wav_opcode_func,
                                        NULL);
    if (!src) {
        printf("[wechat] open_stream wechat_wav fail\n");
        osal_fclose(fp);
        goto exit_thread;
    }

    /* 4. 设置 DAC 滤波 / 静音等（按你项目需要微调） */
    int former_dac_priv = get_audio_dac_set_filter_type();
    audio_dac_set_filter_type(SOUND_PCM);   // 播放原始 PCM
    audio_adc_mute();                       // 关 MIC 防止啸叫
    mute_speaker(0);                        // 打开喇叭

    uint32_t read_total = 0;
    uint32_t data_bytes = hdr.dwDATALen;

    while (read_total < data_bytes)
    {
        struct data_structure *data = get_src_data_f(src);
        if (!data) {
            /* 没空节点，等待一点再试 */
            os_sleep_ms(1);
            continue;
        }

        uint8_t *pcm_buf = (uint8_t *)get_stream_real_data(data);
        if (!pcm_buf) {
            printf("[wechat] get_stream_real_data NULL\n");
            free_data(data);
            data = NULL;
            break;
        }

        uint32_t remain = data_bytes - read_total;
        uint32_t this_len = (remain > WAV_AUDIO_LEN) ? WAV_AUDIO_LEN : remain;

        rlen = osal_fread(pcm_buf, this_len, 1, fp);
        if (rlen != (int)this_len) {
            printf("[wechat] read data fail: expect=%u got=%d\n",
                   (unsigned)this_len, rlen);
            free_data(data);
            data = NULL;
            break;
        }

        read_total += this_len;

        /* 标记数据类型：和 pcmtone 一样 */
        data->type = SET_DATA_TYPE(SOUND, SOUND_PCM);

        /* 发到流里，后面自动送给 R_SPEAKER */
        send_data_to_stream(data);

        /* 如果最后一块 < WAV_AUDIO_LEN，多余部分就保持旧值/0，问题不大，
           或者你也可以 os_memset(pcm_buf+this_len,0,WAV_AUDIO_LEN-this_len) */
    }

    printf("[wechat] wav play end, total=%u/%u\n",
           (unsigned)read_total, (unsigned)data_bytes);

    /* 收尾 */
    audio_dac_set_filter_type(former_dac_priv);  // 恢复原 DAC 模式
    os_sleep_ms(20);
    close_stream(src);
    src = NULL;

    osal_fclose(fp);

exit_thread:
    if (ctx) custom_free(ctx);
    printf("[wechat] wav_play_thread exit\n");
    return 0;
}

/* ========= 对外统一入口：给 wechat 调用 ========= */

int wechat_platform_play_wav(const char *wav_path)
{
    if (!wav_path || !wav_path[0]) {
        printf("[wechat] platform_play_wav invalid path\n");
        return -1;
    }

    printf("[wechat] platform_play_wav: %s\n", wav_path);

    /* 分配一个上下文，把路径拷进去，丢给播放线程 */
    wechat_wav_play_ctx_t *ctx =
        (wechat_wav_play_ctx_t *)custom_malloc(sizeof(wechat_wav_play_ctx_t));
    if (!ctx) {
        printf("[wechat] malloc ctx fail\n");
        return -2;
    }

    os_memset(ctx, 0, sizeof(*ctx));
    os_strncpy(ctx->path, wav_path, sizeof(ctx->path) - 1);
    ctx->path[sizeof(ctx->path) - 1] = '\0';

    /* 起一个线程播放（和 pcmtone 一样异步，不堵 UI） */
    OS_TASK_INIT("wechat_wav_play",
                 &s_wechat_wav_task,
                 wechat_wav_play_thread,
                 (uint32)ctx,
                 OS_TASK_PRIORITY_NORMAL,
                 1024);

    return 0;
}
