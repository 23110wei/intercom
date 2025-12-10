#include "sys_config.h"
#include "typesdef.h"
#include "iokey.h"
#include "keyScan.h"
#include "hal/gpio.h"
#include "osal/string.h"

static void key_iokey_init(key_channel_t *key, uint8_t enable)
{
	iokey_t *iokey = (iokey_t *)key->priv;

	if(enable)
	{
		gpio_set_mode(iokey->pin, iokey->pull, iokey->pull_level);
		gpio_set_dir(iokey->pin, GPIO_DIR_INPUT); 
		os_printf("%s:%d - GPIO%d initialized\n", __FUNCTION__, __LINE__, iokey->pin);
		key->enable = 1;
	}
	else
	{
		gpio_set_mode(iokey->pin,GPIO_PULL_NONE,0);
		gpio_set_dir(iokey->pin, GPIO_DIR_INPUT);
		os_printf("%s:%d - GPIO%d disabled\n", __FUNCTION__, __LINE__, iokey->pin);
		key->enable = 0;
	}
}

static uint8 key_iokey_scan(key_channel_t *key)
{
	iokey_t *iokey = (iokey_t *)key->priv;
	uint32 gpio_val = gpio_get_val(iokey->pin);

	// 记录当前GPIO状态，用于发送到应用层
	key->extern_value = gpio_val;

	// 根据invert配置判断按键状态
	if(iokey->invert)
	{
		// 反相: 高电平表示按下
		return gpio_val ? iokey->keycode : KEY_NONE;
	}
	else
	{
		// 正常: 低电平表示按下
		return gpio_val ? KEY_NONE : iokey->keycode;
	}
}

static const keys_t iokey_arg = {
	.period_long   = 500,
	.period_repeat = 1000,
	.period_dither = 80,
};

// 发射键 (PA14)
static iokey_t call_iokey = {
	.priv = NULL,
	.pin  = SPEECH_KEY,
	.pull = GPIO_PULL_UP,
	.pull_level = GPIO_PULL_LEVEL_100K,
	.invert = 0,        // 按下为低电平
	.keycode = KEY_CALL,
};

key_channel_t call_key = {
	.init      = key_iokey_init,
	.scan      = key_iokey_scan,
	.prepare   = NULL,
	.priv      = (void*)&call_iokey,
	.key_arg   = &iokey_arg,
	.key_table = NULL,   // GPIO按键不需要码值表
};

// 贴纸键 (USB_DP)
static iokey_t sticker_iokey = {
	.priv = NULL,
	.pin  = USB_DP,
	.pull = GPIO_PULL_UP,
	.pull_level = GPIO_PULL_LEVEL_100K,
	.invert = 0,        // 按下为低电平
	.keycode = KEY_STICKER,
};

key_channel_t sticker_key = {
	.init      = key_iokey_init,
	.scan      = key_iokey_scan,
	.prepare   = NULL,
	.priv      = (void*)&sticker_iokey,
	.key_arg   = &iokey_arg,
	.key_table = NULL,
};

// 魔音键 (USB_DM)
static iokey_t m_iokey = {
	.priv = NULL,
	.pin  = USB_DM,
	.pull = GPIO_PULL_UP,
	.pull_level = GPIO_PULL_LEVEL_100K,
	.invert = 0,        // 按下为低电平
	.keycode = KEY_M,
};

key_channel_t m_key = {
	.init      = key_iokey_init,
	.scan      = key_iokey_scan,
	.prepare   = NULL,
	.priv      = (void*)&m_iokey,
	.key_arg   = &iokey_arg,
	.key_table = NULL,
};
  
  