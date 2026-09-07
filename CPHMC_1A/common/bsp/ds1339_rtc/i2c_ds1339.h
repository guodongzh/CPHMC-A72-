/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       i2c_ds1339.h
*@author     xqb
*@date       2024.07.08
*@brief      i2c read ds1339RTC test source.h file
*@par        History
*Date        Version   Author     Description
*2024.07.08   1.0       xqb        example
*2024.12.18  1.1       LiuRui     refactor
******************************************************************************/
#ifndef _I2C_DS1339_H
#define _I2C_DS1339_H

#include <stdio.h>
#include <string.h>
#include <ti/osal/osal.h>
#include <ti/drv/i2c/I2C.h>
#include <ti/drv/i2c/soc/I2C_soc.h>
#include <ti/csl/soc.h>
#include "board/board.h"
#include "debug_config.h"

/* DS1339 address*/
#define DS1339_ADDR                  0x68

/* DS1339 RTCconfig */
#define REGADDR_SECONDS              0x00
#define REGADDR_MINUTES              0x01
#define REGADDR_HOURS                0x02
#define REGADDR_DAY_OF_WEEK          0x03
#define REGADDR_DATE                 0x04
#define REGADDR_MONTH_CENTURY        0x05
#define REGADDR_YEAR                 0x06

#define REGADDR_ALARM1SECONDS        0x07
#define REGADDR_ALARM1MINUTES        0x08
#define REGADDR_ALARM1HOURS          0x09
#define REGADDR_ALARM1DAY_ALARM1DATE 0x0A

#define REGADDR_ALARM2MINUTES        0x0B
#define REGADDR_ALARM2HOURS          0x0C
#define REGADDR_ALARM2DAY_ALARM2DATE 0x0D

#define REGADDR_CONTROL              0x0E
#define REGADDR_STATUS               0x0F
#define REGADDR_TRICKLECHARGER       0x10

typedef struct
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t weekday;
    uint8_t day;
    uint8_t month;
    uint16_t year;  // 00-99
} ds1339_time_t;

extern ds1339_time_t g_rtc_time;

uint8_t ConvertDataToSet(const ds1339_time_t *time, uint8_t regType);
void ConvertDataToGet(uint8_t rData, ds1339_time_t *time, uint8_t regType);
int32_t rtc_ds1339_check_status(void);
int32_t rtc_s1339_init(void);
int32_t rtc_ds1339_set_time(ds1339_time_t *time);
int32_t rtc_ds1339_get_time(ds1339_time_t *time);
void rtc_ds1339_test(void);

#endif /* _I2C_DS1339_H */
