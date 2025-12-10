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
//#include "lib/sdhost/sdhost.h"
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

extern volatile uint8_t bbm_displaydecode_run;
extern Vpp_stream photo_msg;
extern lcd_msg lcd_info;
extern volatile vf_cblk g_vf_cblk;
extern gui_msg gui_cfg;

extern volatile uint8_t itp_finish;
extern uint32_t get_takephoto_thread_status();
extern uint8_t get_bbm_take_photo_status(void);
extern void bbm_take_photo(uint8_t num);
extern int bbm_start_record(void);
extern void bbm_stop_record(void);
extern void client_send_wakeup_cmd(uint8_t cnt);
extern void client_send_sleep_cmd(uint8_t cnt);

extern uint8_t s_wechat_focus_idx ; 
extern void wechat_update_focus_style(void);

uint8_t sundtype =0;
uint8_t cur_volumeSet =0;
#if 0
void create_confirm_dialog(lv_obj_t *parent, lv_event_cb_t confirm_cb, lv_event_cb_t cancel_cb)
{
    /* -------------------- 1. 通用背景遮罩 -------------------- */
    lv_obj_t *bg = lv_obj_create(parent);
    lv_obj_set_size(bg, lv_pct(100), lv_pct(100));  // 占满父对象（屏幕）
    lv_obj_set_style_bg_color(bg, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bg, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);  // 禁止滚动

    /* -------------------- 2. 对话框容器（自适应布局） -------------------- */
    lv_obj_t *dialog = lv_obj_create(bg);
    lv_obj_set_width(dialog, lv_pct(60));  // 父对象宽度的60%（自适应屏幕）
    lv_obj_center(dialog);  // 居中显示
    lv_obj_set_style_bg_color(dialog, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(dialog, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

    /* -------------------- 3. 消息文本（兼容默认字体） -------------------- */
    lv_obj_t *msg = lv_label_create(dialog);
    lv_label_set_text(msg, "对讲已连接，是否确定退出？");
    // 使用 LVGL 默认字体（无需额外加载）
    lv_obj_set_style_text_font(msg, &lv_font_default, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(msg, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);  // 居中对齐
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -20);  // 顶部留白20px

    /* -------------------- 4. 按钮容器（弹性布局） -------------------- */
    lv_obj_t *btn_cont = lv_obj_create(dialog);
    lv_obj_set_size(btn_cont, lv_pct(90), LV_SIZE_CONTENT);  // 对话框宽度的90%
    lv_obj_set_layout(btn_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);  // 水平排列
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY,  // 按钮等间距
                         LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, 20);  // 底部留白20px

    /* -------------------- 5. 取消按钮（通用样式） -------------------- */
    lv_obj_t *btn_cancel = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_cancel, lv_pct(45), 40);  // 按钮容器宽度的45%
    lv_obj_set_style_border_color(btn_cancel, lv_color_hex(0xD9D9D9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn_cancel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn_cancel, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xF5F5F5), LV_PART_MAIN | LV_STATE_PRESSED);  // 按下反馈
    lv_obj_add_event_cb(btn_cancel, cancel_cb, LV_EVENT_CLICKED, dialog);  // 传递对话框对象

    lv_obj_t *label_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(label_cancel, "取消");
    lv_obj_set_style_text_font(label_cancel, &lv_font_default, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_cancel, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label_cancel);

    /* -------------------- 6. 确认按钮（强调样式） -------------------- */
    lv_obj_t *btn_confirm = lv_btn_create(btn_cont);
    lv_obj_set_size(btn_confirm, lv_pct(45), 40);
    lv_obj_set_style_bg_color(btn_confirm, lv_color_hex(0xFF4D4F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_confirm, lv_color_hex(0xCC3D3F), LV_PART_MAIN | LV_STATE_PRESSED);  // 按下反馈
    lv_obj_set_style_radius(btn_confirm, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_confirm, confirm_cb, LV_EVENT_CLICKED, dialog);

    lv_obj_t *label_confirm = lv_label_create(btn_confirm);
    lv_label_set_text(label_confirm, "确定");
    lv_obj_set_style_text_font(label_confirm, &lv_font_default, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_confirm, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label_confirm);
}

/**
 * 确认按钮回调（示例）
 */
static void confirm_event_handler(lv_event_t *e)
{
    lv_obj_t *dialog = lv_event_get_user_data(e);
    lv_obj_t *bg = lv_obj_get_parent(dialog);  // 获取背景遮罩
    printf("执行退出对讲操作\n");
    lv_obj_del(bg);  // 删除整个对话框及遮罩
}

/**
 * 取消按钮回调（示例）
 */
static void cancel_event_handler(lv_event_t *e)
{
    lv_obj_t *dialog = lv_event_get_user_data(e);
    lv_obj_t *bg = lv_obj_get_parent(dialog);
    printf("取消退出操作\n");
    lv_obj_del(bg);  // 删除整个对话框及遮罩
}
void show_dialog_example(void)
{
    create_confirm_dialog(ui_intercomPage, confirm_event_handler, cancel_event_handler);
}
#endif

// static void event_cb(lv_event_t * e)
// {
//     lv_obj_t * obj = lv_event_get_current_target(e);
//     LV_LOG_USER("Button %s clicked", lv_msgbox_get_active_btn_text(obj));
// }
// void lv_example_msgbox_1(void)
// {
//     #if 0
//     static const char * btns[] = {"Apply", "Close", ""};
//     lv_obj_t * mbox1 = lv_msgbox_create(NULL, "Hello", "This is a message box with two buttons.", btns, true);
//     #else
//     static const char * btns[] = {"取消", "确定",""};
//     lv_obj_t * mbox1 = lv_msgbox_create(NULL, "notice", "是否退出？ ", btns, false);
//     #endif

//     lv_obj_add_event_cb(mbox1, event_cb, LV_EVENT_VALUE_CHANGED, NULL);
//     lv_obj_center(mbox1);
// }




volatile uint8_t p0p1_switch_flag=0;

uint8_t viewSwitch_flag=1;
uint8_t un_viewSwitch_flag;
volatile  lcd_delay_time=0;

void delay_open_lcd_flash(uint8_t dms)
{
	struct lcdc_device *lcd_dev;
	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	//lcdc_video_enable_auto_ks(lcd_dev,1);
	lcd_delay_time=dms;
	os_sleep_ms(10);
}
void delay_open_lcd_process(void)
{
	struct lcdc_device *lcd_dev;
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	

	if(lcd_delay_time)
	{
		lcd_delay_time--;
		printf("## lcd_delay_time ==%d \n",lcd_delay_time);
		if(lcd_delay_time==0)
			lcdc_set_start_run(lcd_dev);

			//lcdc_video_enable_auto_ks(lcd_dev,0);
	}
}



void un_view_switch(uint8_t p0p1_flag)
{
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	bbm_displaydecode_run =0; 
	printf("## *** un_view_switch init ****  un_viewSwitch_flag_flag ==%d \r\n",p0p1_flag);
	switch (p0p1_flag)
	{
			case 0 :  //P1__bg  P0_video
						p0p1_switch_flag=0;
						os_sleep_ms(10);
						delay_open_lcd_flash(4);
						lcd_info.lcd_p0p1_state = 3;	
						video_decode_mem  = video_decode_config_mem;
						video_decode_mem1 = video_decode_config_mem1;
						video_decode_mem2 = video_decode_config_mem2;
						lcdc_set_rotate_p0p1_start_location(lcd_dev,(SCALE_WIDTH-128-6),6,0,0);

						jpg_dec_scale_del();						
						set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
						// lcdc_set_video_en(lcd_dev,0);
						jpg_decode_scale_config((uint32)video_decode_mem);		
						set_lcd_photo0_config(128,96,0);							
						scale_to_lcd_config();


						lcdc_set_video_en(lcd_dev,1); //1		 close or open vedio background 	

						//os_sleep_ms(10);	
						bbm_displaydecode_run =1; 											
			break;

		case 1 :  //P1__video  P0_bg
						os_sleep_ms(10);
						delay_open_lcd_flash(4);

						//vpp_close(vpp_dev);							

						jpg_dec_scale_del();
						jpg_decode_scale_to_p0_config((uint32_t)video_psram_mem);  						// set decode scale2 out  p0
						scale3_to_p1_config();// set dvp scale3 out  p1

						extern volatile uint32 decode_num;
						extern uint32 scale3_num ;

						scale3_num =0;
						decode_num=0;

						p0p1_switch_flag=1;
						lcdc_set_video_en(lcd_dev,1); //1		 close or open vedio background 		
						lcd_info.lcd_p0p1_state = 3;
						os_sleep_ms(10);
						bbm_displaydecode_run =1;												
			break;


case 2 :  //_video_all
	
						os_sleep_ms(10);
						delay_open_lcd_flash(4);
						//vpp_close(vpp_dev);							

						jpg_dec_scale_del();
						jpg_decode_scale_to_p0_config((uint32_t)video_psram_mem);  						// set decode scale2 out  p0
						scale3_to_p1_config();// set dvp scale3 out  p1
						p0p1_switch_flag=1;
						lcdc_set_video_en(lcd_dev,1); //1		 close or open vedio background 		
						lcd_info.lcd_p0p1_state = 2;
						os_sleep_ms(10);
						bbm_displaydecode_run =1;	
			break;
		case 3 :  //__bg_all
						os_sleep_ms(10);
						delay_open_lcd_flash(4);
						jpg_dec_scale_del();						
						// lcdc_set_video_en(lcd_dev,0);
						jpg_decode_scale_config((uint32)video_decode_mem);								
						scale_to_lcd_config();
						scale3_num =0;
						decode_num=0;
						p0p1_switch_flag=0;
						lcdc_set_video_en(lcd_dev,1); //1		 close or open vedio background 	
						//os_sleep_ms(10);	
						bbm_displaydecode_run =1; 			
				break;
		
		default:
			printf("## un_view_switch ERR!!!!!  un_viewSwitch_flag_flag ==%d \r\n",p0p1_flag);
			break;
	}
			
	printf("## un_view_switch end OK****  un_viewSwitch_flag_flag ==%d \r\n",p0p1_flag);
}





void view_switch(uint8_t p0p1_flag)
{
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);


#ifdef  P0P1_SWITCH
if(p0p1_flag)
	{
		bbm_displaydecode_run =0;
		os_sleep_ms(10);
		delay_open_lcd_flash(4);

		jpg_dec_scale_del();
		//set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
		
		//set decode scale2 out  p1
		jpg_decode_scale_config((uint32_t)video_decode_mem);
	
		//set_lcd_photo0_config(120,90,0);
	
	// set dvp scale3 out  p0
		scale_to_lcd_config();

		//vpp_open(vpp_dev);		
		p0p1_switch_flag=0;

		lcdc_set_video_en(lcd_dev,1);
		lcd_info.lcd_p0p1_state = 3;
		os_sleep_ms(10);
		bbm_displaydecode_run =1;
	}
	else
	{
		bbm_displaydecode_run =0;
		os_sleep_ms(10);
		delay_open_lcd_flash(4);


		
		// set decode scale2 out  p0
		jpg_dec_scale_del();
		jpg_decode_scale_to_p0_config((uint32_t)video_psram_mem);
		//set_lcd_photo1_config(120,90,0);
		//jpg_decode_scale_config((uint32_t)video_decode_mem);
		
		//scale_to_lcd_config();
	// set dvp scale3 out  p1
		scale3_to_p1_config();

		{
			extern volatile uint32 decode_num;
			extern uint32 scale3_num ;

			scale3_num =0;
			decode_num=0;

#if 0
			hw_memset(video_decode_mem,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_decode_mem1,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_decode_mem2,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_psram_mem,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_psram_mem1,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_psram_mem2,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
#endif
		}
		p0p1_switch_flag=1;

		//vpp_open(vpp_dev);		
		lcdc_set_video_en(lcd_dev,1);
		lcd_info.lcd_p0p1_state = 3;
		os_sleep_ms(10);
		bbm_displaydecode_run =1;

	}
#else
	if(p0p1_flag)
	{
		bbm_displaydecode_run =0;
		os_sleep_ms(10);
		delay_open_lcd_flash(4);
		//lcdc_set_video_en(lcd_dev,0);
		//lcd_info.lcd_p0p1_state = 0;

		lcdc_set_rotate_p0_up(lcd_dev,1); // p0 up
		lcdc_set_rotate_p0p1_start_location(lcd_dev,(SCALE_WIDTH-120-6),6,0,0);


		jpg_dec_scale_del();
		set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
		//set decode scale2 out  p1
		jpg_decode_scale_config((uint32_t)video_decode_mem);
	
		set_lcd_photo0_config(120,90,0);
		scale_to_lcd_config();
		#if 0
		{
			extern volatile uint32 decode_num;
			extern uint32 scale3_num ;

			//scale3_num =0;
			//decode_num=0;
			hw_memset(video_decode_mem,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_decode_mem1,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_decode_mem2,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			
			hw_memset(video_psram_mem,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_psram_mem1,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_psram_mem2,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			//os_sleep_ms(10);
		}
	   #endif
		//vpp_open(vpp_dev);		

		lcdc_set_video_en(lcd_dev,1);
		lcd_info.lcd_p0p1_state = 3;
		os_sleep_ms(10);
		bbm_displaydecode_run =1;
	}
	else
	{
		bbm_displaydecode_run =0;
		os_sleep_ms(10);

		delay_open_lcd_flash(4);
		//lcdc_set_video_en(lcd_dev,0);
		//lcd_info.lcd_p0p1_state = 0;

		lcdc_set_rotate_p0_up(lcd_dev,0);// p1 up
		lcdc_set_rotate_p0p1_start_location(lcd_dev,0,0,(SCALE_WIDTH-120-6),6);
		
		// set decode scale2 out  p0
		jpg_dec_scale_del();
		set_lcd_photo1_config(120,90,0);
		jpg_decode_scale_config((uint32_t)video_decode_mem);
		
		set_lcd_photo0_config(SCALE_WIDTH,SCALE_HIGH,0);
		scale_to_lcd_config();
	#if 0
		{
			extern volatile uint32 decode_num;
			extern uint32 scale3_num ;

			//scale3_num =0;
			//decode_num=0;
			hw_memset(video_decode_mem,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_decode_mem1,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_decode_mem2,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));

			hw_memset(video_psram_mem,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_psram_mem1,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			hw_memset(video_psram_mem2,0,(SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2));
			//os_sleep_ms(10);
		}
	#endif
		//vpp_open(vpp_dev);		
		lcdc_set_video_en(lcd_dev,1);
		lcd_info.lcd_p0p1_state = 3;

		os_sleep_ms(10);
		bbm_displaydecode_run =1;

	}
#endif
}
extern uint8_t  lcd_pair_success;
void ui_event_intercomPage(lv_event_t * e){
	uint32_t* key_val = (uint32_t*)e->param;
	lv_event_code_t code = lv_event_get_code(e);
	struct vpp_device *vpp_dev;
	struct scale_device *scale_dev;
	struct dma_device *dma1_dev;
	uint32_t retval = 0;

	dma1_dev = (struct dma_device *)dev_get(HG_M2MDMA_DEVID);
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);

	if(code==USER_KEY_EVENT)
	{
		switch(*key_val)
		{

			case KEY_M:    //魔音
#ifdef BABY_UI_MAGICSOUND
			//printf("##===AD_A  \n");
			case KEY_MAGIC_SWITCH:
			{
				extern magicSound *magic_sound;
				if(magic_sound)
				{
					sundtype++;
					if(sundtype>5)
						sundtype=0;
					printf("##===sundtype=%d \n",sundtype);
					magicSound_set_type(magic_sound,sundtype);
				}
			}
				break;
#endif

			case AD_LEFT: 
			case AD_RIGHT:    //对讲界面切换
			
				// if(lcd_pair_success != 1)//未配对  
				if(1)
				{
					printf("##**************************** \r\n");
					printf("##===un_viewSwitch_flag change \r\n");
					printf("##**************************** \r\n");		
					un_viewSwitch_flag++;
						if(un_viewSwitch_flag >3)
							un_viewSwitch_flag = 0;	
				
								
					un_view_switch(un_viewSwitch_flag);
					// un_viewSwitch_flag ^=1;

				}else{
					printf("##===viewSwitch_flag change \n");
					viewSwitch_flag ^=1;
					view_switch(viewSwitch_flag);

				}
				break;
			
			case AD_BACK:
			case KEY_BACK:    //退出对讲


				if(get_net_pair_status())
					userPairstop();
				
				printf("##=========== exit pair mode  \n");

				if (sys_cfgs.wifi_mode == WIFI_MODE_STA) {
					ieee80211_iface_stop(WIFI_MODE_STA);
					
				}
				else
				{
					//if ap  connect sta
					ieee80211_disassoc_all(WIFI_MODE_AP);
					os_memset(sys_cfgs.bssid, 0, 6);
					syscfg_save();
				}
				ipf_update_flag = 1;			
				rahmen_open =0;
				lcd_pair_success = 0;
				lv_page_select(PAGE_WECHAT); // 回微聊页
				s_wechat_focus_idx = 0;
				wechat_update_focus_style();
				
				break;


			case AD_VOL_DOWN:        //音量---
			case KEY_VOL_DOWN:
				os_printf("## KEY_VOL_DOWN=\n");

				if(camSetParam.volumeSet)
				{
					camSetParam.volumeSet--;
				}
				render_vol_level(camSetParam.volumeSet);
				volume_adjust(camSetParam.volumeSet);
				volDisAnimationStart();
				os_printf("## cur_volumeSet=%d \n",camSetParam.volumeSet);
				break;

			case AD_VOL_UP:         //音量+++
			case KEY_VOL_UP:
				os_printf("## KEY_VOL_UP=\n");

				if(camSetParam.volumeSet<10)
				{
					camSetParam.volumeSet ++;
				}
				render_vol_level(camSetParam.volumeSet);
				volume_adjust(camSetParam.volumeSet);
				volDisAnimationStart();
				os_printf("## cur_volumeSet=%d \n",camSetParam.volumeSet);
				break;
 
			case KEY_STICKER:       // 大头贴
			case KEY_IPF_SWITCH:   
				if(camera_gvar.specialeffects_index<RAHMEN_MAX_NUMS)
				{
					//vpp_set_ifp_en(vpp_dev,0);
					os_sleep_ms(10);
					get_ifp_addr(ipf_imgSrcTable[camera_gvar.specialeffects_index]);
					//vpp_set_ifp_addr(vpp_dev,(uint32_t)vpp_ifp_addr);
					// vpp_set_ifp_en(vpp_dev,1);
					//ipf_index_num=camera_gvar.specialeffects_index;
					rahmen_open =1;

				}


				if(camera_gvar.specialeffects_index<(RAHMEN_MAX_NUMS))
					++camera_gvar.specialeffects_index;
				else
				{
						camera_gvar.specialeffects_index=0;
						//vpp_set_ifp_en(vpp_dev,0);
						rahmen_open =0;
				}
				ipf_update_flag=1;

				break;
		
			default:
			break;
		}
	}
	else if(code==LV_EVENT_CLICKED)
	{

	}
}

extern uint8_t get_wifi_connect_flag(void);

void ui_intercomPage_screen_init(){

		struct netdev *ndev=(struct netdev*)dev_get(HG_WIFI0_DEVID);
		if(ndev){
			struct netif *netif=(struct netif *)ndev->stack_data;
			netif_set_down(netif);
			netif_set_up(netif);
		}


	static lv_style_t intercomPageStyle;

	static lv_style_t debugstyle;	

	lv_style_init(&debugstyle);
	lv_style_set_text_font(&debugstyle,&lv_font_montserrat_18);
	
	lv_style_reset(&intercomPageStyle);
	lv_style_init(&intercomPageStyle);
	lv_style_set_width(&intercomPageStyle, lv_obj_get_width(lv_scr_act()));
	lv_style_set_height(&intercomPageStyle, lv_obj_get_height(lv_scr_act()));
	 lv_style_set_bg_color(&intercomPageStyle, lv_color_hex(0xb3d9e6));	
	//lv_style_set_bg_color(&intercomPageStyle, lv_color_hex(0x000000));	
	lv_style_set_shadow_color(&intercomPageStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_width(&intercomPageStyle, 0);
	lv_style_set_radius(&intercomPageStyle,0);
	// lv_style_set_outline_color(&menuPanelStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_pad_all(&intercomPageStyle, 0);
	lv_style_set_pad_gap(&intercomPageStyle,0);


	ui_intercomPage = lv_obj_create(lv_scr_act());
	lv_obj_clear_flag( ui_intercomPage, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	curPage_obj = ui_intercomPage;
	lv_obj_add_style(ui_intercomPage, &intercomPageStyle, 0);
	lv_obj_set_style_bg_opa(ui_intercomPage, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_add_event_cb(curPage_obj, event_handler, LV_EVENT_ALL, NULL);


#ifdef DISPLAY_DEBUGINFO_ENABLE

	ui_speedinfo_rx_obj  = lv_label_create(ui_intercomPage);
	ui_speedinfo_tx_obj  = lv_label_create(ui_intercomPage);
	ui_speedinfo_id_obj  = lv_label_create(ui_intercomPage);
	ui_speedinfo_num_obj = lv_label_create(ui_intercomPage);
	
	lv_obj_add_style(ui_speedinfo_rx_obj,&debugstyle,LV_STATE_DEFAULT);
	lv_obj_align(ui_speedinfo_rx_obj,LV_ALIGN_BOTTOM_LEFT,0,0);
	lv_label_set_recolor(ui_speedinfo_rx_obj, 1);

	lv_obj_add_style(ui_speedinfo_tx_obj,&debugstyle,LV_STATE_DEFAULT);
	lv_obj_align(ui_speedinfo_tx_obj,LV_ALIGN_BOTTOM_LEFT,0,-30);
	lv_label_set_recolor(ui_speedinfo_tx_obj, 1);	
	
	lv_obj_add_style(ui_speedinfo_id_obj,&debugstyle,LV_STATE_DEFAULT);
	lv_obj_align(ui_speedinfo_id_obj,LV_ALIGN_BOTTOM_LEFT,0,-60);
	lv_label_set_recolor(ui_speedinfo_id_obj, 1);

	lv_obj_add_style(ui_speedinfo_num_obj,&debugstyle,LV_STATE_DEFAULT);
	lv_obj_align(ui_speedinfo_num_obj,LV_ALIGN_BOTTOM_LEFT,0,-90);
	lv_label_set_recolor(ui_speedinfo_num_obj, 1);	
#endif

	ui_intercomTopBar = lv_obj_create(ui_intercomPage);
	lv_obj_set_width( ui_intercomTopBar, lv_pct(100));
	lv_obj_set_height( ui_intercomTopBar, lv_pct(12));
	lv_obj_set_align( ui_intercomTopBar, LV_ALIGN_TOP_MID );
	lv_obj_clear_flag( ui_intercomTopBar, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_radius(ui_intercomTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_intercomTopBar, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_intercomTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_intercomTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_intercomTopBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_intercomTopBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_intercomTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_intercomTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);


#if 0
	
	ui_isdIconImg = lv_img_create(ui_intercomTopBar);
	lv_img_set_src(ui_isdIconImg, &iconSdc);
	lv_obj_set_width( ui_isdIconImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_isdIconImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_x( ui_isdIconImg, -32 );
	lv_obj_set_y( ui_isdIconImg, 0 );
	lv_obj_set_align( ui_isdIconImg, LV_ALIGN_RIGHT_MID );
	lv_obj_add_flag( ui_isdIconImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_isdIconImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags

	if(camera_gvar.sd_online)
		lv_obj_clear_flag(ui_isdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
	else
		lv_obj_add_flag(ui_isdIconImg, LV_OBJ_FLAG_HIDDEN); 
#endif

#if 1
	ui_DvIconImg = lv_img_create(ui_intercomTopBar);
	lv_img_set_src(ui_DvIconImg, &iconDv_r);
	lv_obj_set_width( ui_DvIconImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_DvIconImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_align(ui_DvIconImg, LV_ALIGN_TOP_MID, 0, 0);

	lv_obj_add_flag( ui_DvIconImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_DvIconImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags

	lv_obj_add_flag(ui_DvIconImg, LV_OBJ_FLAG_HIDDEN); 
#endif

#if 0
	ui_iSdStaLabel = lv_label_create(ui_isdIconImg);
	lv_obj_set_width( ui_iSdStaLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_iSdStaLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_iSdStaLabel, LV_ALIGN_CENTER );
	lv_label_set_text(ui_iSdStaLabel,LV_SYMBOL_OK);//""
	lv_obj_set_style_text_color(ui_iSdStaLabel, lv_color_hex(0x05F80A), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_iSdStaLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_iSdStaLabel, &lv_font_montserrat_14, LV_PART_MAIN| LV_STATE_DEFAULT);
#endif
#if 0
	ui_intercomIconImg = lv_img_create(ui_intercomTopBar);
	lv_img_set_src(ui_intercomIconImg, &iconFlagPhoto);
	lv_obj_set_width( ui_intercomIconImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_intercomIconImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_add_flag( ui_intercomIconImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_intercomIconImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
#endif

#if 1
	ui_intercomBatImg = lv_img_create(ui_intercomTopBar);
	lv_img_set_src(ui_intercomBatImg, ui_imgset_iconBat[get_batlevel()]);
	lv_obj_set_width( ui_intercomBatImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_intercomBatImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_x( ui_intercomBatImg, -4 );
	lv_obj_set_y( ui_intercomBatImg, 0 );
	lv_obj_set_align( ui_intercomBatImg, LV_ALIGN_RIGHT_MID );
	lv_obj_add_flag( ui_intercomBatImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_intercomBatImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
#else
	ui_iBatIconLabel = lv_label_create(ui_intercomTopBar);
	lv_obj_set_width( ui_iBatIconLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_iBatIconLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_iBatIconLabel, LV_ALIGN_TOP_RIGHT );
	lv_label_set_text(ui_iBatIconLabel,LV_SYMBOL_BATTERY_1);//""
	lv_obj_set_style_text_color(ui_iBatIconLabel, lv_color_hex(0x4AA1FF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_iBatIconLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_iBatIconLabel, &lv_font_montserrat_22, LV_PART_MAIN| LV_STATE_DEFAULT);
#endif

	ui_signalPanel = lv_obj_create(ui_intercomTopBar);
	lv_obj_set_width( ui_signalPanel, 32);
	lv_obj_set_height( ui_signalPanel, 22);
	lv_obj_set_align( ui_signalPanel, LV_ALIGN_LEFT_MID );
	lv_obj_set_flex_flow(ui_signalPanel,LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(ui_signalPanel, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag( ui_signalPanel, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_radius(ui_signalPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_signalPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	// lv_obj_set_style_bg_color(ui_signalPanel, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_signalPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_signalPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_signalPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_signalPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_signalPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_signalPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_column(ui_signalPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_row(ui_signalPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);

	for(uint8_t i=0;i<4;i++)
	{
		ui_signalLevels[i]=lv_obj_create(ui_signalPanel);
		lv_obj_set_width( ui_signalLevels[i], 4);
		lv_obj_set_height( ui_signalLevels[i], (20-3*(4-i)));
		lv_obj_set_align( ui_signalLevels[i], LV_ALIGN_CENTER );
		lv_obj_clear_flag( ui_signalLevels[i], LV_OBJ_FLAG_SCROLLABLE );    /// Flags
		lv_obj_set_style_radius(ui_signalLevels[i], 0, LV_PART_MAIN| LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(ui_signalLevels[i], lv_color_hex(0x6D6C6C), LV_PART_MAIN | LV_STATE_DEFAULT );
		lv_obj_set_style_bg_opa(ui_signalLevels[i], 255, LV_PART_MAIN| LV_STATE_DEFAULT);
		lv_obj_set_style_border_width(ui_signalLevels[i], 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	}


	ui_intercomBtmBar = lv_obj_create(ui_intercomPage);
	lv_obj_set_width( ui_intercomBtmBar, lv_pct(100));
	lv_obj_set_height( ui_intercomBtmBar, lv_pct(12));
	lv_obj_set_align( ui_intercomBtmBar, LV_ALIGN_BOTTOM_MID );
	lv_obj_clear_flag( ui_intercomBtmBar, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_radius(ui_intercomBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_intercomBtmBar, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_intercomBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_intercomBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_intercomBtmBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_intercomBtmBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_intercomBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_intercomBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);

#if 0
	ui_cTimeIconLabel = lv_label_create(ui_intercomBtmBar);
	lv_obj_set_width( ui_cTimeIconLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_cTimeIconLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_cTimeIconLabel, LV_ALIGN_BOTTOM_LEFT );
	//lv_label_set_text(ui_cTimeIconLabel,"2023/11/30 14:59:48");
	lv_obj_set_style_text_color(ui_cTimeIconLabel, lv_color_hex(0xF8D00B), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_cTimeIconLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_cTimeIconLabel, &lv_font_montserrat_16, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_clock_display(ui_cTimeIconLabel,&sw_rtc,NULL);

	
	ui_RecTimeIconLabel = lv_label_create(ui_intercomBtmBar);
	lv_obj_set_width( ui_RecTimeIconLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_RecTimeIconLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_RecTimeIconLabel,  LV_ALIGN_TOP_MID );
	lv_label_set_text(ui_RecTimeIconLabel,"00:00:00");
	lv_obj_set_style_text_color(ui_RecTimeIconLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_RecTimeIconLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_RecTimeIconLabel, &lv_font_montserrat_20, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_label_set_recolor(ui_RecTimeIconLabel,true);
	
	lv_obj_add_flag(ui_RecTimeIconLabel, LV_OBJ_FLAG_HIDDEN); 
	#endif

	#ifdef BABY_UI_MAGICSOUND
	ui_voiceinfo_lable = lv_label_create(ui_intercomBtmBar);
	lv_obj_set_width( ui_voiceinfo_lable, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_voiceinfo_lable, LV_SIZE_CONTENT);   /// 1
	lv_obj_align(ui_voiceinfo_lable,LV_ALIGN_RIGHT_MID,0,0);
	lv_label_set_text(ui_voiceinfo_lable,"Voice:normal");

	lv_obj_set_style_text_color(ui_voiceinfo_lable, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_voiceinfo_lable, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_voiceinfo_lable, &lv_font_montserrat_20, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_label_set_recolor(ui_voiceinfo_lable,true);
	#endif
	// ui_photoQualityLabel = lv_label_create(ui_intercomBtmBar);
	// lv_obj_set_width( ui_photoQualityLabel, LV_SIZE_CONTENT);  /// 1
	// lv_obj_set_height( ui_photoQualityLabel, LV_SIZE_CONTENT);   /// 1
	// lv_obj_set_align( ui_photoQualityLabel, LV_ALIGN_BOTTOM_RIGHT );
	// lv_label_set_text(ui_photoQualityLabel,"1M");
	// lv_obj_set_style_text_color(ui_photoQualityLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	// lv_obj_set_style_text_opa(ui_photoQualityLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	// lv_obj_set_style_text_font(ui_photoQualityLabel, &lv_font_montserrat_20, LV_PART_MAIN| LV_STATE_DEFAULT);

	//lv_obj_add_event_cb(ui_intercomPage, ui_event_intercomPage, LV_EVENT_ALL, NULL);
	// ui_focusBtn = lv_imgbtn_create(ui_intercomPage);
	// lv_imgbtn_set_src(ui_focusBtn, LV_IMGBTN_STATE_RELEASED, NULL, &iconFocus, NULL);
	// lv_imgbtn_set_src(ui_focusBtn, LV_IMGBTN_STATE_PRESSED, NULL, &iconFocusP, NULL);
	// lv_obj_set_width( ui_focusBtn, 168);
	// lv_obj_set_height( ui_focusBtn, 128);
	// lv_obj_set_align( ui_focusBtn, LV_ALIGN_CENTER );


	ui_focusImg = lv_img_create(ui_intercomPage);
	lv_img_set_src(ui_focusImg, ui_imgset_iconFocus[0]);
	lv_obj_set_width( ui_focusImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_focusImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_add_flag( ui_focusImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_focusImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_align( ui_focusImg, LV_ALIGN_CENTER );

	lv_obj_add_flag( ui_focusImg, LV_OBJ_FLAG_HIDDEN );   /// Flags

#if 1
	//配对 显示
	ui_pairPanel = lv_obj_create(ui_intercomPage);
	lv_obj_set_width( ui_pairPanel, 180);
	lv_obj_set_height( ui_pairPanel, 120);
	lv_obj_set_align( ui_pairPanel, LV_ALIGN_CENTER );
	lv_obj_set_flex_flow(ui_pairPanel,LV_FLEX_FLOW_COLUMN_WRAP);
	lv_obj_set_flex_align(ui_pairPanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_clear_flag( ui_pairPanel, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_bg_color(ui_pairPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_pairPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_pairPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_pairPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_pairPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_pairPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_pairPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_row(ui_pairPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_column(ui_pairPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_add_flag( ui_pairPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags


	ui_pairTeimlabel = lv_label_create(ui_pairPanel);
	lv_obj_set_style_text_font(ui_pairTeimlabel, &alifangyuan28, 0);
	lv_label_set_recolor(ui_pairTeimlabel, 1);
	lv_obj_set_style_text_color(ui_pairTeimlabel, lv_color_hex(0xFFFFFF), 0);
	//lv_label_set_text(ui_pairTeimlabel, LV_SYMBOL_VOLUME_MAX);
#endif


#if 0// notic dialog
	ui_dialogPanel = lv_obj_create(ui_intercomPage);
	lv_obj_set_width( ui_dialogPanel, 160);
	lv_obj_set_height( ui_dialogPanel, 120);
	lv_obj_set_align( ui_dialogPanel, LV_ALIGN_CENTER );
	// lv_obj_set_flex_flow(ui_dialogPanel,LV_FLEX_FLOW_COLUMN_WRAP);
	// lv_obj_set_flex_align(ui_dialogPanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag( ui_dialogPanel, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_bg_color(ui_dialogPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_dialogPanel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_dialogPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_dialogPanel, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_dialogPanel, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_dialogPanel, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_dialogPanel, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_row(ui_dialogPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_column(ui_dialogPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_add_flag( ui_dialogPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
	lv_obj_set_style_text_font(ui_dialogPanel, &alifangyuan16, 0);


	lv_obj_t * ui_dialogTitle = lv_label_create(ui_dialogPanel);
	lv_obj_set_width( ui_dialogTitle, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_dialogTitle, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_dialogTitle, LV_ALIGN_TOP_LEFT );
	lv_obj_set_style_text_color(ui_dialogTitle, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_dialogTitle, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	//lv_obj_set_style_text_font(ui_dialogTitle, &lv_font_montserrat_16, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_label_set_text(ui_dialogTitle,"提醒:");

	ui_dialogContent = lv_label_create(ui_dialogPanel);
	lv_obj_set_width( ui_dialogContent, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_dialogContent, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_dialogContent, LV_ALIGN_CENTER );
	lv_obj_set_style_text_color(ui_dialogContent, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_dialogContent, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_label_set_recolor(ui_dialogContent, 1);
	lv_label_set_text(ui_dialogContent,"退出会#ff0088 断开连接#");


    lv_obj_t *btn_cancel = lv_btn_create(ui_dialogPanel);
    lv_obj_set_size(btn_cancel, 36, 26);  
	lv_obj_set_align( btn_cancel, LV_ALIGN_BOTTOM_LEFT );

	lv_obj_t *label_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(label_cancel, "取消");
	lv_obj_set_align( label_cancel, LV_ALIGN_CENTER );

    lv_obj_t *btn_confirm = lv_btn_create(ui_dialogPanel);
    lv_obj_set_size(btn_confirm, 36, 26);  
	lv_obj_set_align( btn_confirm, LV_ALIGN_BOTTOM_RIGHT );
	
    lv_obj_t *label_confirm = lv_label_create(btn_confirm);
    lv_label_set_text(label_confirm, "继续");
	lv_obj_set_align( label_confirm, LV_ALIGN_CENTER );

#endif

	// 创建麦克风图片对象
    mic_img = lv_img_create(ui_intercomPage);
    lv_img_set_src(mic_img, &mkf30); 
    lv_obj_set_pos(mic_img, 60, 60);
    lv_obj_set_size(mic_img, 30, 30);
    lv_obj_add_flag(mic_img, LV_OBJ_FLAG_HIDDEN);



#if 1   // 音量显示ui
	ui_volPanel = lv_obj_create(ui_intercomPage);
	lv_obj_set_width( ui_volPanel, 160);
	lv_obj_set_height( ui_volPanel, 120);
	lv_obj_set_align( ui_volPanel, LV_ALIGN_CENTER );
	lv_obj_set_flex_flow(ui_volPanel,LV_FLEX_FLOW_COLUMN_WRAP);
	lv_obj_set_flex_align(ui_volPanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_clear_flag( ui_volPanel, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_bg_color(ui_volPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_volPanel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_volPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_volPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_volPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_volPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_volPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_row(ui_volPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_column(ui_volPanel, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_add_flag( ui_volPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags

	#if 1

	ui_lisglabel = lv_label_create(ui_volPanel);
	lv_obj_set_style_text_font(ui_lisglabel, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(ui_lisglabel, lv_color_hex(0xFCA702), 0);
	lv_label_set_text(ui_lisglabel, LV_SYMBOL_VOLUME_MAX);

	#else
	lv_obj_t *ui_lisgImage = lv_img_create(ui_volPanel);
	lv_img_set_src(ui_lisgImage, &iconMenuVolume1);
	lv_obj_set_width( ui_lisgImage, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_lisgImage, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_lisgImage, LV_ALIGN_CENTER );
	lv_obj_add_flag( ui_lisgImage, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_lisgImage, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	#endif

	lv_obj_t * ui_list = lv_obj_create(ui_volPanel);
	lv_obj_set_width( ui_list, 180);
	lv_obj_set_height( ui_list, 40);
	lv_obj_set_align( ui_list, LV_ALIGN_CENTER );
	lv_obj_set_flex_flow(ui_list,LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(ui_list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag( ui_list, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_radius(ui_list, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_list, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_list, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_list, 0, LV_PART_MAIN| LV_STATE_DEFAULT);

	for(uint8_t i=0;i<10;i++)
	{
		ui_volumeLevels[i]=lv_obj_create(ui_list);
		lv_obj_set_width( ui_volumeLevels[i], 6);
		lv_obj_set_height( ui_volumeLevels[i], 16);
		lv_obj_set_align( ui_volumeLevels[i], LV_ALIGN_CENTER );
		lv_obj_clear_flag( ui_volumeLevels[i], LV_OBJ_FLAG_SCROLLABLE );    /// Flags
		lv_obj_set_style_radius(ui_volumeLevels[i], 0, LV_PART_MAIN| LV_STATE_DEFAULT);
		lv_obj_set_style_bg_color(ui_volumeLevels[i], lv_color_hex(0x6D6C6C), LV_PART_MAIN | LV_STATE_DEFAULT );
		lv_obj_set_style_bg_opa(ui_volumeLevels[i], 255, LV_PART_MAIN| LV_STATE_DEFAULT);
		lv_obj_set_style_border_width(ui_volumeLevels[i], 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	}

	
	if(camSetParam.volumeSet>10)
		camSetParam.volumeSet=10;

	render_vol_level(camSetParam.volumeSet);
#endif

	 lv_obj_set_style_bg_color(ui_intercomPage, lv_color_hex(0x000000), 0);
	#if 1
	if (sys_cfgs.wifi_mode == WIFI_MODE_STA) {
		ieee80211_iface_start(WIFI_MODE_STA);
		os_sleep_ms(50);
	}
	userPairStart();
	#endif

}
