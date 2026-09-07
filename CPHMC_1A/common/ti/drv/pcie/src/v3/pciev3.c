/*
 *
 * Copyright (C) 2010-2019 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 *  File Name: pciev3.c
 *
 *  Processing/configuration functions for the PCIe driver.
 *
 */

#include <ti/drv/pcie/pcie.h>
#include <ti/drv/pcie/src/pcieloc.h>
#include <ti/drv/pcie/src/v3/pcieloc.h>

#include <ti/csl/soc.h>

#include <string.h>

/* Local object simulating IATU window selection from V1.
 * No different in terms of re-entrancy compared to hw
 * reg in v1
 */
Pciev3_LocalObj pciev3LocalObj = {
    {
        {0u, 0u},
        {0u, 0u},
        {0u, 0u},
        {0u, 0u},
    },
};

/*****************************************************************************
 * Set the mode of one interface without depending directly on device
 * dependant registers (via device.c)
 ****************************************************************************/
static void
pcie_set_mode(Pciev3_DevParams *devParams, uint32_t index, pcieMode_e mode); /*for misra warning*/
static void pcie_set_mode(Pciev3_DevParams *devParams, uint32_t index, pcieMode_e mode)
{
    uint32_t val;
    uint32_t curMode, numLane, linkSpd;

    switch (mode)
    {
    case pcie_EP_MODE:
        curMode = 0U;
        break;
    case pcie_RC_MODE:
    default:
        curMode = 1U;
        break;
    }

    if (devParams)
    {
        numLane = devParams->numLane;
        linkSpd = devParams->linkSpeed;
        if (devParams->pcieCtrlAddr)
        {
            val = *(devParams->pcieCtrlAddr);
            pcie_setbits(val, CSL_MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_MODE_SEL, curMode);
            pcie_setbits(val, CSL_MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_GENERATION_SEL, linkSpd - 1);
            pcie_setbits(val, CSL_MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_LANE_COUNT, numLane - 1);
            *(devParams->pcieCtrlAddr) = val;
        }
    }
} /* pcie_set_mode */

/*****************************************************************************
 * Get the mode of one interface without depending directly on device
 * dependant registers (via device.c)
 ****************************************************************************/
int pcie_get_mode(Pcie_Handle handle);
int pcie_get_mode(Pcie_Handle handle)
{
    uint32_t val;
    uint32_t curMode = 0U;
    Pcie_DeviceCfgBaseAddr *cfg = pcie_handle_to_cfg(handle);
    Pciev3_DevParams *devParams;

    if (cfg)
    {
        devParams = (Pciev3_DevParams *)cfg->devParams;
        if (devParams)
        {
            if (devParams->pcieCtrlAddr)
            {
                val = *(devParams->pcieCtrlAddr);
                pcie_getbits(val, CSL_MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_MODE_SEL, curMode);
            }
        }
    }
    return curMode;
} /* pcie_set_mode */

/*****************************************************************************
 **********  External APIs **********************
 ****************************************************************************/

/*********************************************************************
 * FUNCTION PURPOSE: Sets PCIe mode to RC or EP for interface
 * specified by handle
 *********************************************************************/
pcieRet_e Pciev3_setInterfaceMode(Pcie_Handle handle, /**< [in]  The PCIE LLD instance identifier */
                                  pcieMode_e mode     /**< [in] PCIE Mode */
)
{
    Pcie_DeviceCfgBaseAddr *cfg = pcie_handle_to_cfg(handle);
    Pciev3_DevParams *devParams;
    Pcie_IntHandle iHandle = (Pcie_IntHandle)handle;

    pcieRet_e retVal = pcie_RET_INV_HANDLE;

    if (cfg)
    {
        devParams = (Pciev3_DevParams *)cfg->devParams;
        if (devParams)
        {
            pcie_set_mode(devParams, iHandle->pcie_index, mode);
            retVal = pcie_RET_OK;
        }
    }

    return retVal;
} /* Pciev3_setInterfaceMode */

/*********************************************************************
 * FUNCTION PURPOSE: Returns amount of reserved space between beginning
 *                   of hardware's data area and the base returned
 *                   by @ref Pcie_getMemSpaceRange.  This enables
 *                   sw to position windows correctly
 *********************************************************************/
pcieRet_e
Pciev3_getMemSpaceReserved(Pcie_Handle handle, /**< [in]  The PCIE LLD instance identifier */
                           uint32_t *resSize   /**< [out] Reserved space */
)
{
    pcieRet_e retVal = pcie_RET_OK;

    if (pcieLObjIsValid == 0)
    {
        retVal = pcie_RET_NO_INIT;
    }
    else
    {
        if (pcie_check_handle_fcn(handle) == 0)
        {
            retVal = pcie_RET_INV_HANDLE;
        }
        else
        {
            if (resSize)
            {
                Pcie_DeviceCfgBaseAddr *bases = pcie_handle_to_cfg(handle);
                if (bases)
                {
                    *resSize = bases->dataReserved;
                }
                else
                {
                    retVal = pcie_RET_INV_HANDLE;
                }
            }
        }
    }

    return retVal;
} /* Pciev3_getMemSpaceReserved */

