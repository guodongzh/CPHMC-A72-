/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pcie_init.c
 *@author     LiuRui
 *@date       2024.11.05
 *@brief      J721E PCIE RC
 *@par        History
 *Date        Version   Author     Description
 *2024.11.05  1.0       LiuRui     first version
 ******************************************************************************/

/**
 * PCIe0 <----> SLOT0 and SLOT1
 * PCIe2 <----> SLOT3
 * PCIe3 <----> SLOT2
 */


/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <ti/drv/pcie/soc/pcie_soc.h>
#include <ti/csl/csl_chip.h>
#include <ti/csl/arch/csl_arch.h>
#include <ti/osal/osal.h>
#include "pcie_init.h"
#include "debug_config.h"
#include "pcie_serdes.h"
#include <ti/drv/gpio/GPIO.h>
#include <ti/drv/gpio/soc/GPIO_soc.h>
#include <ti/csl/src/ip/gpio/V0/gpio.h>
#include <ti/drv/pcie/src/pcieloc.h>
#include <ti/drv/pcie/src/v3/pcieloc.h>

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

uint8_t pcie0_ib_space[1024 * 1024 * 1] __attribute__((aligned(0x100000), section(".pcie0_ib")));
uint8_t pcie2_ib_space[1024 * 1024 * 1] __attribute__((aligned(0x100000), section(".pcie2_ib")));
uint8_t pcie3_ib_space[1024 * 1024 * 1] __attribute__((aligned(0x100000), section(".pcie3_ib")));

uint32_t pcie_ib_space[4] = {(uint32_t)pcie0_ib_space, (uint32_t)pcie2_ib_space,
                             (uint32_t)pcie2_ib_space, (uint32_t)pcie3_ib_space};

#if defined(BUILD_C66X) || defined(BUILD_C7X_1)
uint8_t pcie0_ib1_space[1024 * 16 *  1] __attribute__((aligned(0x4000), section(".pcie0_ib1")));
#endif

uint64_t pcie_ob_space[4] = {PCIE0_CFG_BASE, PCIE1_CFG_BASE, PCIE2_CFG_BASE, PCIE3_CFG_BASE};

uint32_t g_ep_bar[4][6] = {
    {PCIE0_OB_LO_XDMA_RC, PCIE0_OB_XDMA_BYPASS_RC},
    {PCIE1_OB_LO_XDMA_RC, PCIE1_OB_XDMA_BYPASS_RC},
    {PCIE2_OB_LO_XDMA_RC, PCIE2_OB_XDMA_BYPASS_RC},
    {PCIE3_OB_LO_XDMA_RC, PCIE3_OB_XDMA_BYPASS_RC},
};

uint64_t g_rc_mem_ob[4][6] = {
    {PCIE0_XDMA_BASE, PCIE0_XDMA_BYPASS_BASE},
    {PCIE1_XDMA_BASE, PCIE1_XDMA_BYPASS_BASE},
    {PCIE2_XDMA_BASE, PCIE2_XDMA_BYPASS_BASE},
    {PCIE3_XDMA_BASE, PCIE3_XDMA_BYPASS_BASE},
};

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/*****************************************************************************
 * Function: Utility function a cycle clock
 ****************************************************************************/
static uint32_t read_time32(void)
{
    uint32_t timeVal;

#if defined(_TMS320C6X)
    timeVal = TSCL;
#elif __ARM_ARCH_7A__
    __asm__ __volatile__("MRC p15, 0, %0, c9, c13, 0\t\n" : "=r"(timeVal));
#else
    /* M4 specific implementation*/
    static uint32_t simuTimer = 0;
    simuTimer++;
    timeVal = simuTimer;
#endif
    return timeVal;
}

/*****************************************************************************
 * Function: Utility function to introduce delay
 ****************************************************************************/
static void cycle_delay(uint32_t count)
{
    uint32_t start = read_time32();

    while ((read_time32() - start) < count)
        ;
}

/**
 *
 * @param handle
 * @param enable
 * @return
 */
