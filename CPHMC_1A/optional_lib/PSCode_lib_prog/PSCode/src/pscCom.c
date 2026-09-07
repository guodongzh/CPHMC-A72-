/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscCom.c
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "pscCom.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
tPscCmdBuffer pCmdBuffers_g;      //用于缓存命令和接收到的数据
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// Starting the RTS
void PscPlcStart()
{
    /* call system initialization functions */
#ifndef CORE_R5F0
    Application_InitResVersion();
    PscResVersionShare();
#endif
    /* set status sensible */
    PscSysSetStatus(PSCSTAT_OK, PSCSET);
    PscSysStatus_l = (PSCDWORD)wPscSysFlags_l;

    /* initialize command-server */
    PscCsvInitCmdServer();

    /* clear tables with watch-entries */
    PscWatchInitWatchTab();

    /* clear table with force-entries */
    PscSetInitSetTab();

    /* initialization dwPSCCapabilities */
    PscCapabilitiesInit();

    PscSysSetStatus(PSCSTAT_PROGRAM_VALID, PSCSET);
    PscSysSetStatus(PSCSTAT_C_CODE_MODE, PSCSET);
}

// Restart the network and set the error status
void PscRestartNetAndSetNetError(PSCBYTE bNetStat_p)
{
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;

    PSCTRACE("\n-> PSC: PscRestartNetAndSetNetError... ");

    if (bNetStat_p != NET_RESTART_COMM)
    {
        PscSysSetStatus(PSCSTAT_NETERROR, PSCSET);
    }

    PSCTRACE("\n-> PSC: NetShutDown(NET_ERROR_DOWN) / NetInitialize(NET_ERROR_INIT)... ");

    pCmdBuffer->fRecEnabled = PSCFALSE;
    pCmdBuffer->fPrestCmd = PSCFALSE;
    PscCsvEnableCmdReceipt();
}

// communication broken
void PscEnvCommunicationInterrupted()
{
    /* stop system, disable outputs or whatever */
    PscSetInitSetTab();

    /* clear tables with watch-entries */
    PscWatchInitWatchTab();

    PscSysSetStatus(PSCSTAT_LOGIN, PSCCLR); /* remove login state*/

    /* restart network */
    PscRestartNetAndSetNetError(NET_ERROR);
}

// Command Main Loop of the RTS
PSCBOOL PscCmdMainLoop(void)
{
    static tPscPSCmd PSCmd;
    static uint32_t PSCCommBrokenCnt = 0;
    static uint32_t PSCInternalComm  = 0;
    static uint32_t InternalCommFlag = 0;
    PSCBYTE bRetCode;
    bRetCode = kPscSuccess;

    if(PscCsvCheckCmdReceipt((tPscPSCmd *)&PSCmd))
    {
        if ((kPscCmdAddMulWatchInstrCont == PSCmd.m_bCommand) ||
            (kPscCmdContDwlRawFileSegment == PSCmd.m_bCommand))
        {
            // 以防一直卡在内部命令
            PSCCommBrokenCnt = 0;
            InternalCommFlag = 1;
            PSCInternalComm++;
            if (PSCInternalComm > 3000)
            {
                goto comm_broken;
            }
        }
        else
        {
            InternalCommFlag = 0;
            PSCInternalComm  = 0;
            PSCCommBrokenCnt = 0;
        }

        /* execute received command*/
        bRetCode = PscPlcExecPSCmd((tPscPSCmd *)&PSCmd);

        /* set Flag "Command executed" */
        /* Comment: This flag is meaningless for the RTS and is only used by */
        /*            the enviroment as signal, that a command was executed */
        /*            If necessary, the flag has to be cleared by the enviroment */
        PscSysSetStatus(PSCSTAT_COMMAND_EXECUTED, PSCSET);
    }
    else
    {
        PSCCommBrokenCnt++;
        if (PSCCommBrokenCnt > 3000)
        {
            PSCCommBrokenCnt = 0;

        comm_broken:
            if (InternalCommFlag == 1)
            {
                InternalCommFlag = 0;
                PSCInternalComm = 0;
            }

            /* logged in but no communication */
            if (PscSysGetStatus(PSCSTAT_LOGIN))
            {
                /* check elapsed time without communication */
                /* do i.e. PscForceInitForceTab() there */
                PscEnvCommunicationInterrupted();
            }
        }
    }

    if (bRetCode != kPscSuccess)
    {
        return (PSCFALSE);
    }
    else
    {
        return (PSCTRUE);
    }
}

