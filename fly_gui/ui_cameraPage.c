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
#include "play_pcmtone.h"

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

extern uint8_t g_camera_from_page;
extern uint8_t  s_wechat_focus_idx;
extern void wechat_update_focus_style(void);
void doubleSenor_pdn_set(void)
{
	gpio_iomap_output(PIN_DVP_PDN1,GPIO_IOMAP_OUTPUT);
	gpio_set_val(PIN_DVP_PDN1,1);
	gpio_iomap_output(PIN_DVP_PDN,GPIO_IOMAP_OUTPUT);
	gpio_set_val(PIN_DVP_PDN,1);
	os_sleep_ms(20);

	if(camera_gvar.camera_switch)
	{
		gpio_set_val(PIN_DVP_PDN1,0);
	}
	else
	{
		gpio_set_val(PIN_DVP_PDN,0);
	}
}
void dvp_frontback_exchange(void)
{
	struct jpg_device * jpeg0_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);		

#if DVP_EN
	struct dvp_device *dvp_device = (struct dvp_device*)dev_get(HG_DVP_DEVID);
	struct vpp_device *vpp_device = (struct vpp_device*)dev_get(HG_VPP_DEVID);
#endif

	os_printf("%s %d\r\n",__FUNCTION__);

	//打印机使能
	
	{
#if DVP_EN
		gpio_iomap_output(PIN_DVP_PDN,GPIO_IOMAP_OUTPUT);
		gpio_set_val(PIN_DVP_PDN,1);
		gpio_iomap_output(PIN_DVP_PDN1,GPIO_IOMAP_OUTPUT);
		gpio_set_val(PIN_DVP_PDN1,1);
		
		jpg_close(jpeg0_dev);
		dvp_close(dvp_device);
		vpp_close(vpp_device);

		bool csi_ret;
		bool csi_cfg();
		bool csi_open();



	#if SDH_I2C2_REUSE
		uint32 sdhost_i2c2_exchange(int sdh_stop_en);
		sdhost_i2c2_exchange(1);
	#endif
		csi_ret = csi_cfg();
		if(csi_ret)
			csi_open();
		//vpp_close(vpp_device);
		jpg_open(jpeg0_dev);
	#if SDH_I2C2_REUSE
		sdhost_i2c2_exchange(0);
	#endif

#endif
	}


}