/*********************************************************************
 * FUNCTION PURPOSE: Returns the PCIe Internal Address Range for the
 *                   Memory Space. This range is used for accessing memory.
 *********************************************************************/
pcieRet_e Pciev3_getMemSpaceRange(Pcie_Handle handle, /**< [in]  The PCIE LLD instance identifier */
                                  void **base,        /**< [out] Memory Space base address */
                                  uint32_t *size      /**< [out] Memory Space total size */
)
{
    pcieRet_e retVal = pcie_RET_OK;

    if (pcieLObjIsValid == 0)
    {
        retVal = pcie_RET_NO_INIT;
    }
    else
    {
        if (pcie_check_handle_fcn(handle) == 0)
        {
            retVal = pcie_RET_INV_HANDLE;
        }
        else
        {
            if (base)
            {
                Pcie_DeviceCfgBaseAddr *bases = pcie_handle_to_cfg(handle);
                if (bases)
                {
                    *base = bases->dataBase;
                }
                else
                {
                    retVal = pcie_RET_INV_HANDLE;
                }
            }

            if (size)
            {
                *size = (uint32_t)0x8000000; /* 128 MB */
            }
        }
    }

    return retVal;
} /* Pciev3_getMemSpaceRange */

/*********************************************************************
 * FUNCTION PURPOSE: Reads any register
 ********************************************************************/
