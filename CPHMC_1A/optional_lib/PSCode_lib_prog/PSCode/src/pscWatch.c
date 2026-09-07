/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscWatch.c
 *@author     jinyangh
 *@date       2025.01.20
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.20  1.0       jinyangh    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "pscWatch.h"
#include "pscPlatform.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
tPscWatchTable pWatchTables_g __attribute__((aligned(128), section(".watch_tables"))); //监视列表管理器
PSCDWORD watchData_g[PSCMAXWATCH] __attribute__((aligned(128), section(".watch_data"))); //监视数据，根据WatchId绑定数组的数据

PSCBYTE *pBuff_l    = PSCNULL;   /* ptr to send buffer */
PSCWORD wBuffSize_l = 0;         /* size of send buffer */
PSCWORD wDataSize_l = 0;         /* length of written data */
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// Initialize "class" WatchVariables ("Constructor")
void PscWatchInitWatchTab()
{
    PSCWORD i;
    tPscWatchTable* pWatchTable = (tPscWatchTable*)&(pWatchTables_g);

    PSCTRACE("\n        PscWatchInitWatchTab... ");

    for (i=0; i<PSCMAXWATCH; i++)
    {
        pWatchTable->WatchLst[i].m_Type = kPscAccessUndef;
    }

    pWatchTable->wCurrWatchId        = 0;
    pWatchTable->dwWatchEntriesUsed  = 0;
    pWatchTable->fWatchEnable        = PSCFALSE;
    pWatchTable->fDataAvailable      = PSCFALSE;
    pWatchTable->bWatchMode          = kPscWatchSingle;//单次监视模式
    pWatchTable->fDataRequestsLocked = PSCFALSE;
    pWatchTable->dwWatchCom          = 0;
    PscCacheWb(&pWatchTable->fWatchEnable, sizeof(pWatchTable->fWatchEnable));

    PscWaveInit();
}

// Mark data as valid
void PscWatchMarkDataValid()
{
    tPscWatchTable* pWatchTable = (tPscWatchTable*) &(pWatchTables_g);

    if (pWatchTable->fWatchEnable)
    {
        pWatchTable->fDataAvailable = PSCTRUE;
    }
}

// Cancel all watch orders
PSCBYTE PscWatchDiscardWatchTab()
{
    PSCWORD i;
    tPscWatchTable* pWatchTable = (tPscWatchTable*)&(pWatchTables_g);

    PSCTRACE("\n-> PSC: PscWatchDiscardWatchTab");

    for (i = 0; i < PSCMAXWATCH; i++)
    {
        pWatchTable->WatchLst[i].m_Type = kPscAccessUndef;
    }

    pWatchTable->wCurrWatchId       = 0;
    pWatchTable->dwWatchEntriesUsed = 0;
    pWatchTable->fWatchEnable       = PSCFALSE;
    pWatchTable->fDataAvailable     = PSCFALSE;
    pWatchTable->dwWatchCom         = 0;
    PscCacheWb(&pWatchTable->fWatchEnable, sizeof(pWatchTable->fWatchEnable));

    PscWaveDiscard();

    return (kPscSuccess);
}

// Add multiple watch items
PSCBYTE PscWatchAddMulInstruction(tPscPSCmd *pPscPSCmd_p)
{
    PSCWORD  wSizeOfMulWatch;                //要添加的监视条目大小（全部字节数）
    PSCBYTE  bRetCode = kPscSuccess;
    PSCBYTE *pMulWatchInstr = &gDataBuff[0]; //指向监视条目数据要存放的buffer

    wSizeOfMulWatch = PscMemAbsGetWord(pPscPSCmd_p->m_pParamLst);//获取要添加的监视条目大小

    if (0 == wSizeOfMulWatch)
    {
        goto Exit;
    }

    PSCTRACE("Size of MulWatch: %u", wSizeOfMulWatch);

    //改变从socket端口接收数据后存放的位置：之前从socket接收到数据后是存放在CmdBuffer.bCmdBuff，现在改为存放在pMulWatchInstr中
    NetRecData((PSCBYTE *)pMulWatchInstr, wSizeOfMulWatch, NET_DATA, NET_RxTIMEOUT);

    //存放内部命令，包括参数
    pPscPSCmd_p->m_bCommand = kPscCmdAddMulWatchInstrCont;//修改命令为：获取多个监视数据

    PscCsvPresetCmd(pPscPSCmd_p, kPscEnableExtrnComm);

    bRetCode = kPscSuccess;

Exit:

    return bRetCode;
}

