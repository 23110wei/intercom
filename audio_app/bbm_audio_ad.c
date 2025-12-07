#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "string.h"
#include "osal/task.h"
#include "osal/string.h"
#include "hal/auadc.h"
#include "utlist.h"
#include "audio_adc.h"
#include "osal_file.h"
#include "stream_frame.h"
#include "osal_file.h"
#include "dev/spi/hgspi_xip.h"
#include "webrtc/process/aec_process.h"
#include "webrtc/process/agc_process.h"
#include "webrtc/process/vad_process.h"
#include "webrtc/process/ns_process.h"
#include "intercom/intercom.h"
#include "dev/audio/components/vad/auvad.h"
#include "sonic_process.h"
#include "magic_sound.h"

#define ADC_DEBUG  1

#if ADC_DEBUG
    #define ADC_LOG  printf
#else
    #define ADC_LOG(...)
#endif

#define PCM_SAMPLE_LEN        160          // 每帧samples（8kHz 20ms）
#define PCM_MAX_SEC           60
#define PCM_FRAME_PER_SEC     50           // 20ms一帧
#define PCM_MAX_FRAMES        (PCM_MAX_SEC * PCM_FRAME_PER_SEC) // 3000帧

// 3000帧 × 160samples × 2bytes = 960KB 放PSRAM
int16_t g_pcm_record_buf[PCM_MAX_FRAMES][PCM_SAMPLE_LEN] __attribute__ ((aligned(4),section(".psram.src")));

volatile uint32_t g_pcm_wr_index = 0;
volatile uint8_t  g_pcm_recording = 0;



#define ADC_DIGITMUTE_ENABLE
int adc_digitmute_flag=1;  // 数字静音开关（1=静音）

#define MEDIAN_FILTER        1
#define MID(a,b,c)   ((a>=b)?((a<=c)?a:((b>=c)?b:c)):((a>=c)?a:((b>=c)?c:b)))

#if MEDIAN_FILTER == 1
	#define MEDIAN_FILTER_SAMPLE_LEN	2
#else
	#define MEDIAN_FILTER_SAMPLE_LEN	0
#endif

#define AEC_PROCESS   0		//回声消除	  	
#define AGC_PROCESS   0     //自动增益
#define VAD_PROCESS   0		//语音检测
#define NSX_PROCESS   1     //降噪
#define MAGIC_SOUND   1	    //变音	
int aec_flag = -1;
int agc_flag = -1;
int vad_flag = -1;
int nsx_flag = -1;

#if MAGIC_SOUND
magicSound *magic_sound = NULL;
#endif

#define AUDIONUM	(4)
#define AUDIOLEN	(320)  //160个16bit sample
#define FILTER_SAMPLE_LEN	0

#define ENERGY_THRESHOLD         5000
#define VAD_WEIGHT               1
#define VAD_HOLD_TIME            50

#if AGC_PROCESS == 1
#define SOFT_GAIN	(1)
#else
#define SOFT_GAIN	(8)
#endif
extern uint8_t rec_open;
static uint8_t g_vad_res = 1;
static stream *global_audio_adc_s = NULL;
struct audio_ad_config;
typedef void *(*set_buf)(void *priv_el,void *el_point);
typedef void (*get_buf)(void *priv_el,void *el_point);

typedef int32 (*audio_ad_read)(struct audio_ad_config *audio, void* buf, uint32 len);

void pcm_record_start()
{
    g_pcm_wr_index = 0;
    g_pcm_recording = 1;	
	rec_open = 1 ;
	printf("[wechat] start PCM record\n");
}

void pcm_record_stop()
{
    g_pcm_recording = 0;
	rec_open = 0 ;
	printf("[wechat] stop PCM record, frames=%u\n", g_pcm_wr_index);
}

uint32_t pcm_record_get_frames()
{
    return g_pcm_wr_index;
}

