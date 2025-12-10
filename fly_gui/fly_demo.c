/**
 * @file lv_demo_widgets.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
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
#include "video_app/video_app.h"
#include "lv_demo_widgets.h"
#include "openDML.h"
#include "osal/mutex.h"
#include "avidemux.h"
#include "avi/play_avi.h"
#include "playback/playback.h"
#include "lib/vef/video_ef.h"

#include "ui_language.h"
#include "keyScan.h"
#include "vpp_ipf_src.h"
#include "fly_demo.h"
#include "clock_app.h"
#include "battery.h"
#include "hal/spi_nor.h"
#include "device.h"
#include "lib/syscfg/syscfg.h"
#include "syscfg.h"

#include <csi_kernel.h>

#include "playback/playback.h"
#include "babyprotocol.h"

//用来描述用户配置存储在 SPI NOR 哪个扇区：flash 设备指针 + 起始地址 + 长度。
typedef struct 
{
	struct spi_nor_flash *flash;
	uint32_t addr;
	uint32_t size;
} user_cfg_info; 

user_cfg_info flycfg_info;


// ui旋转角度
#ifdef FLY_DEMO_4KEY_VERSION
#define VEDIO_LAYER_ROTATE	LCD_ROTATE_270
#else
#define VEDIO_LAYER_ROTATE	LCD_ROTATE_90//LCD_ROTATE_90  //LCD_ROTATE_270
#endif

// 状态机
// camera_global_t 包括当前/上一页面、各类动画计数、自动关机计数、欢迎界面次数、配对超时、pair_success等
camera_global_t camera_gvar;
cam_set_t camSetParam;

uint32_t USER_KEY_EVENT = 0;

uint8_t settSub_view=0;
uint8_t gameSub_view=0;

void visible_setting_submenu0(void);
void invisible_setting_submenu0(void);
void back_settMenu(void);


/*key group用于键盘/按键焦点管理*/
lv_group_t * group_cur =NULL;
lv_group_t * home_group;
lv_group_t * rec_group;
lv_group_t * camera_group;
lv_group_t * setting_group;
lv_group_t * music_group;
lv_group_t * album_group;
lv_group_t * game_group;

lv_obj_t * curPage_obj;

/*usbPage objects*/
lv_obj_t * ui_usbPage;    //usb

/*poweronPage objects*/
lv_obj_t * ui_poweronPage;

/*poweroffPage objects*/
lv_obj_t * ui_poweroffPage;

/*homePage objects*/
lv_obj_t * ui_homePage;
lv_obj_t * recPage_btn;	
lv_obj_t * intercomPage_btn;//对讲顶部电池图标
lv_obj_t * cameraPage_btn;
lv_obj_t * settPage_btn;
lv_obj_t * albumPage_btn;
lv_obj_t * gamePage_btn;
lv_obj_t * musicPage_btn;
lv_obj_t * label_rec;
USR_PAGE_BTN user_pagebtn_list[6];
lv_obj_t * ui_homeBatImg; 

/*videoPage objects*/
lv_obj_t *ui_videoPage;
lv_obj_t *ui_videoTopBar;
lv_obj_t *ui_videoIconImg;
lv_obj_t *ui_vsdIconImg;
lv_obj_t *ui_vSdStaLabel;
lv_obj_t *ui_vfilmIconImg;
lv_obj_t *ui_vFilmQualityLabel;
//lv_obj_t *ui_vBatIconLabel;
lv_obj_t *ui_vBatImg;
lv_obj_t *ui_videoBtmBar;
lv_obj_t *ui_vTimeIconLabel;
lv_obj_t *ui_RecTimeIconLabel;

lv_obj_t *ui_voiceinfo_lable;

/*intercomPage objects*/
lv_obj_t *ui_intercomPage;
lv_obj_t *ui_intercomTopBar;
lv_obj_t *ui_intercomBtmBar;
lv_obj_t *ui_isdIconImg;
lv_obj_t *ui_iSdStaLabel;
lv_obj_t *ui_intercomIconImg;
lv_obj_t *ui_iBatIconLabel;
lv_obj_t *ui_intercomBatImg;


/*cameraPage objects*/
lv_obj_t *ui_cameraPage;
lv_obj_t *ui_camTopBar;
lv_obj_t *ui_csdIconImg;
lv_obj_t *ui_cSdStaLabel;
lv_obj_t *ui_camIconImg;
lv_obj_t *ui_cBatIconLabel;
lv_obj_t *ui_camBatImg;
lv_obj_t *ui_camPrinterFlag;
lv_obj_t *ui_camBtmBar;
lv_obj_t *ui_DvIconImg;
lv_obj_t *ui_cTimeIconLabel;
lv_obj_t *ui_photoQualityLabel;
lv_obj_t *ui_focusBtn;
lv_obj_t *ui_focusImg;

#if 1
/*settingPage objects */
lv_obj_t *ui_settingPage;
lv_obj_t *ui_settNextBtn;
lv_obj_t *ui_settPrevBtn;
lv_obj_t *ui_settTabView;
lv_obj_t *ui_settTopBar;
//lv_obj_t *ui_sBatLabel;
lv_obj_t *ui_settBatImg;
lv_obj_t *ui_sTimeIconLabel;

/*gamePage objects*/
lv_obj_t *ui_gamePage;
lv_obj_t *ui_gameTopBar;
lv_obj_t *ui_gameTabView;
lv_obj_t *ui_gamePage;
lv_obj_t *ui_gamePrevBtn;
lv_obj_t *ui_gameNextBtn;
lv_obj_t *ui_gameBatImg;
lv_obj_t *ui_game_opview;
#endif
/*albumPage objects*/
lv_obj_t *ui_albumPage;
lv_obj_t *ui_albumTopBar;
lv_obj_t *ui_aSdIconImg;
lv_obj_t *ui_aSdStaLabel;
lv_obj_t *ui_albumPrinterFlag;
//lv_obj_t *ui_aBatIconLabel;
lv_obj_t *ui_albumBatImg;
lv_obj_t *ui_albumIconImg;
lv_obj_t *ui_fileQualityLabel;
lv_obj_t *ui_albumBtmBar;
lv_obj_t *ui_fileFormatLabel;
lv_obj_t *ui_fileNumsLabel;
lv_obj_t *ui_albumPrevBtn;
lv_obj_t *ui_albumNextBtn;
#if 0
/*musicPage objects*/
lv_obj_t *ui_musicPage;
lv_obj_t *ui_musicTopBar;
lv_obj_t *ui_musicIconImg;
lv_obj_t *ui_msdIconImg;
lv_obj_t *ui_musicBatImg;
lv_obj_t *ui_musiBtmBar;
lv_obj_t *ui_playPauseIcon;
lv_obj_t *ui_playTimeLabel;
lv_obj_t *ui_playTimeIndex;
lv_obj_t *ui_songNumLabel;
lv_obj_t *ui_musiclist;
lv_obj_t *ui_musicbtn0;
lv_obj_t *ui_musicbtn1;
#endif
lv_obj_t * ui_dialogPanel; 
lv_obj_t * ui_dialogContent;

lv_obj_t * ui_volPanel;
lv_obj_t * ui_lisglabel;
lv_obj_t *ui_volumeLevels[10];

lv_obj_t * mic_img;

lv_obj_t *ui_signalPanel;
lv_obj_t *ui_signalLevels[4];


lv_obj_t * ui_pairPanel;
lv_obj_t * ui_pairTeimlabel;


lv_obj_t * ui_speedinfo_rx_obj;
lv_obj_t * ui_speedinfo_tx_obj;
lv_obj_t * ui_speedinfo_id_obj;
lv_obj_t * ui_speedinfo_num_obj;

extern uint16_t rx_speed,tx_speed;
extern uint8_t dispnum;

#ifdef DISPLAY_DEBUGINFO_ENABLE 

char msgtx_str[20];
char msgrx_str[20];
char msgid_str[40];
char msgnum_str[100];

extern int8 bbm_rx_rssi_get(void);
extern int8 bbm_rx_evm_get(void);
extern uint16 bbm_rx_freq_get(void);
extern int32 bbm_rx_freq_offset_get(void);

extern uint8_t mcs_send;
// extern uint8_t frmtype_send ;
// extern int8_t  bbm_disp_rssi;
// extern int16_t bbm_disp_freq;
// extern int8_t bbm_disp_bgr;
#endif
char pair_str[100];

char magicVoice_str[20];

extern Vpp_stream photo_msg;
extern lcd_msg lcd_info;
extern volatile vf_cblk g_vf_cblk;
extern gui_msg gui_cfg;
extern volatile uint8_t bbm_displaydecode_run;

extern uint8_t g_camera_from_page;
extern uint8_t g_img_from_page;
extern uint8_t g_video_intercom_mode;
extern void video_intercom_start(void);
extern void video_intercom_stop(void);

volatile uint8  printer_action = 0;
uint8_t uvc_start = 0;
uint8_t rec_open;
uint8_t enlarge_glo = 10;
uint8_t rahmen_open;
uint8_t name_rec_photo[32];


struct lv_time rec_time;

volatile uint8_t ipf_update_flag =0;
uint8_t ipf_index_num=0;