// Get multiple watch items
PSCBYTE PscWatchAddMulInstructionCont(tPscPSCmd *pPscPSCmd_p)
{
    PSCBYTE bNetStat;
    PSCBYTE bRetCode;
    PSCBYTE *pWorkPtr;
    PSCWORD wCountOfEntriesAdded = 0;
    PSCWORD wWatchId; /* ID of watch job */

    static PSCBYTE RecBuff[WATCHIDLIST_SIZE + PSC_SIZEOF_BYTE]; /* return message for IDE (includes one extra byte for ext. return code) */

    PSCTRACE("\n-> PSC: PscDwlSingleSegmentCont... ");
    tPscWatchTable *pWatchTable = (tPscWatchTable*)&(pWatchTables_g);
    tPscDataAdr *pWatchList = (tPscDataAdr*)&(pWatchTable->WatchLst);//监视列表
    PSCBYTE *pMulWatchInstr = &gDataBuff[0];                         //指向监视条目数据要存放的buffer

    bRetCode = kPscSuccess;

    /* connection to progr. system */
    bNetStat = NetGetRxStatus();//从socket端口接收监视数据，但此时的数据是存放在PscWatchAddMulInstruction命令中为监视条目分配的内存中

    if (bNetStat == NET_SUCCESS)
    {
        /* ================= Net success ==================== */
        /* receipt received segment */
        pPscPSCmd_p->m_fSendRec = PSCTRUE;
    }
    else
    {
        /* =============== Net no success =================== */
        /* don't receipt internal command */
        pPscPSCmd_p->m_fSendRec = PSCFALSE;

        /* no data available */
        if (bNetStat == NET_NODATA)
        {
            /* no data received and also no timeout */
            /* so wait */
            PscCsvPresetCmd (pPscPSCmd_p, kPscEnableExtrnComm);//重新把内部命令存入接收命令缓冲区，并置位内部命令标志
            return kPscSuccess;
        }
        else
        {
            /* TimeOut or network error */
            PSCTRACE(" (*** TimeOut/Network Error ***) ");

            PscRestartNetAndSetNetError(NET_ERROR);
            bRetCode = kPscNetError;
            return bRetCode;
        }
    }

    pWorkPtr = pMulWatchInstr;

    PscMemAbsSetByte((PSCBYTE *)&RecBuff[0], kPscWatchIDList);

    /* Search for unused slot in watch table... */
    for (wWatchId = 0; wWatchId < PSCMAXWATCH; wWatchId++)
    {
        if (pWatchList[wWatchId].m_Type == kPscAccessUndef)
        {
            /* "import" all values from intel format */
            if ( (pMulWatchInstr + (PSCWORD)PscMemAbsGetWord(pPscPSCmd_p->m_pParamLst)) > pWorkPtr )
            {
                pWatchList[wWatchId].m_Type  = PscMemAbsGetByte(pWorkPtr);//获取每条监视项的类型
                pWorkPtr += PSC_SIZEOF_BYTE;

                pWatchList[wWatchId].m_Pgm   = PscMemAbsGetWord(pWorkPtr); //获取每条监视项的程序编号
                pWorkPtr += PSC_SIZEOF_WORD;

                pWatchList[wWatchId].m_Addr  = ((PSCDWORD)PscMemAbsGetWord(pWorkPtr)) << 16; //获取每条监视项的段编号
                pWorkPtr += PSC_SIZEOF_WORD;

                pWatchList[wWatchId].m_Addr |= (PSCDWORD)PscMemAbsGetWord(pWorkPtr);//获取每条监视项的段内偏移量
                pWorkPtr += PSC_SIZEOF_WORD;

                pWatchList[wWatchId].m_Size  = PscMemAbsGetWord(pWorkPtr);//获取每条监视项的字节数
                pWorkPtr += PSC_SIZEOF_WORD;

                PscMemAbsSetWord((PSCBYTE *)&RecBuff[PSC_SIZEOF_BYTE + PSC_SIZEOF_WORD + wCountOfEntriesAdded * PSC_SIZEOF_WORD], wWatchId);//写入两个字节的监视条目的索引

                wCountOfEntriesAdded++;
                pWatchTable->dwWatchEntriesUsed++;

                if (pWatchList[wWatchId].m_Type == PSC_WAVE_TYPE)
                {
                    // Add to the wave list
                    PscAddWaveList(&pWatchList[wWatchId]);
                }
            }
        }
    }
    pWatchTable->dwWatchCom = g_pscode_udp_info.pscode_src_port; //记录监视列表中监视数据的端口号，用于区分是哪个核的监视信息

    PscMemAbsSetWord((PSCBYTE *)&RecBuff[PSC_SIZEOF_BYTE], wCountOfEntriesAdded);//此次占了多少个监视列表条目

    /* store Watch-ID in Intel-Format in receipt buffer */
    //wCountOfEntriesAdded*2是因为RecBuff会记录watchID，每个watchID占2个字节
    if (PscCsvSetExtRetCode((PSCBYTE *)&RecBuff[0], (PSCWORD)((( (PSC_SIZEOF_WORD/PSC_SIZEOF_BYTE)* wCountOfEntriesAdded ) + PSC_SIZEOF_WORD + PSC_SIZEOF_BYTE))))
    {
        /* Watch-ID successfully stored in receipt buffer */
        bRetCode = kPscSuccess;
    }
    else
    {
        /* receipt buffer too small */
        bRetCode = kPscNetRecSizeError;
    }

    return (bRetCode);
}