void ui_event_cameraPage(lv_event_t * e){
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
			case AD_VOL_UP:
				break;

			case AD_VOL_DOWN:
				break;

			case AD_LEFT: 
				break;

			case AD_RIGHT:		
				// case KEY_CAMERA_SWITCH:
				// //os_sleep_ms(500);
				// if(rec_open == 0){
				// 	{
				// 		extern	volatile uint8_t sdh_init_flag;
				// 		if(sdh_init_flag)
				// 		break;
				// 	}
				// 	delay_open_lcd_flash(10);
				// 	camera_gvar.camera_switch = !camera_gvar.camera_switch;
				// 	printf("##camera_gvar.camera_switch =%d \n\r",camera_gvar.camera_switch);
				// 	dvp_frontback_exchange();
				// }
				break;
			case AD_BACK:
			case KEY_BACK:

				if(rec_open)
				{
					bbm_stop_record();
					rec_open = 0;
					// lv_time_reset(&rec_time);
					// lv_time_display(ui_RecTimeIconLabel,&rec_time,NULL);
					// lv_obj_add_flag(ui_RecTimeIconLabel, LV_OBJ_FLAG_HIDDEN); 
					// dv_flash_onoff(rec_open); //stop
				}

				if (g_camera_from_page == PAGE_WECHAT) 
				{ 
					printf("## camera back -> WECHAT\n"); 
					lv_page_select(PAGE_WECHAT); 
					s_wechat_focus_idx = 3; // 0=视频, 1=相机（你前面这么约定的就用 1） 
					wechat_update_focus_style(); 
				} 
				else 
				{ 
					printf("## camera back -> HOME\n"); 
					lv_page_select(PAGE_HOME); 
					camera_gvar.pagebtn_index = 1; // 看你主页相机是第几个 
					camera_gvar.immediately_reflash_flag = 1;
				}
				if(camera_gvar.camera_switch==1)
				{
					camera_gvar.camera_switch = 0;
					printf("##camera_gvar.camera_switch =%d \n\r",camera_gvar.camera_switch);
					dvp_frontback_exchange();
				}
				break;

	

			// case KEY_IPF_SWITCH:
			// if(camera_gvar.specialeffects_index<RAHMEN_MAX_NUMS)
			// {
			// 	//vpp_set_ifp_en(vpp_dev,0);
			// 	os_sleep_ms(10);
			// 	get_ifp_addr(ipf_imgSrcTable[camera_gvar.specialeffects_index]);
			// 	//vpp_set_ifp_addr(vpp_dev,(uint32_t)vpp_ifp_addr);
			// 	// vpp_set_ifp_en(vpp_dev,1);
			// 	//ipf_index_num=camera_gvar.specialeffects_index;
			// 	rahmen_open =1;
			// }


			// if(camera_gvar.specialeffects_index<(RAHMEN_MAX_NUMS))
			// 	++camera_gvar.specialeffects_index;
			// else
			// {
			// 		camera_gvar.specialeffects_index=0;
			// 		//vpp_set_ifp_en(vpp_dev,0);
			// 		rahmen_open =0;
			// }
			// ipf_update_flag=1;

			// break;


			// case KEY_WAKEUP:
			// client_send_wakeup_cmd(2);
			// break;

			// case KEY_SLEEPIN:
			// case KEY_POWEROFF:
			// //client_send_sleep_cmd(1);
			// lv_page_select(PAGE_POWEROFF);
			// //bbm_stop_record();
			// break;

			// case KEY_BROWSE:
			// lv_page_select(PAGE_ALBUM);
			// bbm_stop_record();
			// break;

			case AD_PRESS:
			case KEY_CAMERA:	
				os_printf("## take photo  AD_PRESS\n");		
				if(rec_open==0)
				{
					#if 1
					if(camera_gvar.sd_online==0)
					{
						noticeAnimationStart(4);
						break;
					}

					/*came界面 拍照按键声音*/
					play_pcmtone(&shottone);
					/*came界面 拍照按键声音*/


					os_printf("## take photo\n");
					takePhotoAnimationStart();

					if(get_bbm_take_photo_status()==0)
						bbm_take_photo(1);
					#else
					printf("gui_cfg.take_photo_num:%d\r\n",gui_cfg.take_photo_num);

					if(!get_takephoto_thread_status())
					{
						take_photo_thread_init(gui_cfg.photo_w,gui_cfg.photo_h,gui_cfg.take_photo_num);

						takePhotoAnimationStart();
					}
					else
					{
						os_printf("%s err,get_takephoto_thread_status:%d\n",__FUNCTION__,get_takephoto_thread_status());
					}
					#endif
				}
				break;
			
			case KEY_RECORD:
				if(rec_open == 0){

					if(camera_gvar.sd_online==0)
					{
						noticeAnimationStart(4);
						break;
					}
					printf("rec start\r\n");
					#if 0
					if(gui_cfg.rec_h == gui_cfg.dvp_h){
						jpg_cfg(HG_JPG0_DEVID,VPP_DATA0);
					}
					else{
						scale_from_vpp_to_jpg(scale_dev,(uint32)yuvbuf,gui_cfg.dvp_w,gui_cfg.dvp_h,gui_cfg.rec_w,gui_cfg.rec_h);
						jpg_cfg(HG_JPG0_DEVID,SCALER_DATA);
					}
					photo_msg.out0_h = gui_cfg.rec_h;
					photo_msg.out0_w = gui_cfg.rec_w;
			
				
					start_record_thread(30,16);
					#else
					if(bbm_start_record())
					#endif
					{
						rec_open = 1;
						dv_flash_onoff(rec_open);  //start
						lv_time_reset(&rec_time);
						lv_time_display(ui_RecTimeIconLabel,&rec_time,"#FF0000");
						lv_obj_clear_flag(ui_RecTimeIconLabel, LV_OBJ_FLAG_HIDDEN );   /// Flags 
					}
					
				}
				else{
					#if 0
					scale_close(scale_dev);
					send_stop_record_cmd();
					#else
					bbm_stop_record();
					#endif
					
					rec_open = 0;
					lv_time_reset(&rec_time);
					lv_time_display(ui_RecTimeIconLabel,&rec_time,NULL);
					lv_obj_add_flag(ui_RecTimeIconLabel, LV_OBJ_FLAG_HIDDEN); 

					dv_flash_onoff(rec_open); //stop

				}
				break;



			default:
				break;
		}
	}
	else if(code==LV_EVENT_CLICKED)
	{

	}
}


