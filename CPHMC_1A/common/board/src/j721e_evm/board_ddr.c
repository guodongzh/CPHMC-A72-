/******************************************************************************
 * Copyright (c) 2019 Texas Instruments Incorporated - http://www.ti.com
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
 *****************************************************************************/
#include <string.h>
#include "board/src/j721e_evm/include/board_ddr.h"
/*
 * Select the complete DDRSS register set together with its matching PLL
 * frequency.  Do not change only DDRSS_PLL_FREQUENCY_1/2: controller, PI and
 * PHY timings are frequency dependent.
 */
#ifndef BOARD_DDR_RATE_MT_S
#define BOARD_DDR_RATE_MT_S (2400U)
#endif

#if (BOARD_DDR_RATE_MT_S == 2400U)
#include "board/src/j721e_evm/include/board_ddrRegInit_2400.h"
#elif (BOARD_DDR_RATE_MT_S == 4266U)
#include "board/src/j721e_evm/include/board_ddrRegInit.h"
#else
#error "Unsupported BOARD_DDR_RATE_MT_S; supported values are 2400 and 4266"
#endif


/* Global variables */
static LPDDR4_Config gBoardDdrCfg;
static LPDDR4_PrivateData gBoardDdrPd;

/* Local function prototypes */
static int32_t emif_ConfigureECC(void);

#ifdef BOARD_DDR_ENABLE_PLL_BYPASS
/**
 * \brief   Set DDR PLL to bypass, efectively 20MHz or 19.2MHz (on silicon).
 *
 * \return  none
 */
static void Board_DDRSetPLLExtBypass(void)
{
    uint32_t addrOffset = 0x00000000;
    uint32_t baseAddr = CSL_PLL0_CFG_BASE;
    uint32_t regVal;
    uint32_t fieldVal;
    uint32_t regAddr;

    fieldVal = 1U;
    regAddr = (baseAddr + addrOffset + (DDR_PLL_INDEX * 0x1000) + CONTROL);
    regVal = HW_RD_REG32(regAddr);
    regVal |= (fieldVal << 31);
    HW_WR_REG32(regAddr, regVal);
}
#endif

/**
 * \brief   Set DDR PLL clock value
 *
 * \return  BOARD_SOK in case of success or appropriate error code
 */
static Board_STATUS Board_DDRSetPLLClock(uint64_t frequency)
{
    Board_STATUS status = BOARD_SOK;

    status = Board_PLLInit(TISCI_DEV_DDR0,
                           TISCI_DEV_DDR0_DDRSS_DDR_PLL_CLK,
                           frequency);
    if(BOARD_SOK != status)
    {
        BOARD_DEBUG_LOG("Failed to Set the DDR PLL Clock Frequency\n");
    }

    return status;
}

/**
 * \brief   Controls the DDR PLL clock change sequence during inits
 *
 * \return  None
 */
static void Board_DDRChangeFreqAck(void)
{
    uint32_t reqType;
    uint32_t regVal;
    volatile uint32_t counter;
    volatile uint32_t temp = 0U;

    temp = temp;  /* To suppress compiler warning */
    BOARD_DEBUG_LOG("--->>> LPDDR4 Initialization is in progress ... <<<---\n");

    for(counter = 0U; counter < DDRSS_PLL_FHS_CNT; counter++)
    {
        /* wait for freq change request */
        regVal = HW_RD_REG32(BOARD_DDR_FSP_CLKCHNG_REQ_ADDR) & 0x80U;
        BOARD_DEBUG_LOG("Reg Value: %d \n", regVal);

        while(0x0U == regVal)
        {
            regVal = HW_RD_REG32(BOARD_DDR_FSP_CLKCHNG_REQ_ADDR) & 0x80U;
            BOARD_DEBUG_LOG("Reg Value: %d \n", regVal);
        }

        reqType = HW_RD_REG32(BOARD_DDR_FSP_CLKCHNG_REQ_ADDR) & 0x03U;
        BOARD_DEBUG_LOG("Frequency Change type %d request from Controller \n", reqType);

        if(1U == reqType)
        {
            Board_DDRSetPLLClock(DDRSS_PLL_FREQUENCY_1);
        }
        else if(2U == reqType)
        {
            Board_DDRSetPLLClock(DDRSS_PLL_FREQUENCY_2);
        }
        else if(0U == reqType)
        {
#ifndef BOARD_DDR_ENABLE_PLL_BYPASS
            Board_DDRSetPLLClock(DDRSS_PLL_FREQUENCY_0);
#else
            Board_DDRSetPLLExtBypass();
#endif
        }
        else
        {
            BOARD_DEBUG_LOG("Invalid Request Type\n");
        }

        /* Acknowledge frequency change request */
        HW_WR_REG32(BOARD_DDR_FSP_CLKCHNG_ACK_ADDR, 0x1);

        while(0x80U == (HW_RD_REG32(BOARD_DDR_FSP_CLKCHNG_REQ_ADDR) & 0x80U));

        /* Clear frequency change request acknowledge */
        HW_WR_REG32(BOARD_DDR_FSP_CLKCHNG_ACK_ADDR, 0x0);
    }

    BOARD_DEBUG_LOG("--->>> Frequency Change request handshake is completed... <<<---\n");
}

