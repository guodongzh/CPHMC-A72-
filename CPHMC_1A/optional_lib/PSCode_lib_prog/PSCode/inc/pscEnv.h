/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscEnv.h
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCENV_H
#define _PSCENV_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscCom.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
/*
 *  Application functions in C code generator mode
 */
/* application functions to be called internally by SmartPLC */
PSCBYTE Application_InitResVersion();
PSCBYTE Application_InitTaskSettings();
PSCBYTE Application_InitDataConsistency();
PSCBYTE Application_Init();
PSCBYTE Application_AllTasks_I();
PSCBYTE Application_InitVartab();
/* application functions to be called from the scheduler (on some platforms, these functions need an external interface, that's why they are "public") */
PSCBYTE Application_I1_S();
PSCBYTE Application_I1_FI();
PSCBYTE Application_I1_N();
PSCBYTE Application_I1_FE();
PSCBYTE Application_I2_S();
PSCBYTE Application_I2_FI();
PSCBYTE Application_I2_N();
PSCBYTE Application_I2_FE();
PSCBYTE Application_I3_S();
PSCBYTE Application_I3_FI();
PSCBYTE Application_I3_N();
PSCBYTE Application_I3_FE();
PSCBYTE Application_I4_S();
PSCBYTE Application_I4_FI();
PSCBYTE Application_I4_N();
PSCBYTE Application_I4_FE();
PSCBYTE Application_I5_S();
PSCBYTE Application_I5_FI();
PSCBYTE Application_I5_N();
PSCBYTE Application_I5_FE();
PSCBYTE Application_I6_S();
PSCBYTE Application_I6_FI();
PSCBYTE Application_I6_N();
PSCBYTE Application_I6_FE();
PSCBYTE Application_I7_S();
PSCBYTE Application_I7_FI();
PSCBYTE Application_I7_N();
PSCBYTE Application_I7_FE();
PSCBYTE Application_I8_S();
PSCBYTE Application_I8_FI();
PSCBYTE Application_I8_N();
PSCBYTE Application_I8_FE();
PSCBYTE Application_T1_S();
PSCBYTE Application_T1_FI();
PSCBYTE Application_T1_CI();
PSCBYTE Application_T1_N();
PSCBYTE Application_T1_FE();
PSCBYTE Application_T1_CE();
PSCBYTE Application_T2_S();
PSCBYTE Application_T2_FI();
PSCBYTE Application_T2_CI();
PSCBYTE Application_T2_N();
PSCBYTE Application_T2_FE();
PSCBYTE Application_T2_CE();
PSCBYTE Application_T3_S();
PSCBYTE Application_T3_FI();
PSCBYTE Application_T3_CI();
PSCBYTE Application_T3_N();
PSCBYTE Application_T3_FE();
PSCBYTE Application_T3_CE();
PSCBYTE Application_T4_S();
PSCBYTE Application_T4_FI();
PSCBYTE Application_T4_CI();
PSCBYTE Application_T4_N();
PSCBYTE Application_T4_FE();
PSCBYTE Application_T4_CE();
PSCBYTE Application_T5_S();
PSCBYTE Application_T5_FI();
PSCBYTE Application_T5_CI();
PSCBYTE Application_T5_N();
PSCBYTE Application_T5_FE();
PSCBYTE Application_T5_CE();
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCENV_H */