void format_list();
void language_list();
void sound_list();
void ios_list();
void recres_list();
void game_list();
void camres_list();
void cycle_list();
void continous_shot_list();
uint32_t file_mode (const char *mode);
void start_record_thread(uint8_t video_fps,uint8_t audio_frq);
uint8_t send_stop_record_cmd();

uint8_t  lcd_pair_success;  // 
static uint8_t video_talk_audio_on = 0; //记录当前是否已经开启视频对讲的语音

const lv_img_dsc_t *ui_imgset_iconBat[5] = {&iconBat0,&iconBat1,&iconBat2,&iconBat3,&iconBat4};

const lv_img_dsc_t *ui_imgset_iconFocus[2] = {&iconFocus,&iconFocusP};
const lv_img_dsc_t *ui_imgset_iconNext[2] = {&iconNext,&iconNextP};
const lv_img_dsc_t *ui_imgset_iconPrev[2] = {&iconPrev,&iconPrevP};
const lv_img_dsc_t *ui_imgset_iconHomeintercom[2] = {&iconHomeIntercom0,&iconHomeIntercom1};
const lv_img_dsc_t *ui_imgset_iconHomeCamera[2] = {&iconHomeCamera0,&iconHomeCamera1};
const lv_img_dsc_t *ui_imgset_iconHomeMenu[2] = {&iconHomeMenu0,&iconHomeMenu1};
const lv_img_dsc_t *ui_imgset_iconHomePlayer[2] = {&iconHomePlayer0,&iconHomePlayer1};



// 两帧 VGA（640x480） YUV(或类似) 缓冲区，放在外部 PSRAM
uint8_t vga_room[2][640*480+640*480/2]__attribute__ ((aligned(4),section(".psram.src")));
// jpg解码缓冲区50k，放在外部 PSRAM
uint8_t jpg_psram_mem[50*1024] __attribute__ ((aligned(4),section(".psram.src")));



//清零 h/m/s，录制时间用
void lv_time_reset(struct lv_time *time_now){
	time_now->lv_hour = 0;
	time_now->lv_min  = 0;
	time_now->lv_sec  = 0;
}

// 每秒调用一次，秒++，进位到分和小时，小时最大 99，再归零
void lv_time_add(struct lv_time *time_now){
	if(time_now->lv_sec == 59){
		if(time_now->lv_min == 59){
			if(time_now->lv_hour >= 99){
				time_now->lv_hour = 0;
				time_now->lv_min  = 0;
				time_now->lv_sec  = 0;				
			}else{
				time_now->lv_hour++;
				time_now->lv_min  = 0;
				time_now->lv_sec  = 0;				
			}
		}else{
			time_now->lv_min++;
			time_now->lv_sec = 0;
		}
	}else{
		time_now->lv_sec++;
	}
}

// 把时间格式化到字符串里，然后 lv_label_set_text 显示到 label 上。
void lv_time_display(lv_obj_t * p_label,struct lv_time *time_now,char *recolor_val){
	static char time_str[20];
	printf("%02d:%02d:%02d\r\n",time_now->lv_hour,time_now->lv_min,time_now->lv_sec);
	
	if(recolor_val)
		sprintf(time_str,"%s %02d:%02d:%02d#",recolor_val,time_now->lv_hour,time_now->lv_min,time_now->lv_sec);
	else
		sprintf(time_str,"%02d:%02d:%02d",time_now->lv_hour,time_now->lv_min,time_now->lv_sec);

	lv_label_set_text(p_label,time_str);
	
}
void lv_clock_display(lv_obj_t * p_label, user_clock_t *rtc,char *recolor_val){
	static char time_str[24];

	if(recolor_val)
		sprintf(time_str,"%s %04d/%02d/%02d %02d:%02d:%02d#",recolor_val,rtc->year,rtc->month,rtc->day,rtc->hour,rtc->minute,rtc->second);
	else
		sprintf(time_str,"%04d/%02d/%02d %02d:%02d:%02d",rtc->year,rtc->month,rtc->day,rtc->hour,rtc->minute,rtc->second);

	lv_label_set_text(p_label,time_str);
	
}


