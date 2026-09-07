/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscGlobal.h
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCGLOBAL_H
#define _PSCGLOBAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscBase.h"
#include "pscTypes.h"
#include "pscPlatform.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define USE_SHARED_MEMORY

#define USE_PROJECT_PASSWORD
#define USE_PLATFORM_NAME

#define SIZE_PROJECT_NAME  32
#define SIZE_RESOURCE_NAME 32
#define SIZE_PLATFORM_NAME 32

#define PSCLOBYTE(w)     ((PSCBYTE)((w) & 0x00FF))
#define PSCHIBYTE(w)     ((PSCBYTE)(((PSCSINT)(w) >> 8) & 0x00FF))
#define PSCLOWORD(l)     ((PSCWORD)(PSCDWORD)(l) & 0xFFFF)
#define PSCHIWORD(l)     ((PSCWORD)((((PSCDWORD)(l)) >> 16) & 0xFFFF))

#define DEFAULT_REC_SIZE ((PSC_SIZEOF_BYTE + PSC_SIZEOF_WORD) / PSC_SIZEOF_BYTE)

#define C_CODE_PGM_NUM           0xEEEE
#define PADT_SEG_NUM_STATIC_NULL 4
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
/* resource version info */
extern tPscResVersion resVersion_g;
extern tPscResVersionTable resVersionMultiCore[PSC_CORE_NUM];

extern PSCBOOL fEnableCommunication;

extern PSCBYTE gDataBuff[1*1024*1024];

extern PSCBYTE Error_g;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCGLOBAL_H */
