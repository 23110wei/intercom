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

#include "keyScan.h"
#include "../lvgl.h"
#include "ui_language.h"
#include "fly_demo.h"
#include "clock_app.h"

extern Vpp_stream photo_msg;
extern lcd_msg lcd_info;
extern volatile vf_cblk g_vf_cblk;
extern gui_msg gui_cfg;

extern uint8_t g_img_from_page;
extern uint8_t  s_wechat_focus_idx;
extern void wechat_update_focus_style(void);

void ui_event_albumPage(lv_event_t * e){
	uint8_t name[16];
	uint32_t* key_val = (uint32_t*)e->param;
	lv_event_code_t code = lv_event_get_code(e);
	if(code==USER_KEY_EVENT)
	{
		switch(*key_val)
		{

			case AD_RIGHT:
				#if 1
				album_next_file();
				prevp_AnimationStart();
				sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",album_cur_selected->name);
				printf("name_rec_photo:%s\r\n",name_rec_photo);
				album_file_preview(name_rec_photo,album_cur_selected->filetype);
				lv_label_set_text(ui_fileFormatLabel,album_cur_selected->name);
				#else
				if(ablumlist==NULL)
				break;
				if(camera_gvar.album_fileindex)
				{
					camera_gvar.album_fileindex--;
				}
				else
					camera_gvar.album_fileindex = (camera_gvar.album_filenums-1);

				prevp_AnimationStart();
				sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",ablumlist[camera_gvar.album_fileindex].name);
				printf("name_rec_photo:%s\r\n",name_rec_photo);
				album_file_preview(name_rec_photo,ablumlist[camera_gvar.album_fileindex].filetype);
				lv_label_set_text(ui_fileFormatLabel,ablumlist[camera_gvar.album_fileindex].name);
				#endif
				break;
			

			case AD_LEFT: 
				#if 1
				album_prev_file();
				prevp_AnimationStart();
				sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",album_cur_selected->name);
				printf("name_rec_photo:%s\r\n",name_rec_photo);
				album_file_preview(name_rec_photo,album_cur_selected->filetype);
				lv_label_set_text(ui_fileFormatLabel,album_cur_selected->name);
				#else
				if(ablumlist==NULL)
				break;
				if(camera_gvar.album_fileindex<(camera_gvar.album_filenums-1))
				{
					camera_gvar.album_fileindex++;
				}
				else
				{
					camera_gvar.album_fileindex =0;
				}
				nextp_AnimationStart();

				sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",ablumlist[camera_gvar.album_fileindex].name);
				printf("name_rec_photo:%s\r\n",name_rec_photo);
				album_file_preview(name_rec_photo,ablumlist[camera_gvar.album_fileindex].filetype);
				lv_label_set_text(ui_fileFormatLabel,ablumlist[camera_gvar.album_fileindex].name);
				#endif			
				break;
				
			case AD_VOL_DOWN:
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

				case AD_VOL_UP:
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

			case KEY_CAMERA:
			case AD_PRESS:
				#if 1
				if(album_cur_selected->filetype==FT_AVI)
				{
					if(global_avi_exit){
						os_printf("replay\r\n\r\n");
						album_file_preview(name_rec_photo,album_cur_selected->filetype);
						os_sleep_ms(10);
					}
					global_avi_running ^= BIT(0);
				}
				#else
				if(ablumlist[camera_gvar.album_fileindex].filetype==FT_AVI)
				{
					if(global_avi_exit){
						os_printf("replay\r\n\r\n");
						album_file_preview(name_rec_photo,ablumlist[camera_gvar.album_fileindex].filetype);
						os_sleep_ms(10);
					}
					global_avi_running ^= BIT(0);
				}
				#endif
				break;

			case KEY_RECORD:
			case KEY_DELECT:
				if(camera_gvar.sd_online==0)
					break;
				if((global_avi_running)&&(album_cur_selected->filetype==FT_AVI))
				{
					global_avi_running=0;
					os_sleep_ms(10);
				}

				delete_selected_file();

				if (album_cur_selected)
				{
					//prevp_AnimationStart();
					sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",album_cur_selected->name);
					printf("name_rec_photo:%s\r\n",name_rec_photo);
					album_file_preview(name_rec_photo,album_cur_selected->filetype);
					lv_label_set_text(ui_fileFormatLabel,album_cur_selected->name);
				}
				break;
			
			case KEY_FORMAT_SD:
				if((camera_gvar.sd_online==0)||camera_gvar.notice_anim_times)
					break;

				lv_label_set_text(ui_dialogContent,"SD卡 格式化 #ff0088 开始#");
				noticeAnimationStart(255);
				if(FR_OK==format_sdcard())
				{
					// success	
					lv_label_set_text(ui_dialogContent,"SD卡 格式化 #ff0088 成功#");
					noticeAnimationStart(2);
				}
				else
				{
					//failed 
					lv_label_set_text(ui_dialogContent,"SD卡 格式化 #ff0088 失败#");
					noticeAnimationStart(2);
				}
				break;
			
			case AD_BACK:
			case KEY_BACK:
				global_avi_exit = 1;
				if (g_img_from_page == PAGE_WECHAT) 
				{ 
					lv_page_select(PAGE_WECHAT); 
					s_wechat_focus_idx = 4; 
					wechat_update_focus_style(); 
				} 
				else 
				{ 
					lv_page_select(PAGE_HOME); 
					camera_gvar.pagebtn_index = 2; // 看你主页相机是第几个 
					camera_gvar.immediately_reflash_flag = 1;
				}
				break;
			
			case KEY_BROWSE:
				global_avi_exit = 1;
				lv_page_select(PAGE_CAMERA);
				break;

			case KEY_POWEROFF:
				global_avi_exit = 1;
				lv_page_select(PAGE_POWEROFF);
				break;


			default:
				break;
		}
	}
	
}