static pcieRet_e pcie_host_reset(Pcie_Handle handle, uint8_t enable)
{
    pcieRstCmdReg_t rstCmd;
    pcieRegisters_t regs;
    pcieRet_e       retVal;

    memset(&regs, 0, sizeof(regs));
    regs.rstCmd = &rstCmd;

    if (enable)
    {
        rstCmd.initRst = 1;
    }
    else
    {
        rstCmd.initRst = 0;
    }

    if ((retVal = Pcie_writeRegs(handle, pcie_LOCATION_LOCAL, &regs)) != pcie_RET_OK)
    {
        PCIE_log("Write RESET CMD register failed!\n");
        return retVal;
    }

    return pcie_RET_OK;
}

/*****************************************************************************
 * Function: Enable/Disable LTSSM (Link Training)
 * This function demonstrates how one can write one binary to use either
 * rev of PCIE
 ****************************************************************************/
static pcieRet_e pcie_ltssm_ctrl(Pcie_Handle handle, uint8_t enable)
{
    pcieCmdStatusReg_t       cmdStatus;
    pcieTiConfDeviceCmdReg_t deviceCmd;
    pcieRegisters_t          regs;
    pcieRet_e                retVal;

    memset(&cmdStatus, 0, sizeof(cmdStatus));
    memset(&deviceCmd, 0, sizeof(deviceCmd));
    memset(&regs, 0, sizeof(regs));

    regs.cmdStatus = &cmdStatus;
    if ((retVal = Pcie_readRegs(handle, pcie_LOCATION_LOCAL, &regs)) != pcie_RET_OK)
    {
        if (retVal == pcie_RET_INV_REG)
        {
            /* The cmdStatus register doesn't exist; try the deviceCmd instead */
            regs.cmdStatus = NULL;
            regs.tiConfDeviceCmd = &deviceCmd;
            if ((retVal = Pcie_readRegs(handle, pcie_LOCATION_LOCAL, &regs)) != pcie_RET_OK)
            {
                PCIE_log("Read CMD STATUS and DEVICE CMD registers failed!\n");
                return retVal;
            }
        }
        else
        {
            PCIE_log("Read CMD STATUS register failed!\n");
            return retVal;
        }
    }

    if (enable)
        deviceCmd.ltssmEn = cmdStatus.ltssmEn = 1;
    else
        deviceCmd.ltssmEn = cmdStatus.ltssmEn = 0;

    if ((retVal = Pcie_writeRegs(handle, pcie_LOCATION_LOCAL, &regs)) != pcie_RET_OK)
    {
        PCIE_log("SET CMD STATUS register failed!\n");
        return retVal;
    }

    return pcie_RET_OK;
}

/*****************************************************************************
 * Function: Configure PCIe in Root Complex Mode
 ****************************************************************************/
