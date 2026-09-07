/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscForce.c
 *@author     jinyangh
 *@date       2025.02.17
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.02.17  1.0       jinyangh    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "pscForce.h"
#include "pscPlatform.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
tPscSetData      SetDataTable __attribute__((aligned(128), section(".set_tables"))); //设置功能管理器
tPscForceTable   ForceLst_l __attribute__((aligned(128), section(".force_table"))); //强制监视列表，和监视列表条目数一致
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// Init set table
void PscSetInitSetTab(void)
{
    SetDataTable.SetVariableFlag = PSCFALSE;
    SetDataTable.dwSetCom        = 0;
    memset(&SetDataTable.SetData, 0, sizeof(SetDataTable.SetData));
    PscForceInitForceTab();
}

// Set one variable
PSCBYTE PscSetSingleVariable(tPscPSCmd *pPscPSCmd_p)
{
    PSCWORD i;
    PSCBYTE *pPscSetData;
    PSCWORD m_Seg;  //低16位地址
    PSCWORD m_Offs; //高16位地址
    PSCBYTE bRetCode = kPscSuccess;

    PSCTRACE("\n-> PSC: PscSetSingleVariable... ");

    /* get set data in input buffer */
    pPscSetData = pPscPSCmd_p->m_pParamLst;

    /* valid program exists? */
    if (!PscSysGetStatus(PSCSTAT_PROGRAM_VALID))
    {
        bRetCode = kPscInvalidPgm;
        goto Exit;
    }

    SetDataTable.SetData.m_Type = PscMemAbsGetByte(pPscSetData + STRUCT_OFFSET_SETDATA_TYPE);
    SetDataTable.SetData.m_Pgm  = PscMemAbsGetWord(pPscSetData + STRUCT_OFFSET_SETDATA_PGMNR);
    m_Seg  = PscMemAbsGetWord(pPscSetData + STRUCT_OFFSET_SETDATA_SEGNR);
    m_Offs = PscMemAbsGetWord(pPscSetData + STRUCT_OFFSET_SETDATA_OFFSET);
    SetDataTable.SetData.m_Addr = ((PSCDWORD)m_Seg << 16) | ((PSCDWORD)m_Offs);
    SetDataTable.SetData.m_Size = PscMemAbsGetWord(pPscSetData + STRUCT_OFFSET_SETDATA_SIZE);
    SetDataTable.SetData.m_fMakePersistent = PscMemAbsGetByte(pPscSetData + STRUCT_OFFSET_SETDATA_MAKEPERSISTENT);

    for (i = 0; (i < SetDataTable.SetData.m_Size && i < PSC_SIZEOF_SETDATA); i++)
    {
        SetDataTable.SetData.m_Data[i] = PscMemAbsGetByte(pPscSetData + STRUCT_OFFSET_SETDATA_DATA + i);//将要设置的数据存入m_Data
    }

    SetDataTable.dwSetCom        = g_pscode_udp_info.pscode_src_port; //记录设置列表中设置数据的端口号，用于区分是哪个核的设置信息
    SetDataTable.SetVariableFlag = PSCTRUE;
    PscCacheWb(&SetDataTable, sizeof(SetDataTable));

Exit:

    return bRetCode;
}

