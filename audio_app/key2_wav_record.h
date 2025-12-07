/***************************************************
    Key2按键触发的WAV音频录制功能头文件
***************************************************/
#ifndef __KEY2_WAV_RECORD_H
#define __KEY2_WAV_RECORD_H

// 开始Key2 WAV录音
// 返回值: 0-成功, 负值-失败
int key2_wav_record_start(void);

// 停止Key2 WAV录音
// 返回值: 0-成功, 负值-失败
int key2_wav_record_stop(void);

// 获取录音状态
// 返回值: 1-正在录音, 0-未在录音
int key2_wav_record_is_recording(void);

// 清理Key2 WAV录音资源
void key2_wav_record_cleanup(void);


int wechat_save_pcm_to_wav(const char *prefix,
                           uint32_t frq,
                           const int16_t *pcm,
                           uint32_t sample_cnt);

#endif // __KEY2_WAV_RECORD_H