// Check command reception
PSCBOOL PscCsvCheckCmdReceipt(tPscPSCmd *pPscPSCmd_p)
{
    PSCBYTE bNetStat;
    PSCBOOL fRetCode;
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;

    /* internal command deposited in input buffer ? */
    if (!pCmdBuffer->fPrestCmd)
    {
        //不是内部命令
        /* check for network reception */
        bNetStat = NetGetRxStatus();

        /* set flag for "send receipt" */
        pPscPSCmd_p->m_fSendRec = PSCTRUE;
    }
    else
    {
        //内部命令
        bNetStat = NET_SUCCESS;

        pCmdBuffer->fPrestCmd = PSCFALSE;

        /* allow execution of external commands in following cycle again*/
        PscSysSetStatus(PSCSTAT_EXT_COMM_LOCKED, PSCCLR);

        pPscPSCmd_p->m_fSendRec = PSCFALSE;
        PSCTRACE("\n        (internal command)");
    }

    /* evaluate reception status */
    switch (bNetStat)
    {
    case NET_NODATA:
    {
        /* (still) no command received */
        fRetCode = PSCFALSE;
        break;
    }
    case NET_SUCCESS:
    {
        if (PscSysGetStatus(PSCSTAT_NETERROR))
        {
            PscSysSetStatus(PSCSTAT_NETERROR, PSCCLR);
        }

        pPscPSCmd_p->m_bCommand = PscMemAbsGetByte(&pCmdBuffer->bCmdBuff[0]);

        /* command with long data? */
        if (pPscPSCmd_p->m_bCommand == kPscCmdSetVariableLong || pPscPSCmd_p->m_bCommand == kPscCmdForceVariableLong)
        {
            pPscPSCmd_p->m_wParamSize = PscMemAbsGetWord(&pCmdBuffer->bCmdBuff[1]);
            pPscPSCmd_p->m_pParamLst = &pCmdBuffer->bCmdBuff[3];
        }
        else
        {
            pPscPSCmd_p->m_wParamSize = PscMemAbsGetByte(&pCmdBuffer->bCmdBuff[1]);
            pPscPSCmd_p->m_pParamLst = &pCmdBuffer->bCmdBuff[2];
        }

        pCmdBuffer->fRecEnabled = PSCFALSE;
        fRetCode                = PSCTRUE;
        break;
    }
    default:
    {
        fRetCode = PSCFALSE;
        break;
    }
    }

    return (fRetCode);
}