static pcieRet_e pcie_cfg_rc(Pcie_Handle handle)
{
    pcieRet_e            retVal;
    pcieStatusCmdReg_t   statusCmd;
    pcieDevStatCtrlReg_t devStatCtrl;
    pcieAccrReg_t        accr;
    pcieRegisters_t      setRegs;
    pcieRegisters_t      getRegs;

    memset(&statusCmd, 0, sizeof(statusCmd));
    memset(&devStatCtrl, 0, sizeof(devStatCtrl));
    memset(&accr, 0, sizeof(accr));

    /*Disable link training*/
    if ((retVal = pcie_ltssm_ctrl(handle, FALSE)) != pcie_RET_OK)
    {
        PCIE_log("Failed to disable Link Training!\n");
        return retVal;
    }

    memset(&setRegs, 0, sizeof(setRegs));
    memset(&getRegs, 0, sizeof(getRegs));

    getRegs.statusCmd = &statusCmd;
    if ((retVal = Pcie_readRegs(handle, pcie_LOCATION_LOCAL, &getRegs)) != pcie_RET_OK)
    {
        PCIE_log("Read Status Comand register failed!\n");
        return retVal;
    }

    /* Enable memory access and mastership of the bus */
    statusCmd.memSp = 1;
    statusCmd.busMs = 1;
    statusCmd.resp = 1;
    statusCmd.serrEn = 1;
    setRegs.statusCmd = &statusCmd;
    if ((retVal = Pcie_writeRegs(handle, pcie_LOCATION_LOCAL, &setRegs)) != pcie_RET_OK)
    {
        PCIE_log("SET Status Command register failed!\n");
        return retVal;
    }

    memset(&setRegs, 0, sizeof(setRegs));
    memset(&getRegs, 0, sizeof(getRegs));
    getRegs.devStatCtrl = &devStatCtrl;
    if ((retVal = Pcie_readRegs(handle, pcie_LOCATION_LOCAL, &getRegs)) != pcie_RET_OK)
    {
        PCIE_log("Regad Device Status Control register failed!\n");
        return retVal;
    }

    /* Enable Error Reporting */
    devStatCtrl.maxSz = 3;
    devStatCtrl.maxPayld = 0;
    devStatCtrl.reqRp = 1;
    devStatCtrl.fatalErRp = 1;
    devStatCtrl.nFatalErRp = 1;
    devStatCtrl.corErRp = 1;
    setRegs.devStatCtrl = &devStatCtrl;
    if ((retVal = Pcie_writeRegs(handle, pcie_LOCATION_LOCAL, &setRegs)) != pcie_RET_OK)
    {
        PCIE_log("SET Device Status Control register failed!\n");
        return retVal;
    }

    /* Disable ECRC */
    memset(&setRegs, 0, sizeof(setRegs));
    accr.chkEn = 0;
    accr.chkCap = 0;
    accr.genEn = 0;
    accr.genCap = 0;
    setRegs.accr = &accr;
    if ((retVal = Pcie_writeRegs(handle, pcie_LOCATION_LOCAL, &setRegs)) != pcie_RET_OK)
    {
        PCIE_log("SET ACCR register failed!\n");
        return retVal;
    }

    return pcie_RET_OK;
}

/*****************************************************************************
 * Function: Configure EP BAR
 ****************************************************************************/
static pcieRet_e pcie_cfg_ep_bar(Pcie_Handle handle, uint32_t device_num)
{
    pcieRet_e              retVal;
    pcieType0Bar32bitIdx_t type0Bar32bitIdx;
    pcieRegisters_t        getRegs;

    if (device_num > 3)
        return pcie_RET_INV_DEVICENUM;
    memset(&type0Bar32bitIdx, 0, sizeof(type0Bar32bitIdx));
    memset(&getRegs, 0, sizeof(getRegs));

    getRegs.type0Bar32bitIdx = &type0Bar32bitIdx;

    /* init Type 0 32bits BAR register*/
    type0Bar32bitIdx.reg.reg32 = 0xffffffff;
    for (int j = 0; j < 6; ++j)
    {
        type0Bar32bitIdx.idx = j;
        if ((retVal = Pcie_writeRegs(handle, pcie_LOCATION_REMOTE, &getRegs)) != pcie_RET_OK)
        {
            PCIE_log("SET BAR[%d] MASK register failed!\n", j);
            return retVal;
        }
    }

    PCIE_log("\n");
    for (int j = 0; j < 6; ++j)
    {
        PCIE_log("ep bar[%d] info:", j);
        type0Bar32bitIdx.idx = j;
        if ((retVal = Pcie_readRegs(handle, pcie_LOCATION_REMOTE, &getRegs)) != pcie_RET_OK)
        {
            PCIE_log("SET BAR MASK register failed!\n");
            return retVal;
        }
        else
        {
            PCIE_log("mask=0x%08x, size=0x%08x byte; ",
                     type0Bar32bitIdx.reg.reg32,
                     (~type0Bar32bitIdx.reg.reg32 + 1));

            if (~type0Bar32bitIdx.reg.reg32 + 1 != 0)
            {
                pcieAtuRegionParams_t regionParams;

                /*Configure OB region for memory transfer*/
                regionParams.regionDir = PCIE_ATU_REGION_DIR_OUTBOUND;
                regionParams.tlpType = PCIE_TLP_TYPE_MEM;
                regionParams.enableRegion = 1;

                regionParams.lowerBaseAddr = (uint32_t)g_rc_mem_ob[device_num][j];
                regionParams.upperBaseAddr = g_rc_mem_ob[device_num][j] >> 16 >> 16;
                /* only 32 bits needed given data area size */
                regionParams.regionWindowSize = ~type0Bar32bitIdx.reg.reg32;

                regionParams.lowerTargetAddr = g_ep_bar[device_num][j];
                regionParams.upperTargetAddr = 0;

                /* NOTE: Region[0] for ep's config space access*/
                if ((retVal = Pcie_atuRegionConfig(handle,
                                                   pcie_LOCATION_LOCAL,
                                                   j + 1,
                                                   &regionParams)) != pcie_RET_OK)
                {
                    PCIE_log("CFG RC OB ATU Faild !!!\n");
                    return retVal;
                }

                PCIE_log("rc ob info: mem addr=0x%08x%08x, pcie addr=0x%08x%08x",
                         regionParams.upperBaseAddr,
                         regionParams.lowerBaseAddr,
                         regionParams.upperTargetAddr,
                         regionParams.lowerTargetAddr);

                type0Bar32bitIdx.reg.reg32 = g_ep_bar[device_num][j];
                if ((retVal = Pcie_writeRegs(handle,
                                             pcie_LOCATION_REMOTE,
                                             &getRegs)) != pcie_RET_OK)
                {
                    PCIE_log("SET BAR MASK register failed!\n");
                    return retVal;
                }
            }
        }
        PCIE_log("\n");
    }

    PCIE_log("\n");

    return pcie_RET_OK;
}

