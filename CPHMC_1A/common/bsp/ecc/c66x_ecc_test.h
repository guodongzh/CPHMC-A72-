/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       c66x_ecc_test.h
 *@author     jinyangh
 *@date       2026.06.16
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.06.16  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _C66X_ECC_TEST_H
#define _C66X_ECC_TEST_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef BUILD_C66X
/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <ti/csl/csl_types.h>
#include <ti/csl/soc.h>
#include <ti/csl/arch/csl_arch.h>
#include <ti/csl/hw_types.h>
#include <bsp/uart/uart_test.h>
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define DSP_IRQ_113                     (113U)
#define DSP_ECC_NUM_BYTES               (128U)
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
int32_t dspEccL1PCacheTest(void);
int32_t dspEccL1PDmaTest(void);
void L1P_ED_enable(void);
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _C66X_ECC_TEST_H */