// Call the command functions
PSCBYTE PscPlcExecPSCmd(tPscPSCmd *pPscPSCmd_p)
{
    PSCBYTE bRetCode = kPscGeneralError;

    switch (pPscPSCmd_p->m_bCommand)
    {
    /*--------------------------------------------------------------------- */
    /* Log in to the controller */
    /*--------------------------------------------------------------------- */
    case kPscCmdLogin:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdLogin ");

        PscInfoChange(g_pscode_udp_info.pscode_src_port);

        /* Save maximal size of possible records */
        PscCsvSetMaxRecSize();

        bRetCode = PscCtlLogin();

        break;
    }

    /*--------------------------------------------------------------------- */
    /* Log out of the controller */
    /*--------------------------------------------------------------------- */
    case kPscCmdLogout:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdLogout ");
        bRetCode = PscCtlLogout();
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Reboot PLC */
    /*--------------------------------------------------------------------- */
    case kPscCmdReboot:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdReboot ");
        bRetCode = PscEnvRebootPlc();
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Query firmware library information */
    /*--------------------------------------------------------------------- */
    case kPscCmdGetLibInfo:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdGetLibInfo ");
        bRetCode = PscInfGetFwLibInfo();
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Query firmware version */
    /*--------------------------------------------------------------------- */
    case kPscCmdGetPlcVer:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdGetPlcVer ");
        bRetCode = PscInfGetPlcVersion();
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Query project version */
    /*--------------------------------------------------------------------- */
    case kPscCmdGetResVer:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdGetResVer ");
        bRetCode = PscInfGetResVersion();
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Request for data ("get watch data") */
    /*--------------------------------------------------------------------- */
    case kPscCmdRequestData:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdRequestData ");
        bRetCode = PscCsvSendRequestedData();
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Discard watch table */
    /*--------------------------------------------------------------------- */
    case kPscCmdDiscradWatch:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdDiscardWatch ");
        bRetCode = PscWatchDiscardWatchTab();
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Add multiple watch items */
    /*--------------------------------------------------------------------- */
    case kPscCmdAddMulWatchInstr:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdAddMulWatchInstr ");
        bRetCode = PscWatchAddMulInstruction(pPscPSCmd_p);
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Get multiple watch items */
    /*--------------------------------------------------------------------- */
    case kPscCmdAddMulWatchInstrCont:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdAddMulWatchInstrCont ");
        bRetCode = PscWatchAddMulInstructionCont(pPscPSCmd_p);
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Enable watch jobs */
    /*--------------------------------------------------------------------- */
    case kPscCmdEnableWatch:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdEnableWatch ");
        bRetCode = PscWatchEnableWatching(pPscPSCmd_p);
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Delete watch item */
    /*--------------------------------------------------------------------- */
    case kPscCmdDelWatchInstr:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdDelWatchInstr ");
        bRetCode = PscWatchDelInstruction();
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Set Variables */
    /*--------------------------------------------------------------------- */
    case kPscCmdSetVariable:
    case kPscCmdSetVariableLong:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdSetVariable ");
        bRetCode = PscSetSingleVariable(pPscPSCmd_p);
        break;
    }

    /*--------------------------------------------------------------------- */
    /* force variable */
    /*--------------------------------------------------------------------- */
    case kPscCmdForceVariable:
    case kPscCmdForceVariableLong:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdForceVariable ");
        bRetCode = PscForceAddItem(pPscPSCmd_p);
        break;
    }

    /*--------------------------------------------------------------------- */
    /* disable force variable */
    /*--------------------------------------------------------------------- */
    case kPscCmdDisableForceVariable:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdDisableForceVariable ");
        bRetCode = PscForceDeleteItem(pPscPSCmd_p);
        break;
    }

    /*--------------------------------------------------------------------- */
    /* Set RunMode (Start/Stop/Reset) */
    /*--------------------------------------------------------------------- */
    case kPscCmdSetState:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdSetState ");
        bRetCode = PscCtlSetState(pPscPSCmd_p);
        break;
    }
#if ORIGINAL_DOWNLOAD
    /*-----------------------------------------------------------------*/
    /* Raw Download */
    /*-----------------------------------------------------------------*/
    case kPscCmdDwlRawFile:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdDwlRawFile ");
        bRetCode = PscEnvDwlRawFile(pPscPSCmd_p);
        break;
    }

    /*-----------------------------------------------------------------*/
    /* Raw Download Segment */
    /*-----------------------------------------------------------------*/
    case kPscCmdDwlRawFileSegment:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdDwlRawFileSegment ");
        bRetCode = PscEnvDwlRawFileSegment(pPscPSCmd_p, PSCFALSE);
        break;
    }

    /*-----------------------------------------------------------------*/
    /* Continue Raw Download Segment */
    /*-----------------------------------------------------------------*/
    case kPscCmdContDwlRawFileSegment:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdContDwlRawFileSegment ");
        bRetCode = PscEnvDwlContRawFileSegment(pPscPSCmd_p);
        break;
    }

    /*-----------------------------------------------------------------*/
    /* Raw Download Segment Last*/
    /*-----------------------------------------------------------------*/
    case kPscCmdDwlRawFileLastSegment:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdDwlRawFileLastSegment ");

        /* get the filetime-data and remove it from the command */
        PscEnvSetFileTimeOfRawFile(pPscPSCmd_p);
        bRetCode = PscEnvDwlRawFileSegment(pPscPSCmd_p, PSCTRUE);
        break;
    }
