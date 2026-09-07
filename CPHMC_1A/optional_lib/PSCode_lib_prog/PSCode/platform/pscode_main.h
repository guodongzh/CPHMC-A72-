/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscode_main.h
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCODE_MAIN_H
#define _PSCODE_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscGlobal.h"
#include "pscTypes.h"
#include "pscCom.h"
#include "pscEnv.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define COMM_STACK_DEPTH  1024u
#define TASK_COMM_PRIO    10
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
int pscode_main(void);
PSCBYTE PscEnvInitialize(void);
void PLC_Comm(void *arg);
void PscStart(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCODE_MAIN_H */