/**
 * \brief   Function to handle the configuration requests from DDR lib
 *
 * \return  None
 */
static void Board_DDRInfoHandler(const LPDDR4_PrivateData *pd, LPDDR4_InfoType infotype)
{
    if (LPDDR4_DRV_SOC_PLL_UPDATE == infotype)
    {
        Board_DDRChangeFreqAck();
    }
}

/**
 * \brief   DDR probe function
 *
 * \return  BOARD_SOK in case of success or appropriate error code
 */
static Board_STATUS Board_DDRProbe(void)
{
    uint32_t status = 0U;
    uint16_t configsize = 0U;

    status = LPDDR4_Probe(&gBoardDdrCfg, &configsize);

    if ((CDN_EOK != status) || (configsize != sizeof(LPDDR4_PrivateData)) ||
        (configsize > BOARD_DDR_SRAM_MAX))
    {
        BOARD_DEBUG_LOG("Board_DDRProbe: FAIL\n");
        return BOARD_FAIL;
    }
    else
    {
        BOARD_DEBUG_LOG("Board_DDRProbe: PASS\n");
    }

    return BOARD_SOK;
}

/**
 * \brief   DDR driver initialization function
 *
 * \return  BOARD_SOK in case of success or appropriate error code
 */
static Board_STATUS Board_DDRInitDrv(void)
{
    uint32_t status = 0U;

    if ((sizeof(gBoardDdrPd) != sizeof(LPDDR4_PrivateData)) ||
        (sizeof(gBoardDdrPd) > BOARD_DDR_SRAM_MAX))
    {
        BOARD_DEBUG_LOG("Board_DDRInitDrv: FAIL\n");
        return BOARD_FAIL;
    }

    gBoardDdrCfg.ctlBase = (struct LPDDR4_CtlRegs_s *)BOARD_DDR_CTL_CFG_BASE;
    gBoardDdrCfg.infoHandler = (LPDDR4_InfoCallback) Board_DDRInfoHandler;

    status = LPDDR4_Init(&gBoardDdrPd, &gBoardDdrCfg);

    if ((status > 0U) ||
        (gBoardDdrPd.ctlBase != (struct LPDDR4_CtlRegs_s *)gBoardDdrCfg.ctlBase) ||
        (gBoardDdrPd.ctlInterruptHandler != gBoardDdrCfg.ctlInterruptHandler) ||
        (gBoardDdrPd.phyIndepInterruptHandler != gBoardDdrCfg.phyIndepInterruptHandler))
    {
        BOARD_DEBUG_LOG("Board_DDRInitDrv: FAIL\n");
        return BOARD_FAIL;
    }
    else
    {
        BOARD_DEBUG_LOG("Board_DDRInitDrv: PASS\n");
    }

    return BOARD_SOK;
}

/**
 * \brief   DDR registers initialization function
 *
 * \return  BOARD_SOK in case of success or appropriate error code
 */
