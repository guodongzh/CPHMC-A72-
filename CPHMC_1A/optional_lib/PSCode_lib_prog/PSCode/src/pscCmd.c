/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscCmd.c
 *@author     jinyangh
 *@date       2025.01.07
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.07  1.0       jinyangh    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "pscCmd.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// Command LOGIN
PSCBYTE PscCtlLogin()
{
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;
    PSCWORD wBuffSize;
    PSCWORD useSize;
    PSCBYTE bRetCode; /* ErrorCode of function */

    PSCTRACE("\n-> PSC: PscCtlLogin... ");

    if (PscSysGetStatus(PSCSTAT_OK))
    {
        PscSysSetStatus(PSCSTAT_LOGIN, PSCSET);

        PscSysUpdateMode();

        PscInfoEvalCapabilities();

        bRetCode = kPscSuccess;
    }
    else
    {
        bRetCode = kPscLoginStatusError;
    }

    wBuffSize = pCmdBuffer->wMaxRecSize - pCmdBuffer->wUsedRecSize;
    useSize = PscSysCallback((PSCBYTE *)&(pCmdBuffer->bRecBuff[pCmdBuffer->wUsedRecSize]), wBuffSize);
    pCmdBuffer->wUsedRecSize += useSize;

    wBuffSize = pCmdBuffer->wMaxRecSize - pCmdBuffer->wUsedRecSize;
    useSize = PscInfExtCapCallback((PSCBYTE *)&(pCmdBuffer->bRecBuff[pCmdBuffer->wUsedRecSize]), wBuffSize);
    pCmdBuffer->wUsedRecSize += useSize;

    // 在登录命令时加初始化监视列表和强制列表是因为防止异常断开后，再重新连接时，监视列表和强制列表中还有数据
    /* switch off forcing */
    PscSetInitSetTab();
    /* clear tables with watch-entries */
    PscWatchInitWatchTab();

    PSCTRACE("RetCode=%02X", (PSCWORD)bRetCode);

    return (bRetCode);
}

// Command LOGOUT
PSCBYTE PscCtlLogout()
{
    PSCBYTE bRetCode; /* ErrorCode of function */
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;
    PSCWORD wBuffSize;
    PSCWORD useSize;
    PSCTRACE("\n-> PSC: PscCtlLogout... ");

    /* switch off forcing */
    PscSetInitSetTab();
    /* clear tables with watch-entries */
    PscWatchInitWatchTab();

    if (PscSysGetStatus(PSCSTAT_OK))
    {
        PscSysSetStatus(PSCSTAT_LOGIN, PSCCLR);

        PscSysUpdateMode();

        bRetCode = kPscSuccess;
    }
    else
    {
        bRetCode = kPscLogoutStatusError;
    }

    wBuffSize = pCmdBuffer->wMaxRecSize - pCmdBuffer->wUsedRecSize;
    useSize = PscSysCallback((PSCBYTE *)&(pCmdBuffer->bRecBuff[pCmdBuffer->wUsedRecSize]), wBuffSize);
    pCmdBuffer->wUsedRecSize += useSize;

    PSCTRACE("RetCode=%02X", (PSCWORD)bRetCode);

    return (bRetCode);
}

// Reboot PLC
PSCBYTE PscEnvRebootPlc()
{
    if (!PscSysGetStatus(PSCSTAT_LOGIN))
    {
        return kPscModeErr;
    }

    if (PscSysGetStatus(PSCSTAT_RUNNING))
    {
        return kPscNotValidInRunState;
    }

    PSCTRACE("Rebooting...\n");

    return kPscSuccess;
}

// Request for firmware library information
PSCBYTE PscInfGetFwLibInfo()
{
    static PSCBYTE RecBuff[PLC_FIRMWARE_SIZE];
    PSCBYTE bRetCode; /* error code of the function */

    PSCTRACE("\n-> PSC: PscInfGetFwLibInfo... ");

    if (PscSysGetStatus(PSCSTAT_LOGIN))
    {
        memset(RecBuff, 0, PLC_FIRMWARE_SIZE);
        PscMemAbsSetByte(RecBuff, kPscFwLibInfo);/* extended return code */

        if (PscCsvSetExtRetCode((PSCBYTE *)&RecBuff[0], sizeof(RecBuff) / PSC_SIZEOF_BYTE))
        {
            /* success */
            bRetCode = kPscSuccess;
        }
        else
        {
            /* buffer too small */
            bRetCode = kPscNetRecSizeError;
        }
    }
    else
    {
        bRetCode = kPscModeErr;
    }

    PSCTRACE("RetCode=%02X", (PSCWORD)bRetCode);

    return bRetCode;
}

