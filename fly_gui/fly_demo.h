#ifndef _FLY_DEMO_
#define _FLY_DEMO_

#define BABY_UI_MAGICSOUND
#define DISPLAY_DEBUGINFO_ENABLE
//#define AUTO_POWER_OFF_ENABLE


struct lv_time {
	uint8 lv_hour;
	uint8 lv_min;
	uint8 lv_sec;
	uint8 res;
};


typedef struct {
    lv_obj_t * pagebtn;
    const lv_img_dsc_t *nimg;
    const lv_img_dsc_t *bimg;

} USR_PAGE_BTN;

typedef enum 
{
    AU_MP3=0,
    AU_AMR,
    AU_MAX
} AUDIOTYPE;

typedef enum 
{
    FREQ_50HZ=0,
    FREQ_60HZ,
    FREQ_MAX
} FREQ_NUM;


typedef struct {
    char name[13];
    AUDIOTYPE type;
} audio_info;

extern audio_info*mp3list; 
int scan_mp3_files(void);

typedef enum _SET_MENU_
{
    SETMENU_FORMAT=0,
    SETMENU_CAMRES,
    SETMENU_RECRES,
    SETMENU_VOLUME,
    SETMENU_DATE,
    SETMENU_PRINT_M,
    SETMENU_PRINT_D,
    SETMENU_LANGUAGE,
    SETMENU_VERSION,
    SETMENU_DEFAULT,
    SETMENU_FREQ,
    SETMENU_CYC_REC,
    SETMENU_SCREEN_PR,
    SETMENU_AUTO_OFF,
    SETMENU_MAX
}SETMENU;


typedef enum _PAGE_NUM_
{
    PAGE_HOME=0,	//主页
    PAGE_WECHAT,	//微聊
    PAGE_CAMERA,	//相机
    PAGE_ALBUM,		//相册
    PAGE_VIDEO,
    PAGE_MUSIC,
    PAGE_GAME,
    PAGE_SET,
    PAGE_PWERON,
    PAGE_USB,
    PAGE_POWEROFF,
	PAGE_INTERCOM,
    PAGE_MAX
}PAGENUM;

typedef enum _PURE_NUM_
{
    PURE_LOW=0,
    PURE_MEDIUM,
    PURE_HIGH,

}PURE_NUM;


typedef enum _PRINT_M_
{
    PRINT_GRAY=0,
    PRINT_DOT,
    PRINT_M_MAX
}PRINT_M;


typedef enum _CYC_T_
{
    C_TIME_OFF=0,
    C_TIME_1MIN,
    C_TIME_5MIN,
    C_TIME_10MIN
}CYC_TIME;

typedef enum _PRO_T_
{
    P_TIME_OFF=0,
    P_TIME_1MIN,
    P_TIME_3MIN,
    P_TIME_5MIN
}PRO_TIME;

typedef enum _OFF_T_
{
    O_TIME_OFF=0,
    O_TIME_1MIN,
    O_TIME_3MIN,
    O_TIME_5MIN
}OFF_TIME;

typedef struct
{
    uint8_t data_check;
	uint8_t languageType;		//语言
    FREQ_NUM screenFreq;		//屏幕频率（50Hz/60Hz）

    CYC_TIME cycleRecSet;		//循环录像开关/时间
    PRO_TIME screenProtectSet;	//屏幕保护时间	
    OFF_TIME autOffSet;			//自动关机时间

    uint8_t vedioResSet;		//视频分辨率 / 拍照分辨率
    uint8_t camResSet;
    uint8_t volumeSet;			//音量
    
    PRINT_M printMode;
    PURE_NUM printPure;
}cam_set_t;

extern cam_set_t camSetParam;

