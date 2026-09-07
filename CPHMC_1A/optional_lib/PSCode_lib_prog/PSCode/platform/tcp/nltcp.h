/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       nltcp.h
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _NLTCP_H
#define _NLTCP_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "pscBase.h"
#include "pscCom.h"
#include "net_tcp.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
PSCBYTE NetGetRxStatus();
PSCBYTE NetRecData(PSCBYTE *pData_p, PSCDWORD dwDataSize_p, PSCBYTE bDataType_p, PSCDWORD dRxTimeOut_p);
PSCBYTE NetSendData(PSCBYTE *pData_p, PSCDWORD dwDataSize_p, PSCBYTE bDataType_p, PSCDWORD dTxTimeOut_p);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _NLTCP_H */
