#ifndef _IOKEY_H
#define _IOKEY_H
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "osal/timer.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"
#include "keyScan.h"

struct iokey_t;
struct keys_t;
typedef struct iokey_t iokey_t;

#define USB_DP PC_6
#define USB_DM PC_7
#define SPEECH_KEY PA_14
struct iokey_t
{
	void *priv;
	uint32 pin;          // GPIO引脚
	uint8  pull;         // 上拉下拉配置
	uint8  pull_level;   // 上下拉电阻级别
	uint8  invert;       // 是否反相(0=按下为0, 1=按下为1)
	uint8  keycode;      // 对应的按键码
};

// GPIO按键定义
extern key_channel_t call_key;     // PA14发射键
extern key_channel_t sticker_key;  // USB_DP贴纸键(PC6)
extern key_channel_t m_key;        // USB_DM魔音键(PC7)

#endif