/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscDownload.c
 *@author     jinyangh
 *@date       2025.02.18
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.02.18  1.0       jinyangh    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "pscDownload.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
PSCBYTE g_bRawFileType = 0xFF;             //要下载的文件的类型

/* for CRC checksums */
PSCBOOL bCRCLookupInitialized = PSCFALSE;  //CRC查找表是否初始化的标志
PSCDWORD crcLookupTable[0x100];            //CRC查找表
PSCDWORD dwCRCChecksumFromPS  = 0;         //上位机下发的下载文件校验和
PSCDWORD dwCRCChecksumFromDwl = 0;         //下位机自己计算出的下载文件校验和
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// Init CRC lookup table
void PscInitializeCRCLookup()
{
    PSCDWORD nCRC;
    int indx;
    int iBit;

    bCRCLookupInitialized = PSCTRUE;
    for (indx = 0; indx < 0x100; indx++)
    {
        /* In slow implementations (without lookup table), the CRC is
		* computed bit by bit. Hence, 8 iterations are necessary per
		* byte read. During each iteration, we do:
		*    1.    nCRC = nCRC * 2 + bBitShiftedIn;
		*    2.    if (nCRC & 0x10000) nCRC ^= SSM_CRC_POLYNOMDEFAULT;
		* Because of the constant "0x10000" (bit 16), the sequence of
		* XORs is independant of the initial low-order-byte. It is
		* dependent only on the high-order 8 bit in nCRC before the
		* loop. We simulate this: for all 0x100 = 256 possibilities for
		* the high-byte in the word, we create a table: */
        nCRC = (indx << 8);

        /* Compute operation of shifting 8 bits through */
        for (iBit = 0; iBit < 8; iBit++)
        {
            nCRC *= 2;
            if (nCRC & 0x10000) /* Check bit 16 of (nCRC << 1) */
            {
                nCRC ^= 0x1021; /* 0x1021, polynomial */
            }

            /* 0x1021 is short for 0x11021 */
            /* 0x11021 is the CCITT-polynomial x^16 + x^12 + x^5 + x^0 */
        }

        nCRC &= 0xFFFF; /* Restrict to WORD (16 bit-CRC) */
        crcLookupTable[indx] = nCRC;
    }
}

// CRC check
void PscEnvAddCRCByte(PSCDWORD *pdwCurrentCRCValue, PSCBYTE b)
{
    *pdwCurrentCRCValue = (*pdwCurrentCRCValue << 8) ^ crcLookupTable[0xFF & ((*pdwCurrentCRCValue >> 8) ^ b)];
}

// Download raw file
PSCBYTE PscEnvDwlRawFile(tPscPSCmd *pPscPSCmd_p)
{
    PSCDWORD dwFileSize;
    PSCBYTE bRetCode = kPscSuccess;

    //只有登录状态并且停止模式才能下载文件
    if (!PscSysGetStatus(PSCSTAT_LOGIN))
    {
        bRetCode = kPscModeErr;
        goto Exit;
    }

    dwFileSize          = PscMemAbsGetDword(pPscPSCmd_p->m_pParamLst + 0);  //要下载的文件大小
    g_bRawFileType      = PscMemAbsGetByte(pPscPSCmd_p->m_pParamLst + 4);   //要下载的文件类型
    dwCRCChecksumFromPS = PscMemAbsGetDword(pPscPSCmd_p->m_pParamLst + 5);  //上位机下发的下载文件校验和

    if (bCRCLookupInitialized == PSCFALSE)
    {
        PscInitializeCRCLookup();
    }

    dwCRCChecksumFromDwl = 0;

    switch(g_bRawFileType)
    {
    case kPscRawFileFirmware://固件类型
        break;

        /*case kPscRawFileFPGAData:
            break;*/
        /* not supported */

    default:
        bRetCode = kPscUnknownCmd;
        break;
    }

    /* receipt received command */
    pPscPSCmd_p->m_fSendRec = PSCTRUE;

Exit:

    return bRetCode;
}

