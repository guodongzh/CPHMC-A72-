/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       board_info.h
 *@author     LiuRui
 *@date       2026.04.07
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.04.07  1.0       LiuRui
 ******************************************************************************/

#ifndef __BOARD_INFO_H
#define __BOARD_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
enum PWR_ID
{
    PWRA_ID = 0,
    PWRB_ID = 1,
    POWER_ID_MAX = 2,
};

enum SLOT_ID
{
    SLOTA_ID = 0,
    SLOTB_ID = 1,
    SLOTC_ID = 2,
    SLOTD_ID = 3,
    PEXIST_ID = 4,
    SLOT_ID_MAX = 5,
};

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/
typedef struct _power_status
{
    uint8_t is_online;
    uint8_t fala;
    uint8_t falb;
    uint8_t tmp;
}power_status_t;

extern power_status_t power_status[POWER_ID_MAX];
bool slot_status[SLOT_ID_MAX];
extern int32_t degree_temp_val[5];

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

/**
 * @brief gpio get power status
 */
void get_power_status(void);
void get_slot_ex(void);
void get_pfunc(void);
void print_power_status(void);
void get_all_temp_sensor_value(void);
void print_all_temp_sensor_value(void);
#ifdef __cplusplus
}
#endif

#endif  //__BOARD_INFO_H