pcieRet_e Pciev3_readRegs(Pcie_Handle handle,       /**< [in] The PCIE LLD instance identifier */
                          pcieLocation_e location,  /**< [in] Local or remote peripheral */
                          pcieRegisters_t *readRegs /**< [in/out] List of registers to read */
)
{
    Pcie_IntHandle iHandle = (Pcie_IntHandle)handle;
    pciePlconfIatuIndexReg_t *simIatuWindow = &pciev3LocalObj.simIatuWindow[iHandle->pcie_index];
    Pcie_DeviceCfgBaseAddr *cfg = pcie_handle_to_cfg(handle);
    Pciev3_DeviceCfgBaseAddrs *bases = (Pciev3_DeviceCfgBaseAddrs *)cfg->cfgBase;
    Pciev3_DevParams *params = (Pciev3_DevParams *)cfg->devParams;

    /* Base Address for the Config Space
       These registers can be Local/Remote and Type0(EP)/Type1(RC) */
    /* CSL_pcie_rp_coreRegs *baseCfgRc = (CSL_pcie_rp_coreRegs *)bases->cfgBase; */
    CSL_pcie_ep_coreRegs *baseCfgEp = (CSL_pcie_ep_coreRegs *)bases->cfgBase;

    pcieRet_e retVal = pcie_RET_OK;
    int32_t i;

    if (pcieLObjIsValid == 0)
    {
        retVal = pcie_RET_NO_INIT;
    }
    else
    {
        if (pcie_check_handle_fcn(handle) == 0)
        {
            retVal = pcie_RET_INV_HANDLE;
        }
        else
        {
            /* Get base address for Local or Remote config space */
            if (location != pcie_LOCATION_LOCAL)
            {
                char *remoteBase = (char *)cfg->dataBase + bases->remoteOffset;
                /* baseCfgRc     = (CSL_pcie_rp_coreRegs *)(remoteBase); */
                baseCfgEp = (CSL_pcie_ep_coreRegs *)(remoteBase);
            }
        }
    }

    /*****************************************************************************************
     *Application Registers
     *****************************************************************************************/
    if ((retVal == pcie_RET_OK) && (readRegs->pid != NULL))
    {
        retVal = pciev3_read_pid_reg((CSL_pcie_ep_coreRegs *)params->userCfgBase, readRegs->pid);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->cmdStatus != NULL))
    {
        retVal = pciev3_read_cmdStatus_reg((CSL_pcie_ep_coreRegs *)params->userCfgBase,
                                           readRegs->cmdStatus);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->rstCmd != NULL))
    {
        retVal = pciev3_read_rstCmd_reg((CSL_pcie_ep_coreRegs *)params->userCfgBase,
                                         readRegs->rstCmd);
    }

    if ((retVal == pcie_RET_OK) && (readRegs->epIrqSet != NULL))
    {
        retVal = pciev3_read_epIrqSet_reg(
            (CSL_user_cfgRegs *)params->userCfgBase, readRegs->epIrqSet, 0);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->epIrqClr != NULL))
    {
        retVal = pciev3_read_epIrqClr_reg(
            (CSL_user_cfgRegs *)params->userCfgBase, readRegs->epIrqClr, 0);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->epIrqStatus != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }

    for (i = 0; i < 4; i++)
    {
        if ((retVal == pcie_RET_OK) && (readRegs->legacyIrqStatusRaw[i] != NULL))
        {
            /* Not supported on rev 3 */
            retVal = pcie_RET_INV_REG;
        }
        if ((retVal == pcie_RET_OK) && (readRegs->legacyIrqStatus[i] != NULL))
        {
            retVal = pciev3_read_legacyIrqStatus_reg(
                (CSL_intd_cfgRegs *)params->intCfgBase, readRegs->legacyIrqStatus[i], i);
        }
        if ((retVal == pcie_RET_OK) && (readRegs->legacyIrqEnableSet[i] != NULL))
        {
            retVal = pciev3_read_legacyIrqEnableSet_reg(
                (CSL_intd_cfgRegs *)params->intCfgBase, readRegs->legacyIrqEnableSet[i], i);
        }
        if ((retVal == pcie_RET_OK) && (readRegs->legacyIrqEnableClr[i] != NULL))
        {
            retVal = pciev3_read_legacyIrqEnableClr_reg(
                (CSL_intd_cfgRegs *)params->intCfgBase, readRegs->legacyIrqEnableClr[i], i);
        }
    }

    if ((retVal == pcie_RET_OK) && (readRegs->plForceLink != NULL))
    {
        pciev3_read_plForceLink_reg(baseCfgEp,  readRegs->plForceLink);
    }

    /*****************************************************************************************
     *Configuration Registers
     *****************************************************************************************/

    /*Type 0, Type1 Common Registers*/

    if ((retVal == pcie_RET_OK) && (readRegs->vndDevId != NULL))
    {
        retVal = pciev3_read_vndDevId_reg(
                &(baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_PCIE_BASE.I_VENDOR_ID_DEVICE_ID),
                readRegs->vndDevId);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->statusCmd != NULL))
    {
        retVal = pciev3_read_statusCmd_reg(
            &(baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_PCIE_BASE.I_COMMAND_STATUS),
            readRegs->statusCmd);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->baseAddr != NULL))
    {
        retVal = pciev3_read_baseAddr_reg(
            &(baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_PCIE_BASE.I_BASE_ADDR[0]),
            readRegs->baseAddr);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->revId != NULL))
    {
        retVal = pciev3_read_revId_reg(
            &(baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_PCIE_BASE.I_REVISION_ID_CLASS_CODE),
            readRegs->revId);
    }


    /*Type 0 Registers*/
    if ((retVal == pcie_RET_OK) && (readRegs->type0BarIdx != NULL))
    {
        retVal = pciev3_read_type0Bar_reg(
            baseCfgEp, &(readRegs->type0BarIdx->reg), readRegs->type0BarIdx->idx);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type0Bar32bitIdx != NULL))
    {
        retVal = pciev3_read_type0Bar32bit_reg(baseCfgEp,
                                               &(readRegs->type0Bar32bitIdx->reg),
                                               readRegs->type0Bar32bitIdx->idx,
                                               params->pfNum);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type0BarMask32bitIdx != NULL))
    {
        retVal = pciev3_read_type0Bar32bit_reg(baseCfgEp,
                                               &(readRegs->type0BarMask32bitIdx->reg),
                                               readRegs->type0BarMask32bitIdx->idx,
                                               params->pfNum);
    }

    /*Type 1 Registers*/
    if ((retVal == pcie_RET_OK) && (readRegs->type1BistHeader != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type1BarIdx != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type1Bar32bitIdx != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type1BarMask32bitIdx != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type1BusNum != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type1SecStat != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type1Memspace != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->prefMem != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->prefBaseUpper != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->prefLimitUpper != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type1IOSpace != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type1CapPtr != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type1ExpnsnRom != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->type1BridgeInt != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }


    /*MSI Registers*/
    if ((retVal == pcie_RET_OK) && (readRegs->msiCap != NULL))
    {
        retVal = pciev3_read_msiCap_reg(
            (CSL_pcie_ep_coreRegs *)&(
                baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_MSI_CAP_STRUCT.I_MSI_CTRL_REG),
            readRegs->msiCap);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->msiLo32 != NULL))
    {
        retVal = pciev3_read_msiLo32_reg(baseCfgEp, readRegs->msiLo32);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->msiUp32 != NULL))
    {
        retVal = pciev3_read_msiUp32_reg(baseCfgEp, readRegs->msiUp32);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->msiData != NULL))
    {
        retVal = pciev3_read_msiData_reg(baseCfgEp, readRegs->msiData);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->devStatCtrl != NULL))
    {
        retVal = pciev3_read_devStatCtrl_reg(&(baseCfgEp->EP_PF_I_PCIE[params->pfNum]
                                                   .EP_PF_I_PCIE_CAP_STRUCT.I_PCIE_DEV_CTRL_STATUS),
                                             readRegs->devStatCtrl);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->linkCap != NULL))
    {
        retVal = pciev3_read_linkCap_reg(
            &(baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_PCIE_CAP_STRUCT.I_LINK_CAP),
            readRegs->linkCap);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->linkStatCtrl != NULL))
    {
        retVal = pciev3_read_linkStatCtrl_reg(
            &(baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_PCIE_CAP_STRUCT.I_LINK_CTRL_STATUS),
            readRegs->linkStatCtrl);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->lnkCtrl != NULL))
    {
        retVal = pciev3_read_lnkCtrl_reg((CSL_pcie_ep_coreRegs *)params->pcieCtrlAddr,
                                         readRegs->lnkCtrl);
    }

    if ((retVal == pcie_RET_OK) && (readRegs->plconfIatuIndex != NULL))
    {
        /* Return the simulated window address */
        *readRegs->plconfIatuIndex = *simIatuWindow;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->plconfIatuRegCtrl1 != NULL))
    {
        retVal = pciev3_read_plconfIatuRegCtrl1_reg(
            baseCfgEp, simIatuWindow, readRegs->plconfIatuRegCtrl1);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->plconfIatuRegLowerBase != NULL))
    {
        retVal = pciev3_read_plconfIatuRegLowerBase_reg(
            baseCfgEp, simIatuWindow, readRegs->plconfIatuRegLowerBase);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->plconfIatuRegUpperBase != NULL))
    {
        retVal = pciev3_read_plconfIatuRegUpperBase_reg(
            baseCfgEp, simIatuWindow, readRegs->plconfIatuRegUpperBase);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->plconfIatuRegLimit != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (readRegs->plconfIatuRegLowerTarget != NULL))
    {
        retVal = pciev3_read_plconfIatuRegLowerTarget_reg(
            baseCfgEp, simIatuWindow, readRegs->plconfIatuRegLowerTarget);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->plconfIatuRegUpperTarget != NULL))
    {
        retVal = pciev3_read_plconfIatuRegUpperTarget_reg(
            baseCfgEp, simIatuWindow, readRegs->plconfIatuRegUpperTarget);
    }
    if ((retVal == pcie_RET_OK) && (readRegs->plconfIatuRegCtrl3 != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }

    /* TI CONF registers */
    /* reject hw rev 3 TI CONF registers */
    if ((retVal == pcie_RET_OK) && (readRegs->tiConfDeviceCmd != NULL))
    {
        retVal = pciev3_read_linkStatus_reg((CSL_pcie_ep_coreRegs *)params->userCfgBase,
                                            readRegs->tiConfDeviceCmd);
    }

    return retVal;
} /* Pciev3_readRegs */

