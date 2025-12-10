#include "sys_config.h"
#include "typesdef.h"
#include "lib/video/dvp/cmos_sensor/csi.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "hal/lcdc.h"
#include "hal/spi.h"
#include "osal/irq.h"
#include "osal/string.h"
#include "dev/vpp/hgvpp.h"
#include "dev/scale/hgscale.h"
#include "dev/jpg/hgjpg.h"
#include "dev/lcdc/hglcdc.h"
#include "osal/semaphore.h"
#include "lib/lcd/lcd.h"
#include "lib/lcd/gui.h"
#include "dev/vpp/hgvpp.h"
#include "dev/csi/hgdvp.h"
#include "lib/video/dvp/jpeg/jpg.h"
#include "hal/dma.h"
#include "lv_demo_widgets.h"
#include "openDML.h"
#include "osal/mutex.h"
#include "avidemux.h"
#include "playback/playback.h"
#include "lib/vef/video_ef.h"
#include "vpp_ipf_src.h"
#include "lib/umac/ieee80211.h"
#include "stream_frame.h"
#include "keyScan.h"
#include "../lvgl.h"
#include "ui_language.h"
#include "fly_demo.h"
#include "clock_app.h"
#include "syscfg.h"
#include "sonic_process.h"
#include "magic_sound.h"
#include "vpp_ipf_src.h"

#include "lwip/netif.h"
#include "hal/netdev.h"
#include "hal/timer_device.h"
#include "wechat_msg.h"

#define WECHAT_AVATAR_SIZE      34
#define WECHAT_VOICE_BUBBLE_H   30
#define WECHAT_EMOJI_BUBBLE_H   30

/* 页面 & 布局对象 */
lv_obj_t *ui_wechatPage;
lv_obj_t *ui_wechatStatusBar;   // 最上状态栏（WiFi + 电量）
lv_obj_t *ui_wechatTopBar;      // 顶部标题栏
lv_obj_t *ui_wechatMsgArea;     // 聊天内容区（滚动）
lv_obj_t *ui_wechatBtmBar;      // 底部功能区

/* 状态栏元素 */
lv_obj_t *ui_wechatWifiDot;     // 左侧 WiFi 圆点
lv_obj_t *ui_wechatBattImg;     // 右侧电池图标

/* 底部五个按钮对象 */
lv_obj_t *ui_wechatBtnVideo = NULL;   // 视频通话
lv_obj_t *ui_wechatBtnMic   = NULL;   // 按住说话
lv_obj_t *ui_wechatBtnEmoji = NULL;   // 表情
lv_obj_t *ui_wechatBtnCamera= NULL;   // 拍照
lv_obj_t *ui_wechatBtnPhoto = NULL;   // 相册/图片

/* 方便统一管理：数组 + 当前焦点索引 */
static lv_obj_t *s_wechat_btns[5];
static lv_obj_t *s_wechat_btn_labels[5];
uint8_t s_wechat_focus_idx = 0;  // 0~4，默认 0=视频

static lv_timer_t *ui_pending_voice_timer = NULL;

/* ========= 表情面板相关 ========= */
#define EMOJI_COL_NUM   4

static const char *s_emoji_texts[EMOJI_COUNT] = {
    [WECHAT_EMOJI_SMILE] = "SMILE"  , //"😀"
    [WECHAT_EMOJI_GRIN]  = "GRIN"   , //"😁",
    [WECHAT_EMOJI_LAUGH] = "LAUGH"  , //"😂",
    [WECHAT_EMOJI_ROFL]  = "ROFL"   , //"🤣",
    [WECHAT_EMOJI_BLUSH] = "BLUSH"  , //"😊",
    [WECHAT_EMOJI_LOVE]  = "LOVE"   , //"😍",
    [WECHAT_EMOJI_KISS]  = "KISS"   , //"😘",
    [WECHAT_EMOJI_COOL]  = "COOL"   , //"😎",
    [WECHAT_EMOJI_CRY]   = "CRY"    , //"😢",
    [WECHAT_EMOJI_ANGRY] = "ANGRY"  , //"😡",
    [WECHAT_EMOJI_THUMB] = "THUMB"  , //"👍",
    [WECHAT_EMOJI_PRAY]  = "PRAY"   , //"🙏",
};

static lv_obj_t *ui_emojiPanel = NULL;           // 表情弹窗根对象
static lv_obj_t *s_emoji_btns[EMOJI_COUNT];      // 每个表情是一个按钮
static uint8_t   s_emoji_focus_idx = 0;          // 当前选中的表情 index
static uint8_t   s_emoji_panel_visible = 0;      // 面板是否弹出

extern uint8_t get_batlevel(void);      // 已在别处实现
extern const lv_img_dsc_t *ui_imgset_iconBat[]; // 电池图标数组

uint8_t g_camera_from_page     = PAGE_HOME;   // 记录相机入口
uint8_t g_img_from_page        = PAGE_HOME;   // 记录相册入口

typedef enum {
    WEICHAT_MSG_FROM_PEER = 0,
    WEICHAT_MSG_FROM_ME   = 1,
} wechat_msg_from_t;

#define UI_KEY_EVENT(code, event)   (((code) << 8) | (event))
#define UI_KEY_CALL_LONG_DOWN   UI_KEY_EVENT(KEY_CALL, KEY_EVENT_LDOWN)
#define UI_KEY_CALL_LONG_UP     UI_KEY_EVENT(KEY_CALL, KEY_EVENT_LUP)
/* 录音状态 */
static uint8_t  wechat_recording   = 0;		  // 是否正在录音
static uint32_t wechat_press_tick  = 0;   	  // 记录按下时的 tick（预留） 
static uint32_t wechat_record_start_ms  = 0;  // 开始录音时刻（os_jiffies）
static uint32_t wechat_record_sec       = 0;  // 当前已录制秒数
#define WECHAT_RECORD_MAX_SEC   60            // 最长 60 秒
/* 录音浮层 UI：顶部一条提示 + 进度条 + 秒数字 */
static lv_obj_t  *ui_record_panel = NULL;
static lv_obj_t  *ui_record_arc   = NULL;


/* LVGL 定时器，用来刷新录音进度 */
static lv_timer_t *ui_record_timer = NULL;


/* 本文件内部使用的函数声明 */
static void wechat_open_emoji_panel(void);
static void wechat_close_emoji_panel(void);
static void emoji_update_focus_style(void);
static void emoji_panel_on_select(uint8_t idx);
static void wechat_add_emoji_message(wechat_msg_from_t from, uint16_t  emoji_id, const char *emoji_text);

typedef struct {
    uint8_t  pending;                 // 1 = 有一条待创建的语音气泡
    uint16_t msg_id;
    uint8_t  sec;
    char     wav_path[64];
} wechat_voice_pending_t;

static wechat_voice_pending_t s_wechat_voice_pending;

typedef enum {
    WECHAT_FOCUS_BOTTOM = 0,   // 底部 5 个按钮
    WECHAT_FOCUS_MSG    = 1,   // 聊天消息（只针对语音气泡）
} wechat_focus_mode_t;

typedef enum {
    WECHAT_MSG_TYPE_VOICE = 0,
    WECHAT_MSG_TYPE_EMOJI = 1,
    // 以后可以再加 PHOTO / TEXT / VIDEO ...
} wechat_msg_type;

typedef struct {
    lv_obj_t          *row;      // 一行容器（包含头像 + 气泡）
    lv_obj_t          *bubble;   // 真正的气泡控件（语音 or 表情）

    wechat_msg_type  type;     // 语音 / 表情
    wechat_msg_from_t  from;     // 自己还是对方

    // 语音专用字段
    uint16_t           msg_id;
    uint8_t            sec;
    char               wav_path[64];

    // 表情专用字段
    uint16_t           emoji_id;
} wechat_msg_slot_t;

