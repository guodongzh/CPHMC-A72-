/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscMem.h
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCMEM_H
#define _PSCMEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscBase.h"
#include "pscGlobal.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void PscMemAbsSetByte  (PSCBYTE *pAddr_p, PSCBYTE ByteVal_p);
void PscMemAbsSetWord  (PSCBYTE *pAddr_p, PSCWORD WordVal_p);
void PscMemAbsSetDword (PSCBYTE *pAddr_p, PSCDWORD DwordVal_p);
PSCBYTE  PscMemAbsGetByte  (PSCBYTE *pAddr_p);
PSCWORD  PscMemAbsGetWord  (PSCBYTE *pAddr_p);
PSCDWORD PscMemAbsGetDword (PSCBYTE *pAddr_p);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCMEM_H */
