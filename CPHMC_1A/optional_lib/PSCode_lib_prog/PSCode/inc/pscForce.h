/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscForce.h
 *@author     jinyangh
 *@date       2025.02.17
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.02.17  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCFORCE_H
#define _PSCFORCE_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscWatch.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define FORCEID_SIZE        ((PSC_SIZEOF_BYTE+PSC_SIZEOF_WORD) / PSC_SIZEOF_BYTE)

/* offsets relative to tPscSetSingleData */
#define STRUCT_OFFSET_SETDATA_TYPE            0
#define STRUCT_OFFSET_SETDATA_PGMNR           1
#define STRUCT_OFFSET_SETDATA_SEGNR           3
#define STRUCT_OFFSET_SETDATA_OFFSET          5
#define STRUCT_OFFSET_SETDATA_SIZE            7
#define STRUCT_OFFSET_SETDATA_MAKEPERSISTENT  9
#define STRUCT_OFFSET_SETDATA_DATA            10
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern tPscSetData      SetDataTable;
extern tPscForceTable   ForceLst_l;

void PscSetInitSetTab(void);
PSCBYTE PscSetSingleVariable(tPscPSCmd *pPscPSCmd_p);
PSCBYTE PscSetVariable(void);
void PscForceInitForceTab(void);
PSCBYTE PscForce(void);
PSCBYTE PscForceVariable(tPscForceItem *pPscSetData);
PSCBYTE PscForceAddItem(tPscPSCmd *pPscPSCmd_p);
PSCBYTE PscForceDeleteItem(tPscPSCmd *pPscPSCmd_p);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCFORCE_H */