// Request for version numbers hardware/firmware, feature list
PSCBYTE PscInfGetPlcVersion()
{
    static PSCBYTE RecBuff[PLC_VERSION_SIZE]; /* response to the programming system */
    PSCBYTE *pOutPlcVer;
    PSCBYTE bRetCode;
    PSCBYTE coreNum = PSC_CORE_NUM;

    PSCTRACE("\n-> PSC: PscInfGetPlcVersion... ");

    PscCacheInv(&g_platform_ver_shm, sizeof(g_platform_ver_shm));

    if (PscSysGetStatus(PSCSTAT_LOGIN))
    {
        PSCWORD  MCHwVer     = (g_platform_ver_shm.hw_ver[0] << 8) | g_platform_ver_shm.hw_ver[1];  // 硬件版本
        PSCDWORD MCLowLibVer = g_platform_ver_shm.cpu_version.low_lib_ver;                 // 底层库版本
        PSCWORD  MCVer[coreNum];
        PSCDWORD MCFwReleaseVer[coreNum];
        for (PSCBYTE i = 0; i < coreNum; i++)
        {
            MCVer[i]          = (g_platform_ver_shm.core_ver[i].ver_high << 8) | g_platform_ver_shm.core_ver[i].ver_low;
            MCFwReleaseVer[i] = (PSCDWORD)g_platform_ver_shm.cpu_fw_ver[i].major[0] << 24 |
                                (PSCDWORD)g_platform_ver_shm.cpu_fw_ver[i].major[1] << 16 |
                                (PSCDWORD)g_platform_ver_shm.cpu_fw_ver[i].minor[0] << 8  |
                                (PSCDWORD)g_platform_ver_shm.cpu_fw_ver[i].minor[1];  // 固件版本
        }
        PSCWORD FPGASoftVer = g_platform_ver_shm.fpga_version.main_soft_ver;   // FPGA软件版本
        PSCWORD FPGAFunVer  = g_platform_ver_shm.fpga_version.main_fun_ver;    // FPGA功能版本

        memset(RecBuff, 0, PLC_VERSION_SIZE);

        PscMemAbsSetByte((PSCBYTE *)&RecBuff[0], kPscPlcVer); /* extended return code */
        PscMemAbsSetByte((PSCBYTE *)&RecBuff[1], coreNum);
        pOutPlcVer = (PSCBYTE *)&RecBuff[2];

        strcpy((PSCCHAR *)pOutPlcVer + STRUCT_PLCVER_NAME, (PSCCHAR *)HARDWARE_DESCRIPTION);//描述硬件设置标识
        PscMemAbsSetWord(pOutPlcVer + STRUCT_PLCVER_HW, MCHwVer);
        PscMemAbsSetDword(pOutPlcVer + STRUCT_PLCVER_LOWLIB, MCLowLibVer);
        for (PSCBYTE i = 0; i < coreNum; i++)
        {
            COMBINE_TO_STRING_POINT_2B(g_core_name[i], MCVer[i], (char *)pOutPlcVer + STRUCT_PLCVER_APP + (i * VERSION_STRING_SIZE));
            COMBINE_TO_STRING_POINT_4B(g_core_name[i], MCFwReleaseVer[i], (char *)pOutPlcVer + STRUCT_PLCVER_FW + (i * VERSION_STRING_SIZE));
        }
        PscMemAbsSetWord(pOutPlcVer + STRUCT_PLCVER_FPGASOFT, FPGASoftVer);
        PscMemAbsSetWord(pOutPlcVer + STRUCT_PLCVER_FPGAFUN, FPGAFunVer);

        if (PscCsvSetExtRetCode((PSCBYTE *)&RecBuff[0], sizeof(RecBuff) / PSC_SIZEOF_BYTE))
        {
            /* success */
            bRetCode = kPscSuccess;
        }
        else
        {
            /* buffer too small */
            bRetCode = kPscNetRecSizeError;
        }
    }
    else
    {
        bRetCode = kPscModeErr;
    }

    PSCTRACE("RetCode=%02X", (PSCWORD)bRetCode);

    return bRetCode;
}