// Download raw file segment
PSCBYTE PscEnvDwlRawFileSegment(tPscPSCmd *pPscPSCmd_p, PSCBOOL fLastSegment)
{
    PSCBYTE bRetCode;
    PSCWORD nSegNum;
    PSCWORD wSegSize;
    PSCBYTE *pBuffer = &gDataBuff[0]; //指向存放从上位机接收的数据的缓冲区

    wSegSize = PscMemAbsGetWord(pPscPSCmd_p->m_pParamLst + 0);//要下载的段的大小
    nSegNum  = PscMemAbsGetWord(pPscPSCmd_p->m_pParamLst + 2);//要下载的文件段编号

    PSCTRACE("--------------------[RawFileSegNum=%u, Size=%u]... ", nSegNum, wSegSize);
    PSCTRACE("\n-> PSC: PscDwlRawFileSegment [NetRecData(NET_DATA)] ");

    if (0 == wSegSize)
    {
        wSegSize = 1; /* if segment size is 0, a single dummy byte is send (since 0 bytes cannot be sent via TCPIP driver) */
    }

    //改变从socket端口接收数据后存放的位置：之前从socket接收到数据后是存放在CmdBuffer.bCmdBuff，现在改为存放在pBuffer中（接收的类型为NET_DATA）
    NetRecData((PSCBYTE *)pBuffer, wSegSize, NET_DATA, NET_RxTIMEOUT);

    /* deposit internal command including parameters */
    pPscPSCmd_p->m_bCommand = kPscCmdContDwlRawFileSegment;//内部命令：继续下载文件片段

    /* set flag "last segment" */
    pPscPSCmd_p->m_wParamSize += sizeof(PSCBYTE);             //参数列表大小+1Byte
    *(PSCBYTE *)(pPscPSCmd_p->m_pParamLst + 4) = fLastSegment;//向参数列表的第五个参数赋值为：是否是最后一个段的标志位

    PscCsvPresetCmd(pPscPSCmd_p, kPscEnableExtrnComm);

    bRetCode = kPscSuccess;

    return bRetCode;
}

// Continue download raw file segment
PSCBYTE PscEnvDwlContRawFileSegment(tPscPSCmd *pPscPSCmd_p)
{
    PSCBYTE *pBuffer = &gDataBuff[0];
    PSCBYTE bNetStat;
    PSCBYTE bRetCode;
    PSCWORD nSegNum;
    PSCWORD wSegSize;
    PSCBOOL fLastSegment;
    PSCWORD i;

    PSCTRACE("\n-> PSC: PscDwlContRawFileSegment ... ");

    bRetCode = kPscSuccess;

    wSegSize = PscMemAbsGetWord(pPscPSCmd_p->m_pParamLst + 0);//要下载的段的大小
    nSegNum  = PscMemAbsGetWord(pPscPSCmd_p->m_pParamLst + 2);//要下载的文件段编号

    fLastSegment = *(PSCBYTE *)(pPscPSCmd_p->m_pParamLst + 4);//是否是最后一个段的标志位
    bNetStat = NetGetRxStatus();                              //从socket端口接收到数据后存放在pBuffer中（存放在下载文件临时缓冲区中）
    PSCTRACE("\n\rbNetStat=%d", bNetStat);

    switch (bNetStat)
    {
    /* received segment completely */
    case NET_SUCCESS:
        /* calculate CRC checksum (possibly over multiple segments of the file) */
        for (i = 0; i < wSegSize; i++)
        {
            PscEnvAddCRCByte(&dwCRCChecksumFromDwl, *(pBuffer + i));//计算要下载段的CRC校验和，存放在dwCRCChecksumFromDwl中
        }

        /* write to flash memory */
        /* TODO: write too DDR buffer first*/
        bRetCode = WriteFirmwareDataIntoFlash(pBuffer, wSegSize, fLastSegment);

        if (PSCTRUE == fLastSegment)//最后一段文件
        {
            if (g_bRawFileType == kPscRawFileFirmware)
            {
                /* check CRC checksum - currently, this is only done for firmware (but possible to do for all raw file downloads) */
                if (dwCRCChecksumFromDwl != dwCRCChecksumFromPS)
                {
                    bRetCode = kPscRawFileChecksumError;
                }
            }
        }

        /* receipt received segment */
        pPscPSCmd_p->m_fSendRec = PSCTRUE;
        break;

    /* no data available */
    case NET_NODATA://没有有效数据或没接收完
        /* don't receipt command */
        pPscPSCmd_p->m_fSendRec = PSCFALSE;

        /* no data received, but also no timeout */
        /* so wait */
        PSCTRACE("\n\rwaiting for raw file segment (NET_NODATA)");
        PscCsvPresetCmd(pPscPSCmd_p, kPscEnableExtrnComm);//重新将内部命令放入bCmdBuff，好让下一次还是执行这个内部命令
        break;

    /* timeout or network error */
    default:
        bRetCode = kPscNetError;
        break;
    }

    return bRetCode;
}

// Get download information
PSCBYTE PscEnvSetFileTimeOfRawFile(tPscPSCmd *pPscPSCmd_p)
{
#if MUL_FIRMWARE
    FW_BootFlag = pPscPSCmd_p->m_pParamLst[20];//获取要下载的目的区域和下一次启动时的区域标志

    PSCTRACE("ParamSize:%dBytes, FW_BootFlag:%d\r\n", pPscPSCmd_p->m_wParamSize, FW_BootFlag);
#endif
    return kPscSuccess;
}

// Write firmware
PSCBYTE WriteFirmwareDataIntoFlash(PSCBYTE *pBuffer, PSCWORD wSegSize, PSCBOOL fLastSegment)
{
#ifndef CORE_R5F0
    /*FLASH*/
    /* Open the device */
    // TODO : FW_writeDDR
//    FW_writeDDR(pBuffer,wSegSize,fLastSegment);//wenjunF 231110
    PSCTRACE("\nFW_writeDDR");
    return kPscSuccess;
#endif
}