/*****************************************************************************
 * Function: Configure EP device
 ****************************************************************************/
static pcieRet_e pcie_cfg_ep(Pcie_Handle handle)
{
    pcieRet_e            retVal;
    pcieStatusCmdReg_t   statusCmd;
    pcieDevStatCtrlReg_t devStatCtrl;

    pcieRegisters_t setRegs;
    pcieRegisters_t getRegs;

    memset(&statusCmd, 0, sizeof(statusCmd));
    memset(&devStatCtrl, 0, sizeof(devStatCtrl));
    memset(&getRegs, 0, sizeof(getRegs));
    memset(&setRegs, 0, sizeof(setRegs));

    getRegs.statusCmd = &statusCmd;
    if ((retVal = Pcie_readRegs(handle, pcie_LOCATION_REMOTE, &getRegs)) != pcie_RET_OK)
    {
        PCIE_log("Read Status Comand register failed!\n");
        return retVal;
    }

    /* Enable memory access and mastership of the bus */
    statusCmd.memSp = 1;
    statusCmd.busMs = 1;
    statusCmd.resp = 1;
    statusCmd.serrEn = 1;
    setRegs.statusCmd = &statusCmd;

    if ((retVal = Pcie_writeRegs(handle, pcie_LOCATION_REMOTE, &setRegs)) != pcie_RET_OK)
    {
        PCIE_log("SET Status Command register failed!\n");
        return retVal;
    }

    memset(&setRegs, 0, sizeof(setRegs));
    memset(&getRegs, 0, sizeof(getRegs));

    getRegs.devStatCtrl = &devStatCtrl;
    if ((retVal = Pcie_readRegs(handle, pcie_LOCATION_REMOTE, &getRegs)) != pcie_RET_OK)
    {
        PCIE_log("Regad Device Status Control register failed!\n");
        return retVal;
    }

    /* Enable Error Reporting */
    devStatCtrl.reqRp = 1;
    devStatCtrl.fatalErRp = 1;
    devStatCtrl.nFatalErRp = 1;
    devStatCtrl.corErRp = 1;
    setRegs.devStatCtrl = &devStatCtrl;

    if ((retVal = Pcie_writeRegs(handle, pcie_LOCATION_REMOTE, &setRegs)) != pcie_RET_OK)
    {
        PCIE_log("SET Device Status Control register failed!\n");
        return retVal;
    }

    return pcie_RET_OK;
}

/**
 * @brief Configure Outbound Address Translation for rc
 *        accessing ep's config sapce
 * @param handle pcie driver handle
 * @return
 */
