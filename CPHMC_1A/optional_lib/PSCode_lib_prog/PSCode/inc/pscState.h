/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscState.h
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCSTATE_H
#define _PSCSTATE_H

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
extern PSCWORD wPscSysFlags_l;
extern PSCDWORD PscSysStatus_l;

extern PSCDWORD dwPSCCapabilities[10];

extern PSCBYTE bLastPlcErr_l;
extern PSCBOOL RecordErrFlag;
extern PSCBOOL InfErrFlag;

void PscSysSetStatus(PSCWORD StatusFlag_p, PSCBYTE SetClr_p);
PSCBOOL PscSysGetStatus (PSCWORD StatusFlag_p);
void PscSysUpdateMode();
PSCWORD PscSysCallback(PSCBYTE *pBuff_p, PSCWORD wBuffSize_p);
PSCWORD PscInfExtCapCallback(PSCBYTE *pBuff_p, PSCWORD wBuffSize_p);
void PscCapabilitiesInit();
void PscInfoEvalCapabilities();
PSCBOOL PscGetResVersion(tPscResVersion *pResVersion);
PSCBOOL PscInfRecordError(PSCBYTE bErrCode_p);
PSCBOOL PscInfRegisterError(PSCBYTE bErrCode_p);
PSCWORD PscInfErrCallback(PSCBYTE *pBuff_p, PSCWORD wBuffSize_p);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCSTATE_H */