void ui_albumPage_screen_init(){

	static lv_style_t albumPageStyle;

	album_group = lv_group_create();
	lv_indev_set_group(indev_keypad, album_group);
	group_cur = album_group;

	lv_style_reset(&albumPageStyle);
	lv_style_init(&albumPageStyle);
	lv_style_set_width(&albumPageStyle, lv_obj_get_width(lv_scr_act()));
	lv_style_set_height(&albumPageStyle, lv_obj_get_height(lv_scr_act()));
	lv_style_set_bg_color(&albumPageStyle, lv_color_hex(0x000000));	
	//lv_style_set_bg_opa(&albumPageStyle, 0);	
	lv_style_set_shadow_color(&albumPageStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_width(&albumPageStyle, 0);
	// lv_style_set_outline_color(&menuPanelStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_pad_top(&albumPageStyle, 0);
	lv_style_set_pad_bottom(&albumPageStyle, 0);
	lv_style_set_pad_left(&albumPageStyle,  0);
 	lv_style_set_pad_right(&albumPageStyle, 0);
	lv_style_set_radius(&albumPageStyle,0);


	ui_albumPage = lv_obj_create(lv_scr_act());
	lv_obj_clear_flag( ui_albumPage, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	curPage_obj = ui_albumPage;
	lv_obj_add_style(ui_albumPage, &albumPageStyle, 0);
	lv_obj_set_style_text_font(ui_albumPage, &ui_font_alimamaShuHei16, 0);

	lv_obj_add_event_cb(curPage_obj, event_handler, LV_EVENT_ALL, NULL);


	ui_albumTopBar = lv_obj_create(ui_albumPage);
	lv_obj_set_width( ui_albumTopBar, lv_pct(100));
	lv_obj_set_height( ui_albumTopBar, lv_pct(12));
	lv_obj_set_align( ui_albumTopBar, LV_ALIGN_TOP_MID );
	lv_obj_clear_flag( ui_albumTopBar, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_radius(ui_albumTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_albumTopBar, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_albumTopBar, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_albumTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_albumTopBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_albumTopBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_albumTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_albumTopBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);

	// ui_aSdIconImg = lv_img_create(ui_albumTopBar);
	// lv_img_set_src(ui_aSdIconImg, &ui_img_img_icon32x32_sdicon_32x32_png);
	// lv_obj_set_width( ui_aSdIconImg, LV_SIZE_CONTENT);  /// 1
	// lv_obj_set_height( ui_aSdIconImg, LV_SIZE_CONTENT);   /// 1
	// lv_obj_set_x( ui_aSdIconImg, 32 );
	// lv_obj_set_y( ui_aSdIconImg, 0 );
	// lv_obj_add_flag( ui_aSdIconImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	// lv_obj_clear_flag( ui_aSdIconImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags

	// ui_aSdStaLabel = lv_label_create(ui_aSdIconImg);
	// lv_obj_set_width( ui_aSdStaLabel, LV_SIZE_CONTENT);  /// 1
	// lv_obj_set_height( ui_aSdStaLabel, LV_SIZE_CONTENT);   /// 1
	// lv_obj_set_align( ui_aSdStaLabel, LV_ALIGN_CENTER );
	// lv_label_set_text(ui_aSdStaLabel,LV_SYMBOL_OK); //""
	// lv_obj_set_style_text_color(ui_aSdStaLabel, lv_color_hex(0x05F80A), LV_PART_MAIN | LV_STATE_DEFAULT );
	// lv_obj_set_style_text_opa(ui_aSdStaLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	
	ui_aSdIconImg = lv_img_create(ui_albumTopBar);
	lv_img_set_src(ui_aSdIconImg, &iconSdc);
	lv_obj_set_width( ui_aSdIconImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_aSdIconImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_x( ui_aSdIconImg, 32 );
	lv_obj_set_y( ui_aSdIconImg, 0 );
	lv_obj_add_flag( ui_aSdIconImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_aSdIconImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags

	if(camera_gvar.sd_online)
		lv_obj_clear_flag(ui_aSdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
	else
		lv_obj_add_flag(ui_aSdIconImg, LV_OBJ_FLAG_HIDDEN); 

#if 0
	ui_albumPrinterFlag = lv_img_create(ui_albumTopBar);
	lv_img_set_src(ui_albumPrinterFlag, &iconPrinter);
	lv_obj_set_width( ui_albumPrinterFlag, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_albumPrinterFlag, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_x( ui_albumPrinterFlag, -32 );
	lv_obj_set_y( ui_albumPrinterFlag, 0 );
	lv_obj_set_align( ui_albumPrinterFlag, LV_ALIGN_RIGHT_MID );
	lv_obj_add_flag( ui_albumPrinterFlag, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_albumPrinterFlag, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	//lv_obj_add_flag( ui_albumPrinterFlag, LV_OBJ_FLAG_HIDDEN );   /// Flags
#endif

#if 1
	ui_albumBatImg = lv_img_create(ui_albumTopBar);
	lv_img_set_src(ui_albumBatImg, ui_imgset_iconBat[get_batlevel()]);
	lv_obj_set_width( ui_albumBatImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_albumBatImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_albumBatImg, LV_ALIGN_RIGHT_MID );
	lv_obj_add_flag( ui_albumBatImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_albumBatImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
#else
	ui_aBatIconLabel = lv_label_create(ui_albumTopBar);
	lv_obj_set_width( ui_aBatIconLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_aBatIconLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_aBatIconLabel, LV_ALIGN_TOP_RIGHT );
	lv_label_set_text(ui_aBatIconLabel,LV_SYMBOL_BATTERY_1);//""
	lv_obj_set_style_text_color(ui_aBatIconLabel, lv_color_hex(0x4AA1FF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_aBatIconLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_aBatIconLabel, &lv_font_montserrat_22, LV_PART_MAIN| LV_STATE_DEFAULT);
#endif

	ui_albumIconImg = lv_img_create(ui_albumTopBar);
	lv_img_set_src(ui_albumIconImg, &iconFlagPlay);
	lv_obj_set_width( ui_albumIconImg, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_albumIconImg, LV_SIZE_CONTENT);   /// 1
	lv_obj_add_flag( ui_albumIconImg, LV_OBJ_FLAG_ADV_HITTEST );   /// Flags
	lv_obj_clear_flag( ui_albumIconImg, LV_OBJ_FLAG_SCROLLABLE );    /// Flags



	ui_albumBtmBar = lv_obj_create(ui_albumPage);
	lv_obj_set_width( ui_albumBtmBar, lv_pct(100));
	lv_obj_set_height( ui_albumBtmBar, lv_pct(12));
	lv_obj_set_align( ui_albumBtmBar, LV_ALIGN_BOTTOM_MID );
	lv_obj_clear_flag( ui_albumBtmBar, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	lv_obj_set_style_radius(ui_albumBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(ui_albumBtmBar, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_bg_opa(ui_albumBtmBar, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_albumBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui_albumBtmBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui_albumBtmBar, 5, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui_albumBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui_albumBtmBar, 0, LV_PART_MAIN| LV_STATE_DEFAULT);

	ui_fileFormatLabel = lv_label_create(ui_albumBtmBar);
	lv_obj_set_width( ui_fileFormatLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_fileFormatLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_fileFormatLabel, LV_ALIGN_BOTTOM_LEFT );
	
	//lv_label_set_text(ui_fileFormatLabel,"MOV00003.AVI");
	#if 1
	if(camera_gvar.album_filenums)
	{
		lv_label_set_text(ui_fileFormatLabel,album_cur_selected->name);
	}
	else
		lv_label_set_text(ui_fileFormatLabel,"NULL");

	#else
	lv_label_set_text(ui_fileFormatLabel,ablumlist[camera_gvar.album_fileindex].name);
	#endif

	lv_obj_set_style_text_color(ui_fileFormatLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_fileFormatLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_fileFormatLabel, &lv_font_montserrat_16, LV_PART_MAIN| LV_STATE_DEFAULT);


	ui_albumPrevBtn = lv_obj_create(ui_albumPage);
	lv_obj_set_width( ui_albumPrevBtn, 28);
	lv_obj_set_height( ui_albumPrevBtn, 25);
    lv_obj_align(ui_albumPrevBtn,LV_ALIGN_LEFT_MID,0,0);
	lv_obj_set_style_bg_opa(ui_albumPrevBtn, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_albumPrevBtn, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_bg_img_src( ui_albumPrevBtn, ui_imgset_iconPrev[0], LV_PART_MAIN | LV_STATE_DEFAULT );


	ui_albumNextBtn = lv_obj_create(ui_albumPage);
	lv_obj_set_width( ui_albumNextBtn, 28);
	lv_obj_set_height( ui_albumNextBtn, 25);
    lv_obj_align(ui_albumNextBtn,LV_ALIGN_RIGHT_MID,0,0);
	lv_obj_set_style_bg_opa(ui_albumNextBtn, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(ui_albumNextBtn, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_bg_img_src( ui_albumNextBtn, ui_imgset_iconNext[0], LV_PART_MAIN | LV_STATE_DEFAULT );

#if 0
	ui_fileNumsLabel = lv_label_create(ui_albumBtmBar);
	lv_obj_set_width( ui_fileNumsLabel, LV_SIZE_CONTENT);  /// 1
	lv_obj_set_height( ui_fileNumsLabel, LV_SIZE_CONTENT);   /// 1
	lv_obj_set_align( ui_fileNumsLabel, LV_ALIGN_BOTTOM_RIGHT );
	lv_label_set_text(ui_fileNumsLabel,"0011/0011");
	lv_obj_set_style_text_color(ui_fileNumsLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
	lv_obj_set_style_text_opa(ui_fileNumsLabel, 255, LV_PART_MAIN| LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui_fileNumsLabel, &lv_font_montserrat_16, LV_PART_MAIN| LV_STATE_DEFAULT);
#endif
	// lv_obj_add_event_cb(ui_albumPage, ui_event_albumPage, LV_EVENT_ALL, NULL);

	#if 1// notic dialog
	ui_dialogPanel = lv_obj_create(ui_albumPage);
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
	lv_label_set_text(ui_dialogContent,"SD卡 #ff0088 格式化成功#");

    // lv_obj_t *btn_cancel = lv_btn_create(ui_dialogPanel);
    // lv_obj_set_size(btn_cancel, 36, 26);  
	// lv_obj_set_align( btn_cancel, LV_ALIGN_BOTTOM_LEFT );

	// lv_obj_t *label_cancel = lv_label_create(btn_cancel);
    // lv_label_set_text(label_cancel, "取消");
	// lv_obj_set_align( label_cancel, LV_ALIGN_CENTER );

    // lv_obj_t *btn_confirm = lv_btn_create(ui_dialogPanel);
    // lv_obj_set_size(btn_confirm, 36, 26);  
	// lv_obj_set_align( btn_confirm, LV_ALIGN_BOTTOM_RIGHT );
	
    // lv_obj_t *label_confirm = lv_label_create(btn_confirm);
    // lv_label_set_text(label_confirm, "继续");
	// lv_obj_set_align( label_confirm, LV_ALIGN_CENTER );

	#endif

	ui_volPanel = lv_obj_create(ui_albumPage);	
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
	
	{
		extern void mute_speaker(uint8_t mute);
		mute_speaker(0);

	}



}
