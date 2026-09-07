/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       RS485.h
 *@author     xuesen
 *@date       2024.12.24
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2024.12.24  1.0       wenjunf    example
 ******************************************************************************/
#ifndef _CPHMC_1A_R0_RS485_H
#define _CPHMC_1A_R0_RS485_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

#include "debug_config.h"
#include <string.h>
#include <ti/csl/csl_uart.h>
#include <ti/csl/soc.h>
#include <ti/drv/uart/UART_stdio.h>

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef  struct _rs485_diag
{
    uint32_t recv_oks;
    uint32_t recv_err;
}rs485_diag_t;

extern rs485_diag_t rs485_diag;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void rs485_change_dir(int dir);
void rs485_init();
void rs485_loop_back();

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _CPHMC_1A_R0_RS485_H */