//静音
void audio_adc_mute(void)
{
	adc_digitmute_flag=1;
}
//取消静音
void audio_adc_unmute(void)
{
	adc_digitmute_flag=0;
}

struct audio_ad_config
{
	int buf_size;
    struct auadc_device *adc;
	void *current_node;
	void *reg_node;
	//私有结构元素
	void *priv_el;
	set_buf set_buf;
	get_buf get_buf;
    audio_ad_read irq_func;	
};

void audio_adc_irq(uint32 irq, uint32 irq_data)
{
	static uint32_t half_cnt = 0;
    static uint32_t full_cnt = 0;
	void *buf;
	struct audio_ad_config *audio_ad = (struct audio_ad_config *)irq_data;
    struct audio_ad_config *priv = (struct audio_ad_config*)audio_ad;
	
  	if(irq == AUADC_IRQ_FLAG_HALF)
  	{
		buf = priv->set_buf(priv->priv_el,&priv->reg_node);
		if(priv->reg_node)
		{
       		priv->irq_func(audio_ad , buf, priv->buf_size);
		}
	#if ADC_DEBUG
        if ((half_cnt++ % 100) == 0) {   // 每 100 次打一次
            ADC_LOG("[ADC IRQ] HALF cnt=%lu buf_size=%d reg_node=%p\n",
                    (unsigned long)half_cnt, priv->buf_size, priv->reg_node);
        }
    #endif
		
  	}
  	else if(irq == AUADC_IRQ_FLAG_FULL)
  	{
	  	if(priv->reg_node)
	  	{
		  	priv->get_buf(priv->priv_el,priv->current_node);
		  	priv->current_node = priv->reg_node;
		  	priv->reg_node = NULL;
	  	}
		encode_sema_up();
	#if ADC_DEBUG
        if ((full_cnt++ % 100) == 0) {
            ADC_LOG("[ADC IRQ] FULL cnt=%lu current_node=%p\n",
                    (unsigned long)full_cnt, priv->current_node);
        }
    #endif
  	}
}

void audio_adc_register(void *audio_hdl,void *priv_el,int play_size,set_buf audio_set_buf,get_buf audio_get_buf)
{
	struct audio_ad_config *priv = (struct audio_ad_config*)audio_hdl;
    priv->buf_size = play_size;
	priv->set_buf = audio_set_buf;
	priv->get_buf = audio_get_buf;
	priv->priv_el = priv_el;	
}

static void *audio_set_buf(void *priv_el,void *el_point)
{
	void *buf = NULL;
    struct data_structure  *data = NULL;
	stream *s = (stream *)priv_el;

    data = get_src_data_f(s);
    struct data_structure **point = (struct data_structure**)el_point;
	if(*point)
		force_del_data(*point);   
    if(data)
    {
        buf = get_stream_real_data(data);
		buf = (uint16_t*)buf+MEDIAN_FILTER_SAMPLE_LEN;
    }
    *point = data;
	return buf;	
}

static void audio_get_buf(void *priv_el,void *el_point)
{
	int32_t res;
    stream *s = (stream *)priv_el;
    struct audio_adc_s *self_priv = (struct audio_adc_s*)s->priv;
    struct data_structure *data = (struct data_structure*)el_point;
    
	if(!data)
	{
		_os_printf("%s:%d err\n",__FUNCTION__,__LINE__);
		return;
	}
    set_stream_data_time(data,os_jiffies());
	res = csi_kernel_msgq_put(self_priv->adc_msgq,&data,0,0);
	//正常应该保证不进这里,如果进来代表任务没有获取队列,直接配置下一个buf导致的
	if(res)
	{
		_os_printf("P");
        force_del_data(data);
	}
	return;	
}