#if 1
void dv_flash_onoff(uint8_t flag)
{
	if(flag)
		lv_obj_clear_flag(ui_DvIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
	else
		lv_obj_add_flag(ui_DvIconImg, LV_OBJ_FLAG_HIDDEN); 

}

void rec_dv_flash(uint8_t flash){

		if(flash == 1)
			lv_img_set_src(ui_DvIconImg,&iconDv_w);
		else
			lv_img_set_src(ui_DvIconImg,&iconDv_r);
}
#endif




extern uint8_t osd_menu565_buf[SCALE_HIGH*SCALE_WIDTH*2];

//关机页按键事件&抓屏保存
void ui_event_poweroffPage(lv_event_t * e){

    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
 	uint32_t* key_val = (uint32_t*)e->param;
	static file_write_cnt =8;
	char name[128];
    void *rgb_fd = NULL;

	if(event_code==USER_KEY_EVENT)
    {
		switch(*key_val)
		{
			case AD_LEFT:
			break;

			case AD_RIGHT:
			break;

			case AD_PRESS:
			if((file_write_cnt)&&(camera_gvar.page_cur==PAGE_POWEROFF))
			{
				sprintf(name, "0:%d.rgb", file_write_cnt);
				rgb_fd = osal_fopen(name, "wb+");
				if (rgb_fd) {
					
					osal_fwrite(osd_menu565_buf,1,(SCALE_HIGH*SCALE_WIDTH*2),rgb_fd);
					osal_fclose(rgb_fd);
					os_printf("## finsish rgb write\n ");
				}
				file_write_cnt--;
			}
            break;

			default:
			break;
		}
	}

	
}

// 通用事件处理函数，所有页面的根对象都注册同一个事件处理函数，根据当前页面做不同处理。
void event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	struct lcdc_device *lcd_dev;
	struct scale_device *scale_dev;
	scale_dev = (struct scale_device *)dev_get(HG_SCALE1_DEVID);
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	
	// printf("## event_handler=%d \n",code);

	if((code == LV_EVENT_CLICKED) ||(code==USER_KEY_EVENT))
	{
		LV_LOG_USER("Clicked");
//		printf(" # Clicked camera_gvar.page_cur=%d ,event_handler=%d USER_KEY_EVENT=%d  \n",camera_gvar.page_cur,code,USER_KEY_EVENT);
		
		if(camera_gvar.page_cur == PAGE_HOME){
			ui_event_homePage(e);
		}
		else if(camera_gvar.page_cur == PAGE_INTERCOM){
			ui_event_intercomPage(e);
		}
		else if(camera_gvar.page_cur == PAGE_CAMERA){
			ui_event_cameraPage(e);
		}
		else if(camera_gvar.page_cur == PAGE_ALBUM){
			ui_event_albumPage(e);
		}
		else if(camera_gvar.page_cur == PAGE_WECHAT){
            ui_event_wechatPage(e);  
        }
//		else if(camera_gvar.page_cur == PAGE_SET){
//			ui_event_settPage(e);
//		}
//		else if(camera_gvar.page_cur == PAGE_VIDEO){
//			ui_event_recPage(e);
//		}
//		else if(camera_gvar.page_cur == PAGE_GAME){
//			ui_event_gamePage(e);
//		}
//		else if(camera_gvar.page_cur == PAGE_MUSIC){
//			ui_event_musicPage(e);
//		}

		else if(camera_gvar.page_cur == PAGE_POWEROFF){
			ui_event_poweroffPage(e);
		}
	
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}


// 电池状态图标处理函数
void batteryStatusProcess(void)
{
	uint8_t bt_status=get_batlevel();
	if(camera_gvar.page_cur == PAGE_HOME)  // ⭐ 添加主界面处理
		lv_img_set_src(ui_homeBatImg, ui_imgset_iconBat[bt_status]);
	else if(camera_gvar.page_cur == PAGE_INTERCOM)//photo
		lv_img_set_src(ui_intercomBatImg, ui_imgset_iconBat[bt_status]);
	else if (camera_gvar.page_cur == PAGE_WECHAT) 
            lv_img_set_src(ui_wechatBattImg, ui_imgset_iconBat[bt_status]);
	// else if(camera_gvar.page_cur == PAGE_CAMERA)//photo
	// 	lv_img_set_src(ui_camBatImg, ui_imgset_iconBat[bt_status]);
	// else if(camera_gvar.page_cur == PAGE_ALBUM)//playback
	// 	lv_img_set_src(ui_albumBatImg, ui_imgset_iconBat[bt_status]);
	//else if(camera_gvar.page_cur == PAGE_VIDEO)//rec
	//	lv_img_set_src(ui_vBatImg, ui_imgset_iconBat[bt_status]);
	// else if(camera_gvar.page_cur == PAGE_SET)//sett
	// 	lv_img_set_src(ui_settBatImg, ui_imgset_iconBat[bt_status]);
	// else if(camera_gvar.page_cur == PAGE_GAME)//game
	// 	lv_img_set_src(ui_gameBatImg, ui_imgset_iconBat[bt_status]);
	// else if(camera_gvar.page_cur == PAGE_MUSIC)//music
	// 	lv_img_set_src(ui_musicBatImg, ui_imgset_iconBat[bt_status]);

}

//状态滤波器，把可能抖动的status信号变成稳定的used_status
static uint8_t status_filter(uint8_t status)
{
    static uint8_t used_status = 3;
    static uint8_t old_status;
    static uint8_t sta_counter;

    if (old_status != status) {
        sta_counter = 0;
        old_status = status;
    } else {
        sta_counter++;
		//只有当输入 status 连续出现 4 次相同值，才会把这个值认定为“有效值”并更新 used_status
        if (sta_counter == 4) {
            used_status = status;
        }
    }
    return used_status;
}

//SD 卡在线状态检测 + UI 图标显示控制
void sdcStatusProcess(void)
{
	uint8_t sd_status ;
	static uint8_t sd_backstatus =0;

	static uint8_t count=0;
	static uint8_t b_sta=0xff;

	sd_status = status_filter(get_sd_status());

	if(sd_status !=3)
	{
		camera_gvar.sd_online =1;
	}
	else
		camera_gvar.sd_online =0;

	// if(camera_gvar.sd_online==b_sta)
	// {
	// 	return ;
	// }
	b_sta =camera_gvar.sd_online;

	if(b_sta) // visiable
	{

		if(camera_gvar.page_cur == PAGE_INTERCOM)//photo
			lv_obj_clear_flag( ui_isdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
		else if(camera_gvar.page_cur == PAGE_CAMERA)//photo
			lv_obj_clear_flag( ui_csdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
		else if(camera_gvar.page_cur == PAGE_ALBUM)//playback
			lv_obj_clear_flag( ui_aSdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
		// else if(camera_gvar.page_cur == PAGE_VIDEO)//rec
		// 	lv_obj_clear_flag( ui_vsdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
		// else if(camera_gvar.page_cur == PAGE_MUSIC)//music
		// 	lv_obj_clear_flag( ui_msdIconImg, LV_OBJ_FLAG_HIDDEN );    /// Flags
		//else if(camera_gvar.page_cur == PAGE_SET)//sett
		//lv_obj_clear_flag(ui_settBatImg, ui_imgset_iconBat[bt_status]);
		//else if(camera_gvar.page_cur == PAGE_GAME)//game
		//lv_obj_clear_flag(ui_gameBatImg, ui_imgset_iconBat[bt_status]);

	}
	else  //invisiable
	{
		if(camera_gvar.page_cur == PAGE_INTERCOM)//photo
			lv_obj_add_flag( ui_isdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
		else if(camera_gvar.page_cur == PAGE_CAMERA)//photo
			lv_obj_add_flag( ui_csdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
		else if(camera_gvar.page_cur == PAGE_ALBUM)//playback
			lv_obj_add_flag( ui_aSdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
		// else if(camera_gvar.page_cur == PAGE_VIDEO)//rec
		// 	lv_obj_add_flag( ui_vsdIconImg, LV_OBJ_FLAG_HIDDEN );   /// Flags 
		// else if(camera_gvar.page_cur == PAGE_MUSIC)//music
		// 	lv_obj_add_flag( ui_msdIconImg, LV_OBJ_FLAG_HIDDEN );    /// Flags
		//else if(camera_gvar.page_cur == PAGE_SET)//sett
		//lv_obj_clear_flag(ui_settBatImg, ui_imgset_iconBat[bt_status]);
		//else if(camera_gvar.page_cur == PAGE_GAME)//game
		//lv_obj_clear_flag(ui_gameBatImg, ui_imgset_iconBat[bt_status]);

	}

}

void homePageIconFlash(void) //主界面 图标闪烁
{
	static uint8_t flash =0;
	static uint8_t count_down =7;
	uint8_t i =0;
	lv_obj_t *btn = NULL;


	if(camera_gvar.page_cur == PAGE_HOME){ // homePage icon half flash
		if(camera_gvar.immediately_reflash_flag) 
		{
			count_down=0;
			flash =0;
			camera_gvar.immediately_reflash_flag=0;
		}

		if(count_down)
			count_down--;
	

		if(count_down==0)
		{
			count_down=7;

			if(home_group != NULL){
				//btn = lv_group_get_focused(home_group);	
				//btn = ui_pagebtn_list[camera_gvar.pagebtn_index];
				flash ^=1;
				if(flash)
				{ 
					for(i=0;i<4;i++)   //主界面 按键个数 ；
					{
						if(camera_gvar.pagebtn_index ==i)
							lv_obj_set_style_bg_img_src(user_pagebtn_list[i].pagebtn, user_pagebtn_list[i].bimg, LV_PART_MAIN | LV_STATE_DEFAULT );
						else
							lv_obj_set_style_bg_img_src(user_pagebtn_list[i].pagebtn, user_pagebtn_list[i].nimg, LV_PART_MAIN | LV_STATE_DEFAULT );
					}

				}
				else
				{
					for(i=0;i<4;i++)  //主界面 按键个数 ；
					{
						lv_obj_set_style_bg_img_src(user_pagebtn_list[i].pagebtn, user_pagebtn_list[i].nimg, LV_PART_MAIN | LV_STATE_DEFAULT );
					}
				}
			}
		}
	}
}

#if 0

void dvp_vpp_reset()
{
	uint32 mj1_on,mj0_on;
	struct jpg_device *jpeg0_dev,*jpeg1_dev;	
	jpeg0_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);		
	dvp_close(dvp_test);
	vpp_close(vpp_test);
	mj0_on = jpg_is_online(jpeg0_dev);
	if(mj0_on)
		jpg_close(jpeg0_dev);
	jpeg1_dev = (struct jpg_device *)dev_get(HG_JPG1_DEVID);		
	mj1_on = jpg_is_online(jpeg1_dev);
	if(mj1_on)
		jpg_close(jpeg1_dev);

	
	os_printf("fv\r\n");
	if(mj1_on)
		jpg_open(jpeg1_dev);
	if(mj0_on)
		jpg_open(jpeg0_dev);	
	vpp_open(vpp_test);
	dvp_open(dvp_test);

}
#endif

//拍照
void takePhotoAnimationStart(void)
{
	if((camera_gvar.page_cur==PAGE_INTERCOM)||(camera_gvar.page_cur==PAGE_CAMERA))  //camera page
	{	
		// visiable shot icon
		if(camera_gvar.page_cur==PAGE_INTERCOM)
			bbm_displaydecode_run =0;  //停止对讲图传解码，避免拍照过程干扰图传

		lv_img_set_src(ui_focusImg, ui_imgset_iconFocus[0]);
		//显示 ui_focusImg 聚焦框
		lv_obj_clear_flag( ui_focusImg, LV_OBJ_FLAG_HIDDEN );   /// Flags
		camera_gvar.shot_anim_times =8;
	}
}

// 拍照动画处理函数
void takePhotoAnimationProcess(struct vpp_device * devs)
{
	if((camera_gvar.page_cur==PAGE_INTERCOM)||(camera_gvar.page_cur==PAGE_CAMERA))  //camera page
	{
		if(camera_gvar.shot_anim_times)
		{
			camera_gvar.shot_anim_times --;
			if(camera_gvar.shot_anim_times== 6)
			{
				// ==  2
				//change img
				lv_img_set_src(ui_focusImg, ui_imgset_iconFocus[1]);//切换聚焦图片，表示对焦成功
				
				#if 1
				{
					//关闭JPEG和VPP 表示开始执行拍照，避免图像链路再刷新
					struct jpg_device * jpeg0_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);		
					jpg_close(jpeg0_dev);
					vpp_close(devs);

				}
				
				
				#endif
			}
			else if(camera_gvar.shot_anim_times ==0)
			{
				// ==10
				//invisiable shot icon 隐藏聚焦框，恢复默认图片
				lv_obj_add_flag( ui_focusImg, LV_OBJ_FLAG_HIDDEN );   /// Flags
				lv_img_set_src(ui_focusImg, ui_imgset_iconFocus[0]);
				// test
				
				#if 1
				{	
					//重新打开 JPEG & VPP
					struct jpg_device * jpeg0_dev = (struct jpg_device *)dev_get(HG_JPG0_DEVID);
					jpg_open(jpeg0_dev);
					vpp_open(devs);
				}
				#endif
				//若为对讲页面，继续图传解码
				if(camera_gvar.page_cur==PAGE_INTERCOM)
					bbm_displaydecode_run =1;
			}
		}
	}
}
 
//下一张按钮按下动画触发
void nextp_AnimationStart(void)
{
	if(camera_gvar.nextp_anim_times==0)
	{
		if(camera_gvar.page_cur==PAGE_ALBUM) //camera page
		{
			//把相册页的“下一张按钮”背景图片换成按下态的图标。
			lv_obj_set_style_bg_img_src( ui_albumNextBtn, ui_imgset_iconNext[1], LV_PART_MAIN | LV_STATE_DEFAULT );
			camera_gvar.nextp_anim_times =2;
		}
		// else if(camera_gvar.page_cur==PAGE_GAME) 
		// {
		// 	lv_obj_set_style_bg_img_src( ui_gameNextBtn, ui_imgset_iconNext[1], LV_PART_MAIN | LV_STATE_DEFAULT );
		// 	camera_gvar.nextp_anim_times =2;
		// }
		// else if(camera_gvar.page_cur==PAGE_SET) 
		// {
		// 	lv_obj_set_style_bg_img_src( ui_settNextBtn, ui_imgset_iconNext[1], LV_PART_MAIN | LV_STATE_DEFAULT );
		// 	camera_gvar.nextp_anim_times =2;
		// }
	}

}
//上一张按钮按下动画触发
void prevp_AnimationStart(void)
{
		if(camera_gvar.prevp_anim_times ==0)
		{
			if(camera_gvar.page_cur==PAGE_ALBUM) //camera page
			{
				lv_obj_set_style_bg_img_src( ui_albumPrevBtn, ui_imgset_iconPrev[1], LV_PART_MAIN | LV_STATE_DEFAULT );
				camera_gvar.prevp_anim_times =2;
			}
			// else if(camera_gvar.page_cur==PAGE_GAME) 
			// {
			// 	lv_obj_set_style_bg_img_src( ui_gamePrevBtn, ui_imgset_iconPrev[1], LV_PART_MAIN | LV_STATE_DEFAULT );
			// 	camera_gvar.prevp_anim_times =2;
			// }
			// else if(camera_gvar.page_cur==PAGE_SET) 
			// {
			// 	lv_obj_set_style_bg_img_src( ui_settPrevBtn, ui_imgset_iconPrev[1], LV_PART_MAIN | LV_STATE_DEFAULT );
			// 	camera_gvar.prevp_anim_times =2;
			// }
		}
}

//定时器里做“恢复”
void nextprev_PressAnimationProcess(void)
{
		if(camera_gvar.nextp_anim_times)
		{
			camera_gvar.nextp_anim_times --;
			if(camera_gvar.nextp_anim_times ==0)
			{
				if(camera_gvar.page_cur==PAGE_ALBUM) //camera page			
					lv_obj_set_style_bg_img_src( ui_albumNextBtn, ui_imgset_iconNext[0], LV_PART_MAIN | LV_STATE_DEFAULT );
				// else if(camera_gvar.page_cur==PAGE_GAME) //game page			
				// 	lv_obj_set_style_bg_img_src( ui_gameNextBtn, ui_imgset_iconNext[0], LV_PART_MAIN | LV_STATE_DEFAULT );
				// else if(camera_gvar.page_cur==PAGE_SET) //sett page			
				// 	lv_obj_set_style_bg_img_src( ui_settNextBtn, ui_imgset_iconNext[0], LV_PART_MAIN | LV_STATE_DEFAULT );
			}
		}

		if(camera_gvar.prevp_anim_times)
		{
			camera_gvar.prevp_anim_times --;
			if(camera_gvar.prevp_anim_times ==0)
			{
				if(camera_gvar.page_cur==PAGE_ALBUM) //camera page			
					lv_obj_set_style_bg_img_src( ui_albumPrevBtn, ui_imgset_iconPrev[0], LV_PART_MAIN | LV_STATE_DEFAULT );
				// if(camera_gvar.page_cur==PAGE_GAME) //game page			
				// 	lv_obj_set_style_bg_img_src( ui_gamePrevBtn, ui_imgset_iconPrev[0], LV_PART_MAIN | LV_STATE_DEFAULT );
				// if(camera_gvar.page_cur==PAGE_SET) //sett page			
				// 	lv_obj_set_style_bg_img_src( ui_settPrevBtn, ui_imgset_iconPrev[0], LV_PART_MAIN | LV_STATE_DEFAULT );
				
			}
		}
}

	
void date_time_display(void)
{
	// if(camera_gvar.page_cur == PAGE_VIDEO)
	// 	lv_clock_display(ui_vTimeIconLabel,&sw_rtc,NULL);

	if((camera_gvar.page_cur==PAGE_INTERCOM)||(camera_gvar.page_cur==PAGE_CAMERA))  //camera page
		lv_clock_display(ui_cTimeIconLabel,&sw_rtc,NULL);
	
	// if(camera_gvar.page_cur == PAGE_SET)
	// 	lv_clock_display(ui_sTimeIconLabel,&sw_rtc,NULL);
}

extern struct system_status sys_status;
uint8_t get_wifi_connect_flag(void)
{
	return sys_status.wifi_connected;
}

//WIFI连接遮罩
void wifi_connect_process(void)
{
		
		if(camera_gvar.page_cur == PAGE_INTERCOM){

			if(get_wifi_connect_flag())
				lv_obj_set_style_bg_opa(ui_intercomPage, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
			else
				lv_obj_set_style_bg_opa(ui_intercomPage, 255, LV_PART_MAIN| LV_STATE_DEFAULT);

		}
	//printf("## sys_status.wifi_connected=%d\r\n",sys_status.wifi_connected);

}



#ifdef DISPLAY_DEBUGINFO_ENABLE
void display_msg(uint32 rx_speed,uint32 tx_speed,uint32 lcd_num,uint32 id){
	memset(msgtx_str,0,20);
	sprintf(msgtx_str,"rx:#ff0088 %02d#Kbyte",rx_speed);
	lv_label_set_text(ui_speedinfo_rx_obj,msgtx_str);
	memset(msgrx_str,0,20);
	sprintf(msgrx_str,"tx:#ff0088 %02d#Kbyte",tx_speed);
	lv_label_set_text(ui_speedinfo_tx_obj,msgrx_str);
	
	memset(msgid_str,0,40);
	//sprintf(msgid_str,"num:#ff0088 %02d#",lcd_num);
	sprintf(msgid_str,"num:#ff0088 %02d# fre:#ff0088 %02d# offset:#ff0088 %02d#",lcd_num, bbm_rx_freq_get()-2401,bbm_rx_freq_offset_get());
	lv_label_set_text(ui_speedinfo_id_obj,msgid_str);
	
	memset(msgnum_str,0,100);
	//sprintf(msgnum_str,"mcs:#ff0088 %02x#  frm:#ff0088 %02x#  rssi:#ff0088 %d#",mcs_send,frmtype_send,123);
	sprintf(msgnum_str,"mcs:#ff0088 %02x# evm:#ff0088 %d# sig:#ff0088 %d#",mcs_send,bbm_rx_evm_get(),bbm_rx_rssi_get());
	lv_label_set_text(ui_speedinfo_num_obj,msgnum_str);	
}
#endif

const char *voice_name[] = {
        "normal",
		"alien",
		"robot",
		"hight",
		"deep",
		"ough"
    };

extern uint8_t sundtype;
void display_magicsoud_type(void)
{
#ifdef BABY_UI_MAGICSOUND
	if(camera_gvar.page_cur==PAGE_INTERCOM) //camera page
	{
		memset(magicVoice_str,0,20); //清空缓冲区
		//sprintf(msgid_str,"num:#ff0088 %02d#",lcd_num);
		sprintf(magicVoice_str,"Voice:#ff0088 %s#",voice_name[sundtype]);
		lv_label_set_text(ui_voiceinfo_lable,magicVoice_str);
	}
#endif
}


void intercom_wifi(void)
{
	
	if(camera_gvar.page_cur==PAGE_INTERCOM) //camera page
	{
		if (video_talk_audio_on) {
            printf("intercom_wifi: stop video intercom audio (link reset)\r\n");
            video_intercom_stop();
            video_talk_audio_on = 0;
        }
		#if  BABY_ROLE_SERVER

				if(get_net_pair_status())
					userPairstop();

				ieee80211_disassoc_all(WIFI_MODE_AP);
				os_memset(sys_cfgs.bssid, 0, 6);
				syscfg_save();
				
				os_printf("*** intercom_wifi ap out  *** \r\n");
				ipf_update_flag = 1;
				rahmen_open = 0;
				lcd_pair_success = 0;
				lv_page_select(PAGE_INTERCOM);

		#else	
				if(get_net_pair_status())
					userPairstop();

				if (sys_cfgs.wifi_mode == WIFI_MODE_STA) {
					ieee80211_iface_stop(WIFI_MODE_STA);
					os_printf("*** intercom_wifi sta out  *** \r\n");
				}
				ipf_update_flag = 1;
				rahmen_open = 0;
				lcd_pair_success = 0;
				lv_page_select(PAGE_INTERCOM);
		#endif
	}
}

void noticeAnimationStart(uint8_t tm)
{
	if((camera_gvar.page_cur==PAGE_INTERCOM)||(camera_gvar.page_cur==PAGE_CAMERA)||(camera_gvar.page_cur==PAGE_ALBUM)) 
	{	
		// visiable shot icon
		lv_obj_clear_flag( ui_dialogPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
		camera_gvar.notice_anim_times =tm;
		printf(" ## noticeAnimationStart timers=%d\n",camera_gvar.notice_anim_times);

	}
}

// void noticeAnimationStop(void)
// {
// 	if((camera_gvar.page_cur==PAGE_INTERCOM)||(camera_gvar.page_cur==PAGE_CAMERA)||(camera_gvar.page_cur==PAGE_ALBUM)) 
// 	{	
// 		lv_obj_add_flag( ui_dialogPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
// 		camera_gvar.notice_anim_times =0;
// 		printf(" ## noticeAnimationStop \n");
// 	}
// }

void noticeDisplayProcess(void)
{
 	if((camera_gvar.page_cur==PAGE_INTERCOM)||(camera_gvar.page_cur==PAGE_CAMERA)||(camera_gvar.page_cur==PAGE_ALBUM)) 
 	{
 		if((camera_gvar.notice_anim_times)&&(camera_gvar.notice_anim_times!=255))
 		{
 			camera_gvar.notice_anim_times --;
 			printf(" ## notice_anim_times=%d \n",camera_gvar.notice_anim_times);

		if(camera_gvar.notice_anim_times== 0)
 			{
 				lv_obj_add_flag( ui_dialogPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
 			}
 		}
	}
}
	

void render_vol_level(uint8_t vol)
{

	if(vol>0)
	{
		if(vol>6)
			lv_label_set_text(ui_lisglabel, LV_SYMBOL_VOLUME_MAX);
		else
			lv_label_set_text(ui_lisglabel, LV_SYMBOL_VOLUME_MID);
		lv_obj_set_style_text_color(ui_lisglabel, lv_color_hex(0xFCA702), 0);
	}
	else
	{
		lv_label_set_text(ui_lisglabel, LV_SYMBOL_MUTE);
		lv_obj_set_style_text_color(ui_lisglabel, lv_color_hex(0x6D6C6C), 0);
	}

	for(uint8_t i=0 ; i<10;i++)
	{
		if(i<vol)
			lv_obj_set_style_bg_color(ui_volumeLevels[i], lv_color_hex(0xFCA702), LV_PART_MAIN | LV_STATE_DEFAULT );
		else
			lv_obj_set_style_bg_color(ui_volumeLevels[i], lv_color_hex(0x6D6C6C), LV_PART_MAIN | LV_STATE_DEFAULT );
	}
}

void volDisAnimationStart(void)
{
	if((camera_gvar.page_cur==PAGE_INTERCOM)||(camera_gvar.page_cur==PAGE_ALBUM)) //camera page
	{	
		// visiable shot icon
		lv_obj_clear_flag( ui_volPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
		printf(" ## volDisAnimationStart \n");

		camera_gvar.volume_anim_times =5;
	}
}

void volDisplayProcess(void)
{
	if((camera_gvar.page_cur==PAGE_INTERCOM)||(camera_gvar.page_cur==PAGE_ALBUM)) //camera page
	{
		if(camera_gvar.volume_anim_times)
		{
			camera_gvar.volume_anim_times --;
			printf(" ## volume_anim_times=%d \n",camera_gvar.volume_anim_times);

			if(camera_gvar.volume_anim_times== 0)
			{
				lv_obj_add_flag( ui_volPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
			}
		}

	}
	
}

void signalDisplayProcess(void)
{
	int32_t signal_strength=-100;
	int16_t signal_level=0;
	if(camera_gvar.page_cur==PAGE_INTERCOM) //camera page
	{
		{
			signal_strength=bbm_rx_rssi_get(); //取得射频接收 RSSI(dBm)
			//printf(" ## rssi=%d \n",signal_strength);
			//有图像传输
			if(dispnum)
			{
				if(signal_strength>-60)
				{
					signal_level=4;
				}
				else if(signal_strength>-70)
				{
					signal_level=3;
				}
				else if(signal_strength>-80)
				{
					signal_level=2;
				}
				else if(signal_strength>-90)
				{
					signal_level=1;
				}
			}
			else
			{
				signal_level=0;
			}
		    if(lv_obj_is_valid(ui_signalPanel))
			{
				for(uint8_t i=0 ; i<4;i++)
				{
					if(i<signal_level)
						lv_obj_set_style_bg_color(ui_signalLevels[i], lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT );
					else
						lv_obj_set_style_bg_color(ui_signalLevels[i], lv_color_hex(0x6D6C6C), LV_PART_MAIN | LV_STATE_DEFAULT );
				}
			}
			
		}

	}
}

uint8 conncet_flag;
void userPairStart(void)
{

	printf(" ## userPairStart \n");
	set_pair_mode(1);
	camera_gvar.pair_out_times =60;
	camera_gvar.pair_success=0;
	lcd_pair_success = 0;
	conncet_flag = 0;
	//只有在对讲页面下才显示配对面板
	if(camera_gvar.page_cur==PAGE_INTERCOM) //camera page
	{	
		// visiable shot icon
		// sprintf(pair_str,"配对 时间:#ff0088 %02d# ",camera_gvar.pair_out_times);
		sprintf(pair_str,"配对 时间:%02d",camera_gvar.pair_out_times);
		lv_label_set_text(ui_pairTeimlabel,pair_str);	
		
		lv_obj_clear_flag( ui_pairPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
	}
}
//终止配对
void userPairstop(void)
{
	printf(" ## userPairStop \n");
	set_pair_mode(0);
}
//配对成功
void userPairSuccess(void)
{
	printf(" ## userPairSuccess \n");
	camera_gvar.pair_success=1;
}





// void pairDisplayProcess(void)
// {
// 	if(camera_gvar.page_cur==PAGE_INTERCOM) //intercom page
// 	{
// 		// if(camera_gvar.pair_out_times)
// 		// {
// 		// 	camera_gvar.pair_out_times --;
// 		// 	printf(" ## pair_out_times=%d \n",camera_gvar.pair_out_times);

	
// 			if(camera_gvar.pair_success)
// 			{
// 				//lv_label_set_text(ui_pairTeimlabel,"配对 成功 ");	
					
// 				if(get_net_pair_status())
// 				{
// 					//camera_gvar.pair_out_times=12;
// 					set_pair_mode(0);
// 				}

// 				if(dispnum>0)
// 				{
// 					//lv_obj_add_flag( ui_pairPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
// 					//lv_obj_set_style_bg_opa(ui_intercomPage, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
// 					//lv_obj_set_style_bg_color(ui_intercomPage, lv_color_hex(0x000000), 0);
// 					camera_gvar.pair_out_times =0;
// 					lcd_pair_success = 1;
// 				}
// 			}
// 			else
// 			{
// 				if(get_net_pair_status())
// 					set_pair_mode(0);
// 					//lv_obj_add_flag( ui_pairPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
// 			}

// 		// }
// 	}
// }




void pairDisplayProcess(void)
{
	if(camera_gvar.page_cur==PAGE_INTERCOM) //intercom page
	{
		if(camera_gvar.pair_out_times)
		{
			if(camera_gvar.pair_success == 0)
				camera_gvar.pair_out_times --;             
			printf(" ## pair_out_times=%d \n",camera_gvar.pair_out_times);

			printf(" ## !! camera_gvar.pair_success =%d,sys_status.pair_success=%d \r\n",camera_gvar.pair_success,sys_status.pair_success);
			if(camera_gvar.pair_success)
			{
				lv_label_set_text(ui_pairTeimlabel,"配对 成功 ");	
					
				if(get_net_pair_status())
				{
					camera_gvar.pair_out_times=12;
					set_pair_mode(0);
				}

				printf(" ## !! dispnum =%d \n",dispnum);
				if(dispnum>0)
				{
					lv_obj_add_flag( ui_pairPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
					//lv_obj_set_style_bg_opa(ui_intercomPage, 0, LV_PART_MAIN| LV_STATE_DEFAULT);
					lv_obj_set_style_bg_color(ui_intercomPage, lv_color_hex(0x000000), 0);
					camera_gvar.pair_out_times =0;
					lcd_pair_success = 1;
					
					if(!video_talk_audio_on) {
                        printf("video intercom: enable audio tx\r\n");
						video_intercom_start();  
                        video_talk_audio_on = 1;
                    }
				}
			}
			else
			{
				if(camera_gvar.pair_out_times== 0)
				{
					if(get_net_pair_status())
						set_pair_mode(0);

					lv_label_set_text(ui_pairTeimlabel,"配对 超时");	
                    
					if(video_talk_audio_on) {
                        printf("video intercom: disable audio tx (timeout)\r\n");
                        video_intercom_stop();
                        video_talk_audio_on = 0;
                    }
					
					//lv_obj_add_flag( ui_pairPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
				}	
				else
				{
					//sprintf(pair_str,"配对 时间:#ff0088 %02d# ",camera_gvar.pair_out_times);
					sprintf(pair_str,"配对 时间:%02d",camera_gvar.pair_out_times);
					lv_label_set_text(ui_pairTeimlabel,pair_str);	
				}
			}
			// if(!get_net_pair_status())
			// {
			// 	camera_gvar.pair_out_times=0;
			// 	lv_obj_add_flag( ui_pairPanel, LV_OBJ_FLAG_HIDDEN );   /// Flags
			// }	
		}
	}
}












	
void set_autoPowerOff_times(uint16_t tm)
{
#ifdef AUTO_POWER_OFF_ENABLE
	camera_gvar.autoPowerOff_times=tm;
#endif
}

void autoPowerOff_process(void)
{
#ifdef AUTO_POWER_OFF_ENABLE
	if(camera_gvar.autoPowerOff_times)
	{
		camera_gvar.autoPowerOff_times --;
		printf(" ## autoPowerOff_times=%d \n",camera_gvar.autoPowerOff_times);
		if(camera_gvar.autoPowerOff_times==0)
		{
			//power_off();
			if(camera_gvar.page_cur!=PAGE_POWEROFF)
				lv_page_select(PAGE_POWEROFF);
		}

	}
#endif
}

extern void delay_open_lcd_process(void);

uint8_t photo_flag;
extern uint8 pcm_flag ;

//整机 UI 和对讲状态的“心跳主控”：音频是否打开、对讲是否有效、UI 状态是否正确
void timer_event(){
	static uint8_t half_flash =0;
	static uint8_t rec_num = 1;
	static uint32 timer_count = 0;
	static uint32 usb_tick = 0;
	static uint8_t usb_mode = 0;
	static uint8_t updata = 0;
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);	

	// takePhotoAnimationProcess(vpp_dev); 
	// nextprev_PressAnimationProcess();
	homePageIconFlash();  // //主界面 图标闪烁

	if(photo_flag)  // 配对后 检测对方是否退出
	{
		os_printf("photo_flag \r\n");
		photo_flag = 0;
		intercom_wifi(); //intercom状态
	}

	delay_open_lcd_process();




	//500ms
	if((timer_count%5)== 0) //  500ms
	{

		if(updata !=pcm_flag)
		{
			printf(" ##updata=%d pcm_flag=%d \n",updata,pcm_flag);
			updata = pcm_flag;
			if(updata)
			{
				lv_obj_clear_flag( mic_img, LV_OBJ_FLAG_HIDDEN );   
			}else{
   				lv_obj_add_flag(mic_img, LV_OBJ_FLAG_HIDDEN);
			}
		}

		batteryStatusProcess();  //电池刷新
		volDisplayProcess();     //音量界面
		signalDisplayProcess();  //信号刷新
		display_magicsoud_type(); //魔音刷新

		wifi_connect_process();
		noticeDisplayProcess();
		sdcStatusProcess();   
		date_time_display();

		if(camera_gvar.welcome_times)
		{
			camera_gvar.welcome_times--;
			if(camera_gvar.welcome_times==0)
			lv_page_select(camera_gvar.poweron_nextpage);
		}
	}
	//1s
	if((timer_count%10) == 0){ // one second 
		pairDisplayProcess();  //配对检测
	}	
	//2s
	if((timer_count%20) == 0){ // 
#ifdef DISPLAY_DEBUGINFO_ENABLE  //调试界面数据
		if(camera_gvar.page_cur == PAGE_INTERCOM)
			display_msg(rx_speed,tx_speed,dispnum,0x12356789);	
#endif
	}


	timer_count++;
}


// void timer_event(){
// 	static uint8_t half_flash =0;
// 	static uint8_t rec_num = 1;
// 	static uint32 timer_count = 0;
// 	static uint32 usb_tick = 0;
// 	static uint8_t usb_mode = 0;
// 	struct lcdc_device *lcd_dev;
// 	struct vpp_device *vpp_dev;
// 	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
// 	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);	

// 	// takePhotoAnimationProcess(vpp_dev); 
// 	// nextprev_PressAnimationProcess();
// 	// homePageIconFlash();  // //主界面 图标闪烁



// 	// if(camera_gvar.dly_save_cnt)
// 	// {
// 	// 	if(--camera_gvar.dly_save_cnt==0)
// 	// 	{
// 	// 		fly_info_save();
// 	// 	}
// 	// }
	
// 	if(photo_flag)  // 配对后 检测对方是否退出
// 	{
// 		os_printf("photo_flag \r\n");
// 		photo_flag = 0;
// 		intercom_wifi(); //intercom状态
// 	}


// 	delay_open_lcd_process();

// 	if((timer_count%5)== 0) //  500ms
// 	{
		
// 	//	wifi_connect_process();

// 		batteryStatusProcess();  //电池刷新

// 		volDisplayProcess();     //音量界面
// 		// noticeDisplayProcess();
// 		signalDisplayProcess();  //信号刷新

// 		display_magicsoud_type(); //魔音刷新

// 		// sdcStatusProcess();   
// 		//date_time_display();
// 		//musicPage_timer();
// 		// poweron back homepage
// 		if(camera_gvar.welcome_times)
// 		{
// 			camera_gvar.welcome_times--;
// 			if(camera_gvar.welcome_times==0)
// 			lv_page_select(camera_gvar.poweron_nextpage);
// 		}

// 	}

// 	if((timer_count%20) == 0){ // one second 
// #ifdef DISPLAY_DEBUGINFO_ENABLE  //调试界面数据
// 		if(camera_gvar.page_cur == PAGE_INTERCOM)
// 			display_msg(rx_speed,tx_speed,dispnum,0x12356789);	
// #endif

// 	}

// 	if((timer_count%10) == 0){ // one second 

// 		pairDisplayProcess();  //配对检测
// 		// autoPowerOff_process();






// 		// if((camera_gvar.page_cur == PAGE_INTERCOM)||(camera_gvar.page_cur == PAGE_CAMERA))
// 		// { // recPage dv flash
// 		// 	if(rec_open){
// 		// 		lv_time_add(&rec_time);
// 		// 		lv_time_display(ui_RecTimeIconLabel,&rec_time,"#FF0000");	
// 		// 		rec_dv_flash(rec_num&BIT(0));
// 		// 		rec_num++;
				
// 		// 		if(is_bbmavi_running()==0) // 异常退出 取消时间继续加减
// 		// 		{
// 		// 			rec_open=0;
// 		// 			lv_time_reset(&rec_time);
// 		// 			lv_time_display(ui_RecTimeIconLabel,&rec_time,NULL);
// 		// 			lv_obj_add_flag(ui_RecTimeIconLabel, LV_OBJ_FLAG_HIDDEN); 
// 		// 			dv_flash_onoff(rec_open); //stop
// 		// 		}

// 		// 	}

// 		// }
// 	}
	
// #if 0
// 	if((timer_count%40) == 0){
// 		if(scsi_count != usb_tick){
// 			usb_tick = scsi_count;
// 			if(usb_mode == 0){
// 				lv_page_select(PAGE_USB);
// 				usb_mode = 1;
// 			}
// 		}else{
// 			if(usb_mode == 1){
// 				usb_mode = 0;
// 				lv_page_select(PAGE_HOME);
// 			}
// 		}
// 	}
// #endif
// 	timer_count++;
// }


//注册全局心跳定时器
void lv_time_set(){
	static uint32_t user_data = 10;
	lv_timer_create(timer_event, 100,  &user_data);
}

//让设备进入深度睡眠模式
void goto_dsleep(void)
{
	os_printf("#### goto_dsleep \n");
	// {
	// 	struct hgadc_v0* hgadc_tm;
	// 	hgadc_tm = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);
	// 	adc_close((struct adc_device *)(hgadc_tm));
	// }

	struct system_sleep_param sleep_args;
    memset(&sleep_args,0,sizeof(sleep_args));
    sleep_args.wkup_io_sel[0] = PA_14;
    sleep_args.sleep_ms = 0;
    sleep_args.wkup_io_en = 0x00;
    sleep_args.wkup_io_edge = 0x01;
    system_sleep(SYSTEM_SLEEP_TYPE_RTCC, &sleep_args);

}

//开机 Power-On 页面
void lv_page_poweron_menu_config()
{
	static lv_style_t poweonMenuStyle;

	lv_style_reset(&poweonMenuStyle);
	lv_style_init(&poweonMenuStyle);
	lv_style_set_width(&poweonMenuStyle, lv_obj_get_width(lv_scr_act()));
	lv_style_set_height(&poweonMenuStyle, lv_obj_get_height(lv_scr_act()));
	lv_style_set_bg_color(&poweonMenuStyle, lv_color_hex(0x000000));	//0x101018
	// lv_style_set_bg_opa(&menuPanelStyle, 255);	
	lv_style_set_shadow_color(&poweonMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&poweonMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&poweonMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_width(&poweonMenuStyle, 0);
	lv_style_set_radius(&poweonMenuStyle,0);
	lv_style_set_pad_all(&poweonMenuStyle, 0);
	lv_style_set_pad_gap(&poweonMenuStyle,0);

	ui_poweronPage = lv_obj_create(lv_scr_act());
	lv_obj_add_style(ui_poweronPage, &poweonMenuStyle, 0);
	lv_obj_clear_flag( ui_poweronPage, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	curPage_obj = ui_poweronPage;
	backlight_on();
}

void usr_close_adc()
{
	struct hgadc_v0* hgadc_tm;
	hgadc_tm = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);
	adc_close((struct adc_device *)(hgadc_tm));
	adc_delete_channel((struct adc_device *)(hgadc_tm),PIN_ADKEY_IO);
	adc_delete_channel((struct adc_device *)(hgadc_tm),BAT_ADC_IO);

}

//关机页面+真正下电/休眠
void lv_page_poweroff_menu_config()
{
	static lv_style_t poweoffMenuStyle;

	lv_style_reset(&poweoffMenuStyle);
	lv_style_init(&poweoffMenuStyle);
	lv_style_set_width(&poweoffMenuStyle, lv_obj_get_width(lv_scr_act()));
	lv_style_set_height(&poweoffMenuStyle, lv_obj_get_height(lv_scr_act()));
	lv_style_set_bg_color(&poweoffMenuStyle, lv_color_hex(0x000000));	//0x101018
	// lv_style_set_bg_opa(&menuPanelStyle, 255);	
	lv_style_set_shadow_color(&poweoffMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&poweoffMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&poweoffMenuStyle, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_width(&poweoffMenuStyle, 0);
	lv_style_set_radius(&poweoffMenuStyle,0);
	lv_style_set_pad_all(&poweoffMenuStyle, 0);
	lv_style_set_pad_gap(&poweoffMenuStyle,0);

	ui_poweroffPage = lv_obj_create(lv_scr_act());
	lv_obj_add_style(ui_poweroffPage, &poweoffMenuStyle, 0);
	lv_obj_clear_flag( ui_poweroffPage, LV_OBJ_FLAG_SCROLLABLE );    /// Flags
	curPage_obj = ui_poweroffPage;

	lv_obj_add_event_cb(curPage_obj, event_handler, LV_EVENT_ALL, NULL);


	os_sleep_ms(100);
	power_holdoff();
	backlight_off();
	usr_close_adc();

	os_sleep_ms(100);
	goto_dsleep();


	while(1);
}




void lv_page_usb_config(){	
	static lv_style_t usbPage_style;	
	lv_style_reset(&usbPage_style);
	lv_style_init(&usbPage_style);
	lv_style_set_bg_color(&usbPage_style, lv_color_make(0x00, 0x00, 0x00));	
	lv_style_set_shadow_color(&usbPage_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_border_color(&usbPage_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_outline_color(&usbPage_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_radius(&usbPage_style,0);

	ui_usbPage = lv_obj_create(lv_scr_act());//
	curPage_obj = ui_usbPage;
	lv_obj_set_size(ui_usbPage, SCALE_WIDTH, SCALE_HIGH);
	lv_obj_add_style(ui_usbPage, &usbPage_style, 0);
}





void power_holdoff(void)
{
#ifdef POWER_HOLD_PORT
	gpio_iomap_output(POWER_HOLD_PORT,GPIO_IOMAP_OUTPUT);
	gpio_set_val(POWER_HOLD_PORT,0);
#endif
}

void power_hold_init(void)
{
#ifdef POWER_HOLD_PORT
	gpio_iomap_output(POWER_HOLD_PORT,GPIO_IOMAP_OUTPUT);
	gpio_set_val(POWER_HOLD_PORT,1);
#endif
}

void lv_user_msg_send(void* key)
{
	printf("## lv_user_msg_send \n");
	lv_event_send(curPage_obj,USER_KEY_EVENT,key);
} 	

void lv_page_init(){
	k_task_handle_t rec_vfx_task_handle;

	//uvc_open = 0;		
	//enable_video_usb_to_lcd(0);	
	USER_KEY_EVENT=lv_event_register_id();
}


void poweron_welcome(void)
{
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);
	
	//1、现在先把“实时视频”这条链路全部关掉，准备好一张静态 Logo 当开机画面。
	lcd_info.lcd_p0p1_state = 2;  	//当前LCD视频层使用模式： 2全屏背景模式  3对讲模式：一边视频小窗，一边主界面
	bbm_displaydecode_run =0;  		//停掉“图传解码线程”的显示工作
	lcdc_set_video_en(lcd_dev,0); 	//把 LCD 视频层关掉，先准备好 buffer 和图像，再一次性打开，避免花屏/撕裂。
	//vpp_close(vpp_dev);
	video_decode_mem  = video_decode_config_mem;
	video_decode_mem1 = video_decode_config_mem;
	video_decode_mem2 = video_decode_config_mem;


#if LCD_SET_ROTATE_180	
	lcdc_set_rotate_mirror(lcd_dev,0,LCD_ROTATE_180);
	jpg_dec_scale_del();
	set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,1);
#else
	lcdc_set_rotate_mirror(lcd_dev,0,VEDIO_LAYER_ROTATE);
	jpg_dec_scale_del();
	set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
#endif
	jpg_decode_scale_config((uint32)video_decode_mem);

	if(sys_cfgs.wifi_mode==WIFI_MODE_AP)
		memcpy(jpg_psram_mem,ui_bgLogo_ap,sizeof(ui_bgLogo_ap));
	else
		memcpy(jpg_psram_mem,ui_bgLogo,sizeof(ui_bgLogo));

	jpg_decode_to_lcd((uint32)jpg_psram_mem,320,240,SCALE_WIDTH,SCALE_HIGH);
	//jpg_decode_to_lcd((uint32)jpg_psram_mem,640,480,SCALE_WIDTH,SCALE_HIGH);
	lcdc_set_video_en(lcd_dev,1); //1		 打开视频背景层
	lv_page_poweron_menu_config();
	set_autoPowerOff_times(300); 	//自动关机300s
	camera_gvar.welcome_times =5;	// 5 个定时器周期后进入下一页

	camera_gvar.pagebtn_index =0;
	camera_gvar.gametab_index =2;
	camera_gvar.poweron_nextpage=PAGE_HOME;//PAGE_HOME;//PAGE_CAMERA;//PAGE_INTERCOM;//PAGE_WECHAT;
	camSetParam.volumeSet =6 ;   //初始 音量设置(0-10);
	camSetParam.languageType=1;
	
	volume_adjust(camSetParam.volumeSet);

	backlight_init();
	
	///backlight_on();
	//camera_gvar.settingtab_index =SETMENU_LANGUAGE;
}


#if 0
void fly_info_dlysave(uint8_t tm)
{
	camera_gvar.dly_save_cnt =tm;
}



void fly_info_save(void)
{
#if 0
	printf("##fly_info_save \n");
	spi_nor_sector_erase(flycfg_info.flash, flycfg_info.addr);
	spi_nor_write(flycfg_info.flash,flycfg_info.addr,(uint8*)&camSetParam,sizeof(cam_set_t));
#endif
}

void fly_info_read(void)
{
	#if 0
    spi_nor_read(flycfg_info.flash,flycfg_info.addr,(uint8*)&camSetParam,sizeof(cam_set_t));
	#endif
}



void fly_info_init(void)
{
	struct syscfg_info info;

	if (syscfg_info_get(&info)) {
       return ;
	}
	flycfg_info.flash = info.flash1;
    flycfg_info.addr =  info.flash1->size - info.flash1->sector_size;
    flycfg_info.size =  info.flash1->sector_size;

    printf("##flashsize =%x flycfg_info->addr=%x \n",info.flash1->size,flycfg_info.size);
    spi_nor_open(flycfg_info.flash);

    os_sleep_ms(10);
	fly_info_read();

    printf("##camSetParam.data_check=%x \n",camSetParam.data_check);

    if(camSetParam.data_check!=0x55)
    {
        camSetParam.data_check =0x55;

		//camSetParam_default();
		fly_info_save();
    }

}
#endif




void fly_demo(void)
{
	bat_det_init();		// 电池检测初始化
	//soft_clock_init();

	//fly_info_init();
	lv_page_init(); 	//初始化LVGL相关全局
	poweron_welcome(); 	//显示开机 Logo，进入欢迎/对讲页面
	lv_time_set();		// 创建 100ms 的定时器 timer_event
	printf("## fly_demo 3\n");

}


void cfg_vediop1_bg_img(void)
{
	static uint8_t setFlag=0;
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;

	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);

#if 0
	if(gameSub_view==1)
		setFlag =0;
	if(setFlag)
	{

		if((camera_gvar.page_back==PAGE_HOME) || (camera_gvar.page_back==PAGE_GAME)||\
		 (camera_gvar.page_back==PAGE_MUSIC)||(camera_gvar.page_back==PAGE_SET))
		 {
			printf("\n ## cfg_vediop1_bg_img 1 gameSub_view=%d ",gameSub_view);
			return;
		 }
		
	}
	setFlag=1;
#endif
	printf("\n ## cfg_vediop1_bg_img 2");
	lcd_info.lcd_p0p1_state = 2;
	lcdc_set_video_en(lcd_dev,0);
	//vpp_close(vpp_dev);
	video_decode_mem  = video_decode_config_mem;
	video_decode_mem1 = video_decode_config_mem;
	video_decode_mem2 = video_decode_config_mem;

#if LCD_SET_ROTATE_180	
	lcdc_set_rotate_mirror(lcd_dev,0,LCD_ROTATE_180);
	jpg_dec_scale_del();
	set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,1);
#else
	lcdc_set_rotate_mirror(lcd_dev,0,VEDIO_LAYER_ROTATE);
	jpg_dec_scale_del();
	set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
#endif

	jpg_decode_scale_config((uint32)video_decode_mem);
	
	if(sys_cfgs.wifi_mode==WIFI_MODE_AP)
		memcpy(jpg_psram_mem,ui_bgLogo_ap,sizeof(ui_bgLogo_ap));
	else
		memcpy(jpg_psram_mem,ui_bgLogo,sizeof(ui_bgLogo));
	jpg_decode_to_lcd((uint32)jpg_psram_mem,320,240,SCALE_WIDTH,SCALE_HIGH);
	lcdc_set_video_en(lcd_dev,1); //1		 close or open vedio background 
}

extern uint8_t pro_page_cur;
extern uint8_t un_viewSwitch_flag;
void lv_page_select(uint8_t page)
{
	uint8_t name[16];
	struct lcdc_device *lcd_dev;
	struct vpp_device *vpp_dev;
	
	lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);	
	vpp_dev = (struct vpp_device *)dev_get(HG_VPP_DEVID);

	// 记录“从哪一页切过来”
	camera_gvar.page_back = camera_gvar.page_cur;  // 协议线程也会用它判断当前是不是对讲页面
	camera_gvar.page_cur = page;				   // 现在要切到的新页面
	printf("camera_gvar.page_cur:%d\r\n",page);
	pro_page_cur=camera_gvar.page_cur;				// 另一个全局备份（给协议线程或别的模块用）
	
	//从“对讲页”切走时，关闭视频对讲音频链路 
	if ((camera_gvar.page_back == PAGE_INTERCOM) && (camera_gvar.page_cur != PAGE_INTERCOM)) {
        if (g_video_intercom_mode) {
            printf("leave INTERCOM page, stop video intercom audio\r\n");
            video_intercom_stop();      // 停掉编码线程、解码心跳等
            video_talk_audio_on = 0;    // 标志位也一并清掉
        }
    }
	
	if(page == PAGE_HOME){   
	#ifdef  P0P1_SWITCH
		{
			delay_open_lcd_flash(1);
			os_sleep_ms(10);
			extern volatile uint8_t p0p1_switch_flag;
			if((camera_gvar.page_back==PAGE_INTERCOM)&&(p0p1_switch_flag==1))
			{
				view_switch(1);
			}
			bbm_displaydecode_run =0;  // 回 home，不再解码网络视频
		}
	#endif

		cfg_vediop1_bg_img();     // 背景 logo 图

		if(curPage_obj){
			//lv_obj_clean(curPage_obj);
			lv_obj_del(curPage_obj);

			printf("## come to here \n");
			curPage_obj = NULL;
		}

		ui_homePage_screen_init();		
	}
	else if(page == PAGE_INTERCOM){	

	#ifdef  P0P1_SWITCH
		{
			extern volatile uint8_t p0p1_switch_flag;
			extern uint8_t viewSwitch_flag;
			viewSwitch_flag=1;
			p0p1_switch_flag=0;
			delay_open_lcd_flash(6);
			os_sleep_ms(10);
		}
	#endif
		lcd_info.lcd_p0p1_state = 3;
		//lcdc_set_video_en(lcd_dev,0);
		//vpp_close(vpp_dev);
		//vpp_open(vpp_dev);
		
		video_decode_mem  = video_decode_config_mem;
		video_decode_mem1 = video_decode_config_mem1;
		video_decode_mem2 = video_decode_config_mem2;

		//lcdc_set_rotate_p0_up(lcd_dev,1); // p0 up
		lcdc_set_rotate_p0p1_start_location(lcd_dev,(SCALE_WIDTH-120-6),6,0,0);

		jpg_dec_scale_del();
		#if LCD_SET_ROTATE_180	
		set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,1);
		#else
		set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
		#endif
		
		jpg_decode_scale_config((uint32_t)video_decode_mem);
		
		set_lcd_photo0_config(120,90,0); //前景小窗 120*90
		
		scale_to_lcd_config();


		lcdc_set_video_en(lcd_dev,1);
		bbm_displaydecode_run =1;  // 允许解码显示网络来的 JPEG
		un_viewSwitch_flag = 0;
		lcd_pair_success = 0;      // 进入对讲页面时，默认还没配对成功
		rec_open  = 0;
		photo_flag = 0;
		if(curPage_obj){
			//lv_obj_clean(curPage_obj);
			lv_obj_del(curPage_obj);
			printf("## come to here \n");
			curPage_obj = NULL;
		}

		ui_intercomPage_screen_init();		

	}

	else if(page == PAGE_CAMERA){
	g_camera_from_page = camera_gvar.page_back;
	#ifdef  P0P1_SWITCH
		{
			delay_open_lcd_flash(3);
			os_sleep_ms(10);
			extern volatile uint8_t p0p1_switch_flag;
			if((camera_gvar.page_back==PAGE_INTERCOM)&&(p0p1_switch_flag==1))
			{
				view_switch(1);
			}
			bbm_displaydecode_run =0;
		}
	#endif
	#if 0
		//lcdc_set_video_en(lcd_dev,0);
		//vpp_close(vpp_dev);
		vpp_open(vpp_dev);
		
		video_decode_mem  = video_decode_config_mem;
		video_decode_mem1 = video_decode_config_mem1;
		video_decode_mem2 = video_decode_config_mem2;

		//lcdc_set_rotate_p0_up(lcd_dev,1); // p0 up
		lcdc_set_rotate_p0p1_start_location(lcd_dev,(SCALE_WIDTH-120-6),6,0,0);

		jpg_dec_scale_del();
		#if LCD_SET_ROTATE_180	
		set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,1);
		#else
		set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
		#endif
		
		jpg_decode_scale_config((uint32_t)video_decode_mem);
		
		set_lcd_photo0_config(120,90,0);

		
		scale_to_lcd_config();
	#else
		lcd_info.lcd_p0p1_state = 1;
		//lcdc_set_video_en(lcd_dev,0);
		//vpp_close(vpp_dev);
		//rahmen_open = 0;
		//vpp_open(vpp_dev);

		lcdc_set_rotate_p0p1_start_location(lcd_dev,0,0,0,0);

		set_lcd_photo0_config(SCALE_WIDTH,SCALE_HIGH,0);

		scale_to_lcd_config();

		lcdc_set_video_en(lcd_dev,1);
	#endif
		if(curPage_obj){
			//lv_obj_clean(curPage_obj);
			lv_obj_del(curPage_obj);
			printf("## come to here \n");
			curPage_obj = NULL;
		}

		ui_cameraPage_screen_init();		
	}
	else if(page == PAGE_ALBUM){
	g_img_from_page = camera_gvar.page_back;
	#ifdef  P0P1_SWITCH
		{
			delay_open_lcd_flash(3);
			os_sleep_ms(10);
			extern volatile uint8_t p0p1_switch_flag;
			if((camera_gvar.page_back==PAGE_INTERCOM)&&(p0p1_switch_flag==1))
			{
				view_switch(1);
			}
			bbm_displaydecode_run =0;
		}
	#endif
		lcd_info.lcd_p0p1_state = 2;
		//lcdc_set_video_en(lcd_dev,0);
		video_decode_mem  = video_decode_config_mem;
		video_decode_mem1 = video_decode_config_mem;
		video_decode_mem2 = video_decode_config_mem;	
		bbm_displaydecode_run =0;

		#if 1
		free_album_list();
		#else
		if(ablumlist)
		{
			custom_free_psram(ablumlist);
			ablumlist=NULL;
		}
		#endif
		
		camera_gvar.album_filenums=scan_album_files();


		jpg_dec_scale_del();
		#if LCD_SET_ROTATE_180	
		set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,1);
		#else
		set_lcd_photo1_config(SCALE_WIDTH,SCALE_HIGH,0);
		#endif
		jpg_decode_scale_config((uint32)video_decode_mem);

		if(camera_gvar.album_filenums)
		{
			#if 1
			album_cur_selected = album_list_tail;
			print_album_list();
			sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",album_cur_selected->name);
			album_file_preview(name_rec_photo,album_cur_selected->filetype);	
			#else
			camera_gvar.album_fileindex =(camera_gvar.album_filenums-1);
			printf("ablumlist[%d].name:%s \n",camera_gvar.album_fileindex,ablumlist[camera_gvar.album_fileindex].name);
			sprintf((char *)name_rec_photo,"%s%s","0:DCIM/",ablumlist[camera_gvar.album_filenums-1].name);

			printf("name_rec_photo:%s\r\n",name_rec_photo);
			album_file_preview(name_rec_photo,ablumlist[camera_gvar.album_fileindex].filetype);	
			#endif
		}
		
		lcdc_set_video_en(lcd_dev,1);
		//vpp_close(vpp_dev);

		if(curPage_obj){
			//lv_obj_clean(curPage_obj);
			lv_obj_del(curPage_obj);
			printf("## come to here \n");
			curPage_obj = NULL;
		}

		ui_albumPage_screen_init();		
	}
	else if(page == PAGE_POWEROFF)
	{
			// lcd_info.lcd_p0p1_state = 2;
			// lcdc_set_video_en(lcd_dev,0);
			// vpp_close(vpp_dev);
			bbm_displaydecode_run =0;

			if(curPage_obj){
				lv_obj_clean(curPage_obj);
				printf("## come to here \n");
				curPage_obj = NULL;
			}
			lv_page_poweroff_menu_config();
	}	
	else if(page == PAGE_WECHAT)
	{
		/* 微聊页：目前只做纯 UI，不用视频小窗，关掉图传解码 */
		bbm_displaydecode_run = 0;
		
//		struct lcdc_device *lcd_dev;
//		lcd_dev = (struct lcdc_device *)dev_get(HG_LCDC_DEVID);
//		lcdc_set_video_en(lcd_dev, 0);

		lcd_info.lcd_p0p1_state = 2;   // 统一成全屏 UI 模式

		if (curPage_obj) {
			lv_obj_del(curPage_obj);
			printf("## delete old page, go to WECHAT\n");
			curPage_obj = NULL;
		}
		ui_wechatPage2_screen_init();	
	}	

	else 
	{	
		printf("## PAGE select err !!! \r\n");
		lv_page_select(PAGE_HOME);
	}
	
}