static Board_STATUS Board_DDRHWRegInit(void)
{
    uint32_t status = 0U;

    status = LPDDR4_WriteCtlConfig(&gBoardDdrPd,
                                            DDRSS_ctlReg,
                                            DDRSS_ctlRegNum,
                                            (uint16_t)DDRSS_CTL_REG_INIT_COUNT);
    if (!status)
    {
        status = LPDDR4_WritePhyIndepConfig(&gBoardDdrPd,
                                                     DDRSS_phyIndepReg,
                                                     DDRSS_phyIndepRegNum,
                                                     (uint16_t)DDRSS_PHY_INDEP_REG_INIT_COUNT);
    }

    if (!status)
    {
        status = LPDDR4_WritePhyConfig(&gBoardDdrPd,
                                                DDRSS_phyReg,
                                                DDRSS_phyRegNum,
                                                (uint16_t)DDRSS_PHY_REG_INIT_COUNT);
    }

    if (status)
    {
        BOARD_DEBUG_LOG(" ERROR: Board_DDRHWRegInit failed!!\n");
        return BOARD_FAIL;
    }

    return BOARD_SOK;
}

/**
 * \brief   DDR priority init function
 *
 * \return  BOARD_SOK in case of success or appropriate error code
 */

static void Board_DDRPriorityInit(void)
{
    /*
     * Apply the final DDRSS policy here, after LPDDR4_WriteCtlConfig().  The
     * generated table writes controller registers such as CTL_276, so an
     * earlier PRIORITY_EN write could be overwritten.  Keep the V2A maps in
     * the same final stage so there is a single authoritative configuration.
     *
     * Route IDs used at the DDR V2A ingress on J721E:
     *   A72 direct: 0x000, 0x001 and 0x004
     *   C7x direct: 0x00c
     *   DRU0:       0x068
     *   C66SS0 MDMA: 0x240
     *   C66SS1 MDMA: 0x242
     *
     * Priority 0 is highest and priority 7 is lowest.  Promote the C66x
     * memory masters and demote A72, C7x and DRU0.  Program both LPT and HPT
     * maps so the per-master priority is independent of the incoming thread.
     *
     * R1: mask A=1 matches RouteID 0x000-0x001; exact B matches 0x004.
     * R2: exact A/B matches RouteID 0x00c and 0x068.
     * R3: exact A/B matches RouteID 0x240 and 0x242.  R3 has the highest
     *     range-match precedence, although these ranges do not overlap.
     */
    const uint32_t priMapLinear = 0x01234567U;
    const uint32_t priMapHigh = 0x00000000U;
    const uint32_t priMapLow = 0x77777777U;
    const uint32_t routeMatchA72 = 0x90008004U;
    const uint32_t routeMatchC7xDru0 = 0x800C8068U;
    const uint32_t routeMatchC66x = 0x82408242U;
    uint32_t regVal;

    HW_WR_REG32(0x02980024U, routeMatchA72);    /* DDRSS_V2A_R1_MAT_REG */
    HW_WR_REG32(0x02980028U, routeMatchC7xDru0); /* DDRSS_V2A_R2_MAT_REG */
    HW_WR_REG32(0x0298002CU, routeMatchC66x);   /* DDRSS_V2A_R3_MAT_REG */

    HW_WR_REG32(0x02980030U, priMapLinear); /* LPT default */
    HW_WR_REG32(0x02980034U, priMapLow);    /* LPT R1: A72 */
    HW_WR_REG32(0x02980038U, priMapHigh);    /* LPT R2: C7x/DRU0 */
    HW_WR_REG32(0x0298003CU, 0x22222222U);   /* LPT R3: C66x */

    HW_WR_REG32(0x0298004CU, priMapLinear); /* HPT default */
    HW_WR_REG32(0x02980050U, priMapLow);    /* HPT R1: A72 */
    HW_WR_REG32(0x02980054U, priMapHigh);    /* HPT R2: C7x/DRU0 */
    HW_WR_REG32(0x02980058U, 0x22222222U);   /* HPT R3: C66x */

    /* Preserve the generated DDR scheduler settings and enable priority. */
    regVal = HW_RD_REG32(0x02990450U); /* DDRSS_CTL_276 */
    HW_WR_REG32(0x02990450U, regVal | 0x1U);

}
/**
 * \brief   DDR start function
 *
 * \return  BOARD_SOK in case of success or appropriate error code
 */