static int vad_filter(int16_t *buffer, uint32_t sampleRate, uint32_t samplesCount, int per_ms_frames)
{
	int32_t vad_ret = -1;
	uint32 vad_result[2] = {0};
	static uint32 energy_threshold = ENERGY_THRESHOLD;
	static int32 talking = VAD_HOLD_TIME;
	
	struct auvad_device *vad_dev = (struct auvad_device*)dev_get(HG_AUVAD_DEVID);
	if(auvad_calc(vad_dev, buffer, samplesCount*2, AUVAD_CALC_MODE_ENERGY, vad_result) == -1)
		return 0;
	if(vad_result[0]>=energy_threshold)
	{
		if(vad_flag == 0)
			vad_ret = vad_process(buffer, sampleRate, samplesCount, per_ms_frames);
		if((vad_ret == -1) || (vad_ret >= VAD_WEIGHT))
			talking = VAD_HOLD_TIME;
		else {
			talking--;
			if(talking<=0) {
				talking = 0;
				return 1;
			}	
		}		
	}
	else {
		talking--;
		if(talking<=0) {
			talking = 0;
			return 1;
		}			
	}
	return 0;
}

uint8_t auadc_get_vad_res(void)
{
	return g_vad_res;
}
extern uint8_t rec_open;
static void audio_deal_task(void *arg)
{
#if MEDIAN_FILTER == 1
	int16_t median_filter_prev_buf[MEDIAN_FILTER_SAMPLE_LEN] = {0};
#endif	 	 
	int16_t *p_buf = NULL;
	int32_t res;
	int32_t temp32 = 0;
	uint32_t sample_len;
	struct data_structure *data = NULL ;
	static uint32_t frame_idx = 0;     // 新增：帧计数
	stream *s = (stream *)arg;
	struct audio_adc_s *self_priv = (struct audio_adc_s*)s->priv;
#if MEDIAN_FILTER == 1//开启中值滤波，保留前一帧末尾的几个样点，用于跨帧平滑
	os_memset(median_filter_prev_buf, 0, MEDIAN_FILTER_SAMPLE_LEN*2);
#endif
	while(1)
	{
		res = csi_kernel_msgq_get(self_priv->adc_msgq,&data,-1);
		if(!res)
		{
            p_buf = get_stream_real_data(data);
			sample_len = get_stream_real_data_len(data)/2;
			
		 #if ADC_DEBUG
            if ((frame_idx % 50) == 0) {  // 每 50 帧打一条
                ADC_LOG("[ADC TASK] frame=%lu samples=%lu data=%p type=0x%x mute=%d rec_open=%d\n",
                        (unsigned long)frame_idx,
                        (unsigned long)sample_len,
                        data,
                        data->type,
                        adc_digitmute_flag,
                        rec_open);
                // 打印前 8 个 sample 看波形是否全 0
                uint32_t dump_n = (sample_len > 8) ? 8 : sample_len;
                ADC_LOG("[ADC TASK] first %lu samples:", (unsigned long)dump_n);
                for (uint32_t i = 0; i < dump_n; i++) {
                    ADC_LOG(" %d", p_buf[i]);
                }
                ADC_LOG("\n");
            }
        #endif
			
		#if MEDIAN_FILTER == 1
			os_memcpy(p_buf, median_filter_prev_buf, MEDIAN_FILTER_SAMPLE_LEN*2);
			for(uint32_t i=MEDIAN_FILTER_SAMPLE_LEN; i<(sample_len+MEDIAN_FILTER_SAMPLE_LEN); i++) {
				p_buf[i-MEDIAN_FILTER_SAMPLE_LEN] = MID(p_buf[i-MEDIAN_FILTER_SAMPLE_LEN],p_buf[i-MEDIAN_FILTER_SAMPLE_LEN+1],p_buf[i]);
			}
			os_memcpy(median_filter_prev_buf, p_buf+sample_len, MEDIAN_FILTER_SAMPLE_LEN*2);	
		#endif	

		#if AEC_PROCESS == 1
			if(aec_flag != -1) {
				aec_process(p_buf, sample_len, 3, 8000);
			}				
		#endif        

		#if VAD_PROCESS
			int filter = vad_filter(p_buf, 8000, sample_len, sample_len/8);
			if(filter) {
				force_del_data(data);
				g_vad_res = 0;
				continue;
			}
			g_vad_res = 1;
		#endif

		#if NSX_PROCESS == 1
			if(nsx_flag != -1) {
				ns_process(p_buf, 8000, sample_len);
			}
		#endif
		
		#if AGC_PROCESS
			if(agc_flag != -1) {
				agc_process(p_buf, 8000, sample_len);
			}
		#endif

			
			// for(uint32_t i=0;i<sample_len;i++) {
			// 	temp32 = (*p_buf)*SOFT_GAIN;
			// 	if(temp32>32767)
			// 		*p_buf = 32767;
			// 	else if(temp32<-32767)
			// 		*p_buf = -32767;
			// 	else
			// 		*p_buf = temp32;
			// 	p_buf++;
			// }
			// p_buf -= sample_len;

		#if	MAGIC_SOUND
			if(magic_sound) {
				magicSound_process(magic_sound, p_buf, sample_len);
			}
		#endif

		int64_t temp = 0;
				for(uint32_t i=0;i<sample_len;i++) {
				
		#ifdef ADC_DIGITMUTE_ENABLE
					
					if((adc_digitmute_flag)&&(rec_open==0))
						temp = 0;
					else
			#endif
					temp = (*p_buf)*SOFT_GAIN;
					if(temp>32767)
						*p_buf = 32767;
					else if(temp<-32767)
						*p_buf = -32767;
					else
						*p_buf = temp;


					p_buf++;
				}
				p_buf -= sample_len;
			/************** 在分发之前存储 PCM **************/
			if (g_pcm_recording && g_pcm_wr_index < PCM_MAX_FRAMES) {
				/* p_buf 是当前帧处理完毕后的 PCM 数据指针 */
				memcpy(g_pcm_record_buf[g_pcm_wr_index],
					   p_buf,
					   sample_len * sizeof(int16_t));   // sample_len = 160
				g_pcm_wr_index++;
			}
			/********************************************************/
			
			data->type = SET_DATA_TYPE(SOUND,SOUND_MIC);
		#if ADC_DEBUG
            if ((frame_idx % 50) == 0) {
                ADC_LOG("[ADC TASK] send_data_to_stream frame=%lu type=0x%x\n",
                        (unsigned long)frame_idx, data->type);
            }
        #endif
		
		
            send_data_to_stream(data);
			frame_idx++;   // 放在最后
		}
		else
		{
		 	_os_printf("%s:%d err\n",__FUNCTION__,__LINE__);
		}
	}
}