// Enable watch jobs
PSCBYTE PscWatchEnableWatching(tPscPSCmd *pPscPSCmd_p)
{
    tPscWatchTable* pWatchTable = (tPscWatchTable*)&(pWatchTables_g);

    PSCTRACE("\n-> PSC: PscWatchEnableWatching ");

    pWatchTable->bWatchMode   = PscMemAbsGetByte(pPscPSCmd_p->m_pParamLst);//监视模式（单次/全部）
    pWatchTable->fWatchEnable = PSCTRUE;
    pWatchTable->wCurrWatchId = 0;
    PscCacheWb(pWatchTable, sizeof(*pWatchTable));

    PscWaveEnable();

    return (kPscSuccess);
}

// Store watch data in send buffer
PSCBOOL PscWatchSendData(tPscWatchTable *pWatchTable_p)
{
    PSCWORD wWatchId;
    PSCBOOL fRetCode;

    PSCTRACE("\n-> PSC: PscWatchSendData ");
    fRetCode = PSCTRUE;

    if (pWatchTable_p->fWatchEnable)
    {
        switch (pWatchTable_p->bWatchMode)
        {
        case kPscWatchSingle:
            /* watch a single variable */
            PSCTRACE("[Single] ");

            /* execute current watch job */
            fRetCode = PscWatchExecInstruction(pWatchTable_p, pWatchTable_p->wCurrWatchId);

            /* determine index for next watch job */
            for (wWatchId = 0; wWatchId < PSCMAXWATCH; wWatchId++)
            {
                pWatchTable_p->wCurrWatchId++;

                if (pWatchTable_p->wCurrWatchId >= PSCMAXWATCH)
                {
                    pWatchTable_p->wCurrWatchId = 0;
                }

                /* stop search at used watch job */
                if (pWatchTable_p->WatchLst[pWatchTable_p->wCurrWatchId].m_Type != kPscAccessUndef)
                {
                    break;
                }
            }

            /* if no new job, disable watching */
            if (wWatchId >= PSCMAXWATCH)
            {
                pWatchTable_p->fWatchEnable = PSCFALSE;
                PscCacheWb(&pWatchTable_p->fWatchEnable, sizeof(pWatchTable_p->fWatchEnable));
            }
            break;

        case kPscWatchAll:
            /* watch all variables */
            PSCTRACE("[All] ");

            /* By starting with the execution of watch jobs at */
            /* <wCurrWatchId_l>, watch data, that could not be stored completely  */
            /* in the send buffer,could be partioned over different cycles  */
            if (pWatchTable_p->wCurrWatchId >= PSCMAXWATCH)
            {
                pWatchTable_p->wCurrWatchId = 0;//重头开始
            }

            for (wWatchId = pWatchTable_p->wCurrWatchId; wWatchId < PSCMAXWATCH; wWatchId++)
            {
                if (pWatchTable_p->WatchLst[wWatchId].m_Type != kPscAccessUndef)//从即将要处理的监视条目ID开始，向后搜索全部已使用的监视条目
                {
                    fRetCode = PscWatchExecInstruction(pWatchTable_p, wWatchId);//执行单个监视条目

                    if (!fRetCode) /* send buffer full */
                    {
                        break;
                    }
                }
            }
            pWatchTable_p->wCurrWatchId = wWatchId;//记录执行到哪一个监视条目的ID（有可能全部执行完，wWatchId=PSCMAXWATCH；有可能buffer已满，记录的是下一次要执行的监视条目ID；）
            break;

        default:
            /* invalid watch mode */
            pWatchTable_p->fWatchEnable = PSCFALSE;
            PscCacheWb(&pWatchTable_p->fWatchEnable, sizeof(pWatchTable_p->fWatchEnable));
            break;
        }  /* end SWITCH */
    } /* end IF */

    return fRetCode;
}

