/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscCom.h
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCCOM_H
#define _PSCCOM_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscTypes.h"
#include "pscState.h"
#include "pscEnv.h"
#include "pscCmd.h"
#include "pscWatch.h"
#include "pscForce.h"
#include "pscMem.h"
#include "pscInfo.h"
#include "pscGlobal.h"
#include "pscDownload.h"
#include "nltcp.h"
#include "net_tcp.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
/* Definition of Errorcodes*/
#define NET_SUCCESS         0x00        /* OK, Data available*/
#define NET_NODATA          0xFD        /* wait for data reception*/
#define NET_RX_SUCCESS      0xFC        /* data received successfull */
#define NET_TX_SUCCESS      0xFB        /* data sent successfull */
#define NET_ERROR           0xFA        /* general network error */
#define NET_TIMEOUT         0xF9        /* TimeOut while waiting for reception */
#define NET_RESTART_COMM    0xEE        /* reset of network layer (e.g. CANopen-command "ResetNode" or "ResetCommunication") */

/* constants for symbolic identification of attributes */
#define PSCCOLDSTART       0x00                /* identifier "New start" */
#define PSCWARMSTART       0xFF                /* identifier "warm start" */
#define PSCINITIALIZING    0x80                /* identifier "Boot-Phase" */
#define PSCSYSTEMHALTED    0x81                /* identifier "System Halted" */
#define PSCSET             1
#define PSCCLR             0
#define PSCPRIMARY         0                   /* "column index" for access */
#define PSCSECUNDARY       1                   /*  in segment table */

/* bit definitions to store (current) system properties */
#define PSCSTAT_OK                0x00000001L       /* B0:  Controller OK */
#define PSCSTAT_LOGIN             0x00000002L       /* B1:  Controller in Login-Mode */
#define PSCSTAT_DOWNLOADING       0x00000004L       /* B2:  Download activ */
#define PSCSTAT_TASKDEFTAB_VALID  0x00000008L       /* B3:  TaskDefTab present */
#define PSCSTAT_SEGTAB_CREATED    0x00000010L       /* B4:  Segmenttabelle present */
#define PSCSTAT_PROGRAM_VALID     0x00000020L       /* B5:  valid program present */
#define PSCSTAT_COMMAND_EXECUTED  0x00000040L       /* B6:  command executed                             */
#define PSCSTAT_HW_STOP		  0x00000080L       /* B7:  state of hardware run/stop switch */
#define PSCSTAT_EXT_COMM_LOCKED   0x00000100L       /* B8:  external Comm. stopped */
#define PSCSTAT_RECOVERING        0x00000200L       /* B9:  Recover-Mode activ */
#define PSCSTAT_NIOERROR          0x00000400L       /* B10: Networkerror Remote-IO */
#define PSCSTAT_C_CODE_MODE       0x00000800L       /* B11: C code generator mode is configured */
#define PSCSTAT_RUN_EVENT_TASK    0x00001000L       /* B12: Execution of Start/Stop/Error-Task */
#define PSCSTAT_RUNNING           0x00002000L       /* B13: program execution activ */
#define PSCSTAT_NETERROR          0x00004000L       /* B14: Communication-Error with PC */
#define PSCSTAT_PLCERROR          0x00008000L       /* B15: Stop after internal error */
#define PSCSTAT_ACCPT_DOWNLOAD    0x00010000L       /* B16: Download Resource possible */
#define PSCSTAT_ACCPT_SEGMENT     0x00020000L       /* B17: Download Segment allowed */
#define PSCSTAT_ACCPT_STOP_CMD    0x00040000L       /* B18: Stop-command allowed */
#define PSCSTAT_ACCPT_COLD_START  0x00080000L       /* B19: Cold-start possible/supported */
#define PSCSTAT_ACCPT_WARM_START  0x00100000L       /* B20: Warm-start possible/supported */
#define PSCSTAT_ACCPT_HOT_START   0x00200000L       /* B21: Continuation possible/supported */
#define PSCSTAT_ACCPT_POWERFLOW   0x00400000L       /* B22: Powerflow possible/supported */
#define PSCSTAT_ACCPT_FORCE_BIT   0x00800000L       /* B23: Force Bit possible/supported */
#define PSCSTAT_ACCPT_FORCE_BYTE  0x01000000L       /* B24: Force Byte possible/supported */

/* Definition of "TxTypes"*/
#define NET_CMD             0x01        /* DataType "COMMAND"*/
#define NET_DATA            0x02        /* DataType "DATA"*/
#define NET_REC             0x03        /* DataType "RECEIPT"*/

/* Definition of shutdown modes*/
#define NET_SHUTDOWN        0xFF        /* shutdown network layer*/
#define NET_ERROR_DOWN      0xFE        /* shutdown after network crash*/

/* Definition of times*/
#define NET_RxTIMEOUT       5000        /* TimeOut for reception of data*/
#define NET_TxTIMEOUT       5000        /* TimeOut for sending data*/
#define NET_RECTIMEOUT      5000        /* TimeOut for reception of receipts*/
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern tPscCmdBuffer pCmdBuffers_g;

PSCBOOL PscCmdMainLoop(void);
void PscPlcStart();
void PscRestartNetAndSetNetError(PSCBYTE bNetStat_p);
void PscEnvCommunicationInterrupted();
PSCBOOL PscCsvCheckCmdReceipt(tPscPSCmd *pPscPSCmd_p);
void PscCsvInitCmdServer();
void PscCsvEnableCmdReceipt();
PSCBYTE PscPlcExecPSCmd(tPscPSCmd *pPscPSCmd_p);
PSCBOOL PscCsvSetExtRetCode(PSCBYTE *pExtErrCode_p, PSCWORD wSize_p);
void PscCsvSendReceipt(PSCBYTE bErrCode_p);
void PscCsvSetMaxRecSize();
void PscCsvPresetCmd(tPscPSCmd *pPscPSCmd_p, PSCBYTE bMode_p);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCCOM_H */