static uint32_t get_sound_data_len(void *data)
{
    struct data_structure  *d = (struct data_structure  *)data;
	return (uint32_t)d->priv;
}

static uint32_t set_sound_data_len(void *data,uint32_t len)
{
	struct data_structure  *d = (struct data_structure  *)data;
	d->priv = (void*)AUDIOLEN;
	return (uint32_t)AUDIOLEN;
}

static uint32_t set_sound_data_time(void *data,uint32_t len)
{
	struct data_structure  *d = (struct data_structure  *)data;
	d->timestamp = os_jiffies();
	return (uint32_t)0;
}

static stream_ops_func stream_sound_ops = 
{
	.get_data_len = get_sound_data_len,
	.set_data_len = set_sound_data_len,
};

static int opcode_func(stream *s,void *priv,int opcode)
{
    static uint8_t *adc_audio_buf = NULL;
	int res = 0;
	switch(opcode)
	{
		case STREAM_OPEN_EXIT:
		{
			s->priv = (void*)os_malloc(sizeof(struct audio_adc_s));
			if(s->priv)
			{
				struct audio_adc_s *self_priv = (struct audio_adc_s*)s->priv;
				self_priv->adc_msgq  = (void*)csi_kernel_msgq_new(1,sizeof(uint8_t*));
				OS_TASK_INIT("adc_audio_deal", &self_priv->thread_hdl, audio_deal_task, s, OS_TASK_PRIORITY_ABOVE_NORMAL, 10240);
			}
            adc_audio_buf = os_malloc(AUDIONUM * (AUDIOLEN + MEDIAN_FILTER_SAMPLE_LEN*2));
            if(adc_audio_buf)
            {
			    stream_data_dis_mem(s,AUDIONUM);
            }
			streamSrc_bind_streamDest(s,R_INTERCOM_AUDIO);
			streamSrc_bind_streamDest(s,R_SPEECH_RECOGNITION);
			streamSrc_bind_streamDest(s,R_RECORD_AUDIO);
            streamSrc_bind_streamDest(s,R_RTP_AUDIO);
            streamSrc_bind_streamDest(s,R_AUDIO_TEST);
           	streamSrc_bind_streamDest(s,R_SPEAKER);
			streamSrc_bind_streamDest(s,R_AT_SAVE_AUDIO);
			streamSrc_bind_streamDest(s,R_AT_AVI_AUDIO);
		}
		break;

		case STREAM_DATA_DIS:
		{
			struct data_structure *data = (struct data_structure *)priv;
			int data_num = (int)data->priv;
            data->priv = (void*)AUDIOLEN;
			data->ops = &stream_sound_ops;
			data->data = adc_audio_buf + (data_num)*(AUDIOLEN + MEDIAN_FILTER_SAMPLE_LEN*2);
		}
		break;

		default:
			break;
	}
	return res;
}