/*********************************************************************
 * FUNCTION PURPOSE: Writes any register
 ********************************************************************/
pcieRet_e Pciev3_writeRegs(Pcie_Handle handle,        /**< [in] The PCIE LLD instance identifier */
                           pcieLocation_e location,   /**< [in] Local or remote peripheral */
                           pcieRegisters_t *writeRegs /**< [in] List of registers to write */
)
{
    Pcie_IntHandle iHandle = (Pcie_IntHandle)handle;
    pciePlconfIatuIndexReg_t *simIatuWindow = &pciev3LocalObj.simIatuWindow[iHandle->pcie_index];
    Pcie_DeviceCfgBaseAddr *cfg = pcie_handle_to_cfg(handle);
    Pciev3_DeviceCfgBaseAddrs *bases = (Pciev3_DeviceCfgBaseAddrs *)cfg->cfgBase;
    Pciev3_DevParams *params = (Pciev3_DevParams *)cfg->devParams;

    /* Base Address for the Config Space
       These registers can be Local/Remote and Type0(EP)/Type1(RC) */
    /* CSL_pcie_rp_coreRegs *baseCfgRc = (CSL_pcie_rp_coreRegs *)bases->cfgBase; */
    CSL_pcie_ep_coreRegs *baseCfgEp = (CSL_pcie_ep_coreRegs *)bases->cfgBase;

    pcieRet_e retVal = pcie_RET_OK;
    int32_t i;

    if (pcieLObjIsValid == 0)
    {
        retVal = pcie_RET_NO_INIT;
    }
    else
    {
        if (pcie_check_handle_fcn(handle) == 0)
        {
            retVal = pcie_RET_INV_HANDLE;
        }
        else
        {
            /* Get base address for Local/Remote config space */
            if (location != pcie_LOCATION_LOCAL)
            {
                char *remoteBase = (char *)cfg->dataBase + bases->remoteOffset;
                /* baseCfgRc     = (CSL_pcie_rp_coreRegs *)(remoteBase); */
                baseCfgEp = (CSL_pcie_ep_coreRegs *)(remoteBase);
            }
        }
    }
    /*****************************************************************************************
     * Reject hw rev 0 app registers (these are similar but not identical to TI CONF on rev 3)
     *****************************************************************************************/
    if ((retVal == pcie_RET_OK) && (writeRegs->cmdStatus != NULL))
    {
        retVal = pciev3_write_cmdStatus_reg((CSL_pcie_ep_coreRegs *)params->userCfgBase,
                                            writeRegs->cmdStatus);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->rstCmd != NULL))
    {
        retVal = pciev3_write_rstCmd_reg((CSL_pcie_ep_coreRegs *)params->userCfgBase,
                                            writeRegs->rstCmd);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->epIrqSet != NULL))
    {
        retVal = pciev3_write_epIrqSet_reg(
            (CSL_user_cfgRegs *)params->userCfgBase, writeRegs->epIrqSet, 0);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->epIrqClr != NULL))
    {
        retVal = pciev3_write_epIrqClr_reg(
            (CSL_user_cfgRegs *)params->userCfgBase, writeRegs->epIrqClr, 0);
    }

    for (i = 0; i < 4; i++)
    {
        if ((retVal == pcie_RET_OK) && (writeRegs->legacyIrqStatusRaw[i] != NULL))
        {
            /* Not supported on rev 3 */
            retVal = pcie_RET_INV_REG;
        }
        if ((retVal == pcie_RET_OK) && (writeRegs->legacyIrqStatus[i] != NULL))
        {
            retVal = pciev3_write_legacyIrqStatus_reg(
                (CSL_intd_cfgRegs *)params->intCfgBase, writeRegs->legacyIrqStatus[i], i);
        }
        if ((retVal == pcie_RET_OK) && (writeRegs->legacyIrqEnableSet[i] != NULL))
        {
            retVal = pciev3_write_legacyIrqEnableSet_reg(
                (CSL_intd_cfgRegs *)params->intCfgBase, writeRegs->legacyIrqEnableSet[i], i);
        }
        if ((retVal == pcie_RET_OK) && (writeRegs->legacyIrqEnableClr[i] != NULL))
        {
            retVal = pciev3_write_legacyIrqEnableClr_reg(
                (CSL_intd_cfgRegs *)params->intCfgBase, writeRegs->legacyIrqEnableClr[i], i);
        }
    }

    if ((retVal == pcie_RET_OK) && (writeRegs->plForceLink != NULL))
    {
        pciev3_write_plForceLink_reg(baseCfgEp,  writeRegs->plForceLink);
    }

    /*****************************************************************************************
     *Configuration Registers
     *****************************************************************************************/

    /*Type 0, Type1 Common Registers*/

    if ((retVal == pcie_RET_OK) && (writeRegs->vndDevId != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->statusCmd != NULL))
    {
        retVal = pciev3_write_statusCmd_reg(
            &(baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_PCIE_BASE.I_COMMAND_STATUS),
            writeRegs->statusCmd);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->baseAddr != NULL))
    {
        retVal = pciev3_write_baseAddr_reg(
            &(baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_PCIE_BASE.I_BASE_ADDR[0]),
            writeRegs->baseAddr);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->revId != NULL))
    {
        retVal = pciev3_write_revId_reg(
            &(baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_PCIE_BASE.I_REVISION_ID_CLASS_CODE),
            writeRegs->revId);
    }

    /*Type 0 Registers*/
    if ((retVal == pcie_RET_OK) && (writeRegs->type0BarIdx != NULL))
    {
        retVal = pciev3_write_type0Bar_reg(
            baseCfgEp, &(writeRegs->type0BarIdx->reg), writeRegs->type0BarIdx->idx);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->type0BarMask32bitIdx != NULL))
    {
        retVal = pciev3_write_type0Bar32bit_reg(baseCfgEp,
                                                &(writeRegs->type0BarMask32bitIdx->reg),
                                                writeRegs->type0BarMask32bitIdx->idx,
                                                params->pfNum);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->type0Bar32bitIdx != NULL))
    {
        retVal = pciev3_write_type0Bar32bit_reg(baseCfgEp,
                                                &(writeRegs->type0Bar32bitIdx->reg),
                                                writeRegs->type0Bar32bitIdx->idx,
                                                params->pfNum);
    }

    /*MSI Registers*/
    if ((retVal == pcie_RET_OK) && (writeRegs->msiCap != NULL))
    {
        retVal = pciev3_write_msiCap_reg(
            (CSL_pcie_ep_coreRegs *)&(
                baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_MSI_CAP_STRUCT.I_MSI_CTRL_REG),
            writeRegs->msiCap);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->msiLo32 != NULL))
    {
        retVal = pciev3_write_msiLo32_reg(baseCfgEp, writeRegs->msiLo32);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->msiUp32 != NULL))
    {
        retVal = pciev3_write_msiUp32_reg(baseCfgEp, writeRegs->msiUp32);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->msiData != NULL))
    {
        retVal = pciev3_write_msiData_reg(baseCfgEp, writeRegs->msiData);
    }

    if ((retVal == pcie_RET_OK) && (writeRegs->devStatCtrl != NULL))
    {
        retVal = pciev3_write_devStatCtrl_reg(&(baseCfgEp->EP_PF_I_PCIE[params->pfNum]
                                                    .EP_PF_I_PCIE_CAP_STRUCT.I_PCIE_DEV_CTRL_STATUS),
                                              writeRegs->devStatCtrl);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->linkCap != NULL))
    {
        retVal = pciev3_write_linkCap_reg(
            &(baseCfgEp->EP_PF_I_PCIE[params->pfNum].EP_PF_I_PCIE_CAP_STRUCT.I_LINK_CAP),
            writeRegs->linkCap);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->accr != NULL))
    {
        retVal = pciev3_write_accr_reg(baseCfgEp, writeRegs->accr);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->lnkCtrl != NULL))
    {
        retVal = pciev3_write_lnkCtrl_reg((CSL_pcie_ep_coreRegs *)params->pcieCtrlAddr,
                                          writeRegs->lnkCtrl);
        if (retVal == pcie_RET_OK)
            params->numLane = writeRegs->lnkCtrl->lnkMode + 1;
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->plconfIatuIndex != NULL))
    {
        /* Set the simulated window address */
        *simIatuWindow = *writeRegs->plconfIatuIndex;
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->plconfIatuRegLowerBase != NULL))
    {
        /* EP or RC */
        if (pcie_get_mode(handle) == 0)
            retVal = pciev3_write_plconfIatuRegLowerBase_reg(
                baseCfgEp, simIatuWindow, writeRegs->plconfIatuRegLowerBase);
        else
            retVal = pciev3_write_plconfIatuRegLowerBaseRc_reg(
                baseCfgEp, simIatuWindow, writeRegs->plconfIatuRegLowerBase);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->plconfIatuRegUpperBase != NULL))
    {
        retVal = pciev3_write_plconfIatuRegUpperBase_reg(
            baseCfgEp, simIatuWindow, writeRegs->plconfIatuRegUpperBase);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->plconfIatuRegLimit != NULL))
    {
        /* Not supported on rev 3 */
        retVal = pcie_RET_INV_REG;
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->plconfIatuRegLowerTarget != NULL))
    {
        retVal = pciev3_write_plconfIatuRegLowerTarget_reg(
            baseCfgEp, simIatuWindow, writeRegs->plconfIatuRegLowerTarget);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->plconfIatuRegUpperTarget != NULL))
    {
        retVal = pciev3_write_plconfIatuRegUpperTarget_reg(
            baseCfgEp, simIatuWindow, writeRegs->plconfIatuRegUpperTarget);
    }
    if ((retVal == pcie_RET_OK) && (writeRegs->plconfIatuRegCtrl3 != NULL))
    {
        /* Pure RO register */
        retVal = pcie_RET_INV_REG;
    }
    /* Ctrl1 is done last since it has enable bit */
    if ((retVal == pcie_RET_OK) && (writeRegs->plconfIatuRegCtrl1 != NULL))
    {
        retVal = pciev3_write_plconfIatuRegCtrl1_reg(
            baseCfgEp, simIatuWindow, writeRegs->plconfIatuRegCtrl1);
    }

    if ((retVal == pcie_RET_OK) && (writeRegs->tiConfDeviceCmd != NULL))
    {
        retVal = pciev3_write_linkStatus_reg((CSL_pcie_ep_coreRegs *)params->userCfgBase,
                                             writeRegs->tiConfDeviceCmd);
    }

    return retVal;
} /* Pciev3_writeRegs */