typedef struct
{
    bool sd_online 			:1;     //SD 卡是否插入
    bool usb_online 		:1;
    bool mute				:1;		//静音标志（配合音量显示）
    bool printer_flag 		:1;
    uint8_t welcome_times;  		//开机欢迎界面倒计时	
    uint8_t cur_menu;				//在设置页里当前选中的二级菜单
    uint8_t anim_times;			
    uint8_t specialeffects_index;
    PAGENUM page_back;   			//上一页面
    PAGENUM page_cur ;	 			//当前页面
    PAGENUM page_num;	 			//可以用于统计可用页面数量
    uint8_t poweron_nextpage;  		//开机欢迎结束后要跳转的页面
    uint8_t pagebtn_index;			//主页上当前选中的那个图标的索引（HOME 页的“对讲”按钮对应 index=0）
    uint8_t gametab_index;
    uint8_t settingtab_index;
    uint32_t  album_filenums;
    uint32_t  album_fileindex;
    uint8_t shot_anim_times;
    uint8_t nextp_anim_times;
    uint8_t prevp_anim_times;
    uint16_t  mp3_filenums;
    uint16_t  mp3_fileindex;
    double  mp3_playtime;
    double  mp3_songtime;
    uint32_t mp3_size;
    uint8_t lcden_delaytimes;
    uint8_t immediately_reflash_flag;
    uint8_t dly_save_cnt;
    uint8_t volume_anim_times;
    uint8_t notice_anim_times;
    uint8_t pair_out_times;			//配对倒计时
    uint8_t pair_success;			//配对成功标记
    uint16_t autoPowerOff_times;  	//自动关机倒计时
    uint8_t camera_switch;			//用于控制摄像头开关状态
} camera_global_t;


extern camera_global_t camera_gvar;
extern lv_obj_t *ui_settNextBtn;
extern lv_obj_t *ui_settPrevBtn;
extern lv_obj_t *ui_gameNextBtn;
extern lv_obj_t *ui_gamePrevBtn;
extern lv_obj_t *ui_settTabView;
extern lv_obj_t *ui_gameTabView;
extern uint8_t settSub_view;
extern uint8_t gameSub_view;


/* jpg hex table */
extern const unsigned char ui_bgLogo[15386];
extern const unsigned char ui_bgLogo_ap[11950];

extern const unsigned char ui_bgHome[36532];
extern const unsigned char ui_bgUsb[7392];


void lv_page_select(uint8_t page);
extern uint32_t USER_KEY_EVENT;


extern lv_group_t * group_golop;
extern lv_indev_t * indev_keypad;

extern lv_obj_t * curPage_obj;

void event_handler(lv_event_t * e);

/*key group*/
extern lv_group_t * group_cur;
extern lv_group_t * home_group;
extern lv_group_t * rec_group;
extern lv_group_t * camera_group;
extern lv_group_t * setting_group;
extern lv_group_t * music_group;
extern lv_group_t * album_group;
extern lv_group_t * game_group;

// SCREEN:ui_homePage
void ui_homePage_screen_init();
void ui_event_homePage(lv_event_t * e);

extern lv_obj_t * ui_homePage;
extern lv_obj_t * recPage_btn;		 // 录像页按钮	
extern lv_obj_t * intercomPage_btn;	 // 对讲页按钮
extern lv_obj_t * cameraPage_btn;	 // 摄像头页面按钮
extern lv_obj_t * settPage_btn;		 // 设置页按钮
extern lv_obj_t * albumPage_btn;	 // 相册页面按钮
extern lv_obj_t * gamePage_btn;
extern lv_obj_t * musicPage_btn;
extern lv_obj_t * ui_homeBatImg;

extern USR_PAGE_BTN user_pagebtn_list[6];


// SCREEN:ui_videoPage
void ui_videoPage_screen_init();
void ui_event_recPage( lv_event_t * e);
extern lv_obj_t *ui_videoPage;
extern lv_obj_t *ui_videoTopBar;
extern lv_obj_t *ui_videoIconImg;
extern lv_obj_t *ui_vsdIconImg;
extern lv_obj_t *ui_vSdStaLabel;
extern lv_obj_t *ui_vfilmIconImg;
extern lv_obj_t *ui_vFilmQualityLabel;
//extern lv_obj_t *ui_vBatIconLabel;
extern lv_obj_t *ui_vBatImg;
extern lv_obj_t *ui_videoBtmBar;
extern lv_obj_t *ui_vTimeIconLabel;
extern lv_obj_t *ui_RecTimeIconLabel;
extern lv_obj_t *ui_voiceinfo_lable;

// SCREEN:ui_intercomPage
void ui_intercomPage_screen_init();
void ui_event_intercomPage( lv_event_t * e);
extern lv_obj_t *ui_intercomPage;
extern lv_obj_t *ui_intercomTopBar;
extern lv_obj_t *ui_isdIconImg;
extern lv_obj_t *ui_iSdStaLabel;
extern lv_obj_t *ui_intercomIconImg;
extern lv_obj_t *ui_iBatIconLabel;
extern lv_obj_t *ui_intercomBatImg;
extern lv_obj_t *ui_intercomBtmBar;