// Set variable
PSCBYTE PscSetVariable()
{
    PSCWORD    wProgIndex;
    PSCDWORD   m_Addr;
    PSCWORD    wSize;                  /* number of bytes to set */
    uint32_t   port_id  = PORT_ID_BASE + PscGetCoreId();
    PSCBYTE    bRetCode = kPscSuccess;

    PscSetVariableStart();

    PscCacheInv(&setDataTablePtr->SetVariableFlag, 16); // 刷缓存，获取最新的标志位和端口号
    if ((setDataTablePtr->SetVariableFlag == PSCFALSE) || (setDataTablePtr->dwSetCom != port_id))
    {
        return kPscError;
    }

    PscCacheInv(setDataTablePtr, sizeof(*setDataTablePtr));
    wProgIndex = setDataTablePtr->SetData.m_Pgm; //程序编号
    if (C_CODE_PGM_NUM != wProgIndex)
    {
        bRetCode = kPscInvalidPgm;
        goto Exit;
    }

    m_Addr = setDataTablePtr->SetData.m_Addr;
    wSize  = setDataTablePtr->SetData.m_Size; //要设置的字节数

    switch (PscMemAbsGetByte(&setDataTablePtr->SetData.m_Type))
    {
    case kPscAccessByteByAdr:
    {
        if(1 == wSize)
        {
            *((PSCBYTE*)m_Addr) = *((PSCBYTE*)&setDataTablePtr->SetData.m_Data[0]);//向地址赋值
        }
        else if(2 == wSize)
        {
            *((PSCWORD*)m_Addr) = *((PSCWORD*)&setDataTablePtr->SetData.m_Data[0]);//向地址赋值
        }
        else if(4 == wSize)
        {
            *((PSCDWORD*)m_Addr) = *((PSCDWORD*)&setDataTablePtr->SetData.m_Data[0]);//向地址赋值
        }
        break;
    }
    default:
    {
        PSCTRACE(" %02X=Invalid AccessType !\n", (PSCWORD)setDataTablePtr->SetData.m_Type);
        bRetCode = kPscWatchTypeError;
        goto Exit;
    }
    }

    setDataTablePtr->SetVariableFlag = PSCFALSE;
    PscCacheWb(&setDataTablePtr->SetVariableFlag, sizeof(setDataTablePtr->SetVariableFlag));

    PscSetVariableEnd();
Exit:

    PSCTRACE("RetCode=%02X", (PSCWORD)bRetCode);

    return bRetCode;
}

// Init force table
void PscForceInitForceTab(void)
{
    PSCWORD i;
    PSCTRACE("\n        PscWatchInitWatchTab... ");

    ForceLst_l.dwForceCom           = 0;
    ForceLst_l.dwForceEntriesUsed   = 0;

    for (i = 0; i < PSCMAXWATCH; i++)
    {
        ForceLst_l.ForceLst[i].m_Type = kPscAccessUndef;
    }
}

// Force variable
PSCBYTE PscForce(void)
{
    PSCWORD i, tempForceEntriesUsed;
    uint32_t   port_id  = PORT_ID_BASE + PscGetCoreId();
    PSCTRACE("\n        PscForce... ");

    // 刷缓存，获取最新的端口号和已使用条目数
    PscCacheInv(&ForceLst_l.dwForceCom, 16);
    if ((ForceLst_l.dwForceEntriesUsed <= 0) || (ForceLst_l.dwForceCom != port_id))
    {
        return kPscError;
    }

    PscCacheInv(&ForceLst_l, sizeof(ForceLst_l));

   /* is there a valid program  ? */
   if (!PscSysGetStatus(PSCSTAT_PROGRAM_VALID))
   {
       return PSCFALSE;
   }

   tempForceEntriesUsed = ForceLst_l.dwForceEntriesUsed;

   for (i = 0; i < PSCMAXWATCH; i++)
   {
       if (ForceLst_l.ForceLst[i].m_Type != kPscAccessUndef)
       {
           PscForceVariable((tPscForceItem *)&ForceLst_l.ForceLst[i]);

           tempForceEntriesUsed--;//tempForceEntriesUsed表示还有多少个需要处理的强制监视项，避免一直找，浪费时间
           if (tempForceEntriesUsed == 0)
           {
               break;
           }
       }
   }

    return PSCTRUE;
}