#define WECHAT_MAX_MSG_SLOTS  32

static wechat_msg_slot_t s_msg_slots[WECHAT_MAX_MSG_SLOTS];
static uint8_t           s_msg_cnt = 0;      // 当前存了多少条消息（语音 + 表情）
static int16_t           s_wechat_msg_focus_idx = -1;  // 当前选中的消息索引
static uint8_t s_wechat_focus_mode    = WECHAT_FOCUS_BOTTOM;



static void wechat_update_msg_focus_style(void)
{
    for (uint8_t i = 0; i < s_msg_cnt && i < WECHAT_MAX_MSG_SLOTS; i++) {
        wechat_msg_slot_t *slot = &s_msg_slots[i];
        if (!slot->bubble) continue;

        if ((int16_t)i == s_wechat_msg_focus_idx &&
            s_wechat_focus_mode == WECHAT_FOCUS_MSG) {

            /* 只在气泡上画边框，不改 row 的背景 */
            lv_obj_set_style_border_width(slot->bubble, 2, 0);
            lv_obj_set_style_border_color(slot->bubble, lv_color_hex(0xFCA702), 0);
            lv_obj_set_style_radius(slot->bubble, 6, 0);
        } else {
            lv_obj_set_style_border_width(slot->bubble, 0, 0);
        }
    }
}


/* 轮询 pending 的语音气泡，有就创建 UI */
static void wechat_pending_voice_timer_cb(lv_timer_t *t)
{
    (void)t;

    if (!s_wechat_voice_pending.pending) return;

    printf("[wechat] pending->create bubble in UI thread: msg_id=%u sec=%u file=%s\r\n",
           s_wechat_voice_pending.msg_id,
           s_wechat_voice_pending.sec,
           s_wechat_voice_pending.wav_path);

    /* 用“我发的语音气泡”创建绑定 */
    wechat_add_voice_message_bound(WEICHAT_MSG_FROM_ME,
                                   s_wechat_voice_pending.sec,
                                   s_wechat_voice_pending.msg_id,
                                   s_wechat_voice_pending.wav_path);

    s_wechat_voice_pending.pending = 0;
}


/* 简单环形分配一个气泡槽位 */
static wechat_msg_slot_t *wechat_msg_alloc(void)
{
    wechat_msg_slot_t *slot = &s_msg_slots[s_msg_cnt % WECHAT_MAX_MSG_SLOTS];
    s_msg_cnt++;
    os_memset(slot, 0, sizeof(*slot));
    return slot;
}


/* 被按键线程调用：只写入数据，不碰 LVGL */
void wechat_set_pending_voice_bubble(uint16_t msg_id,
                                     uint8_t  sec,
                                     const char *wav_path)
{
    s_wechat_voice_pending.pending = 1;
    s_wechat_voice_pending.msg_id  = msg_id;
    s_wechat_voice_pending.sec     = sec;

    if (wav_path) {
        os_strncpy(s_wechat_voice_pending.wav_path,
                   wav_path,
                   sizeof(s_wechat_voice_pending.wav_path) - 1);
        s_wechat_voice_pending.wav_path[sizeof(s_wechat_voice_pending.wav_path) - 1] = '\0';
    } else {
        s_wechat_voice_pending.wav_path[0] = '\0';
    }

    printf("[wechat] pending voice bubble: msg_id=%u sec=%u file=%s\r\n",
           msg_id, sec, s_wechat_voice_pending.wav_path);
}
/*------------------------------------------------
 *  工具函数
 *----------------------------------------------*/

/* 计算语音条宽度：sec 秒 -> 页面宽度 20%~80% */
/* 计算语音条宽度：sec 秒 -> 在可用宽度里线性变化，避免越界 */
/* 计算语音条宽度：
 * 要求：60s ≈ 一行可用宽度的 80%，短语音不要太短
 */
static lv_coord_t calc_voice_bar_width(int sec, lv_coord_t page_w)
{
    const int max_sec = 60;

    if (sec < 1)       sec = 1;
    if (sec > max_sec) sec = max_sec;

    /* 预留头像和两边 padding，避免越界、顶边 */
    const lv_coord_t side_pad   = 4 * 2;   // row 左右 pad_left/pad_right≈4
    const lv_coord_t avatar_w   = 32;      // 头像宽度
    const lv_coord_t extra_gap  = 8;       // 气泡与头像/文本的间距

    /* 一行里真正给语音气泡可用的总宽度 */
    lv_coord_t usable_w = page_w - side_pad - avatar_w - extra_gap;
    if (usable_w < 60) usable_w = 60;      // 兜底，防止负数或太小

    /* 
     * 约束：
     *   1s  ≈ 30% usable_w
     *   60s ≈ 80% usable_w
     */
    lv_coord_t max_bubble_w = usable_w * 80 / 100;  // 80%
    lv_coord_t min_bubble_w = usable_w * 20 / 100;  // 10%

    if (max_bubble_w < min_bubble_w + 10) {
        // 极端小屏幕时兜底，至少保证一定差值
        max_bubble_w = min_bubble_w + 10;
    }

    /* 对 1~60 秒做线性插值：sec = 1 -> min, sec = 60 -> max */
    lv_coord_t w = min_bubble_w;
    if (max_sec > 1) {
        w = min_bubble_w +
            (max_bubble_w - min_bubble_w) * (sec - 1) / (max_sec - 1);
    }

    /* 再保护一次范围 */
    if (w < min_bubble_w)  w = min_bubble_w;
    if (w > max_bubble_w)  w = max_bubble_w;

    return w;
}



/*------------------------------------------------
 *  聊天内容区：新增一条“语音消息”
 *----------------------------------------------*/
/* 仅负责创建 UI，不做绑定；返回 row 对象 */
static lv_obj_t *wechat_add_voice_message_ui(wechat_msg_from_t from,
                                             uint8_t           sec,
                                             lv_obj_t        **out_bubble)

