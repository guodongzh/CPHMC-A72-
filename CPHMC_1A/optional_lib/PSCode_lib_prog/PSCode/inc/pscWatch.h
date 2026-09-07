/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscWatch.h
 *@author     jinyangh
 *@date       2025.01.20
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.20  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCWATCH_H
#define _PSCWATCH_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscBase.h"
#include "pscTypes.h"
#include "pscMem.h"
#include "pscWave.h"
#include "nltcp.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define WATCHIDLIST_SIZE    (((PSC_SIZEOF_WORD/PSC_SIZEOF_BYTE)* PSCMAXWATCH) + PSC_SIZEOF_WORD)
#define WATCHID_DELETE_SIZE ((PSC_SIZEOF_BYTE+PSC_SIZEOF_WORD) / PSC_SIZEOF_BYTE)
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern tPscWatchTable pWatchTables_g;
extern PSCDWORD watchData_g[PSCMAXWATCH];

extern PSCBYTE *pBuff_l;
extern PSCWORD wBuffSize_l;
extern PSCWORD wDataSize_l;

void PscWatchInitWatchTab ();
void PscWatchMarkDataValid();
PSCBYTE PscWatchDiscardWatchTab();
PSCBYTE PscWatchAddMulInstruction(tPscPSCmd *pPscPSCmd_p);
PSCBYTE PscWatchAddMulInstructionCont (tPscPSCmd *pPscPSCmd_p);
PSCBYTE PscWatchEnableWatching(tPscPSCmd *pPscPSCmd_p);
PSCBOOL PscWatchSendData(tPscWatchTable *pWatchTable_p);
PSCBOOL PscWatchExecInstruction(tPscWatchTable *pWatchTable_p, PSCWORD wWatchId_p);
PSCBYTE PscWatchDelInstruction ();
void    PscGetWatchData();
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCWATCH_H */
