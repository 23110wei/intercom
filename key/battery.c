#include "sys_config.h"
#include "typesdef.h"
#include "battery.h"
 
#include "dev/adc/hgadc_v0.h"
#include "hal/gpio.h"
#include "osal/string.h"

#if (BAT_ADC_IO!=255)

#define BAT_ADC_DATA_NUM   10
#define MAX_BAT_LEV     5

static uint16_t batAdcFilterTable[BAT_ADC_DATA_NUM];

struct hgadc_v0 *bat_adc ;

static uint8_t battery_level =0;


// 1.5/2.5 = 0.6 
// 1/2 = 0.5 

const uint16_t batterylvTable[MAX_BAT_LEV+1]=
{
#if 1

    2500, // 3.3V    bat1
    2650, // 3.5V    bat2
    2800, // 3.7v	 bat3
    3030, // 4.0v	 bat4
    3185, //4.2V	 bat5
    0xffff,
#else
    2730, // 3.0V *0.6 = 1.8
    3003, // 3.3V *0.6 = 1.98
    3276, // 3.6 *0.6 = 2.16
    3640, // 4.0V*0.6 = 2.4
    3822, //4.2V* 0.6 = 2.52
    0xffff,
#endif
};


static uint8_t bat_compare_jitter(uint16_t cur_val, uint16_t prev_val)
{

    uint16_t err;
	uint8_t cmp_lit = 5;

	if(cur_val > prev_val)
	{
		err = cur_val - prev_val;
	}
	else
	{
		err = prev_val - cur_val;
	}

	if(err > cmp_lit)//40*(3.3/4095) *1000 = XXxmV
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static uint16_t  battery_samp_filter(void)
{
    uint8_t i, j;
	uint16_t temp;
    uint16_t temp_adctable[BAT_ADC_DATA_NUM];
    uint16_t bat_val = 0;

    for (i = 0; i < BAT_ADC_DATA_NUM; i++)
    {
        temp_adctable[i] = batAdcFilterTable[i];
    }

    for (j = 0; j < BAT_ADC_DATA_NUM - 1; j++)
    {
    	for (i = 0; i < BAT_ADC_DATA_NUM - 1 - j; i++)
        {
        	 if (temp_adctable[i] < temp_adctable[i + 1])
        	 {
        		temp = temp_adctable[i];
        		temp_adctable[i] = temp_adctable[i + 1];
        		temp_adctable[i + 1] = temp;
        	 }
         }
	 }

    //去掉3个最小值
    for (i = 0; i < (BAT_ADC_DATA_NUM - 3); i++)
    {
        bat_val += temp_adctable[i];
    }

    return (uint16_t)((bat_val / (BAT_ADC_DATA_NUM - 3)) );//本次ADC的值的计算结果
}

static uint8_t judge_bat_level_(uint16_t ad_v)
{
    uint8_t i;

	i = 0;
	while(ad_v > batterylvTable[i]){
		i++;
	}
	if(i>(MAX_BAT_LEV-1))
	{
		i = (MAX_BAT_LEV-1);
	}
    return i;
}

static void bat_ad_init()
{
    bat_adc = (struct hgadc_v0*)dev_get(HG_ADC0_DEVID);

	adc_open((struct adc_device *)bat_adc);	
	gpio_set_mode(BAT_ADC_IO,GPIO_PULL_NONE,GPIO_PULL_LEVEL_NONE);
	adc_add_channel((struct adc_device *)bat_adc, BAT_ADC_IO);	
}



static void bat_ad_scan()
{
	static uint8_t filter_index, count;
    static uint16_t pre_val;
	static uint8_t overVoltage_cnt=0;
	static uint8_t low_power_cnt = 5;
    uint32_t vol;

    adc_get_value((struct adc_device *)bat_adc, BAT_ADC_IO, &vol);
	//os_printf("bat vol:%d \n",vol);

    count++;

// save ad to list
    if(filter_index >= BAT_ADC_DATA_NUM)
    {
        filter_index = 0;
    }
    batAdcFilterTable[filter_index] = (uint16_t)vol;
    filter_index++;

	
	if (count % 5 == 0)
	{
		uint16_t cur_val = battery_samp_filter();
		//printf("-------------------cur_val=%d \n",cur_val);
		
		if (bat_compare_jitter(cur_val,pre_val))
		{
			pre_val = cur_val;
			battery_level =  judge_bat_level_(cur_val);
			//printf("## battery level update:%d cur_val=%d \n",battery_level,cur_val);
		}

	    {

			if (battery_level == 0
	#if TCFG_CHARGE_DET_CFG
	      && !user_get_charging_status()
	#endif
	        )
			{
				if (low_power_cnt)
				{
					low_power_cnt--;
				}
				else if (low_power_cnt == 0)
				{
					printf("------------------low power\n");
					low_power_cnt = 20;
					//user_send_msg(MSG_POWER_OFF,NULL,0);
				}
			}
			else
			{
				low_power_cnt = 10;
			}
		}
	}

}
static struct os_timer  bat_timer;

void bat_det_init(void)
{
    bat_ad_init();

    os_timer_init(&bat_timer, bat_ad_scan, OS_TIMER_MODE_PERIODIC, NULL);
	os_timer_start(&bat_timer, 100);
}


uint8_t get_batlevel(void)
{
    return battery_level;
}
#else
void bat_det_init(void)
{

}
uint8_t get_batlevel(void)
{
    return 4;
}
#endif