{
    if (!ui_wechatMsgArea) return NULL;
    if (sec == 0) sec = 1;
    if (sec > WECHAT_RECORD_MAX_SEC) sec = WECHAT_RECORD_MAX_SEC;

    int page_w = lv_obj_get_width(ui_wechatPage);
    lv_coord_t bar_w = calc_voice_bar_width(sec, page_w);

    /* 一行容器 */
    lv_obj_t *row = lv_obj_create(ui_wechatMsgArea);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(row, lv_color_hex(0x101018), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row, 0, 0);

    lv_obj_set_style_pad_top(row, 0, 0);
    lv_obj_set_style_pad_bottom(row, 0, 0);
    lv_obj_set_style_pad_left(row, 4, 0);
    lv_obj_set_style_pad_right(row, 4, 0);
    lv_obj_set_style_radius(row, 0, 0);

    lv_obj_set_flex_grow(row, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);

    char sec_txt[8];
    snprintf(sec_txt, sizeof(sec_txt), "%u\"", (unsigned)sec);

    if (from == WEICHAT_MSG_FROM_PEER) {
        /* 对方语音：整行靠左 */
        lv_obj_set_flex_align(row,
                              LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        /* 左侧头像 */
        lv_obj_t *avatar = lv_obj_create(row);
        lv_obj_set_size(avatar, WECHAT_AVATAR_SIZE, WECHAT_AVATAR_SIZE);
        lv_obj_set_style_radius(avatar, 4, 0);
        lv_obj_set_style_bg_color(avatar, lv_color_hex(0x3A6EA5), 0);
        lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(avatar, 0, 0);
        lv_obj_clear_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);

        /* 语音气泡 */
        lv_obj_t *bubble = lv_obj_create(row);
        lv_obj_set_size(bubble, bar_w, WECHAT_VOICE_BUBBLE_H);
        lv_obj_set_style_radius(bubble, 6, 0);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0x2C2C34), 0);
        lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bubble, 0, 0);
        lv_obj_set_style_pad_left(bubble, 6, 0);
        lv_obj_set_style_pad_right(bubble, 6, 0);
        lv_obj_set_style_pad_top(bubble, 4, 0);
        lv_obj_set_style_pad_bottom(bubble, 4, 0);
        lv_obj_set_style_clip_corner(bubble, true, 0);

        lv_obj_t *len_label = lv_label_create(bubble);
        lv_label_set_text(len_label, sec_txt);
        lv_obj_set_style_text_color(len_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(len_label, LV_ALIGN_RIGHT_MID, -2, 0);
		if (out_bubble) *out_bubble = bubble;

    } else {
        /* 自己语音：整行靠右 */
        lv_obj_set_flex_align(row,
                              LV_FLEX_ALIGN_END,
                              LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);

        lv_obj_t *bubble = lv_obj_create(row);
        lv_obj_set_size(bubble, bar_w, WECHAT_VOICE_BUBBLE_H);
        lv_obj_set_style_radius(bubble, 6, 0);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0x4AA1FF), 0);
        lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bubble, 0, 0);
        lv_obj_set_style_pad_left(bubble, 6, 0);
        lv_obj_set_style_pad_right(bubble, 6, 0);
        lv_obj_set_style_pad_top(bubble, 4, 0);
        lv_obj_set_style_pad_bottom(bubble, 4, 0);
        lv_obj_set_style_clip_corner(bubble, true, 0);

        lv_obj_t *len_label = lv_label_create(bubble);
        lv_label_set_text(len_label, sec_txt);
        lv_obj_set_style_text_color(len_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(len_label, LV_ALIGN_LEFT_MID, 2, 0);
		
		if (out_bubble) *out_bubble = bubble;
		
        lv_obj_t *avatar = lv_obj_create(row);
        lv_obj_set_size(avatar, WECHAT_AVATAR_SIZE, WECHAT_AVATAR_SIZE);
        lv_obj_set_style_radius(avatar, 4, 0);
        lv_obj_set_style_bg_color(avatar, lv_color_hex(0xF5A623), 0);
        lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(avatar, 0, 0);
        lv_obj_clear_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* 滚动到底部 */
    lv_obj_scroll_to_view(row, LV_ANIM_OFF);
    
	if (out_bubble && *out_bubble == NULL) {
        // 兜底：理论不会进来
        printf("[wechat] warn: voice bubble ptr is NULL\n");
    }
    return row;
}

/* 老接口：仅创建 UI，不做绑定 */
static void wechat_add_voice_message(wechat_msg_from_t from, uint8_t sec)
{
    (void)wechat_add_voice_message_ui(from, sec, NULL);
}


/* 点击语音气泡时的事件回调 */
static void wechat_voice_bubble_event_cb(lv_event_t *e)
{
    wechat_msg_slot_t *slot =
        (wechat_msg_slot_t *)lv_event_get_user_data(e);

    if (!slot) return;

    printf("[wechat] voice bubble clicked: msg_id=%u, sec=%u, file=%s\r\n",
           slot->msg_id, slot->sec, slot->wav_path);

    // TODO: 播放接口
    // wechat_voice_play_from_wav(slot->wav_path);
}


/* 创建“带绑定”的语音气泡：UI + 填充绑定表 + 安装事件回调 */
/* 对外接口：创建“带绑定信息”的语音气泡 */
void wechat_add_voice_message_bound(wechat_msg_from_t from,
                                    uint8_t           sec,
                                    uint16_t          msg_id,
                                    const char       *wav_path)
{
    lv_obj_t *bubble = NULL;
    lv_obj_t *row = wechat_add_voice_message_ui(from, sec, &bubble);
    if (!row || !bubble) return;

    wechat_msg_slot_t *slot = wechat_msg_alloc();
    slot->row    = row;
    slot->bubble = bubble;
    slot->type   = WECHAT_MSG_TYPE_VOICE;
    slot->from   = from;

    slot->msg_id = msg_id;
    slot->sec    = sec;

    if (wav_path) {
        os_strncpy(slot->wav_path, wav_path, sizeof(slot->wav_path) - 1);
        slot->wav_path[sizeof(slot->wav_path) - 1] = '\0';
    } else {
        slot->wav_path[0] = '\0';
    }

    /* 行 row 可点击：后面也可以用点击事件来播放 */
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, wechat_voice_bubble_event_cb, LV_EVENT_CLICKED, (void *)slot);

    /* 新增消息后刷新选中样式 */
    wechat_update_msg_focus_style();
}



/*------------------------------------------------
 *  录音 UI：浮层 + 进度条 + 秒数
 *----------------------------------------------*/

/* 录音进度定时器回调（LVGL 定时器） */
static void wechat_record_timer_cb(lv_timer_t *timer)
{
    if (!wechat_recording) return;
    if (!ui_record_panel)  return;

    uint32 now     = os_jiffies();
    uint32 diff_ms = now - wechat_record_start_ms;

    /* ① 用 0.02 秒一个刻度更新圆圈（更快更顺） */
    const uint32 step_ms   = 20;                                   // 20ms 一步
    uint32 units           = diff_ms / step_ms;                    // 0.02s 为单位
    uint32 max_units       = WECHAT_RECORD_MAX_SEC * (1000/step_ms); // 60s → 60*50 = 3000

    if (units > max_units) units = max_units;

    if (ui_record_arc) {
        lv_arc_set_value(ui_record_arc, (int)units);
    }

    /* ② 秒数还是按 1s 统计，用来做超时保护 */
    uint32 sec = diff_ms / 1000;
    if (sec > WECHAT_RECORD_MAX_SEC) sec = WECHAT_RECORD_MAX_SEC;

    if (sec != wechat_record_sec) {
        wechat_record_sec = sec;
        // 如果以后想加数字显示，也可以在这里更新 label
    }

    /* ③ 超时自动停止录音 */
    if (wechat_record_sec >= WECHAT_RECORD_MAX_SEC) {
        wechat_recording = 0;
        if (ui_record_timer) lv_timer_pause(ui_record_timer);
        if (ui_record_panel) lv_obj_add_flag(ui_record_panel, LV_OBJ_FLAG_HIDDEN);

        wechat_add_voice_message(WEICHAT_MSG_FROM_ME,
                                 (uint8_t)wechat_record_sec);
    }
}