// Set one force variable
PSCBYTE PscForceVariable(tPscForceItem *pPscSetData)
{
    PSCDWORD   Addr;
    PSCWORD    Size;                  /* Number of bytes to be set */
    PSCBYTE    bRetCode = 0 ;         /* ErrorCode of function */
    PSCWORD    wProgIndex;

    wProgIndex = pPscSetData->m_Pgm;
    if (C_CODE_PGM_NUM != wProgIndex)
    {
        bRetCode = kPscInvalidPgm;
        goto Exit;
    }

    Addr = pPscSetData->m_Addr;
    Size = pPscSetData->m_Size;

    switch (pPscSetData->m_Type)
    {
    case kPscAccessByteByAdr:
    {
        if(1 == Size)
        {
            *((PSCBYTE*)Addr) = *((PSCBYTE*)&pPscSetData->m_Data[0]);//向地址赋值
        }
        else if(2 == Size)
        {
            *((PSCWORD*)Addr) = *((PSCWORD*)&pPscSetData->m_Data[0]);//向地址赋值
        }
        else if(4 == Size)
        {
            *((PSCDWORD*)Addr) = *((PSCDWORD*)&pPscSetData->m_Data[0]);//向地址赋值
        }
        break;
    }
    default:
        PSCTRACE(" %02X=Invalid AccessType !\n",(PSCWORD)pPscSetData->m_Type);
        bRetCode = kPscForceTypeError;
        goto Exit;
    }

Exit:

    PSCTRACE("RetCode=%02X", (PSCWORD)bRetCode);

    return (bRetCode);
}

// Record force entry
PSCBYTE PscForceAddItem(tPscPSCmd *pPscPSCmd_p)
{
    PSCBYTE            RecBuff[FORCEID_SIZE]; /* return message to IDE */
    tPscForceItem     *pForceItem;            /* struct with information about watch-job */
    PSCWORD            wForceId;              /* ID of the watch-job */
    PSCWORD            m_Seg;                 //段编号
    PSCWORD            m_Offs;                //段内偏移
    PSCBYTE            bRetCode;              /* error code of the function */

    PSCTRACE("\n-> PSC: PscForceAddItem... ");

    /* evaluate data in input buffer with struct */
    pForceItem = (tPscForceItem *)pPscPSCmd_p->m_pParamLst;

    /* search for free entry in force table */
    for (wForceId = 0; wForceId < PSCMAXWATCH; wForceId++)
    {
        if (ForceLst_l.ForceLst[wForceId].m_Type == kPscAccessUndef)
        {
            break;
        }
    }

    /* fill entry */
    if (wForceId < PSCMAXWATCH)
    {
        /* "import" all values from intel-format *///将强制变量的地址、字节数等信息填入ForceTable
        ForceLst_l.ForceLst[wForceId].m_Type = PscMemAbsGetByte(((PSCBYTE *)pForceItem) + STRUCT_OFFSET_SETDATA_TYPE);
        ForceLst_l.ForceLst[wForceId].m_Pgm  = PscMemAbsGetWord(((PSCBYTE *)pForceItem) + STRUCT_OFFSET_SETDATA_PGMNR);
        m_Seg  = PscMemAbsGetWord(((PSCBYTE *)pForceItem) + STRUCT_OFFSET_SETDATA_SEGNR);
        m_Offs = PscMemAbsGetWord(((PSCBYTE *)pForceItem) + STRUCT_OFFSET_SETDATA_OFFSET);
        ForceLst_l.ForceLst[wForceId].m_Addr = ((PSCDWORD)m_Seg << 16) | ((PSCDWORD)m_Offs);
        ForceLst_l.ForceLst[wForceId].m_Size = PscMemAbsGetByte(((PSCBYTE *)pForceItem) + STRUCT_OFFSET_SETDATA_SIZE);

        //将变量要设置的目标值存入ForceTable的m_Data
        memcpy((PSCBYTE *)&(ForceLst_l.ForceLst[wForceId].m_Data),((PSCBYTE *)pForceItem)+STRUCT_OFFSET_SETDATA_DATA,
               ForceLst_l.ForceLst[wForceId].m_Size);

        /* deposit force ID (Intel-Format) in receipt buffer */
        PscMemAbsSetByte((PSCBYTE *)&RecBuff[0], kPscWatchID);
        PscMemAbsSetWord((PSCBYTE *)&RecBuff[1], wForceId);//向返回数组填入该强制变量使用的ForceTable的索引

        if (PscCsvSetExtRetCode((PSCBYTE *)&RecBuff[0], sizeof(RecBuff) / PSC_SIZEOF_BYTE))
        {
            /* force ID successfully stored in receipt buffer */
            bRetCode = kPscSuccess;
            ForceLst_l.dwForceEntriesUsed++;
            PSCTRACE("\nPSCFORCE ADD");
        }
        else
        {
            /* receipt buffer too small */
            ForceLst_l.ForceLst[wForceId].m_Type = kPscAccessUndef;
            bRetCode = kPscNetRecSizeError;
        }
    }
    else
    {
        /* no force ID available */
        bRetCode = kPscNoWatchTabEntry;
    }

    ForceLst_l.dwForceCom = g_pscode_udp_info.pscode_src_port; //记录强制列表中强制数据的端口号，用于区分是哪个核的强制信息
    PscCacheWb(&ForceLst_l, sizeof(ForceLst_l));

    PSCTRACE("RetCode=%02X [WatchID=%04X]", (PSCWORD)bRetCode, wForceId);

    return (bRetCode);
}