// SCREEN:ui_cameraPage
void ui_cameraPage_screen_init();
void ui_event_cameraPage( lv_event_t * e);
extern lv_obj_t *ui_cameraPage;
extern lv_obj_t *ui_camTopBar;
extern lv_obj_t *ui_csdIconImg;
extern lv_obj_t *ui_cSdStaLabel;
extern lv_obj_t *ui_camIconImg;
extern lv_obj_t *ui_cBatIconLabel;
extern lv_obj_t *ui_camBatImg;
extern lv_obj_t *ui_camBtmBar;

extern lv_obj_t *ui_DvIconImg;
extern lv_obj_t *ui_camPrinterFlag;
extern lv_obj_t *ui_cTimeIconLabel;
extern lv_obj_t *ui_photoQualityLabel;
extern lv_obj_t *ui_focusBtn;
extern lv_obj_t *ui_focusImg;

#if 0
// SCREEN:ui_settingPage
void ui_settingPage_screen_init();
void ui_event_settPage( lv_event_t * e);
extern lv_obj_t *ui_settingPage;
extern lv_obj_t *ui_settNextBtn;
extern lv_obj_t *ui_settPrevBtn;
extern lv_obj_t *ui_settTabView;
extern lv_obj_t *ui_settTopBar;
//extern lv_obj_t *ui_sBatLabel;
extern lv_obj_t *ui_settBatImg;
extern lv_obj_t *ui_sTimeIconLabel;

// SCREEN:ui_gamePage
void ui_gamePage_screen_init();
void ui_event_gamePage( lv_event_t * e);
extern lv_obj_t *ui_gamePage;
extern lv_obj_t *ui_gameTopBar;
extern lv_obj_t *ui_gameTabView;
extern lv_obj_t *ui_gamePage;
extern lv_obj_t *ui_gamePrevBtn;
extern lv_obj_t *ui_gameNextBtn;
extern lv_obj_t *ui_gameBatImg;
extern lv_obj_t *ui_game_opview;
#endif
extern lv_obj_t * ui_dialogPanel; 
extern lv_obj_t * ui_dialogContent;

extern lv_obj_t * ui_volPanel;
extern lv_obj_t * ui_lisglabel;
extern lv_obj_t * ui_volumeLevels[10];

extern lv_obj_t * mic_img;

extern lv_obj_t *ui_signalPanel;
extern lv_obj_t *ui_signalLevels[4];

extern lv_obj_t * ui_pairPanel;

extern lv_obj_t * ui_pairTeimlabel;

// SCREEN:ui_albumPage
void ui_albumPage_screen_init();
void ui_event_albumPage( lv_event_t * e);
extern lv_obj_t *ui_albumPage;
extern lv_obj_t *ui_albumTopBar;
extern lv_obj_t *ui_aSdIconImg;
extern lv_obj_t *ui_aSdStaLabel;
extern lv_obj_t *ui_albumPrinterFlag;
//lv_obj_t *ui_aBatIconLabel;
extern lv_obj_t *ui_albumBatImg;
extern lv_obj_t *ui_albumIconImg;
extern lv_obj_t *ui_fileQualityLabel;
extern lv_obj_t *ui_albumBtmBar;
extern lv_obj_t *ui_fileFormatLabel;
extern lv_obj_t *ui_fileNumsLabel;
extern lv_obj_t *ui_albumPrevBtn;
extern lv_obj_t *ui_albumNextBtn;


// WECHAT: ui_WeChatPage
extern lv_obj_t *ui_intercomBatImg;
extern lv_obj_t *ui_wechatBattImg;
extern lv_obj_t *ui_wechatBattLabel;


// SCREEN: ui_musicPage
#if 0
void ui_musicPage_screen_init(void);
void ui_event_musicPage( lv_event_t * e);
extern lv_obj_t *ui_musicPage;
extern lv_obj_t *ui_musicTopBar;
extern lv_obj_t *ui_musicIconImg;
extern lv_obj_t *ui_msdIconImg;
extern lv_obj_t *ui_musicBatImg;
extern lv_obj_t *ui_musiBtmBar;
extern lv_obj_t *ui_playPauseIcon;
extern lv_obj_t *ui_playTimeLabel;
extern lv_obj_t *ui_playTimeIndex;
extern lv_obj_t *ui_songNumLabel;
extern lv_obj_t *ui_musiclist;
extern lv_obj_t *ui_musicbtn0;
extern lv_obj_t *ui_musicbtn1;
#endif

