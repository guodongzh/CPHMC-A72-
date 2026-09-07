/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       gpio_intr_init.h
 *@author     LiuRui
 *@date       2025.10.09
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.10.09  1.0       LiuRui
 ******************************************************************************/

#ifndef _CPHMC_1A_R2_GPIO_INTR_INIT_H
#define _CPHMC_1A_R2_GPIO_INTR_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <ti/osal/osal.h>
#include <ti/drv/gpio/GPIO.h>
#include <ti/drv/gpio/soc/GPIO_soc.h>
#include <ti/csl/src/ip/gpio/V0/gpio.h>
#include "board/board.h"
#include "debug_config.h"
#include "gpio_ctrl.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

void gpio_intr_init(void);
void gpio_call_back(uintptr_t arg);

#ifdef __cplusplus
}
#endif

#endif  //_CPHMC_1A_R2_GPIO_INTR_INIT_H