// Delete force item
PSCBYTE PscForceDeleteItem(tPscPSCmd *pPscPSCmd_p)
{
    PSCBYTE  RecBuff[FORCEID_SIZE];     /* Returnmassage to IDE */
    tPscUnForceItem *pForceItem;        /* Struct with information about watch-job */
    PSCWORD  wForceId = 0;
    PSCWORD  wPgm, wSeg, wOffs;
    PSCDWORD Addr;
    PSCBYTE  bRetCode;
    PSCLONG  nIndex;

    PSCTRACE("\n-> PSC: PscForceDeleteItem... ");

    /* evaluate data in input buffer with struct */
    pForceItem = (tPscUnForceItem *)pPscPSCmd_p->m_pParamLst;

    wPgm  = PscMemAbsGetWord(((PSCBYTE *)pForceItem) + STRUCT_OFFSET_SETDATA_PGMNR);
    wSeg  = PscMemAbsGetWord(((PSCBYTE *)pForceItem) + STRUCT_OFFSET_SETDATA_SEGNR);
    wOffs = PscMemAbsGetWord(((PSCBYTE *)pForceItem) + STRUCT_OFFSET_SETDATA_OFFSET);
    Addr  = ((PSCDWORD)wSeg << 16) | ((PSCDWORD)wOffs);

    /* search for the entry in force table - in reverse order,
     * so that in case of multiple entries for the same address (in C code mode),
     * older entries, if any, become effective */
    for (nIndex = (PSCMAXWATCH - 1); nIndex >= 0; nIndex--)
    {
        if (ForceLst_l.ForceLst[nIndex].m_Type != kPscAccessUndef)
        {
            if ((ForceLst_l.ForceLst[nIndex].m_Pgm  == wPgm)
            && (ForceLst_l.ForceLst[nIndex].m_Addr  == Addr))
            {
                ForceLst_l.ForceLst[nIndex].m_Type = kPscAccessUndef;
                ForceLst_l.dwForceEntriesUsed--;
                wForceId = nIndex;
                PSCTRACE("\nPSCFORCE DEL [ForceID=%04X]",wForceId);
                break;
            }
        }
    }

    /* store force ID (Intel-Format) in receipt buffer */
    PscMemAbsSetByte((PSCBYTE *)&RecBuff[0], kPscWatchID);
    PscMemAbsSetWord((PSCBYTE *)&RecBuff[1], wForceId);//向返回数组填入该强制变量删除的ForceTable的索引

    if (PscCsvSetExtRetCode((PSCBYTE *)&RecBuff[0], sizeof(RecBuff) / PSC_SIZEOF_BYTE))
    {
        /* force ID stored successful in receipt buffer */
        bRetCode = kPscSuccess;
    }
    else
    {
        /* receipt buffer too small*/
        bRetCode = kPscNetRecSizeError;
    }

    ForceLst_l.dwForceCom = g_pscode_udp_info.pscode_src_port; //记录强制列表中强制数据的端口号，用于区分是哪个核的强制信息
    PscCacheWb(&ForceLst_l, sizeof(ForceLst_l));

    PSCTRACE("RetCode=%02X", (PSCWORD)bRetCode);

    return (bRetCode);
}
