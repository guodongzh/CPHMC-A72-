/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscode_main.c
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
#include "pscode_main.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
int pscode_main(void)
{
    /**************************** initalization area ****************************/
    /* initialize runtime system and communication */
    if (PscEnvInitialize() != kPscSuccess)
    {
        PSCode_log("Error initializing SmartPLC!\n");
        return 1;
    }

    /**************************** start SmartPLC ****************************/
    /* enable communication */
    fEnableCommunication = PSCTRUE;
    PscStart();

    return 0;
}

PSCBYTE PscEnvInitialize(void)
{
    PSCBYTE bRetCode = kPscSuccess;

    /* boot system */
    PscPlcStart();

    return bRetCode;
}

void PLC_Comm(void *arg)
{
    PscGetResVersionShare();
    /* enable upload of watch data */
    if (PscSysGetStatus(PSCSTAT_PROGRAM_VALID))
    {
        PscWatchMarkDataValid();
    }

    if (fEnableCommunication)
    {
        /* execute commands from PSCode */
        PscCmdMainLoop();
    }
}

void PscStart(void)
{
    PSCBYTE bRetCode = kPscSuccess;

    if (!PscSysGetStatus(PSCSTAT_RUNNING)) /* do start only in stop mode */
    {
        Error_g = kIpOK;

        /* run init mode */
        bRetCode = Application_Init();//初始化上位机定义的变量值（相当于把输入变量不为0的值放入地址，其余区域全为0）
        if (bRetCode == kPscSuccess)
        {
            bRetCode = Application_AllTasks_I();//执行上位机的init模式的算法块
        }

        if (bRetCode == kPscSuccess)
        {
            PscSysSetStatus(PSCSTAT_RUNNING, PSCSET);
            PscSysUpdateMode();
        }
    }
}