void ui_cameraPage_screen_init(){
	static lv_style_t cameraPageStyle;

	static lv_style_t debugstyle;	

	lv_style_init(&debugstyle);
	lv_style_set_text_font(&debugstyle,&lv_font_montserrat_18);
	
	lv_style_reset(&cameraPageStyle);
	lv_style_init(&cameraPageStyle);
	lv_style_set_width(&cameraPageStyle, lv_obj_get_width(lv_scr_act()));
	lv_style_set_height(&cameraPageStyle, lv_obj_get_height(lv_scr_act()));
	//lv_style_set_bg_color(&cameraPageStyle, lv_color_hex(0x040404));	
	lv_style_set_bg_color(&cameraPageStyle, lv_color_hex(0x000000));	
	//lv_style_set_bg_opa(&cameraPageStyle, 0);	
	lv_style_set_shadow_color(&cameraPageStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_width(&cameraPageStyle, 0);
	lv_style_set_radius(&cameraPageStyle,0);
	// lv_style_set_outline_color(&menuPanelStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_pad_all(&cameraPageStyle, 0);
	lv_style_set_pad_gap(&cameraPageStyle,0);


	ui_cameraPage = lv_obj_create(lv_scr_act());

	lv_obj_clear_flag( ui_cameraPage, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	curPage_obj = ui_cameraPage;
	lv_obj_add_style(ui_cameraPage, &cameraPageStyle, 0);
	lv_obj_set_style_text_font(ui_cameraPage, &ui_font_alimamaShuHei16, 0);

	//if(get_wifi_connect_flag())
	//	lv_obj_set_style_bg_opa(ui_cameraPage, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	//else
	lv_obj_set_style_bg_opa(ui_cameraPage, 255, LV_PART_MAIN| LV_STATE_DEFAULT);

	lv_obj_add_event_cb(curPage_obj, event_handler, LV_EVENT_ALL, NULL);




	ui_camTopBar = lv_obj_create(ui_cameraPage);
	lv_obj_set_width( ui_camTopBar, lv_pct(100));
	lv_obj_set_height( ui_camTopBar, lv_pct(12));
	lv_obj_set_align( ui_camTopBar, LV_ALIGN_TOP_MID );
	lv_obj_clear_flag( ui_camTopBar, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_radius(ui_camTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_camTopBar, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_camTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_camTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_camTopBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_camTopBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_camTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_camTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);


#if 1
	ui_csdIconImg = lv_img_create(ui_camTopBar);
	lv_img_set_src(ui_csdIconImg, &iconSdc);
	lv_obj_set_width( ui_csdIconImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_csdIconImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_x( ui_csdIconImg, -32 );
	lv_obj_set_y( ui_csdIconImg, 0 );
	lv_obj_set_align( ui_csdIconImg, LV_ALIGN_RIGHT_MID );
	lv_obj_add_flag( ui_csdIconImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_csdIconImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags

	if(camera_gvar.sd_online)
		lv_obj_clear_flag(ui_csdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
	else
		lv_obj_add_flag(ui_csdIconImg, LV_OBJ_FLAG_HIDDEN); 
#endif

#if 1
	ui_DvIconImg = lv_img_create(ui_camTopBar);
	lv_img_set_src(ui_DvIconImg, &iconDv_r);
	lv_obj_set_width( ui_DvIconImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_DvIconImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_align(ui_DvIconImg, LV_ALIGN_TOP_MID, 0, 0);

	lv_obj_add_flag( ui_DvIconImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_DvIconImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags

	lv_obj_add_flag(ui_DvIconImg, LV_OBJ_FLAG_HIDDEN); 
#endif

#if 1
	ui_cSdStaLabel = lv_label_create(ui_csdIconImg);
	lv_obj_set_width( ui_cSdStaLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_cSdStaLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_cSdStaLabel, LV_ALIGN_CENTER );
	lv_label_set_text(ui_cSdStaLabel,LV_SYMBOL_OK);//""
	lv_obj_set_style_text_color(ui_cSdStaLabel, lv_color_hex(0x05F80A), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_cSdStaLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_cSdStaLabel, &lv_font_montserrat_14, LV_PART_MAIN| LV_STATE_DEFAULT);
#endif
#if 1
	ui_camIconImg = lv_img_create(ui_camTopBar);
	lv_img_set_src(ui_camIconImg, &iconFlagPhoto);
	lv_obj_set_width( ui_camIconImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_camIconImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_add_flag( ui_camIconImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_camIconImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
#endif

#if 1
	ui_camBatImg = lv_img_create(ui_camTopBar);
	lv_img_set_src(ui_camBatImg, ui_imgset_iconBat[get_batlevel()]);
	lv_obj_set_width( ui_camBatImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_camBatImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_camBatImg, LV_ALIGN_RIGHT_MID );

	lv_obj_add_flag( ui_camBatImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_camBatImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
#else
	ui_cBatIconLabel = lv_label_create(ui_camTopBar);
	lv_obj_set_width( ui_cBatIconLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_cBatIconLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_cBatIconLabel, LV_ALIGN_TOP_RIGHT );
	lv_label_set_text(ui_cBatIconLabel,LV_SYMBOL_BATTERY_1);//""
	lv_obj_set_style_text_color(ui_cBatIconLabel, lv_color_hex(0x4AA1FF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_cBatIconLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_cBatIconLabel, &lv_font_montserrat_22, LV_PART_MAIN| LV_STATE_DEFAULT);
#endif
	ui_camPrinterFlag = lv_img_create(ui_camTopBar);
	lv_img_set_src(ui_camPrinterFlag, &iconPrinter);
	lv_obj_set_width( ui_camPrinterFlag, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_camPrinterFlag, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_x( ui_camPrinterFlag, -64 );
	lv_obj_set_y( ui_camPrinterFlag, 0 );
	lv_obj_set_align( ui_camPrinterFlag, LV_ALIGN_RIGHT_MID );
	lv_obj_add_flag( ui_camPrinterFlag, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_camPrinterFlag, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_add_flag( ui_camPrinterFlag, LV_OBJ_FLAG_HIDDEN );   /// Flags
	camera_gvar.printer_flag =0;

	ui_camBtmBar = lv_obj_create(ui_cameraPage);
	lv_obj_set_width( ui_camBtmBar, lv_pct(100));
	lv_obj_set_height( ui_camBtmBar, lv_pct(12));
	lv_obj_set_align( ui_camBtmBar, LV_ALIGN_BOTTOM_MID );
	lv_obj_clear_flag( ui_camBtmBar, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_radius(ui_camBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_camBtmBar, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_camBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_camBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_camBtmBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_camBtmBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_camBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_camBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);

#if 0
	ui_cTimeIconLabel = lv_label_create(ui_camBtmBar);
	lv_obj_set_width( ui_cTimeIconLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_cTimeIconLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_cTimeIconLabel, LV_ALIGN_BOTTOM_LEFT );
	//lv_label_set_text(ui_cTimeIconLabel,"2023/11/30 14:59:48");
	lv_obj_set_style_text_color(ui_cTimeIconLabel, lv_color_hex(0xF8D00B), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_cTimeIconLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_cTimeIconLabel, &lv_font_montserrat_16, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_clock_display(ui_cTimeIconLabel,&sw_rtc,NULL);

#endif
	
	ui_RecTimeIconLabel = lv_label_create(ui_camBtmBar);
	lv_obj_set_width( ui_RecTimeIconLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_RecTimeIconLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_RecTimeIconLabel,  LV_ALIGN_TOP_MID );
	lv_label_set_text(ui_RecTimeIconLabel,"00:00:00");
	lv_obj_set_style_text_color(ui_RecTimeIconLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_RecTimeIconLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_RecTimeIconLabel, &lv_font_montserrat_20, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_label_set_recolor(ui_RecTimeIconLabel,true);
	
	lv_obj_add_flag(ui_RecTimeIconLabel, LV_OBJ_FLAG_HIDDEN); 



	// ui_photoQualityLabel = lv_label_create(ui_camBtmBar);
	// lv_obj_set_width( ui_photoQualityLabel, LV_SIZE_CONTENT);  /// 1
	// lv_obj_set_height( ui_photoQualityLabel, LV_SIZE_CONTENT);   /// 1
	// lv_obj_set_align( ui_photoQualityLabel, LV_ALIGN_BOTTOM_RIGHT );
	// lv_label_set_text(ui_photoQualityLabel,"1M");
	// lv_obj_set_style_text_color(ui_photoQualityLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	// lv_obj_set_style_text_opa(ui_photoQualityLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	// lv_obj_set_style_text_font(ui_photoQualityLabel, &lv_font_montserrat_20, LV_PART_MAIN| LV_STATE_DEFAULT);

	//lv_obj_add_event_cb(ui_cameraPage, ui_event_cameraPage, LV_EVENT_ALL, NULL);
	// ui_focusBtn = lv_imgbtn_create(ui_cameraPage);
	// lv_imgbtn_set_src(ui_focusBtn, LV_IMGBTN_STATE_RELEASED, NULL, &iconFocus, NULL);
	// lv_imgbtn_set_src(ui_focusBtn, LV_IMGBTN_STATE_PRESSED, NULL, &iconFocusP, NULL);
	// lv_obj_set_width( ui_focusBtn, 168);
	// lv_obj_set_height( ui_focusBtn, 128);
	// lv_obj_set_align( ui_focusBtn, LV_ALIGN_CENTER );


	ui_focusImg = lv_img_create(ui_cameraPage);
	lv_img_set_src(ui_focusImg, ui_imgset_iconFocus[0]);
	lv_obj_set_width( ui_focusImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_focusImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_add_flag( ui_focusImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_focusImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_align( ui_focusImg, LV_ALIGN_CENTER );

	lv_obj_add_flag( ui_focusImg, LV_OBJ_FLAG_HIDDEN );   /// Flags


	#if 1// notic dialog
	ui_dialogPanel = lv_obj_create(ui_cameraPage);
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
	lv_label_set_text(ui_dialogContent,"请插入#ff0088 存储卡#");

	#if 0
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

	#endif

}
