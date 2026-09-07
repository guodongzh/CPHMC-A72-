/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       mcan.h
 *@author     xuesen
 *@date       2024.12.26
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2024.12.26  1.0       wenjunf    example
 ******************************************************************************/
#ifndef _CPHMC_1A_R0_MCAN_H
#define _CPHMC_1A_R0_MCAN_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "stdint.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define TEST_SLOT              0

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef struct _mcan_diag
{
    uint32_t recv_oks;
    uint32_t recv_err;
} mcan_diag_t;

extern mcan_diag_t mcan_diag;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void mcan_change_dir(int dir);

void mcan_set_up_lpbk(uint32_t mcan_tx_addr, uint32_t mcan_rx_addr);


/**
 * \brief   This function will transmit fixed pattern from one CAN instance
 * and recieve the same pattern at another CAN instance, given that both
 * are connected externally.
 *
 * \return  None.
 */
void mcan_ext_lpbk_test(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _CPHMC_1A_R0_MCAN_H */