static Board_STATUS Board_DDRStart(void)
{
    uint32_t status = 0U;
    uint32_t regval = 0U;
    uint32_t offset = 0U;

    offset = BOARD_DDR_CTL_REG_OFFSET;

    status = LPDDR4_ReadReg(&gBoardDdrPd, LPDDR4_CTL_REGS, offset, &regval);
    if ((CDN_EOK < status) || (0U != (regval & 0x1U)))
    {
        BOARD_DEBUG_LOG("Board_DDRStart: FAIL\n");
        return BOARD_FAIL;
    }

    status = LPDDR4_Start(&gBoardDdrPd);
    if (CDN_EOK < status)
    {
        BOARD_DEBUG_LOG("Board_DDRStart: FAIL\n");
        return BOARD_FAIL;
    }

    status = LPDDR4_ReadReg(&gBoardDdrPd, LPDDR4_CTL_REGS, offset, &regval);
    if ((CDN_EOK < status) || (1U != (regval & 0x1U)))
    {
        BOARD_DEBUG_LOG("Board_DDRStart: FAIL\n");
        return BOARD_FAIL;
    }
    else
    {
        BOARD_DEBUG_LOG("LPDDR4_Start: PASS\n");
    }

    return BOARD_SOK;
}

int32_t DDR_clearAllECCError ()
{
    Board_STATUS   status    = BOARD_SOK;
    status = CSL_emifClearAllECCErrors((CSL_emif_sscfgRegs *)CSL_COMPUTE_CLUSTER0_SS_CFG_BASE);
    if (status == BOARD_SOK)
    {
        status = CSL_emifClearECCInterruptStatus((CSL_emif_sscfgRegs *)CSL_COMPUTE_CLUSTER0_SS_CFG_BASE,
                                                 CSL_EMIF_SSCFG_V2A_INT_SET_REG_ECC1BERR_EN_MASK
                                                     | CSL_EMIF_SSCFG_V2A_INT_SET_REG_ECCM1BERR_EN_MASK
                                                     | CSL_EMIF_SSCFG_V2A_INT_SET_REG_ECC2BERR_EN_MASK);
    }

    return status;
}

uintptr_t DDRGetTranslatedAddress(uintptr_t nonECCAddress);

#define DDR_ECC_REGION0_START  0x00000000
//#define DDR_ECC_REGION0_END    0x71C70000   //2G
#define DDR_ECC_REGION0_END    0xE38E0000   //4G
/**
 * \brief Configures DDR ECC
 *
 * Invokes EMIF CSL APIs to configure ECC and Primes the memory
 *
 * \return  BOARD_SOK in case of success or appropriate error code
 *
 */

uint8_t test_buffer[128U] __attribute__((aligned(128)));;