// Request for actual project (Version of resource)
PSCBYTE PscInfGetResVersion()
{
    static PSCBYTE RecBuff[RES_VERSION_SIZE]; /* response to the programming system */
    PSCBYTE *pOutResVer;
    PSCBYTE bRetCode;
    tPscResVersion resVer;

    PSCTRACE("\n-> PSC: PscInfGetResVersion... ");

    if (PscSysGetStatus(PSCSTAT_LOGIN))
    {
        memset(RecBuff, 0, RES_VERSION_SIZE);

        PscMemAbsSetByte((PSCBYTE *)&RecBuff[0], kPscResVer);
        pOutResVer = (PSCBYTE *)&RecBuff[1];

        PscInfoChange(g_pscode_udp_info.pscode_src_port);

        /* get resource version */
        if (PscGetResVersion(&resVer))//获取资源版本
        {
            /* copy data to output buffer (in PC-side layout!) */
            strncpy((PSCCHAR *)pOutResVer + STRUCT_OFFSET_RES_PRJNAME, resVer.m_szPrjName, SIZE_PROJECT_NAME);//项目名
            strncpy((PSCCHAR *)pOutResVer + STRUCT_OFFSET_RES_RESNAME, resVer.m_szResName, SIZE_RESOURCE_NAME);//资源名

            PscMemAbsSetDword(pOutResVer + STRUCT_OFFSET_RES_PLCVER, resVer.m_PlcVer.m_TimeStamp); //版本号
            PscMemAbsSetDword(pOutResVer + STRUCT_OFFSET_RES_BUILDDATE, resVer.m_dwBuildDate);     //构建日期
            PscMemAbsSetDword(pOutResVer + STRUCT_OFFSET_RES_LOADDATE, resVer.m_dwLoadDate);       //加载日期

            strncpy((PSCCHAR *)pOutResVer + STRUCT_OFFSET_RES_PLATFORMNAME, resVer.m_szPlatformName, SIZE_PLATFORM_NAME);       //平台名

            strncpy((PSCCHAR *)pOutResVer + STRUCT_OFFSET_RES_PASSWORD_DEVELOPER, resVer.m_szPasswordDeveloper, SIZE_PASSWORD); //开发密码
            strncpy((PSCCHAR *)pOutResVer + STRUCT_OFFSET_RES_PASSWORD_PROJECT, resVer.m_szPasswordProject, SIZE_PASSWORD);     //项目密码
            strncpy((PSCCHAR *)pOutResVer + STRUCT_OFFSET_RES_PASSWORD_CUSTOMER, resVer.m_szPasswordCustomer, SIZE_PASSWORD);   //客户密码

            if (PscCsvSetExtRetCode((PSCBYTE *)&RecBuff[0], sizeof(RecBuff) / PSC_SIZEOF_BYTE))
            {
                /* success */
                bRetCode = kPscSuccess;
            }
            else
            {
                /* buffer too small */
                bRetCode = kPscNetRecSizeError;
            }
        }
        else
        {
            bRetCode = kPscInvalidPgm;
        }
    }
    else
    {
        bRetCode = kPscModeErr;
    }

    PSCTRACE("RetCode=%02X", (PSCWORD)bRetCode);

    return bRetCode;
}

// Request for data ("get watch data")
PSCBYTE PscCsvSendRequestedData()
{
    PSCBYTE bRetCode;
    tPscWatchTable* pWatchTable = (tPscWatchTable*) &(pWatchTables_g);
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;

    if(pWatchTable->fDataAvailable)
    {
        pBuff_l = &(pCmdBuffer->bRecBuff[pCmdBuffer->wUsedRecSize]);
        wBuffSize_l = pCmdBuffer->wMaxRecSize - pCmdBuffer->wUsedRecSize;
        wDataSize_l = 0;

        if (PscWatchSendData(pWatchTable))
        {
            /* all data sent, new data not available until next interpreter cycle */
            pWatchTable->fDataAvailable = PSCFALSE;//所有数据已发送，新数据要到下一个解释器周期才可用
        }

        pCmdBuffer->wUsedRecSize += wDataSize_l;//更新缓存已使用长度

        bRetCode = kPscSuccess;
    }
    else
    {
        bRetCode = kPscSuccess;
    }

    return bRetCode;
}

// Command RUN/STOP/INIT
PSCBYTE PscCtlSetState(tPscPSCmd *pPscPSCmd_p)
{
    PSCBYTE bCtrCmd;
    PSCBYTE bRetCode;
    tPscCmdBuffer *pCmdBuffer = &pCmdBuffers_g;
    PSCWORD wBuffSize;
    PSCWORD useSize;

    bCtrCmd = PscMemAbsGetByte(pPscPSCmd_p->m_pParamLst);//获取命令参数列表中下发的状态类型

    if(PscSysGetStatus(PSCSTAT_PROGRAM_VALID))
    {
        /* First clear error status reasonable */
        PscSysSetStatus(PSCSTAT_PLCERROR, PSCCLR);

        switch(bCtrCmd)
        {
        case kPscCtrStop: /* control command "Stop" */
        {
            PscSysSetStatus(PSCSTAT_RUNNING, PSCCLR);
            PscSysUpdateMode();

            wBuffSize = pCmdBuffer->wMaxRecSize - pCmdBuffer->wUsedRecSize;
            useSize = PscSysCallback((PSCBYTE *)&(pCmdBuffer->bRecBuff[pCmdBuffer->wUsedRecSize]), wBuffSize);
            pCmdBuffer->wUsedRecSize += useSize;

            bRetCode = kPscSuccess;
            break;
        }
        default:
            bRetCode = kPscUnknownCmd;
            break;
        }
    }
    else
    {
        bRetCode = kPscInvalidPgm;
    }

    PSCTRACE("RetCode=%02X", (PSCWORD)bRetCode);

    return bRetCode;
}

// 因为是R0代理其他核执行可视化底层逻辑，所以不同核连接在线的时候，需要切换信息
void PscInfoChange(PSCDWORD prot_id)
{
    PSCBYTE prot_offset = prot_id - PORT_ID_BASE;

    if (prot_offset >= PSC_CORE_NUM || prot_offset < 0)
    {
        prot_offset = 0;
    }

    memcpy((void*)&resVersion_g, (void*)&resVersionMultiCore[prot_offset], sizeof(tPscResVersion));
    memcpy(g_hardware_name, resVersion_g.m_szPlatformName, SIZE_PLATFORM_NAME);
    memcpy(g_platform_name, resVersion_g.m_szPlatformName, SIZE_PLATFORM_NAME);
}
