/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscState.c
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
#include "pscState.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
PSCWORD  wPscSysFlags_l = 0;    //系统标志位
PSCDWORD PscSysStatus_l = 0;    //记录系统状态的中间变量

PSCDWORD dwPSCCapabilities[10]; //登录命令回复数组

PSCBYTE bLastPlcErr_l      = kPscSuccess;  /* Marker for last PLC exec error */
PSCBOOL RecordErrFlag      = PSCFALSE;
PSCBOOL InfErrFlag         = PSCFALSE;
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// Request one status bit
PSCBOOL PscSysGetStatus(PSCWORD StatusFlag_p)
{
    if (wPscSysFlags_l & StatusFlag_p)
    {
        return PSCTRUE;
    }

    return PSCFALSE;
}

// Set one status bit
void PscSysSetStatus(PSCWORD StatusFlag_p, PSCBYTE SetClr_p)
{
    if (SetClr_p == PSCSET)
    {
        wPscSysFlags_l |= StatusFlag_p;
    }
    else
    {
        StatusFlag_p   ^= 0xFFFF; /* OR-Mask XOR 0xFF = AND-Mask */
        wPscSysFlags_l &= StatusFlag_p;
    }
}

// Update all presently allowed/supported features
void PscSysUpdateMode()
{
    PSCDWORD PscSysStatus;

    /* take over flag register to bit positions 0..15 of system status */
    /* bit positions 16..31 of system status must be determined in this function */
    PscSysStatus = (PSCDWORD)wPscSysFlags_l;

    if (wPscSysFlags_l & PSCSTAT_NETERROR)
    {
        /* net error -> unset PSCSTAT_OK and do nothing else */
        PscSysStatus &= ~((PSCDWORD)PSCSTAT_OK);
        goto SetStat;
    }

    /* never accept download in C code generator mode */
    PscSysStatus &= ~((PSCDWORD)PSCSTAT_ACCPT_DOWNLOAD);

    if (wPscSysFlags_l & PSCSTAT_PROGRAM_VALID && /* -hle- CRQ2007/0006: enable start button not before completion of dwl */ !(wPscSysFlags_l & PSCSTAT_DOWNLOADING))
    {
        PscSysStatus |= PSCSTAT_ACCPT_COLD_START | PSCSTAT_ACCPT_WARM_START | PSCSTAT_ACCPT_HOT_START;
    }

    if (wPscSysFlags_l & PSCSTAT_RUNNING)
    {
        PscSysStatus |= PSCSTAT_ACCPT_STOP_CMD;
        PscSysStatus &= ~((PSCDWORD)(PSCSTAT_ACCPT_COLD_START | PSCSTAT_ACCPT_WARM_START | PSCSTAT_ACCPT_HOT_START | PSCSTAT_ACCPT_DOWNLOAD));
    }

    if (wPscSysFlags_l & PSCSTAT_RUN_EVENT_TASK)
    {
        PscSysStatus &= ~( (PSCDWORD)(PSCSTAT_ACCPT_STOP_CMD   |
                                     PSCSTAT_ACCPT_COLD_START |
                                     PSCSTAT_ACCPT_WARM_START |
                                     PSCSTAT_ACCPT_HOT_START  |
                                     PSCSTAT_ACCPT_DOWNLOAD)  );
    }

    if (wPscSysFlags_l & PSCSTAT_DOWNLOADING)
    {
        PscSysStatus |= PSCSTAT_ACCPT_DOWNLOAD;
        if (wPscSysFlags_l & PSCSTAT_SEGTAB_CREATED)
        {
            PscSysStatus |= PSCSTAT_ACCPT_SEGMENT;
        }
        goto SetStat;
    }

    /* clear flag "download active" */
    PscSysStatus &= ~((PSCDWORD)PSCSTAT_DOWNLOADING);

SetStat:

    if ( !(wPscSysFlags_l & PSCSTAT_LOGIN) )            /* no connection to TUI -> remove all flags except PSCSTAT_OK */
    {
        PscSysStatus &= PSCSTAT_OK;
    }

    /* status changed? */
    if (PscSysStatus_l != PscSysStatus)//状态是否有改变
    {
        PscSysStatus_l = PscSysStatus;
    }
}

// initialization dwPSCCapabilities
void PscCapabilitiesInit()
{
    dwPSCCapabilities[0]=0x00000001; /* 1 = capabilities available */
    dwPSCCapabilities[1]=0x00000000;
    dwPSCCapabilities[2]=0x00000000;
    dwPSCCapabilities[3]=0x00000000;
    dwPSCCapabilities[4]=0x00000000;
    dwPSCCapabilities[5]=0x00000000;
    dwPSCCapabilities[6]=0x00000000;
    dwPSCCapabilities[7]=0x00000000;
    dwPSCCapabilities[8]=0x00000000;
    dwPSCCapabilities[9]=0x00000000;
}

////TODO:hjy [8]-[9]没有分配动态内存,暂时写死
void PscInfoEvalCapabilities()
{
    dwPSCCapabilities[0]  = 0xc410002f;
    dwPSCCapabilities[1]  = 0x00000729;
    dwPSCCapabilities[2] |= (PSCBYTE) PSC_SIZEOF_SETDATA;//（强制）设置变量功能的数据最大字节数
    dwPSCCapabilities[3]  = 0x00000000;
    dwPSCCapabilities[4]  = 0x00000000;
    dwPSCCapabilities[5]  = 0x00000000;
    dwPSCCapabilities[6]  = (PSCDWORD) PSCMAXWATCH;//监视表最大条目数
    dwPSCCapabilities[7]  = pWatchTables_g.dwWatchEntriesUsed;//已使用的条目数
    dwPSCCapabilities[8]  = 0x00100000;
    dwPSCCapabilities[9]  = 0x00000830;
}