#endif
    /*--------------------------------------------------------------------- */
    /* Query of system informations */
    /*--------------------------------------------------------------------- */
    case kPscCmdGetErrorInf:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd:kPscCmdGetErrorInf ");
        bRetCode = PscInfGetError();
        break;
    }

    /*--------------------------------------------------------------------- */
    /* unknown command */
    /*--------------------------------------------------------------------- */
    default:
    {
        PSCTRACE("\nPscCom-PscPlcExecPSCmd: ERROR - unknown command! ");
        bRetCode = kPscUnknownCmd;
        break;
    }
    }

    if (pPscPSCmd_p->m_fSendRec)
    {
         /* save ErrorCode of command function for future requests  */
         PscInfRegisterError(bRetCode);

         /* Send execution confirmation of command to IDE*/
         PscCsvSendReceipt(bRetCode);
    }
    else
    {
         //Only record error code ,no sent return data
         PscInfRecordError(bRetCode);
    }

    /* Clear input buffer for next command */
    PscCsvEnableCmdReceipt();

    return (bRetCode);
}

//发送接收消息。该消息包含命令的返回码（bErrCode_p）和附加数据的大小（wBuffSize）。
// Send execution confirmation for command
void PscCsvSendReceipt(PSCBYTE bErrCode_p)
{
    PSCWORD wBuffSize;
    PSCBYTE bNetStat;

    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;
    wBuffSize = pCmdBuffer->wMaxRecSize;

    PscMemAbsSetByte(&(pCmdBuffer->bRecBuff[0]), bErrCode_p);//向bRecBuff写入返回码

    /* remaining buffersize sufficient? */
    if (wBuffSize >= pCmdBuffer->wUsedRecSize)//判断接收缓冲区是否还有未使用的空闲位置
    {
         /* Save amount of piggyback data in buffer */
         wBuffSize = pCmdBuffer->wUsedRecSize - DEFAULT_REC_SIZE;//计算负载数据的大小，不算头部的3Byte

         PscMemAbsSetWord(&(pCmdBuffer->bRecBuff[1]), wBuffSize);//将负载大小写入pCmdBuffer->bRecBuff[1]和[2]

         /* determine complete buffer size for <NetSendData> */
         wBuffSize += DEFAULT_REC_SIZE;//确定<NetSendData>的完整缓冲区大小
    }
    else
    {
         /* Specification of the IDE for maximum size of the receipt shipment */
         /* too small                                                         */
         PscMemAbsSetByte(&(pCmdBuffer->bRecBuff[0]), kPscNetRecSizeError);

         PscMemAbsSetWord(&(pCmdBuffer->bRecBuff[1]), 0);

         /* set complete buffer size for <NetSendData> */
         wBuffSize = DEFAULT_REC_SIZE;
    }

    /* send buffer */
    if (!PscSysGetStatus(PSCSTAT_NETERROR))
    {
         PSCTRACE("\n-> PSC: PscCsvSendReceipt [NetSendData(NET_REC)] ");
         bNetStat = NetSendData(&(pCmdBuffer->bRecBuff[0]), wBuffSize, NET_REC, NET_TxTIMEOUT);

         if (bNetStat != NET_SUCCESS)
         {
                PSCTRACE("\n-> PSC: *** ERROR *** NetSendData/NetSendStart=0x%02X", (PSCWORD)bNetStat);
                PscRestartNetAndSetNetError(NET_ERROR);
         }
    }

    /* set fill level of receipt buffer to standard value again */
    pCmdBuffer->wUsedRecSize = DEFAULT_REC_SIZE;
}