/* 开始录音：在 KEY_CALL 长按按下的时候调用 */
static void wechat_record_ui_start(void)
{
    if (wechat_recording) return;  // 已经在录了

    wechat_recording       = 1;
    wechat_record_start_ms = os_jiffies();
    wechat_record_sec      = 0;

    if (!ui_record_panel) {
		/* 1. 全屏透明容器，只用来居中圆圈 */
		ui_record_panel = lv_obj_create(ui_wechatPage);
		lv_obj_set_size(ui_record_panel,
						lv_obj_get_width(ui_wechatPage),
						lv_obj_get_height(ui_wechatPage));
		lv_obj_align(ui_record_panel, LV_ALIGN_CENTER, 0, 0);

		lv_obj_clear_flag(ui_record_panel, LV_OBJ_FLAG_SCROLLABLE);
		/* 半透明黑色遮罩，让录音状态更突出 */
		lv_obj_set_style_bg_color(ui_record_panel, lv_color_hex(0x000000), 0);
		lv_obj_set_style_bg_opa(ui_record_panel, LV_OPA_60, 0);   // 60% 透明度
		lv_obj_set_style_border_width(ui_record_panel, 0, 0);
		lv_obj_set_style_pad_all(ui_record_panel, 0, 0);

		/* 2. 圆形进度条本体 */
		ui_record_arc = lv_arc_create(ui_record_panel);
		lv_obj_set_size(ui_record_arc, 72, 72);  // 大小你可以再微调
		lv_obj_align(ui_record_arc, LV_ALIGN_CENTER, 0, 20);  // 往下 20 像素

		lv_arc_set_bg_angles(ui_record_arc, 0, 360);
		lv_arc_set_rotation(ui_record_arc, 270);   // 从顶部开始

		/* 用 0.1 秒为单位：范围 0 ～ 60*10 = 600 */
		lv_arc_set_range(ui_record_arc, 0, WECHAT_RECORD_MAX_SEC * (1000/20));
		lv_arc_set_value(ui_record_arc, 0);

		/* 背景弧隐藏 */
		lv_obj_set_style_arc_opa(ui_record_arc, LV_OPA_0, LV_PART_MAIN);

		/* 指示弧改成细红圈，更突出录音 */
		lv_obj_set_style_arc_width(ui_record_arc, 4, LV_PART_INDICATOR);
		lv_obj_set_style_arc_color(ui_record_arc, lv_color_hex(0xFF4444), LV_PART_INDICATOR);

		/* 去掉中间小球 */
		lv_obj_set_style_opa(ui_record_arc, LV_OPA_0, LV_PART_KNOB);
		lv_obj_set_style_bg_opa(ui_record_arc, LV_OPA_0, LV_PART_KNOB);
		lv_obj_set_style_border_width(ui_record_arc, 0, LV_PART_KNOB);

		/* 不可点击 */
		lv_obj_clear_flag(ui_record_arc, LV_OBJ_FLAG_CLICKABLE);

    }

    /* 显示浮层并重置进度 */
    lv_obj_clear_flag(ui_record_panel, LV_OBJ_FLAG_HIDDEN);
    if (ui_record_arc) lv_arc_set_value(ui_record_arc, 0);

    /* 创建/恢复定时器 */
    if (!ui_record_timer) {
        ui_record_timer = lv_timer_create(wechat_record_timer_cb, 20, NULL);
    } else {
        lv_timer_resume(ui_record_timer);
    }

//	wechat_audio_record_start();
    printf("[wechat] record start\n");
}



/* 结束录音并插入语音消息：KEY_CALL 长按释放时调用 */
static void wechat_record_ui_stop_and_commit(void)
{
    if (!wechat_recording) return;

    wechat_recording = 0;
    if (ui_record_timer) {
        lv_timer_pause(ui_record_timer);
    }

    if (ui_record_panel) {
        lv_obj_add_flag(ui_record_panel, LV_OBJ_FLAG_HIDDEN);
    }

    printf("[wechat] record ui stop, sec=%u\n", (unsigned)wechat_record_sec);
}




/* 焦点/非焦点样式更新函数（底部 5 个按钮） */
void wechat_update_focus_style(void)
{
    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn   = s_wechat_btns[i];
        lv_obj_t *label = s_wechat_btn_labels[i];
        if (!btn || !label) continue;

        if (i == s_wechat_focus_idx) {
            /* 焦点：白底 / 橙边 / 图标黑色 */
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_bg_opa(btn,  LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(btn, 2, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0xFCA702), 0);
            lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0);
        } else {
            /* 非焦点：深灰背景 / 无边框 / 白色图标 */
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
            lv_obj_set_style_bg_opa(btn,  LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(btn, 0, 0);
            lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        }
    }
}

/* WiFi 状态更新：connected=1 红点；connected=0 灰点 */
void wechat_update_wifi_status(uint8_t connected)
{
    if (!ui_wechatWifiDot) return;

    lv_color_t c = connected
                   ? lv_color_hex(0xFF4444)   // 红色：已连接
                   : lv_color_hex(0x666666);  // 灰色：未连接

    lv_obj_set_style_bg_color(ui_wechatWifiDot, c, 0);
}

/* 电量更新：0~100 -> 只更新电池图标（3 档） */
void wechat_update_batt_percent(int percent)
{
    if (!ui_wechatBattImg) {
        return; // UI 还没创建好
    }

    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;

    /* 3 档图标：0、1、2 */
    uint8_t level = 0;  // 对应 ui_imgset_iconBat[0..2]

    if (percent > 66)
        level = 2;      // 高电量
    else if (percent > 33)
        level = 1;      // 中电量
    else
        level = 0;      // 低电量

    lv_img_set_src(ui_wechatBattImg, ui_imgset_iconBat[level]);
}

/* 记录是谁打开了相机 / 相册 */
void start_camera_from(uint8_t from_page)
{
    g_camera_from_page = from_page; // 记录是谁打开的相机
    lv_page_select(PAGE_CAMERA);    // 当前页面跳转到相机
}

void start_img_from(uint8_t from_page)
{
    g_img_from_page = from_page;    // 记录是谁打开的相册
    lv_page_select(PAGE_ALBUM);
}

/*------------------------------------------------
 *  聊天内容区：新增一条“表情消息”
 *----------------------------------------------*/
/* 在聊天内容区域新增一条“表情消息”
 * from = WEICHAT_MSG_FROM_PEER：左侧；WEICHAT_MSG_FROM_ME：右侧
 * emoji_text = 要显示的表情字符串，比如 "😊"
 */