// Execute single watch job
PSCBOOL PscWatchExecInstruction(tPscWatchTable *pWatchTable_p, PSCWORD wWatchId_p)
{
    PSCWORD WatchSize;             //字节数
    PSCBYTE wAdminSize;            //用于管理数据的字节数
    PSCWORD CorrectedWatchSize;    //上位机想要的返回数据的字节数
    PSCBOOL fRetCode;
    PSCBYTE bDataByte;
    PSCBYTE Data[4] = {0};//目前上位机最大也就4byte类型变量，并且以4byte对齐，所以读写内存的原子操作可以直接以4byte操作

    PSCTRACE("[Exec WatchId %04X] ", wWatchId_p);

    PscCacheInv(watchData_g, sizeof(watchData_g));

    WatchSize = pWatchTable_p->WatchLst[wWatchId_p].m_Size;
    if (WatchSize == 0)
    {
        WatchSize = PSC_SIZEOF_BYTE / PSC_SIZEOF_BYTE;//最少都是1byte
    }

    CorrectedWatchSize = WatchSize;

    wAdminSize = (PSC_SIZEOF_BYTE+PSC_SIZEOF_WORD+PSC_SIZEOF_WORD) / PSC_SIZEOF_BYTE;// 5 = 标识符 + 监视条目ID + ？

    //返回给上位机的一条监视条目的长度 = 用于管理数据的字节数 + 上位机对应单个监视条目下发的字节数
    if (wBuffSize_l >= (CorrectedWatchSize + wAdminSize))//如果buffer可用空间大于等于“返回给上位机的一条监视条目的长度”
    {
        /* enough send buffer to store watch data */
        //管理数据，5byte
        PscMemAbsSetByte(pBuff_l+0, kPscWatchData);  //返回的监视数据
        PscMemAbsSetWord(pBuff_l+1, (PSCWORD)(CorrectedWatchSize + PSC_SIZEOF_WORD / PSC_SIZEOF_BYTE));
        PscMemAbsSetWord(pBuff_l+3, wWatchId_p);    //监视条目ID

        pBuff_l += wAdminSize;

        //取出监视条目的值放入buffer，返回值是取出的数据长度
        switch ((tPscAccessType) pWatchTable_p->WatchLst[wWatchId_p].m_Type)
        {
        case kPscAccessByteByAdr:
        case kPscAccessByteByWave: // 录波的前提：监视
        {
            if ((WatchSize != 1) && (WatchSize != 2) && (WatchSize != 4))
            {
                for (uint16_t i = 0; i < WatchSize; i++)
                {
                    PscMemAbsSetByte(pBuff_l++, 0x00);
                }
                break;
            }
			
            memcpy(Data, &watchData_g[wWatchId_p], WatchSize);
            for (uint16_t i = 0; i < WatchSize; i++)
            {
                bDataByte = Data[i];//根据段号和偏移量获取数据
                PscMemAbsSetByte(pBuff_l++, bDataByte);
            }
            break;
        }
        default:
            break;
        }

        /* Update values for used length and remaining size of buffer */
        wDataSize_l += CorrectedWatchSize + wAdminSize;
        wBuffSize_l -= CorrectedWatchSize + wAdminSize;

        fRetCode = PSCTRUE;
    }
    else
    {
        /* send buffer to small to take up watch data */
        fRetCode = PSCFALSE;
    }

    return fRetCode;
}