// Store extended returncode in receiptbuffer
PSCBOOL PscCsvSetExtRetCode(PSCBYTE *pExtErrCode_p, PSCWORD wSize_p)
{
    PSCBYTE *pRecBuff;   /* Pointer to receiptbuffer */
    PSCBOOL fRetCode;
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;

    /* Remaining size of receiptbuffer sufficient? */
    if ((pCmdBuffer->wMaxRecSize - pCmdBuffer->wUsedRecSize) >= wSize_p)
    {
         /* set pointer to receiptbuffer */
         pRecBuff = &(pCmdBuffer->bRecBuff[pCmdBuffer->wUsedRecSize]);

         /* update remaining buffersize... */
         pCmdBuffer->wUsedRecSize += wSize_p;

         /* ...and store extended errorCode in receiptbuffer */
         while (wSize_p--)
         {
            *pRecBuff++ = *pExtErrCode_p++;
         }

         fRetCode = PSCTRUE;
    }
    else
    {
         fRetCode = PSCFALSE;
    }

    return (fRetCode);
}

// Set maximal size of permissible receipt
void PscCsvSetMaxRecSize()
{
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;

    /*  pCmdBuffer->wMaxRecSize = RECBUFFSIZE;*/
    pCmdBuffer->wMaxRecSize = sizeof(pCmdBuffer->bRecBuff) / PSC_SIZEOF_BYTE;
}

// Enable buffer for command reception
void PscCsvEnableCmdReceipt()
{
    PSCWORD wDataSize;
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;

    wDataSize = sizeof(pCmdBuffer->bCmdBuff) / PSC_SIZEOF_BYTE;

    if (!pCmdBuffer->fPrestCmd && !pCmdBuffer->fRecEnabled)
    {
         /* enable input buffer for command reception without TimeOut */
         NetRecData((PSCBYTE *)&(pCmdBuffer->bCmdBuff[0]), wDataSize, NET_CMD, 0);

         pCmdBuffer->fRecEnabled = PSCTRUE;
    }
}

// Initializing of "Class" Kommando-Server ("Constructor")
void PscCsvInitCmdServer()
{
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;

    PSCTRACE("\n        PscCsvInitCmdServer... ");

    /* clear flag for "enable input buffer" */
    pCmdBuffer->fRecEnabled = PSCFALSE;

    /* clear flag for "internal command" */
    pCmdBuffer->fPrestCmd = PSCFALSE;

    /* enable inputbuffer for receiving commands */
    if (!PscSysGetStatus(PSCSTAT_NETERROR))
    {
         PscCsvEnableCmdReceipt();
    }

    /* set fill level of receiptbuffer to standard value */
    /* Receipt    =  ErrorCode  +  DataLen */
    pCmdBuffer->wUsedRecSize = (PSC_SIZEOF_BYTE + PSC_SIZEOF_WORD) / PSC_SIZEOF_BYTE;

    /* set maximal size of permissible receipt */
    pCmdBuffer->wMaxRecSize = 0;
}

// Deposit internal command in input buffer */
void PscCsvPresetCmd(tPscPSCmd *pPscPSCmd_p, PSCBYTE bMode_p)
{
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;
    PscMemAbsSetByte(&pCmdBuffer->bCmdBuff[0], pPscPSCmd_p->m_bCommand);
    if (pPscPSCmd_p->m_bCommand == kPscCmdSetVariableLong || pPscPSCmd_p->m_bCommand == kPscCmdForceVariableLong)
    {
         PscMemAbsSetWord(&pCmdBuffer->bCmdBuff[1], pPscPSCmd_p->m_wParamSize);
    }
    else
    {
         PscMemAbsSetByte(&pCmdBuffer->bCmdBuff[1], (PSCBYTE)pPscPSCmd_p->m_wParamSize);
    }
    /* set flag for internal command */
    pCmdBuffer->fPrestCmd = PSCTRUE;
    if (bMode_p == kPscDisableExtrnComm)
    {
         /* stop external communication till internal command was executed */
         /* (no acceptance of external data from IDE)  */
         PscSysSetStatus(PSCSTAT_EXT_COMM_LOCKED, PSCSET);
    }
}