static pcieRet_e pcie_rc_init_cfg_ob(Pcie_Handle handle, pcieAddrTransCfg_t *obCfg)
{
    pcieAtuRegionParams_t regionParams;
    pcieRet_e             retVal;
    uint32_t              resSize;

    if (obCfg == NULL)
        return pcie_RET_INV_HANDLE;

    memset(&regionParams, 0, sizeof(regionParams));

    if ((retVal = Pcie_getMemSpaceReserved(handle, &resSize)) != pcie_RET_OK)
    {
        PCIE_log("getMemSpaceReserved failed (%d)\n", (int)retVal);
        return retVal;
    }

    /*Configure OB region for remote configuration access space*/
    regionParams.regionDir = PCIE_ATU_REGION_DIR_OUTBOUND;
    regionParams.tlpType = PCIE_TLP_TYPE_CFG;
    regionParams.enableRegion = 1;

    regionParams.lowerBaseAddr = (uint32_t)obCfg->ram_addr;
    regionParams.upperBaseAddr = obCfg->ram_addr >> 16 >> 16;
    regionParams.regionWindowSize = obCfg->mask;

    /* This aligns the base addr of buffer to ibCfg->ibMask,
     * which needs to be compensated by the application */
    if ((obCfg->ram_addr & obCfg->mask) != 0)
    {
        return pcie_RET_INV_INITCFG;
    }

    regionParams.lowerTargetAddr = 0U;
    regionParams.upperTargetAddr = 0U;
    return Pcie_atuRegionConfig(handle, pcie_LOCATION_LOCAL, (uint32_t)0U, &regionParams);
}

/**
 * @brief  Configure and enable Inbound Address Translation for rc
 * @param handle
 * @param ibCfg
 * @return
 */
static pcieRet_e pcie_rc_ib_cfg(Pcie_Handle handle, pcieAddrTransCfg_t *ibCfg)
{
    pcieAtuRegionParams_t regionParams;
    pcieRet_e             retVal;

    if (ibCfg == NULL)
        return pcie_RET_INV_HANDLE;

    memset(&regionParams, 0, sizeof(regionParams));

    /*Configure IB region for memory transfer*/
    regionParams.regionDir = PCIE_ATU_REGION_DIR_INBOUND;
    regionParams.tlpType = PCIE_TLP_TYPE_MEM;
    regionParams.enableRegion = 1;
    regionParams.matchMode = PCIE_ATU_REGION_MATCH_MODE_ADDR;

    regionParams.lowerBaseAddr = ibCfg->start_addr_lo;
    regionParams.upperBaseAddr = ibCfg->start_addr_hi;
    regionParams.regionWindowSize = ibCfg->mask;

    /* This aligns the base addr of buffer to ibCfg->ibMask,
     * which needs to be compensated by the application */
    if ((ibCfg->ram_addr & ibCfg->mask) != 0)
    {
        return pcie_RET_INV_INITCFG;
    }
    regionParams.lowerTargetAddr = (uint32_t)ibCfg->ram_addr;
    regionParams.upperTargetAddr = ibCfg->ram_addr >> 16 >> 16;

    if ((retVal = Pcie_atuRegionConfig(handle, pcie_LOCATION_LOCAL, ibCfg->region_num, &regionParams)) != pcie_RET_OK)
    {
        return retVal;
    }

    return retVal;
}

/*****************************************************************************
 * Function: Check LTSSM status and wait for the link to be up
 ****************************************************************************/
static int pcie_wait_link_up(Pcie_Handle handle)
{
    pcieRegisters_t getRegs;
    int             try_cnt = 10;

    memset(&getRegs, 0, sizeof(getRegs));

    pcieTiConfDeviceCmdReg_t ltssmStateReg;
    getRegs.tiConfDeviceCmd = &ltssmStateReg;

    memset(&ltssmStateReg, 0, sizeof(ltssmStateReg));

    uint8_t ltssmState = 0;

    while (ltssmState != (pcie_LTSSM_L0) && try_cnt > 0)
    {
        cycle_delay(100);
        if (Pcie_readRegs(handle, pcie_LOCATION_LOCAL, &getRegs) != pcie_RET_OK)
        {
            return -1;
        }
        ltssmState = ltssmStateReg.ltssmState;
        try_cnt--;
    }
    if (try_cnt == 0)
        return -1;
    else
        return 0;
}