/*********************************************************************
 * FUNCTION PURPOSE: Configures a BAR Register (32bits)
 ********************************************************************/
pcieRet_e Pciev3_cfgBar(Pcie_Handle handle,        /**< [in] The PCIE LLD instance identifier */
                        const pcieBarCfg_t *barCfg /**< [in] BAR configuration parameters */
)
{
    Pcie_DeviceCfgBaseAddr *cfg = pcie_handle_to_cfg(handle);
    Pciev3_DeviceCfgBaseAddrs *bases = (Pciev3_DeviceCfgBaseAddrs *)cfg->cfgBase;
    CSL_pcie_rp_coreRegs *baseCfgRc = (CSL_pcie_rp_coreRegs *)bases->cfgBase;
    pcieRet_e retVal = pcie_RET_OK;
    pcieType0BarIdx_t type0BarIdx;
    pcieRegisters_t setRegs;
    pcieRegisters_t getRegs;
    uint32_t barAddrField = 0;

    if (pcieLObjIsValid == 0)
    {
        retVal = pcie_RET_NO_INIT;
    }
    else
    {
        if (pcie_check_handle_fcn(handle) == 0)
        {
            retVal = pcie_RET_INV_HANDLE;
        }
        else
        {
            memset(&getRegs, 0, sizeof(getRegs));
            memset(&setRegs, 0, sizeof(setRegs));
            memset(&type0BarIdx, 0, sizeof(type0BarIdx));

            if (barCfg->mode == pcie_RC_MODE)
            {
                uint32_t rc_bar_cfg = (1 << 31);
                uint32_t temp = (barCfg->barxa | (barCfg->barxc << 6));
                rc_bar_cfg |= temp;
                temp  = (barCfg->bar1a << 9);
                temp |= (barCfg->bar1c << 14);
                rc_bar_cfg |= temp;
                baseCfgRc->LM_I_REGF_LM_PCIE_BASE.I_RC_BAR_CONFIG_REG = rc_bar_cfg;
            }
            else
            {
                getRegs.type0BarIdx = &type0BarIdx;
                retVal = Pciev3_readRegs(handle, barCfg->location, &getRegs);

                type0BarIdx.reg.base = barAddrField;
                type0BarIdx.reg.barxc = barCfg->barxc;
                type0BarIdx.reg.barxa = barCfg->barxa;
                type0BarIdx.idx = barCfg->idx;

                setRegs.type0BarIdx = &type0BarIdx;
                retVal = Pciev3_writeRegs(handle, barCfg->location, &setRegs);
            }
        }
    }
    return retVal;
} /* Pciev3_cfgBar */