// Delete watch item
PSCBYTE PscWatchDelInstruction()
{
    PSCBYTE bRetCode = kPscSuccess;

    PscWatchDiscardWatchTab();

    return bRetCode;
}

/* 让管理核代理其他核与上位机通信后，监视功能的优化方案：
 * 1.将监视列表放入共享DDR区域
 * 2.管理核将从上位机收到的要监视的数据信息填入监视列表
 * 3.其他核每次循环去读监视列表的enable标志位，如果enable有效，则说明监视列表有新数据（或者说明上位机此时开启了监视功能）
 * 4.当其他核检测到enable标志位后，根据数据端口判断是否是本核的数据源，如果是，则把对应地址中的数据读出放入监视列表中
 * 5.管理核每次直接将监视列表中的数据发送给上位机
 *
 * 注：
 * 1.为什么不是管理核根据地址直接去读其他核的监视数据？
 * (1)因为自己核访问自己核的内存范围比较安全
 * (2)比如CPH，C66的可视化数据在L2缓存中，R0管理核无法访问C66的L2缓存，
 *    所以C66只能先从DDR中读出共享监视列表，再根据共享监视列表中的数据地址去L2缓存中取出数据，再写入到DDR中的共享监视列表。
 * 2.其他核获取监视数据的功能放在大循环中处理，不需要占用中断时间：
 * (1)因为监视功能只是肉眼看，肯定会存在丢数的情况，不需要按照中断时间严格要求
 * (2)监视功能只是从下位机读数返给上位机，不像置数功能是向地址写数，必须放在中断中
 *   （因为置数功能如果放在循环中，可能引脚值写一半就会被中断打断，而中断中又会执行算法块，可能会根据算法块引脚的值去做处理，而这个时候引脚值是个错误值）
 * (3)也不需要考虑不同核之间同步的问题，因为目前上位机一次只能监视一个核.
 * 3.根据端口号区分不同核数据，而不是根据数据地址范围区分不同核数据的原因：
 * (1)因为CPH的两个C66的L2缓存地址一模一样，所以需要用端口号区分不同核心
 */
void PscGetWatchData()
{
    PSCDWORD        m_Addr;
    PSCWORD         m_Size;
    PSCDWORD       *m_Data;
    uint32_t        port_id     = PORT_ID_BASE + PscGetCoreId();

    PscCacheInv(&pWatchTable->fWatchEnable, 16); //刷16Byte的原因是想包含dwWatchCom字段
    if ((!pWatchTable->fWatchEnable) || (pWatchTable->dwWatchCom != port_id))
    {
        // 当监视功能未使能或者监视数据不是本核的数据
        return;
    }

    PscCacheInv(pWatchTable, sizeof(*pWatchTable));

    for (uint32_t wWatchId_p = 0; wWatchId_p < pWatchTable->dwWatchEntriesUsed; wWatchId_p++)
    {
        m_Addr =  pWatchTable->WatchLst[wWatchId_p].m_Addr; //数据地址
        m_Size =  pWatchTable->WatchLst[wWatchId_p].m_Size; //字节数
        m_Data = &pWatchData[wWatchId_p];                   //读出的数据
        if (C_CODE_PGM_NUM == pWatchTable->WatchLst[wWatchId_p].m_Pgm)
        {
            if(1 == m_Size)
            {
                *((PSCBYTE*)m_Data) = *((PSCBYTE*)m_Addr);
            }
            else if(2 == m_Size)
            {
                *((PSCWORD*)m_Data) = *((PSCWORD*)m_Addr);
            }
            else if(4 == m_Size)
            {
                *((PSCDWORD*)m_Data) = *((PSCDWORD*)m_Addr);
            }
        }
    }
    PscCacheWb(pWatchData, sizeof(watchData_g));
    PscGetWatchDataEnd();
}