int audio_adc_start(void *audio_hdl)
{
	int ret = 0;
	int res = 0;
	void *buf;
	struct audio_ad_config *priv = (struct audio_ad_config*)audio_hdl;
	buf = priv->set_buf(priv->priv_el,&priv->current_node);
	if(!buf)
	{
		ret = -1;
		goto audio_adc_start_err;
	}
    priv->irq_func(priv , buf, priv->buf_size);
	audio_adc_start_err:
	return res;
}

static int32 global_audio_ad_read(struct audio_ad_config *audio, void* buf, uint32 len)
{
	auadc_read(audio->adc, buf, len);
	return 0;
}

stream *audio_adc_stream_init(const char *name)
{
	stream *s = open_stream_available(name,AUDIONUM,0,opcode_func,NULL);
	if(s) {
		global_audio_adc_s = s;
	}
	return s;
}

void audio_adc_stream_deinit()
{
	int res;
	if(global_audio_adc_s) {
		res = close_stream(global_audio_adc_s);
		if(!res) {
			global_audio_adc_s = NULL;
		}
	}
}

int audio_adc_init()
{	
	int res = 0;
	stream *s = NULL;

    struct auadc_device *adc = (struct auadc_device *)dev_get(HG_AUADC_DEVID);
	s = audio_adc_stream_init(S_ADC_AUDIO);
	if(!s)
	{
        res = -1;
		goto audio_adc_init_err;
	}

#if AEC_PROCESS == 1
	aec_flag = aec_init(8000, 2);
	if(aec_flag == 0) 
		os_printf("AEC Init success\n");
#endif
#if NSX_PROCESS == 1
	nsx_flag = ns_init(8000);
	if(nsx_flag == 0)
		os_printf("NSX Init success\n");
#endif
#if AGC_PROCESS == 1
	agc_flag = agc_init(kAgcModeAdaptiveDigital, 8000);
	if(agc_flag == 0)
		os_printf("AGC Init success\n");
#endif	
#if VAD_PROCESS == 1
	struct auvad_device *vad_dev = (struct auvad_device*)dev_get(HG_AUVAD_DEVID);
	auvad_open(vad_dev, AUVAD_CALC_MODE_ENERGY|AUVAD_CALC_MODE_ZCR);
	vad_flag = vad_init(kVadNormal);
	if(vad_flag == 0)
		os_printf("VAD Init success\n");
#endif
#if MAGIC_SOUND
	magic_sound = magicSound_init(8000,1,AUDIOLEN);
#endif
    struct audio_adc_s *audio_priv = (struct audio_adc_s*)s->priv;
	if(audio_priv)
	{
        struct audio_ad_config *ad_config = (struct audio_ad_config*)os_malloc(sizeof(struct audio_ad_config));
        memset(ad_config,0,sizeof(struct audio_ad_config));
        ad_config->adc = adc;
        ad_config->priv_el = s;
		audio_priv->audio_hardware_hdl = ad_config;
		audio_adc_register(ad_config,s,AUDIOLEN,audio_set_buf,audio_get_buf);
        ad_config->irq_func = global_audio_ad_read;
        auadc_open(adc, AUADC_SAMPLE_RATE_8K);
        auadc_request_irq(adc, AUADC_IRQ_FLAG_HALF | AUADC_IRQ_FLAG_FULL, (auadc_irq_hdl)audio_adc_irq, (uint32)ad_config);
        audio_adc_start(ad_config);
	#if ADC_DEBUG
        ADC_LOG("[ADC INIT] ok, AUDIOLEN=%d, sample_rate=8K, stream=%p, ad_config=%p\n",
                AUDIOLEN, s, ad_config);
    #endif
	}

	audio_adc_init_err:
	return res;
}