uint32_t getNumPassBitFromWinSize(uint32_t winSize)
{
    uint32_t numPassBit = 0;
    uint32_t temp = winSize;

    while (temp != 0)
    {
        numPassBit++;
        temp >>= 1;
    }

    return (numPassBit - 1);
}

/*********************************************************************
 * FUNCTION PURPOSE: Configures an ATU (address translation) region
 ********************************************************************/
pcieRet_e
Pciev3_atuRegionConfig(Pcie_Handle handle,      /**< [in] The PCIE LLD instance identifier */
                       pcieLocation_e location, /**< [in] local/remote */
                       uint32_t atuRegionIndex, /* [in] index number to configure */
                       const pcieAtuRegionParams_t *atuRegionParams /* [in] config structure */
)
{
    Pcie_DeviceCfgBaseAddr *cfg = pcie_handle_to_cfg(handle);
    Pciev3_DeviceCfgBaseAddrs *bases = (Pciev3_DeviceCfgBaseAddrs *)cfg->cfgBase;
    CSL_pcie_rp_coreRegs *baseCfgRc = (CSL_pcie_rp_coreRegs *)bases->cfgBase;
    CSL_pcie_ep_coreRegs *baseCfgEp = (CSL_pcie_ep_coreRegs *)bases->cfgBase;
    Pciev3_DevParams *params = (Pciev3_DevParams *)cfg->devParams;

    pcieRet_e retVal = pcie_RET_OK;
    pciePlconfIatuIndexReg_t index;
    pciePlconfIatuRegCtrl1Reg_t ctrl1;
    pciePlconfIatuRegLowerBaseReg_t lowerBase;
    pciePlconfIatuRegUpperBaseReg_t upperBase;
    /* pciePlconfIatuRegLimitReg_t       limit; */
    pciePlconfIatuRegLowerTargetReg_t lowerTarget;
    pciePlconfIatuRegUpperTargetReg_t upperTarget;
    pcieRegisters_t regs;

    /* Set up register pointer for interesting registers */
    memset(&regs, 0, sizeof(regs));
    regs.plconfIatuIndex = &index;

    /* Read current values for index */
    retVal = Pciev3_readRegs(handle, location, &regs);
    if (retVal == pcie_RET_OK)
    {
        /* Update ATU index register with new region direction and region index.
         **/
        switch (atuRegionParams->regionDir)
        {
        case PCIE_ATU_REGION_DIR_OUTBOUND:
            index.regionDirection = 0U; /* Outbound - emulates v1 CSL */
            break;
        case PCIE_ATU_REGION_DIR_INBOUND:
        default:
            index.regionDirection = 1U; /* Inbound - emulates v1 CSL */
            break;
        }
        index.regionIndex = atuRegionIndex;

        /* Writeback the new values for index */
        retVal = Pciev3_writeRegs(handle, location, &regs);
        if (retVal == pcie_RET_OK)
        {
            regs.plconfIatuIndex = NULL;
            regs.plconfIatuRegCtrl1 = &ctrl1;
            regs.plconfIatuRegLowerBase = &lowerBase;
            regs.plconfIatuRegUpperBase = &upperBase;
            regs.plconfIatuRegLimit = NULL; /* plconfIatuRegLimit is not supported on J7ES */
            regs.plconfIatuRegLowerTarget = &lowerTarget;
            regs.plconfIatuRegUpperTarget = &upperTarget;

            /* Read current values of rest of registers for this index */
            retVal = Pciev3_readRegs(handle, location, &regs);
            if (retVal == pcie_RET_OK)
            {
                /* Set TLP(Transaction Layer packet) type. */
                switch (atuRegionParams->tlpType)
                {
                case PCIE_TLP_TYPE_MEM:
                    ctrl1.type = 2U;
                    break;
                case PCIE_TLP_TYPE_CFG:
                    ctrl1.type = 10U;
                    break;
                default:
                    ctrl1.type = 2U;
                    break;
                }

                /* Configure lower base. */
                lowerBase.iatuRegLowerBase = atuRegionParams->lowerTargetAddr >> 8;
                lowerBase.zero = getNumPassBitFromWinSize(atuRegionParams->regionWindowSize);

                /* Configure upper base. */
                upperBase.iatuRegUpperBase = atuRegionParams->upperTargetAddr;

                /* Configure lower target. */
                lowerTarget.iatuRegLowerTarget = atuRegionParams->lowerBaseAddr >> 8;
                lowerTarget.zero = getNumPassBitFromWinSize(atuRegionParams->regionWindowSize);

                /* Configure Upper target. */
                upperTarget.iatuRegUpperTarget = atuRegionParams->upperBaseAddr;

                /* Writeback the new values */
                retVal = Pciev3_writeRegs(handle, location, &regs);

                /* For INBOUND, set the PCIE_CORE_RP_I_RC_BAR_x properly */
                if (atuRegionParams->regionDir == PCIE_ATU_REGION_DIR_INBOUND)
                {
                    if (pcie_get_mode(handle) != 0)
                    {
                        if (atuRegionIndex == 0)
                            baseCfgRc->RC_I_RC_PCIE_BASE.I_RC_BAR_0 =
                                atuRegionParams->lowerBaseAddr;
                        else
                            baseCfgRc->RC_I_RC_PCIE_BASE.I_RC_BAR_1 =
                                atuRegionParams->lowerBaseAddr;
                    }
                    else
                    {
                        baseCfgEp->EP_PF_I_PCIE[params->pfNum]
                            .EP_PF_I_PCIE_BASE.I_BASE_ADDR[atuRegionIndex] =
                            atuRegionParams->lowerBaseAddr;
                    }
                }
            }
        }
    }
    return retVal;
} /* Pciev3_atuRegionConfig */