// Sending status changes
PSCWORD PscSysCallback(PSCBYTE *pBuff_p, PSCWORD wBuffSize_p)
{
    PSCWORD wSize;

    PSCTRACE("\n        [Append Status] ");

    wSize = PSC_SIZEOF_BYTE + PSC_SIZEOF_DWORD;

    /* check remaining buffer size against needed size */
    if (wSize > wBuffSize_p)
    {
        /* communication buffer is too small */
        return PSCINVALIDVALUE;
    }

    PscMemAbsSetByte(pBuff_p++, kPscStateChg);

    PscMemAbsSetDword(pBuff_p, PscSysStatus_l);

    return wSize;
}

// Sending extended capabilities
PSCWORD PscInfExtCapCallback(PSCBYTE *pBuff_p, PSCWORD wBuffSize_p)
{
    PSCWORD wExtCapSize;
    PSCINT i;

    wExtCapSize = (PSC_SIZEOF_BYTE + 10 * PSC_SIZEOF_DWORD);

    /* check remaining buffer size against needed size */
    if (wExtCapSize > wBuffSize_p)
    {
        /* communication buffer is too small */
        return PSCINVALIDVALUE;
    }

    PscMemAbsSetByte(pBuff_p++, kPscExtCap);
    for (i = 0; i < 10; i++)
    {
        PscMemAbsSetDword(pBuff_p + i * PSC_SIZEOF_DWORD, dwPSCCapabilities[i]);
    }

    return wExtCapSize;
}

// Get resource version
PSCBOOL PscGetResVersion(tPscResVersion *pResVersion)
{
    memcpy((void*)pResVersion, (void*)&resVersion_g, sizeof(tPscResVersion));

    return PSCTRUE;
}

// Only record error code
PSCBOOL PscInfRecordError(PSCBYTE bErrCode_p)
{
    if (((bLastPlcErr_l == kPscSuccess) || (bLastPlcErr_l == kPscNetError)) && (bErrCode_p != kPscSuccess))
    {
        /* Special treatment for ErrorCode <kPscInvalidPgm> necessary, */
        /* see remark "Handling Errorcode" above */
        if (bErrCode_p == kPscInvalidPgm)
        {
            if (!PscSysGetStatus(PSCSTAT_PROGRAM_VALID))
            {
                return PSCFALSE;
            }
        }

        bLastPlcErr_l = bErrCode_p;

        /* signal occured error status */
        PscSysUpdateMode();

        RecordErrFlag = PSCTRUE;

        return PSCTRUE;
    }

    return PSCFALSE;
}

// Registration of the last error occurred
PSCBOOL PscInfRegisterError(PSCBYTE bErrCode_p)
{
    PSCWORD wBuffSize;
    PSCWORD useSize;
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;

    if (((bLastPlcErr_l == kPscSuccess) || (bLastPlcErr_l == kPscNetError)) && (bErrCode_p != kPscSuccess))
    {
        /* Special treatment for ErrorCode <kPscInvalidPgm> necessary, */
        /* see remark "Handling Errorcode" above */
        if (bErrCode_p == kPscInvalidPgm)
        {
            if (!PscSysGetStatus(PSCSTAT_PROGRAM_VALID))
            {
                return PSCFALSE;
            }
        }

        bLastPlcErr_l = bErrCode_p;

        /* signal occured error status */
        PscSysUpdateMode();

        /* register send request for error message */
        wBuffSize = pCmdBuffer->wMaxRecSize - pCmdBuffer->wUsedRecSize;
        useSize = PscInfErrCallback((PSCBYTE *)&(pCmdBuffer->bRecBuff[pCmdBuffer->wUsedRecSize]), wBuffSize);
        pCmdBuffer->wUsedRecSize += useSize;

        return PSCTRUE;
    }

    if(RecordErrFlag || InfErrFlag)
    {
        /* register send request for error message */
        wBuffSize = pCmdBuffer->wMaxRecSize - pCmdBuffer->wUsedRecSize;
        useSize = PscInfErrCallback((PSCBYTE *)&(pCmdBuffer->bRecBuff[pCmdBuffer->wUsedRecSize]), wBuffSize);
        pCmdBuffer->wUsedRecSize += useSize;

        return PSCTRUE;
    }

    return PSCFALSE;
}

// Signal an error event to programming system
PSCWORD PscInfErrCallback(PSCBYTE *pBuff_p, PSCWORD wBuffSize_p)
{
    PSCWORD wSize = 0;

    if(bLastPlcErr_l != kPscSuccess)
    {
        PSCTRACE("\n        [Append hint to PLC/Runtime Error] ");

        /* only send error event, the programming system will get the */
        /* error reason and error number/message later by explicit calling */
        /* of <PscInfGetError> */
        PscMemAbsSetByte(pBuff_p, kPscError);
        wSize = sizeof(PSCBYTE);

        /* check remaining buffer size against needed size */
        if (wSize > wBuffSize_p)
        {
            /* communication buffer is too small */
            return PSCINVALIDVALUE;
        }

        /* signal error again until the communication partner reads */
        /* the error reason and error number/message later by calling */
        /* <PscInfGetError> */
        InfErrFlag = PSCTRUE;
    }
    else
    {
        /* The programming system already has evaluated the error by calling */
        /* <PscInfGetError>, the error is no longer existing */
        wSize = 0;
    }

    return wSize;
}