int audio_adc_deinit()
{
	#if VAD_PROCESS == 1
	struct auvad_device *vad_dev = (struct auvad_device*)dev_get(HG_AUVAD_DEVID);
	auvad_close(vad_dev);
	#endif
	
	struct auadc_device *adc = (struct auadc_device *)dev_get(HG_AUADC_DEVID);
	auadc_close(adc);
	stream *s = NULL;
	s = audio_adc_stream_init(S_ADC_AUDIO);
	struct audio_adc_s *audio_priv = (struct audio_adc_s*)s->priv;
	struct audio_ad_config *ad_config = audio_priv->audio_hardware_hdl;
	if(ad_config->current_node) {
		os_printf("adc force current data:%X\n",ad_config->current_node);
		force_del_data(ad_config->current_node);
		ad_config->current_node = NULL;
	}

	if(ad_config->reg_node) {
		os_printf("adc force reg_node data:%X\n",ad_config->reg_node);
		force_del_data(ad_config->reg_node);
		ad_config->reg_node = NULL;
	}
	
	audio_adc_stream_deinit();
	return 0;
}

int audio_adc_reinit()
{
	stream *s = NULL;
	struct auadc_device *adc = (struct auadc_device *)dev_get(HG_AUADC_DEVID);
	s = audio_adc_stream_init(S_ADC_AUDIO);
    struct audio_adc_s *audio_priv = (struct audio_adc_s*)s->priv;
	if(s && audio_priv) {
        struct audio_ad_config *ad_config = audio_priv->audio_hardware_hdl;
        memset(ad_config,0,sizeof(struct audio_ad_config));
        ad_config->adc = adc;
        ad_config->priv_el = s;
		audio_priv->audio_hardware_hdl = ad_config;
		audio_adc_register(ad_config,s,AUDIOLEN,audio_set_buf,audio_get_buf);
        ad_config->irq_func = global_audio_ad_read;
		
		#if VAD_PROCESS == 1
		struct auvad_device *vad_dev = (struct auvad_device*)dev_get(HG_AUVAD_DEVID);
		auvad_open(vad_dev, AUVAD_CALC_MODE_ENERGY|AUVAD_CALC_MODE_ZCR);
		#endif
		
        auadc_open(adc, AUADC_SAMPLE_RATE_8K);
        auadc_request_irq(adc, AUADC_IRQ_FLAG_HALF | AUADC_IRQ_FLAG_FULL, (auadc_irq_hdl)audio_adc_irq, (uint32)ad_config);
        audio_adc_start(ad_config);
	}
	audio_adc_stream_deinit();
	return 0;
}