/*********************************************************************
 * FUNCTION PURPOSE: Get pending functional (MSI/legacy) interrupts
 ********************************************************************/
pcieRet_e
Pciev3_getPendingFuncInts(Pcie_Handle handle,  /**< [in] The PCIE LLD instance identifier */
                          void *pendingBits,   /**< [out] rev-specific pending bits */
                          int32_t sizeMsiBits, /**< [in] size of msiBits in MAU */
                          void *msiBits        /**< [out] rev-specific msi pending bits to check */
)
{
    pcieRet_e retVal = pcie_RET_OK;

    /* As all MSI or Legacy interrupts can be routed to individual
     * SPI or LPI, there is no need for special demux here */
    return retVal;
} /* Pciev3_getPendingFuncInts */

/*********************************************************************
 * FUNCTION PURPOSE: Clear pending functional (MSI/legacy) interrupts
 ********************************************************************/
pcieRet_e
Pciev3_clrPendingFuncInts(Pcie_Handle handle,  /**< [in] The PCIE LLD instance identifier */
                          void *pendingBits,   /**< [in] rev-specific pending bits */
                          int32_t sizeMsiBits, /**< [in] size of msiBits in MAU */
                          void *msiBits        /**< [in] rev-specific msi pending bits to ack */
)
{
    pcieRet_e retVal = pcie_RET_OK;

    /* As all MSI or Legacy interrupts can be routed to individual
     * SPI or LPI, there is no need for special clear here */
    return retVal;
} /* Pciev3_clrPendingFuncInts */

/* Nothing past this point */