static int pcie_check_link_params(Pcie_Handle handle,
                                  uint8_t     exp_speed,
                                  uint8_t     exp_lanes)
{
    pcieRegisters_t       regs;
    pcieLinkStatCtrlReg_t linkStatCtrl;

    /* Get link status */
    memset(&regs, 0, sizeof(regs));
    regs.linkStatCtrl = &linkStatCtrl;

    PCIE_log("Checking link speed and # of lanes\n");
    Pcie_readRegs(handle, pcie_LOCATION_LOCAL, &regs);
    if ((exp_lanes != linkStatCtrl.negotiatedLinkWd) ||
        (exp_speed != linkStatCtrl.linkSpeed))
    {
        PCIE_log("pcie link or lanes not match\n");
        return -1;
    }
    PCIE_log("Expect %d lanes, found %d lanes Pass\n",
             exp_lanes,
             (int)linkStatCtrl.negotiatedLinkWd);
    PCIE_log("Expect gen %d speed, found gen %d speed Pass\n",
             (int)exp_speed,
             (int)linkStatCtrl.linkSpeed);

    return 0;
}

static void pcie_set_lanes(Pcie_Handle handle, uint8_t lane_num)
{
    pcieLnkCtrlReg_t lnkCtrlReg;
    pcieRegisters_t  regs;
    uint8_t          origLanes;

    memset(&regs, 0, sizeof(regs));
    regs.lnkCtrl = &lnkCtrlReg;
    if (Pcie_readRegs(handle, pcie_LOCATION_LOCAL, &regs) != pcie_RET_OK)
    {
        PCIE_log("Read pcieCtrlAddr register failed!\n");
    }
    origLanes = lnkCtrlReg.lnkMode;

    /* lane_num=2 ofr 2 lane */
    if (lane_num == 0 || lane_num > 2)
        lnkCtrlReg.lnkMode = 1;
    else
        lnkCtrlReg.lnkMode = lane_num - 1;

    if (origLanes != lnkCtrlReg.lnkMode)
    {
        if (Pcie_writeRegs(handle, pcie_LOCATION_LOCAL, &regs) != pcie_RET_OK)
        {
            PCIE_log("Write pcieCtrlAddr register failed!\n");
            exit(1);
        }
        PCIE_log("Set lanes from %d to %d\n", (int)origLanes + 1, (int)lnkCtrlReg.lnkMode + 1);
    }
}

/* *********************** SRIS ENABLE DISABLE ****************************** */
static void pcie_sris_ctrl(Pcie_Handle handle, uint32_t enable)
{
    Pcie_DeviceCfgBaseAddr *cfg = pcie_handle_to_cfg(handle);
    Pciev3_DevParams       *params = (Pciev3_DevParams *)cfg->devParams;

    CSL_user_cfgRegs *userCfg = (CSL_user_cfgRegs *)params->userCfgBase;
    uint32_t          val = userCfg->INITCFG;

    pcie_setbits(val, CSL_USER_CFG_INITCFG_SRIS_ENABLE, enable);

    userCfg->INITCFG = val;
}

void pcie_print_bar_info(Pcie_Handle handle)
{
    pcieRegisters_t getRegs;

    pcieType0Bar32bitIdx_t type0Bar32bitIdx = {0};
    memset(&getRegs, 0, sizeof(getRegs));
    getRegs.type0Bar32bitIdx = &type0Bar32bitIdx;

    PCIE_log("\nEP type0: ");
    for (int j = 0; j < 6; ++j)
    {
        type0Bar32bitIdx.idx = j;
        if (Pcie_readRegs(handle, pcie_LOCATION_REMOTE, &getRegs) != pcie_RET_OK)
        {
            PCIE_log("SET BAR MASK register failed!\n");
        }
        else
        {
            PCIE_log("bar[%d]=0x%08x ", type0Bar32bitIdx.idx, type0Bar32bitIdx.reg.reg32);
        }
    }

    PCIE_log("\nRC type1: ");

    for (int j = 0; j < 2; ++j)
    {
        type0Bar32bitIdx.idx = j;
        if (Pcie_readRegs(handle, pcie_LOCATION_LOCAL, &getRegs) != pcie_RET_OK)
        {
            PCIE_log("SET BAR MASK register failed!\n");
        }
        else
        {
            PCIE_log("bar[%d]=0x%08x ", j, type0Bar32bitIdx.reg.reg32);
        }
    }

    PCIE_log("\n");
}