extern lv_obj_t * ui_speedinfo_rx_obj;
extern lv_obj_t * ui_speedinfo_tx_obj;
extern lv_obj_t * ui_speedinfo_id_obj;
extern lv_obj_t * ui_speedinfo_num_obj;


LV_IMG_DECLARE( iconHomeMenu0); 
LV_IMG_DECLARE( iconHomeMenu1); 
LV_IMG_DECLARE( iconHomeCamera0); 
LV_IMG_DECLARE( iconHomeCamera1); 
LV_IMG_DECLARE( iconHomePlayer0); 
LV_IMG_DECLARE( iconHomePlayer1); 
LV_IMG_DECLARE( iconHomeIntercom0); 
LV_IMG_DECLARE( iconHomeIntercom1); 

#if 0
LV_IMG_DECLARE( iconHomeGame0); 
LV_IMG_DECLARE( iconHomeGame1); 
LV_IMG_DECLARE( iconHomeMenu0); 
LV_IMG_DECLARE( iconHomeMenu1); 
LV_IMG_DECLARE( iconHomeMusic0); 
LV_IMG_DECLARE( iconHomeMusic1); 
LV_IMG_DECLARE( iconHomeCamera0); 
LV_IMG_DECLARE( iconHomeCamera1); 
LV_IMG_DECLARE( iconHomePlayer0); 
LV_IMG_DECLARE( iconHomePlayer1); 
LV_IMG_DECLARE( iconHomeVideo0); 
LV_IMG_DECLARE( iconHomeVideo1); 


LV_IMG_DECLARE( iconMenuFormat); 
LV_IMG_DECLARE( iconMenuPres); 
LV_IMG_DECLARE( iconMenuVres); 
LV_IMG_DECLARE( iconMenuVolume); 
LV_IMG_DECLARE( iconMenuDate); 
LV_IMG_DECLARE( iconMenuPrintSel); 
LV_IMG_DECLARE( iconMenuPstramp); 
LV_IMG_DECLARE( iconMenuLanguage); 
LV_IMG_DECLARE( iconMenuVersion); 
LV_IMG_DECLARE( iconMenuSd); 
LV_IMG_DECLARE( iconMenuHz); 
LV_IMG_DECLARE( iconMenuRec); 
LV_IMG_DECLARE( iconMenuPoff); 
LV_IMG_DECLARE( iconMenuSoff); 

LV_IMG_DECLARE( iconMenuFormat1); 
LV_IMG_DECLARE( iconMenuPres1); 
LV_IMG_DECLARE( iconMenuVres1); 
//LV_IMG_DECLARE( iconMenuVolume1); 
LV_IMG_DECLARE( iconMenuDate1); 
LV_IMG_DECLARE( iconMenuPrintSel1); 
LV_IMG_DECLARE( iconMenuPstramp1); 
LV_IMG_DECLARE( iconMenuLanguage1); 
LV_IMG_DECLARE( iconMenuVersion1); 
LV_IMG_DECLARE( iconMenuSd1); 
LV_IMG_DECLARE( iconMenuHz1); 
LV_IMG_DECLARE( iconLoop); 
LV_IMG_DECLARE( iconMenuPoff1); 
LV_IMG_DECLARE( iconMenuSoff1); 
#endif
LV_IMG_DECLARE( iconMenuVolume1); 

LV_IMG_DECLARE( iconPrevP); 
LV_IMG_DECLARE( iconNextP); 
LV_IMG_DECLARE( iconPrev); 
LV_IMG_DECLARE( iconNext); 
LV_IMG_DECLARE( iconPlay); 
LV_IMG_DECLARE( iconPause); 

LV_IMG_DECLARE( iconBat0); 
LV_IMG_DECLARE( iconBat1); 
LV_IMG_DECLARE( iconBat2); 
LV_IMG_DECLARE( iconBat3); 
LV_IMG_DECLARE( iconBat4); 


LV_IMG_DECLARE( iconSdc); 
LV_IMG_DECLARE( iconFlagPhoto); 
LV_IMG_DECLARE( iconFlagPlay); 
LV_IMG_DECLARE( iconFlagRecord); 
LV_IMG_DECLARE( iconFlagMusic); 
LV_IMG_DECLARE( iconPrinter)

