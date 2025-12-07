#ifndef _BATTERY_H
#define _BATTERY_H
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"
#include "hal/gpio.h"
#include "osal/timer.h"
#include "osal/semaphore.h"
#include "osal/mutex.h"



void bat_det_init(void);
uint8_t get_batlevel(void);

#endif