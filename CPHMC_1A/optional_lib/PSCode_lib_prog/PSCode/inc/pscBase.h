/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscBase.h
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCBASE_H
#define _PSCBASE_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
/**************************** definition of base types ****************************/
#define PSCTRUE  0xFF
#define PSCFALSE 0x00
#define PSCNULL  0

/* 1 byte */
#define PSCBOOL     uint8_t
#define PSCCHAR     char
#define PSCSHORT    int8_t
#define PSCBYTE     uint8_t
/* 2 bytes */
#define PSCSINT     int16_t
#define PSCINT      int16_t
#define PSCUINT     uint16_t
#define PSCWORD     uint16_t
/* 4 bytes */
#define PSCLONG     int32_t
#define PSCDWORD    uint32_t
#define PSCFLOAT    float
/* 8 bytes */
#define PSCLONGLONG int64_t
#define PSCQWORD    uint64_t
#define PSCDOUBLE   double

#define PSC_SIZEOF_BYTE   1
#define PSC_SIZEOF_CHAR   1
#define PSC_SIZEOF_INT    2
#define PSC_SIZEOF_SINT   2
#define PSC_SIZEOF_WORD   2
#define PSC_SIZEOF_DWORD  4
#define PSC_SIZEOF_LONG   4
#define PSC_SIZEOF_DOUBLE 8
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCBASE_H */