/* Refer EMIF ECC Configuration Section in TRM */
static Board_STATUS emif_ConfigureECC(void)
{
    Board_STATUS   status    = BOARD_SOK;
    int32_t        cslResult = CSL_PASS;
    CSL_EmifConfig emifCfg;

    BOARD_DEBUG_LOG("\r\n Configuring ECC");

    memset(&emifCfg, 0, sizeof(emifCfg));

    emifCfg.bEnableMemoryECC = BTRUE;
    emifCfg.bReadModifyWriteEnable = BTRUE;
    emifCfg.bECCCheck = BFALSE;
    emifCfg.bWriteAlloc = BTRUE;
    emifCfg.ECCThreshold = 1U;
    emifCfg.pMemEccCfg.startAddr[0] = DDR_ECC_REGION0_START;
    emifCfg.pMemEccCfg.endAddr[0] = DDR_ECC_REGION0_END;
    cslResult = CSL_emifConfig((CSL_emif_sscfgRegs *)CSL_COMPUTE_CLUSTER0_SS_CFG_BASE,
                               &emifCfg);

    if (CSL_PASS != cslResult)
    {
        BOARD_DEBUG_LOG("\r\n CSL_emifConfig Failed");
        status = BOARD_FAIL;
    }

    /* Prime the memory */
#ifdef BOARD_DDR_ENABLE_DDR_MEM_PRIME
    if ( BOARD_SOK == status )
    {
        status = BOARD_udmaPrimeDDR(BOARD_DDR_START_ADDR, BOARD_DDR_SIZE);

        status = BOARD_udmaPrimeDDR(0x880000000, DDR_ECC_REGION0_END - DDR_ECC_REGION0_START - BOARD_DDR_SIZE + 128);

        /* check DDR */
        memset(&test_buffer[0U], 0x0, 128);
        CacheP_wb(&test_buffer[0U], 128);
        BOARD_udmaCopy((uint64_t)&test_buffer[0U], 0x880000000, 128);
        CacheP_Inv(&test_buffer[0U], 128);

        memset(&test_buffer[0U], 0x0, 128);
        CacheP_wb(&test_buffer[0U], 128);
        BOARD_udmaCopy((uint64_t)&test_buffer[0U], 0x8a0000000, 128);
        CacheP_Inv(&test_buffer[0U], 128);

        memset(&test_buffer[0U], 0x0, 128);
        CacheP_wb(&test_buffer[0U], 128);
        BOARD_udmaCopy((uint64_t)&test_buffer[0U], 0x880000000 + DDR_ECC_REGION0_END - BOARD_DDR_SIZE - 128, 128);
        CacheP_Inv(&test_buffer[0U], 128);

        memset(&test_buffer[0U], 0x0, 128);
        CacheP_wb(&test_buffer[0U], 128);
        BOARD_udmaCopy((uint64_t)&test_buffer[0U], 0x880000000 + DDR_ECC_REGION0_END - BOARD_DDR_SIZE, 128);
        CacheP_Inv(&test_buffer[0U], 128);
    }
#else
    BOARD_DEBUG_LOG("\r\n DDR Memory is not primed (BOARD_DDR_ENABLE_DDR_MEM_PRIME is disabled)");
#endif
    if ( BOARD_SOK == status )
    {
        /* Clears ECC errors */
        status = DDR_clearAllECCError();

        /* open ecc check */
        if (status == BOARD_SOK)
        {
            uint32_t regVal = 0U;
            regVal = ((CSL_emif_sscfgRegs *)CSL_COMPUTE_CLUSTER0_SS_CFG_BASE)->ECC_CTRL_REG;
            regVal |= CSL_FMK(EMIF_SSCFG_ECC_CTRL_REG_ECC_CK, 1U);
            CSL_REG32_WR( &((CSL_emif_sscfgRegs *)CSL_COMPUTE_CLUSTER0_SS_CFG_BASE)->ECC_CTRL_REG, regVal );
        }
    }

    /*ECC test code, Don't Uncomment follow  easily.*/
//    ddr_ecc_test();

    return status;
}

/**
 * \brief DDR4 Initialization function
 *
 * Invokes DDR CSL APIs to configure the DDR timing parameters and ECC configuration
 *
 * \return  BOARD_SOK in case of success or appropriate error code
 *
 */
Board_STATUS Board_DDRInit(Bool eccEnable)
{
    Board_STATUS status = BOARD_SOK;

    /* Unlock the PLL register access for DDR clock bypass */
    HW_WR_REG32(BOARD_PLL12_LOCK0, KICK0_UNLOCK);
    HW_WR_REG32(BOARD_PLL12_LOCK1, KICK1_UNLOCK);

#ifdef BOARD_DDR_ENABLE_PLL_BYPASS
    /* Bypass PLL while configuring the DDR */
    Board_DDRSetPLLExtBypass();
#else
    /* Set to Boot Frequency(F0) while configuring the DDR */
    Board_DDRSetPLLClock(DDRSS_PLL_FREQUENCY_0);
#endif

    /* Partition5 lockkey0 */
    HW_WR_REG32(BOARD_CTRL_MMR_PART5_LOCK0, KICK0_UNLOCK);
    /* Partition5 lockkey1 */
    HW_WR_REG32(BOARD_CTRL_MMR_PART5_LOCK1, KICK1_UNLOCK);

    status = Board_DDRProbe();
    if(BOARD_SOK != status)
    {
        return status;
    }

    status = Board_DDRInitDrv();
    if(BOARD_SOK != status)
    {
        return status;
    }

    status = Board_DDRHWRegInit();
    if(BOARD_SOK != status)
    {
        return status;
    }

    Board_DDRPriorityInit();
    
    status = Board_DDRStart();
    if(BOARD_SOK != status)
    {
        return status;
    }

    if (UTRUE == eccEnable)
    {
         status = emif_ConfigureECC();
    }

    /* Lock the PLL registers access */
    HW_WR_REG32(BOARD_PLL12_LOCK0, KICK_LOCK);
    HW_WR_REG32(BOARD_PLL12_LOCK1, KICK_LOCK);

    return status;
}