static void wechat_add_emoji_message(wechat_msg_from_t from,
                                     uint16_t          emoji_id,
                                     const char       *emoji_text)
{
    if (!ui_wechatMsgArea || !emoji_text) return;

    /* 整行容器 */
    lv_obj_t *row = lv_obj_create(ui_wechatMsgArea);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(row, lv_color_hex(0x101018), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row, 0, 0);

    lv_obj_set_style_pad_top(row, 0, 0);
    lv_obj_set_style_pad_bottom(row, 0, 0);
    lv_obj_set_style_pad_left(row, 4, 0);
    lv_obj_set_style_pad_right(row, 4, 0);
    lv_obj_set_style_radius(row, 0, 0);

    lv_obj_set_flex_grow(row, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);

    if (from == WEICHAT_MSG_FROM_PEER) {
        /* 对方消息：整行靠左 */
        lv_obj_set_flex_align(row,
                              LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);

        /* 左侧头像 */
        lv_obj_t *avatar = lv_obj_create(row);
        lv_obj_set_size(avatar, WECHAT_AVATAR_SIZE, WECHAT_AVATAR_SIZE);
        lv_obj_set_style_radius(avatar, 4, 0);
        lv_obj_set_style_bg_color(avatar, lv_color_hex(0x3A6EA5), 0);
        lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(avatar, 0, 0);
        lv_obj_clear_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);

        /* 表情气泡 */
        lv_obj_t *bubble = lv_obj_create(row);
        lv_obj_set_height(bubble, WECHAT_EMOJI_BUBBLE_H);
        lv_obj_set_width(bubble, LV_SIZE_CONTENT);
        lv_obj_set_style_radius(bubble, 6, 0);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0x2C2C34), 0);
        lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bubble, 0, 0);
        lv_obj_set_style_pad_left(bubble, 8, 0);
        lv_obj_set_style_pad_right(bubble, 8, 0);
        lv_obj_set_style_pad_top(bubble, 4, 0);
        lv_obj_set_style_pad_bottom(bubble, 4, 0);
        lv_obj_set_style_clip_corner(bubble, true, 0);

        lv_obj_t *label = lv_label_create(bubble);
        lv_label_set_text(label, emoji_text);
        lv_obj_center(label);
		
		// 填通用消息槽
        wechat_msg_slot_t *slot = wechat_msg_alloc();
        slot->row      = row;
        slot->bubble   = bubble;
        slot->type     = WECHAT_MSG_TYPE_EMOJI;
        slot->from     = from;
        slot->emoji_id = emoji_id;
    } else {
        /* 自己消息：整行靠右 */
        lv_obj_set_flex_align(row,
                              LV_FLEX_ALIGN_END,
                              LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);

        /* 先气泡、再头像（跟右侧语音风格一致） */
        lv_obj_t *bubble = lv_obj_create(row);
        lv_obj_set_height(bubble, WECHAT_EMOJI_BUBBLE_H);
        lv_obj_set_width(bubble, LV_SIZE_CONTENT);
        lv_obj_set_style_radius(bubble, 6, 0);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0x4AA1FF), 0);
        lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bubble, 0, 0);
        lv_obj_set_style_pad_left(bubble, 8, 0);
        lv_obj_set_style_pad_right(bubble, 8, 0);
        lv_obj_set_style_pad_top(bubble, 4, 0);
        lv_obj_set_style_pad_bottom(bubble, 4, 0);
        lv_obj_set_style_clip_corner(bubble, true, 0);

        lv_obj_t *label = lv_label_create(bubble);
        lv_label_set_text(label, emoji_text);
        lv_obj_center(label);

        lv_obj_t *avatar = lv_obj_create(row);
        lv_obj_set_size(avatar, WECHAT_AVATAR_SIZE, WECHAT_AVATAR_SIZE);
        lv_obj_set_style_radius(avatar, 4, 0);
        lv_obj_set_style_bg_color(avatar, lv_color_hex(0xF5A623), 0);
        lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(avatar, 0, 0);
        lv_obj_clear_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);
		
		wechat_msg_slot_t *slot = wechat_msg_alloc();
        slot->row      = row;
        slot->bubble   = bubble;
        slot->type     = WECHAT_MSG_TYPE_EMOJI;
        slot->from     = from;
        slot->emoji_id = emoji_id;
    }

    /* 滚动到底部，把新消息露出来 */
    lv_obj_scroll_to_view(row, LV_ANIM_OFF);
}

/*------------------------------------------------
 *  表情面板相关实现
 *----------------------------------------------*/

/* 更新表情面板中某个按钮的选中样式 */
static void emoji_update_focus_style(void)
{
    for (int i = 0; i < EMOJI_COUNT; i++) {
        lv_obj_t *btn = s_emoji_btns[i];
        if (!btn) continue;

        if (i == s_emoji_focus_idx) {
            /* 选中：亮一点 + 边框 */
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xFCA702), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(btn, 2, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), 0);
        } else {
            /* 未选中：暗灰色，无边框 */
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(btn, 0, 0);
        }
    }
}

/* 当用户在面板中按下“确定”选中某个表情时的处理 */
static void emoji_panel_on_select(uint8_t idx)
{
    if (idx >= EMOJI_COUNT) return;

    const char *emoji = s_emoji_texts[idx];

    printf("[wechat] emoji selected: %s (idx=%d)\n", emoji, idx);

    /* 1) 通过 UDP 发出去：这里的 idx 就是协议里的 emoji_id */
    if (wechat_emoji_send((uint16_t)idx) == 0) {
        printf("[wechat] emoji_send ok, id=%u\n", (unsigned)idx);
    } else {
        printf("[wechat] emoji_send FAIL, id=%u\n", (unsigned)idx);
    }

    /* 2) 本机 UI 上直接插一条“我发的表情消息” */
    wechat_add_emoji_message(WEICHAT_MSG_FROM_ME, (uint16_t)idx, emoji);

    /* 3) 关闭弹窗 */
    wechat_close_emoji_panel();
}


/* 关闭表情面板 */
static void wechat_close_emoji_panel(void)
{
    if (!ui_emojiPanel) return;

    lv_obj_add_flag(ui_emojiPanel, LV_OBJ_FLAG_HIDDEN);
    s_emoji_panel_visible = 0;
}