LV_IMG_DECLARE( iconFocusP); 
LV_IMG_DECLARE( iconFocus); 

LV_IMG_DECLARE( mkf30); 
#if 0
LV_IMG_DECLARE( iconGameBall); 
LV_IMG_DECLARE( iconGameBlock); 
LV_IMG_DECLARE( iconGameBox); 
LV_IMG_DECLARE( iconGameSnake); 
LV_IMG_DECLARE( iconTest6); 
LV_IMG_DECLARE( iconTest6A); 
#endif
LV_IMG_DECLARE( iconDv_w);
LV_IMG_DECLARE( iconDv_r);
LV_IMG_DECLARE(intercom_bglogo);

LV_FONT_DECLARE( ui_font_alimamaShuHei16);
LV_FONT_DECLARE( simplch_light);
LV_FONT_DECLARE( alifangyuan16);
LV_FONT_DECLARE( alifangyuan28);
//LV_FONT_DECLARE( alifangyuan18);


extern const lv_img_dsc_t *ui_imgset_iconBat[];
extern const lv_img_dsc_t *ui_imgset_iconGameSubImgs[];
extern const lv_img_dsc_t *ui_imgset_iconSettSubImgs[];
extern const lv_img_dsc_t *ui_imgset_iconHomeGame[];
extern const lv_img_dsc_t *ui_imgset_iconHomeMenu[];
extern const lv_img_dsc_t *ui_imgset_iconHomeMusic[];
extern const lv_img_dsc_t *ui_imgset_iconHomeCamera[];
extern const lv_img_dsc_t *ui_imgset_iconHomeintercom[];
extern const lv_img_dsc_t *ui_imgset_iconHomePlayer[];
extern const lv_img_dsc_t *ui_imgset_iconHomeVideo[];
extern const lv_img_dsc_t *ui_imgset_iconFocus[];
extern const lv_img_dsc_t *ui_imgset_iconNext[];
extern const lv_img_dsc_t *ui_imgset_iconPrev[];



extern uint8 *video_psram_mem;
extern uint8 *video_psram_mem1;
extern uint8 *video_psram_mem2;
extern uint8 *video_psram_mem3;

extern uint8 *video_decode_mem;
extern uint8 *video_decode_mem1;
extern uint8 *video_decode_mem2;

extern uint8 video_decode_config_mem[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2];
extern uint8 video_decode_config_mem1[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2];
extern uint8 video_decode_config_mem2[SCALE_PHOTO1_CONFIG_W*PHOTO1_H+SCALE_PHOTO1_CONFIG_W*PHOTO1_H/2];

extern uint8 video_psram_config_mem[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2];
extern uint8 video_psram_config_mem1[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2];
extern uint8 video_psram_config_mem2[SCALE_CONFIG_W*SCALE_HIGH+SCALE_CONFIG_W*SCALE_HIGH/2];

extern int global_avi_running;
extern int global_avi_exit;
extern uint8_t uvc_open;
extern volatile uint8_t vfx_open;
extern volatile uint8_t vfx_pingpang; 
extern uint32_t scsi_count;
extern uint8 *yuvbuf;
extern volatile uint8_t ipf_update_flag;
extern uint8_t ipf_index_num;

extern void hexagon_ve(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void hexagon_ve1(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void block_9(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void block_4(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void block_2_yinv(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void block_2_xinv(uint8* in, uint8* tem, uint32 w, uint32 h);
extern void uv_offset(uint8* in, uint8* tmp, uint32 w, uint32 h, int32 uoff, int32 voff);
extern volatile uint32 ve_sel;
extern volatile uint32 fr_down;

extern uint8_t *vfx_linebuf;
extern uint8_t  vga_room[2][640*480+640*480/2];
extern volatile uint8  printer_action;



void take_photo_thread_init(uint16_t w,uint16_t h,uint8_t continuous_spot);
extern uint8_t uvc_start ;
extern uint8_t rec_open;
extern uint8_t enlarge_glo;
extern uint8_t rahmen_open;
extern uint8_t name_rec_photo[];
extern struct lv_time rec_time;

extern void camSetParam_default(void);
extern void fly_info_dlysave(uint8_t tm);
extern void volume_adjust(uint8_t vol);
extern void volDisAnimationStart(void);
extern void render_vol_level(uint8_t vol);

extern void noticeAnimationStart(uint8_t tm);
extern void noticeAnimationStop(void);
#endif