/*****************************************************************************
 * Function: pcie main task
 ****************************************************************************/
pcieRet_e pcie_init(uint32_t slot_id)
{
    pcieRet_e          retVal;
    pcieAddrTransCfg_t ibCfg;
    pcieAddrTransCfg_t obCfg;
    pcieBarCfg_t       barCfg;
    Pcie_Handle        handle = NULL;
    pcieRegisters_t    getRegs;
    uint32_t device_num;

    if (slot_id > 3)
        return pcie_RET_INV_DEVICENUM;

    PCIE_log("**********************************************\n");
    PCIE_log("*          init slot[%d] PCIe Start          *\n", slot_id);
    PCIE_log("*                RC mode                     *\n");
    PCIE_log("**********************************************\n\n");

    if (slot_id == 0 || slot_id == 1)
        device_num = 0;
    else if (slot_id == 2)
        device_num = 3;
    else
        device_num = 2;


    /**
     * NOTE: SBL R5 core config serdes
     */

    /* Pass device config to LLD */
    Pcie_init(&pcieInitCfg);

    /* device_num = 0 for pcie 0 */
    if ((retVal = Pcie_open(device_num, &handle)) != pcie_RET_OK)
    {
        PCIE_log("Open failed (%d)\n", (int)retVal);
        return pcie_RET_INV_HANDLE;
    }

    /* SRIS disable */
    pcie_sris_ctrl(handle, 0);

    /* Configure application registers for Root Complex*/
    if ((retVal = pcie_cfg_rc(handle)) != pcie_RET_OK)
    {
        PCIE_log("Failed to configure PCIe in RC mode (%d)\n", (int)retVal);
        return pcie_RET_INV_REG;
    }

    /* Configure RC BAR */
    barCfg.location = pcie_LOCATION_LOCAL;
    barCfg.mode = pcie_RC_MODE;
    barCfg.barxa = PCIE_RCBARA_1M;
    barCfg.barxc = PCIE_BARC_32B_MEM_BAR_NON_PREFETCH;
    barCfg.bar1a = PCIE_RCBARA_16K;
    barCfg.bar1c = PCIE_BARC_32B_MEM_BAR_NON_PREFETCH;
    if ((retVal = Pcie_cfgBar(handle, &barCfg)) != pcie_RET_OK)
    {
        PCIE_log("Failed to configure BAR (%d)\n", (int)retVal);
        return pcie_RET_INV_REG;
    }

    /* Configure Address Translation */
    if (device_num == 0)
    {
        ibCfg.mask = PCIE0_INBOUND0_MASK;
    }
    else if (device_num == 1)
    {
        ibCfg.mask = PCIE1_INBOUND0_MASK;
    }
    else if (device_num == 2)
    {
        ibCfg.mask = PCIE2_INBOUND0_MASK;
    }
    else
    {
        ibCfg.mask = PCIE3_INBOUND0_MASK;
    }
    ibCfg.ram_addr = pcie_ib_space[device_num];
    ibCfg.start_addr_lo = ibCfg.ram_addr;
    ibCfg.start_addr_hi = 0;
    ibCfg.region_num = 0;
    if ((retVal = pcie_rc_ib_cfg(handle, &ibCfg)) != pcie_RET_OK)
    {
        PCIE_log("Failed to configure Inbound Translation (%d)\n", (int)retVal);
        return pcie_RET_INV_REG;
    }
    else
    {
        PCIE_log("Successfully configured Inbound Translation!\n");
    }

    if (device_num == 0)
    {
        ibCfg.mask = PCIE0_INBOUND0_MASK0;
        ibCfg.ram_addr = PCIE0_IB1_RAM_ADDR;
        ibCfg.start_addr_lo = PCIE0_IB1_PCIE_ADDR;
        ibCfg.start_addr_hi = 0;
        ibCfg.region_num = 1;
        if ((retVal = pcie_rc_ib_cfg(handle, &ibCfg)) != pcie_RET_OK)
        {
            PCIE_log("Failed to configure Inbound Translation (%d)\n", (int)retVal);
            return pcie_RET_INV_REG;
        }
        else
        {
            PCIE_log("Successfully configured Inbound Translation!\n");
        }
    }


    /* Configure Address Translation */
    obCfg.mask = PCIE0_CFG_MASK;
    obCfg.ram_addr = pcie_ob_space[device_num];
    obCfg.start_addr_lo = 0;
    obCfg.start_addr_hi = 0;
    obCfg.region_num = 0;
    if ((retVal = pcie_rc_init_cfg_ob(handle, &obCfg)) != pcie_RET_OK)
    {
        PCIE_log("Failed to configure Outbound Address Translation (%d)\n", (int)retVal);
        return pcie_RET_INV_REG;
    }
    else
    {
        PCIE_log("Successfully configured Outbound Translation!\n");
    }

    /* Configure/limit number of lanes */
    if (device_num == 0 || device_num == 1)
        pcie_set_lanes(handle, 2);
    else
        pcie_set_lanes(handle, 1);

    PCIE_log("Starting link training...\n");

    /*Enable link training*/
    if ((retVal = pcie_ltssm_ctrl(handle, TRUE)) != pcie_RET_OK)
    {
        PCIE_log("Failed to Enable Link Training! (%d)\n", (int)retVal);
        return pcie_RET_INV_REG;
    }
    /* Wait for link to be up */
    while (1)
    {
        if (pcie_wait_link_up(handle) < 0)
        {
            PCIE_log("link failed.\n");
            Osal_delay(100);
        }
        else
            break;
    }

    PCIE_log("link is up.\n");
    uint8_t exp_speed;
    uint8_t exp_lanes;
    if (device_num == 0 || device_num == 1)
    {
        exp_speed = 3; //gen3
        exp_lanes = 2; //x2
    }
    else
    {
        exp_speed = 1; //gen1
        exp_lanes = 1; //x1
    }
    while (pcie_check_link_params(handle, exp_speed, exp_lanes) < 0)
    {
        pcie_host_reset(handle, 1);
        Osal_delay(100);
        pcie_host_reset(handle, 0);

        memset(&getRegs, 0, sizeof(getRegs));
        pciePlForceLinkReg_t plForceLink;
        getRegs.plForceLink = &plForceLink;
        Pcie_readRegs(handle, pcie_LOCATION_LOCAL, &getRegs);

        if (device_num == 0)
        {
            plForceLink.linkNumMap = 3;  // 3 = x2
            plForceLink.lnkRate = 2;     // 2 = gen3
        }
        else
        {
            plForceLink.linkNumMap = 1;  // 1 = x1
            plForceLink.lnkRate = 0;     // 0 = gen1
        }

        plForceLink.reTrainingLinkNum = 1;
        plForceLink.reTrainingLnkRate = 1;
        Pcie_writeRegs(handle, pcie_LOCATION_LOCAL, &getRegs);
        Osal_delay(100);
        while (1)
        {
            if (pcie_wait_link_up(handle) < 0)
            {
                PCIE_log("link failed.\n");
                Osal_delay(100);
            }
            else
                break;
        }
    }

    pcieVndDevIdReg_t vendorDevId;

    memset(&getRegs, 0, sizeof(getRegs));
    getRegs.vndDevId = &vendorDevId;

    Pcie_readRegs(handle, pcie_LOCATION_REMOTE, &getRegs);
    PCIE_log("Endpoint Device ID: 0x%04x, Vendor ID: 0x%04x\r\n",
             vendorDevId.devId,
             vendorDevId.vndId);

    pcie_cfg_ep_bar(handle, device_num);
    pcie_cfg_ep(handle);
    pcie_print_bar_info(handle);

    PCIE_log("*          init slot[%d] PCIe end       *\n", slot_id);

    return pcie_RET_OK;
}
