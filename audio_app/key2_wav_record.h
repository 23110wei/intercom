/***************************************************
    Key2按键触发的WAV音频录制功能头文件
***************************************************/
#ifndef __KEY2_WAV_RECORD_H
#define __KEY2_WAV_RECORD_H
#include "stdint.h"


#define PCM_SAMPLE_LEN        160          // 每帧samples（8kHz 20ms）
#define PCM_MAX_SEC           60
#define PCM_FRAME_PER_SEC     50           // 20ms一帧
#define PCM_MAX_FRAMES        (PCM_MAX_SEC * PCM_FRAME_PER_SEC) // 3000帧

extern int16_t  g_pcm_record_buf[PCM_MAX_FRAMES][PCM_SAMPLE_LEN];
extern volatile uint32_t g_pcm_wr_index;
extern volatile uint8_t  g_pcm_recording;
extern uint8_t           rec_open;

// 开始Key2 WAV录音
// 返回值: 0-成功, 负值-失败
int key_wav_record_start(uint32_t frq, const char *prefix, uint32_t max_minute);

// 停止Key2 WAV录音
void key_wav_record_stop(void);
int wechat_save_record_buf_to_wav_ex(const char *prefix, uint32_t  frq, char *out_path, uint32_t out_path_len);
int wechat_save_record_buf_to_wav(const char *prefix, uint32_t frq);
void pcm_record_dump(void);

#endif // __KEY2_WAV_RECORD_H