/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       rat_test.h
 *@author     LiuRui
 *@date       2024.07.11
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2024.07.11  1.0       LiuRui
 ******************************************************************************/

#ifndef _ALL_RAT_TEST_H
#define _ALL_RAT_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ti/csl/csl_types.h>
#include <ti/csl/soc.h>
#include <ti/csl/arch/csl_arch.h>
#include <ti/csl/hw_types.h>
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

/*===========================================================================*/
/*                         Macros                                            */
/*===========================================================================*/
#define CSL_APP_TEST_NOT_RUN (-(int32_t)(2))
#define CSL_APP_TEST_FAILED  (-(int32_t)(1))
#define CSL_APP_TEST_PASS    ((int32_t)(0))

int32_t r5_rat_test(void);

#ifdef __cplusplus
}
#endif

#endif  //_ALL_RAT_TEST_H