/* 打开 / 创建 表情面板 */
static void wechat_open_emoji_panel(void)
{
    if (s_emoji_panel_visible) {
        /* 已经打开了，就不重复创建，可以改成切换逻辑 */
        return;
    }

    if (!ui_wechatPage) {
        printf("[wechat] ui_wechatPage not ready, can't open emoji panel\n");
        return;
    }

    int page_w = lv_obj_get_width(ui_wechatPage);
    int page_h = lv_obj_get_height(ui_wechatPage);

    if (!ui_emojiPanel) {
        /* 第一次创建表情面板 */
        ui_emojiPanel = lv_obj_create(ui_wechatPage);
        lv_obj_set_size(ui_emojiPanel, page_w * 4 / 5, page_h * 3 / 5);
        lv_obj_align(ui_emojiPanel, LV_ALIGN_CENTER, 0, 0);

        lv_obj_clear_flag(ui_emojiPanel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(ui_emojiPanel, 8, 0);
        lv_obj_set_style_bg_color(ui_emojiPanel, lv_color_hex(0x202020), 0);
        lv_obj_set_style_bg_opa(ui_emojiPanel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(ui_emojiPanel, 2, 0);
        lv_obj_set_style_border_color(ui_emojiPanel, lv_color_hex(0xFCA702), 0);
        lv_obj_set_style_pad_all(ui_emojiPanel, 6, 0);

        /* 顶部标题 */
        lv_obj_t *title = lv_label_create(ui_emojiPanel);
        lv_label_set_text(title, "选择表情");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

        /* 表情区域容器 */
        lv_obj_t *emoji_cont = lv_obj_create(ui_emojiPanel);
        lv_obj_set_size(emoji_cont, lv_pct(100), lv_pct(100));
        lv_obj_align(emoji_cont, LV_ALIGN_BOTTOM_MID, 0, -4);

        lv_obj_clear_flag(emoji_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(emoji_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(emoji_cont, 0, 0);
        lv_obj_set_style_pad_all(emoji_cont, 4, 0);

        lv_obj_set_flex_flow(emoji_cont, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_align(emoji_cont,
                              LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER);

        /* 创建若干个按钮，每个按钮显示一个 emoji */
        int btn_size = 32;

        for (int i = 0; i < EMOJI_COUNT; i++) {
            lv_obj_t *btn = lv_btn_create(emoji_cont);
            lv_obj_set_size(btn, btn_size, btn_size);
            lv_obj_set_style_radius(btn, btn_size / 2, 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(btn, 0, 0);

            lv_obj_t *label = lv_label_create(btn);
            lv_label_set_text(label, s_emoji_texts[i]);
            lv_obj_center(label);

            s_emoji_btns[i] = btn;
        }
    }

    /* 面板置顶 + 显示 */
    lv_obj_move_foreground(ui_emojiPanel);
    lv_obj_clear_flag(ui_emojiPanel, LV_OBJ_FLAG_HIDDEN);

    s_emoji_panel_visible = 1;
    s_emoji_focus_idx = 0;
    emoji_update_focus_style();

    printf("[wechat] emoji panel opened\n");
}

/*------------------------------------------------
 *  按键事件处理
 *----------------------------------------------*/
 
void ui_event_wechatPage(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    uint32_t *key_val = (uint32_t *)e->param;

    if (event_code != USER_KEY_EVENT || key_val == NULL)
        return;

    /* 表情面板打开时，优先处理它的按键 */
    if (s_emoji_panel_visible) {
        switch (*key_val)
        {
            case AD_LEFT:
                if (s_emoji_focus_idx > 0) {
                    s_emoji_focus_idx--;
                    emoji_update_focus_style();
                }
                return;

            case AD_RIGHT:
                if (s_emoji_focus_idx + 1 < EMOJI_COUNT) {
                    s_emoji_focus_idx++;
                    emoji_update_focus_style();
                }
                return;

            case AD_PRESS:   /* OK 选中当前表情 */
                emoji_panel_on_select(s_emoji_focus_idx);
                return;

            case AD_BACK:    /* 返回键：关闭表情面板 */
                wechat_close_emoji_panel();
                return;

            default:
                return;
        }
    }

    /* 表情面板没打开时，走主逻辑 */
    switch (*key_val)
    {
        case AD_LEFT:
            if (s_wechat_focus_mode == WECHAT_FOCUS_BOTTOM) {
                if (s_wechat_focus_idx == 0)
                    s_wechat_focus_idx = 4;
                else
                    s_wechat_focus_idx--;
                wechat_update_focus_style();
            } else {
                /* 消息模式：向前选上一条消息（语音 or 表情） */
                if (s_msg_cnt > 0 && s_wechat_msg_focus_idx > 0) {
                    s_wechat_msg_focus_idx--;
                    wechat_update_msg_focus_style();

                    wechat_msg_slot_t *slot =
                        &s_msg_slots[s_wechat_msg_focus_idx];
                    if (slot->row) lv_obj_scroll_to_view(slot->row, LV_ANIM_OFF);
                }
            }
            break;

        case AD_RIGHT:
            if (s_wechat_focus_mode == WECHAT_FOCUS_BOTTOM) {
                s_wechat_focus_idx = (s_wechat_focus_idx + 1) % 5;
                wechat_update_focus_style();
            } else {
                /* 消息模式：向后选下一条消息（语音 or 表情） */
                if (s_msg_cnt > 0 &&
                    s_wechat_msg_focus_idx + 1 < (int16_t)s_msg_cnt) {

                    s_wechat_msg_focus_idx++;
                    wechat_update_msg_focus_style();
                    wechat_msg_slot_t *slot =
                        &s_msg_slots[s_wechat_msg_focus_idx];
                    if (slot->row) lv_obj_scroll_to_view(slot->row, LV_ANIM_OFF);
                }
            }
            break;

        case AD_VOL_UP:
            /* TODO: 音量+ */
            break;

        case AD_VOL_DOWN:
            /* TODO: 音量- */
            break;

        case AD_BACK:
            if (s_wechat_focus_mode == WECHAT_FOCUS_MSG) {
                /* 从消息模式退回到底部按钮模式 */
                s_wechat_focus_mode    = WECHAT_FOCUS_BOTTOM;
                s_wechat_msg_focus_idx = -1;
                wechat_update_msg_focus_style();
                wechat_update_focus_style();
                printf("[wechat] exit msg focus mode, back to bottom bar\n");
            } else {
                printf("[wechat] back to HOME\n");
                lv_page_select(PAGE_HOME);
                camera_gvar.pagebtn_index = 0;
            }
            break;

        case AD_PRESS:   /* OK 键 */
            switch (s_wechat_focus_idx)
            {
                case 0: // 视频通话
                    printf("[wechat] video button OK\n");
                    lv_page_select(PAGE_INTERCOM);
                    break;

                case 1: // 语音按钮：在底部模式下，OK = 进入消息模式；在消息模式下，OK = 执行当前消息操作
                    if (s_wechat_focus_mode == WECHAT_FOCUS_BOTTOM) {
                        if (s_msg_cnt > 0) {
                            s_wechat_focus_mode    = WECHAT_FOCUS_MSG;
                            s_wechat_msg_focus_idx = (int16_t)(s_msg_cnt - 1);
                            wechat_update_msg_focus_style();

                            wechat_msg_slot_t *slot =
                                &s_msg_slots[s_wechat_msg_focus_idx];
                            if (slot->row) {
                                lv_obj_scroll_to_view(slot->row, LV_ANIM_OFF);
                            }
                            printf("[wechat] enter msg focus mode, idx=%d\n",
                                   (int)s_wechat_msg_focus_idx);
                        } else {
                            printf("[wechat] no messages yet\n");
                        }
                    } else {
                        if (s_msg_cnt > 0 &&
                            s_wechat_msg_focus_idx >= 0 &&
                            s_wechat_msg_focus_idx < (int16_t)s_msg_cnt) {

                            wechat_msg_slot_t *slot =
                                &s_msg_slots[s_wechat_msg_focus_idx];

                            if (slot->type == WECHAT_MSG_TYPE_VOICE) {
                                printf("[wechat] play selected voice: msg_id=%u, sec=%u, file=%s\n",
                                       slot->msg_id, slot->sec, slot->wav_path);
                                /* TODO: 真正播放接口 */
                                // wechat_voice_play_by_msg_id(slot->msg_id);
                                // wechat_voice_play_from_wav(slot->wav_path);
                            } else if (slot->type == WECHAT_MSG_TYPE_EMOJI) {
                                printf("[wechat] selected emoji: id=%u from=%d\n",
                                       slot->emoji_id, slot->from);
                                /* TODO: 以后实现“表情放大查看”就在这里 */
                            }
                        } else {
                            printf("[wechat] no valid selected msg idx=%d\n",
                                   (int)s_wechat_msg_focus_idx);
                        }
                    }
                    break;

                case 2: // 表情
                    printf("[wechat] emoji panel\n");
                    wechat_open_emoji_panel();
                    break;

                case 3: // 拍照发送
                    printf("[wechat] capture & send photo\n");
                    start_camera_from(PAGE_WECHAT);
                    break;

                case 4: // 相册图片
                    printf("[wechat] open photo selector\n");
                    start_img_from(PAGE_WECHAT);
                    break;

                default:
                    break;
            }
            break;

        case UI_KEY_CALL_LONG_DOWN:
            /* 只有在“语音按钮”被选中的时候，才开始录音 */
            if (s_wechat_focus_mode == WECHAT_FOCUS_BOTTOM &&
                s_wechat_focus_idx == 1) {
                wechat_record_ui_start();
            }
            break;

        case UI_KEY_CALL_LONG_UP:
            if (s_wechat_focus_mode == WECHAT_FOCUS_BOTTOM &&
                s_wechat_focus_idx == 1) {
                wechat_record_ui_stop_and_commit();
            }
            break;

        default:
            break;
    }
}

 
//void ui_event_wechatPage(lv_event_t *e)
//{
//    lv_event_code_t event_code = lv_event_get_code(e);
//    uint32_t *key_val = (uint32_t *)e->param;
//
//    if (event_code != USER_KEY_EVENT || key_val == NULL)
//        return;
//
//    /* 表情面板打开时，优先处理它的按键 */
//    if (s_emoji_panel_visible) {
//        switch (*key_val)
//        {
//            case AD_LEFT:
//                /* 向左移动选中的表情，不循环，到最左就停住 */
//                if (s_emoji_focus_idx > 0) {
//                    s_emoji_focus_idx--;
//                    emoji_update_focus_style();
//                }
//                return;
//
//            case AD_RIGHT:
//                /* 向右移动选中的表情，不循环，到最右就停住 */
//                if (s_emoji_focus_idx + 1 < EMOJI_COUNT) {
//                    s_emoji_focus_idx++;
//                    emoji_update_focus_style();
//                }
//                return;
//
//            case AD_PRESS:   /* 在表情面板里按 OK：选中当前表情 */
//                emoji_panel_on_select(s_emoji_focus_idx);
//                return;
//
//            case AD_BACK:    /* 返回键：关闭表情面板，回到微聊主界面 */
//                wechat_close_emoji_panel();
//                return;
//
//            default:
//                return;  /* 其他按键在面板打开时忽略 */
//        }
//    }
//
//    /* 表情面板没打开时，走原来的底部 5 个按钮逻辑 */
//    switch (*key_val)
//    {
//        case AD_LEFT:   /* 左键：焦点向左循环 */
//            if (s_wechat_focus_idx == 0)
//                s_wechat_focus_idx = 4;
//            else
//                s_wechat_focus_idx--;
//            wechat_update_focus_style();
//            break;
//
//        case AD_RIGHT:  /* 右键：焦点向右循环 */
//            s_wechat_focus_idx = (s_wechat_focus_idx + 1) % 5;
//            wechat_update_focus_style();
//            break;
//
//        case AD_VOL_UP:
//            break;
//
//        case AD_VOL_DOWN:
//            break;
//
//        case AD_BACK:
//            printf("[wechat] back to HOME\n");
//            lv_page_select(PAGE_HOME);
//            camera_gvar.pagebtn_index = 0;
//            break;
//
//        case AD_PRESS:   /* OK 键 */
//            switch (s_wechat_focus_idx)
//            {
//                case 0: // 视频通话
//                    printf("[wechat] video button OK\n");
//                    lv_page_select(PAGE_INTERCOM);
//                    break;
//
//                case 1: // 语音对讲
//                    break;
//
//                case 2: // 表情
//                    printf("[wechat] emoji panel\n");
//                    wechat_open_emoji_panel();   // 焦点在表情按钮时才会弹出
//                    break;
//
//                case 3: // 拍照发送
//                    printf("[wechat] capture & send photo\n");
//                    start_camera_from(PAGE_WECHAT);
//                    break;
//
//                case 4: // 相册图片
//                    printf("[wechat] open photo selector\n");
//                    start_img_from(PAGE_WECHAT);
//                    break;
//
//                default:
//                    break;
//            }
//            break;
//        /* 发射键长按逻辑 */
//        case UI_KEY_CALL_LONG_DOWN:
//            /* 只有在“语音按钮”被选中的时候，才开始录音 */
//            if (s_wechat_focus_idx == 1) {
//                wechat_record_ui_start();
//			
//            }
//            break;
//
//        case UI_KEY_CALL_LONG_UP:
//			os_sleep_ms(1000);
//            if (s_wechat_focus_idx == 1) {
//				
//                wechat_record_ui_stop_and_commit();
//				
//            }
//            break;
//		
//        default:
//            break;
//    }
//}

/*------------------------------------------------
 *  页面初始化
 *----------------------------------------------*/
void ui_wechatPage2_screen_init(void)
{
    /* 根据当前屏幕尺寸布局，避免和 SCALE_* 不一致 */
    lv_obj_t *scr = lv_scr_act();
    int page_w = lv_obj_get_width(scr);
    int page_h = lv_obj_get_height(scr);

    const int status_bar_h = 16;  /* 最上方状态栏高度 */
    const int top_bar_h    = 24;  /* 标题栏 */
    const int bottom_bar_h = 44;  /* 底栏稍微高一点，放 36x36 的按钮 */

    /* 根页面 */
    ui_wechatPage = lv_obj_create(scr);
    curPage_obj   = ui_wechatPage;

    camera_gvar.page_cur = PAGE_WECHAT;

    lv_obj_set_size(ui_wechatPage, page_w, page_h);
    lv_obj_clear_flag(ui_wechatPage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(ui_wechatPage, 0, 0);
    lv_obj_set_style_border_width(ui_wechatPage, 0, 0);
    lv_obj_set_style_bg_color(ui_wechatPage, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ui_wechatPage, LV_OPA_COVER, 0);

    /* 事件：让本页自己处理按键（event_handler 内会调用 ui_event_wechatPage） */
    lv_obj_add_event_cb(ui_wechatPage, event_handler, LV_EVENT_ALL, NULL);

    /* 区域0：最上面的状态栏（WiFi 状态 + 电池图标 + 文本）*/
    ui_wechatStatusBar = lv_obj_create(ui_wechatPage);
    lv_obj_set_size(ui_wechatStatusBar, page_w, status_bar_h);
    lv_obj_align(ui_wechatStatusBar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_clear_flag(ui_wechatStatusBar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_wechatStatusBar, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(ui_wechatStatusBar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ui_wechatStatusBar, 0, 0);
    lv_obj_set_style_pad_all(ui_wechatStatusBar, 0, 0);
    lv_obj_set_style_radius(ui_wechatStatusBar, 0, 0);

    /* 底部分隔线 */
    lv_obj_set_style_border_side(ui_wechatStatusBar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(ui_wechatStatusBar, 1, 0);
    lv_obj_set_style_border_color(ui_wechatStatusBar, lv_color_hex(0x222222), 0);

    /* 左侧 WiFi 圆点 */
    ui_wechatWifiDot = lv_obj_create(ui_wechatStatusBar);
    lv_obj_set_size(ui_wechatWifiDot, 10, 10);
    lv_obj_align(ui_wechatWifiDot, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_set_style_radius(ui_wechatWifiDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ui_wechatWifiDot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ui_wechatWifiDot, 0, 0);

    /* 默认灰色：未连接 */
    lv_obj_set_style_bg_color(ui_wechatWifiDot, lv_color_hex(0x666666), 0);

    /* 右侧：电池图标 */
    lv_obj_t *batt_container = lv_obj_create(ui_wechatStatusBar);
    lv_obj_set_size(batt_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(batt_container, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_clear_flag(batt_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(batt_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(batt_container, 0, 0);
    lv_obj_set_style_pad_all(batt_container, 0, 0);
    lv_obj_set_style_radius(batt_container, 0, 0);

    ui_wechatBattImg = lv_img_create(batt_container);
    lv_img_set_src(ui_wechatBattImg, ui_imgset_iconBat[get_batlevel()]);
    lv_obj_set_width(ui_wechatBattImg, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wechatBattImg, LV_SIZE_CONTENT);
    lv_obj_align(ui_wechatBattImg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(ui_wechatBattImg, LV_OBJ_FLAG_SCROLLABLE);

    /* 初始化刷新一次 */
    wechat_update_wifi_status(0);
    // wechat_update_batt_percent(get_batlevel() * 50);

    /* 区域1：顶部栏：标题 “WeChat”*/
    ui_wechatTopBar = lv_obj_create(ui_wechatPage);
    lv_obj_set_size(ui_wechatTopBar, page_w, top_bar_h);
    lv_obj_align(ui_wechatTopBar, LV_ALIGN_TOP_MID, 0, status_bar_h);
    lv_obj_clear_flag(ui_wechatTopBar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_wechatTopBar, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(ui_wechatTopBar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ui_wechatTopBar, 0, 0);
    lv_obj_set_style_pad_left(ui_wechatTopBar, 4, 0);
    lv_obj_set_style_pad_right(ui_wechatTopBar, 4, 0);
    lv_obj_set_style_pad_top(ui_wechatTopBar, 2, 0);
    lv_obj_set_style_pad_bottom(ui_wechatTopBar, 2, 0);
    lv_obj_set_style_radius(ui_wechatTopBar, 0, 0);

    /* 顶部栏底部分隔线 */
    lv_obj_set_style_border_side(ui_wechatTopBar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(ui_wechatTopBar, 1, 0);
    lv_obj_set_style_border_color(ui_wechatTopBar, lv_color_hex(0x222222), 0);

    lv_obj_t *title_label = lv_label_create(ui_wechatTopBar);
    lv_label_set_text(title_label, "WeChat");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    /* 区域2：中间聊天内容区（可滚动）*/
    ui_wechatMsgArea = lv_obj_create(ui_wechatPage);
    lv_obj_set_width(ui_wechatMsgArea, page_w);

    int msg_h = page_h - status_bar_h - top_bar_h - bottom_bar_h;
    if (msg_h < 40) msg_h = 40;
    lv_obj_set_height(ui_wechatMsgArea, msg_h);

    lv_obj_align(ui_wechatMsgArea, LV_ALIGN_TOP_MID, 0, status_bar_h + top_bar_h);
    lv_obj_set_scroll_dir(ui_wechatMsgArea, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui_wechatMsgArea, LV_SCROLLBAR_MODE_AUTO);

    /* 聊天区背景 */
    lv_obj_set_style_bg_color(ui_wechatMsgArea, lv_color_hex(0x101018), 0);
    lv_obj_set_style_bg_opa(ui_wechatMsgArea, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ui_wechatMsgArea, 0, 0);
    lv_obj_set_style_pad_all(ui_wechatMsgArea, 2, 0);
    lv_obj_set_style_radius(ui_wechatMsgArea, 0, 0);

    lv_obj_set_style_pad_row(ui_wechatMsgArea, 2, 0);
    lv_obj_set_style_pad_column(ui_wechatMsgArea, 0, 0);

    lv_obj_set_flex_flow(ui_wechatMsgArea, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_wechatMsgArea,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
						  
    /* 区域3：底部功能区：5 个等宽按钮 */
    ui_wechatBtmBar = lv_obj_create(ui_wechatPage);
    lv_obj_set_size(ui_wechatBtmBar, page_w, bottom_bar_h);
    lv_obj_align(ui_wechatBtmBar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(ui_wechatBtmBar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_wechatBtmBar, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(ui_wechatBtmBar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ui_wechatBtmBar, 0, 0);
    lv_obj_set_style_pad_left(ui_wechatBtmBar, 4, 0);
    lv_obj_set_style_pad_right(ui_wechatBtmBar, 4, 0);
    lv_obj_set_style_pad_top(ui_wechatBtmBar, 4, 0);
    lv_obj_set_style_pad_bottom(ui_wechatBtmBar, 4, 0);
    lv_obj_set_style_radius(ui_wechatBtmBar, 0, 0);

    lv_obj_set_flex_flow(ui_wechatBtmBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_wechatBtmBar,
                          LV_FLEX_ALIGN_SPACE_AROUND,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    const int btn_sz = 36;

    /* 1. 视频按钮 */
    lv_obj_t *btn_video = lv_btn_create(ui_wechatBtmBar);
    lv_obj_set_size(btn_video, btn_sz, btn_sz);
    lv_obj_set_style_radius(btn_video, btn_sz / 2, 0);
    lv_obj_set_style_bg_color(btn_video, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(btn_video, 0, 0);
    lv_obj_t *label_video = lv_label_create(btn_video);
    lv_label_set_text(label_video, LV_SYMBOL_CALL);
    lv_obj_center(label_video);
    ui_wechatBtnVideo = btn_video;

    /* 2. 语音按钮 */
    lv_obj_t *btn_mic = lv_btn_create(ui_wechatBtmBar);
    lv_obj_set_size(btn_mic, btn_sz, btn_sz);
    lv_obj_set_style_radius(btn_mic, btn_sz / 2, 0);
    lv_obj_set_style_bg_color(btn_mic, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(btn_mic, 0, 0);
    lv_obj_t *label_mic = lv_label_create(btn_mic);
    lv_label_set_text(label_mic, LV_SYMBOL_AUDIO);
    lv_obj_center(label_mic);
    ui_wechatBtnMic = btn_mic;

    /* 3. 表情按钮 */
    lv_obj_t *btn_emoji = lv_btn_create(ui_wechatBtmBar);
    lv_obj_set_size(btn_emoji, btn_sz, btn_sz);
    lv_obj_set_style_radius(btn_emoji, btn_sz / 2, 0);
    lv_obj_set_style_bg_color(btn_emoji, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(btn_emoji, 0, 0);
    lv_obj_t *label_emoji = lv_label_create(btn_emoji);
    lv_label_set_text(label_emoji, "😊");
    lv_obj_center(label_emoji);
    ui_wechatBtnEmoji = btn_emoji;

    /* 4. 拍照按钮 */
    lv_obj_t *btn_camera = lv_btn_create(ui_wechatBtmBar);
    lv_obj_set_size(btn_camera, btn_sz, btn_sz);
    lv_obj_set_style_radius(btn_camera, btn_sz / 2, 0);
    lv_obj_set_style_bg_color(btn_camera, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(btn_camera, 0, 0);
    lv_obj_t *label_camera = lv_label_create(btn_camera);
    lv_label_set_text(label_camera, LV_SYMBOL_IMAGE);
    lv_obj_center(label_camera);
    ui_wechatBtnCamera = btn_camera;

    /* 5. 图片/相册按钮 */
    lv_obj_t *btn_photo = lv_btn_create(ui_wechatBtmBar);
    lv_obj_set_size(btn_photo, btn_sz, btn_sz);
    lv_obj_set_style_radius(btn_photo, btn_sz / 2, 0);
    lv_obj_set_style_bg_color(btn_photo, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(btn_photo, 0, 0);
    lv_obj_t *label_photo = lv_label_create(btn_photo);
    lv_label_set_text(label_photo, LV_SYMBOL_IMAGE);
    lv_obj_center(label_photo);
    ui_wechatBtnPhoto = btn_photo;

    /* 填充数组，给 wechat_update_focus_style 使用 */
    s_wechat_btns[0]       = ui_wechatBtnVideo;
    s_wechat_btns[1]       = ui_wechatBtnMic;
    s_wechat_btns[2]       = ui_wechatBtnEmoji;
    s_wechat_btns[3]       = ui_wechatBtnCamera;
    s_wechat_btns[4]       = ui_wechatBtnPhoto;

    s_wechat_btn_labels[0] = label_video;
    s_wechat_btn_labels[1] = label_mic;
    s_wechat_btn_labels[2] = label_emoji;
    s_wechat_btn_labels[3] = label_camera;
    s_wechat_btn_labels[4] = label_photo;

    /* 默认焦点在“视频通话” */
    s_wechat_focus_idx = 0;
    wechat_update_focus_style();
	
	if (!ui_pending_voice_timer) {
        ui_pending_voice_timer = lv_timer_create(wechat_pending_voice_timer_cb,
                                                 50,   /* 50ms 轮询一次够用了 */
                                                 NULL);
    }
    printf("## ui_wechatPage2_screen_init (status bar + dark theme) done\n");
}
/* 这个函数覆盖 wechat_msg.c 里的 weak 版本，用来在 UI 里显示表情 */
void wechat_on_emoji_packet(uint16_t msg_id,
                            uint16_t seq,
                            uint16_t total,
                            const uint8_t *data,
                            uint32_t len)
{
    if (len < 2 || data == NULL) {
        printf("[wechat] emoji pkt too short, len=%u\n", (unsigned)len);
        return;
    }

    uint16_t emoji_id = (uint16_t)(data[0] | (data[1] << 8));

    printf("[wechat] emoji pkt: msg_id=%u seq=%u/%u emoji_id=%u\n",
           msg_id, seq, total, emoji_id);

    if (emoji_id >= EMOJI_COUNT) {
        printf("[wechat] emoji_id(%u) out of range\n", emoji_id);
        return;
    }

    const char *emoji = s_emoji_texts[emoji_id];

    /* 对方发来的表情 -> 左侧气泡显示 */
    wechat_add_emoji_message(WEICHAT_MSG_FROM_PEER, emoji_id, emoji);
}
