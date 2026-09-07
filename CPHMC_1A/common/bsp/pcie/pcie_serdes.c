#include "pcie_init.h"
#include "pcie_serdes.h"
#include <ti/csl/csl_serdes.h>
#include <ti/csl/csl_serdes_pcie.h>
#include <debug_config.h>

/* define the unlock and lock values */
#define KICK0_UNLOCK_VAL        0x68EF3490
#define KICK1_UNLOCK_VAL        0xD172BC5A
#define KICK_LOCK_VAL           0x00000000

#define MAIN_MMR_BASE_ADDRESS   CSL_CTRL_MMR0_CFG0_BASE

#define MAIN_CTRL_ACSPCIE0_CTRL (0x18090)
#define MAIN_CTRL_ACSPCIE1_CTRL (0x18094)

void pcie_refclk_to_io(uint32_t ints_num, uint32_t ref_clk)
{
    switch (ints_num)
    {
    case 0:
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK0_CLKSEL) = ref_clk;
        /* Enable output clock */
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK0_CLKSEL) |= 0x100;
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + MAIN_CTRL_ACSPCIE0_CTRL) &= 0xFFFFFFFC;
        break;
    case 1:
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK1_CLKSEL) = ref_clk;
        /* Enable output clock */
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK1_CLKSEL) |= 0x100;
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + MAIN_CTRL_ACSPCIE0_CTRL) &= 0xFFFFFFFC;
        break;
    case 2:
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK2_CLKSEL) = ref_clk;
        /* Enable output clock */
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK2_CLKSEL) |= 0x100;
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + MAIN_CTRL_ACSPCIE1_CTRL) &= 0xFFFFFFFC;
        break;
    case 3:
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK3_CLKSEL) = ref_clk;
        /* Enable output clock */
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK3_CLKSEL) |= 0x100;
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + MAIN_CTRL_ACSPCIE1_CTRL) &= 0xFFFFFFFC;
        break;
    default:
        break;
    }
}

void pcie_set_mode(uint32_t ints_num, uint32_t rate,
                   uint32_t mode, uint32_t lane_count)
{
    switch (ints_num)
    {
    case 0:
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE0_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_GENERATION_SEL, 2);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE0_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_MODE_SEL, 1);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE0_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_LANE_COUNT, 1);
        break;
    case 1:
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE1_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_GENERATION_SEL, 2);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE1_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_MODE_SEL, 1);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE1_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_LANE_COUNT, 1);
        break;
    case 2:
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE2_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_GENERATION_SEL, 2);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE2_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_MODE_SEL, 1);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE2_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_LANE_COUNT, 1);
        break;
    case 3:
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE3_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_GENERATION_SEL, 2);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE3_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_MODE_SEL, 1);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE3_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_LANE_COUNT, 1);
        break;
    default:
        break;
    }
}

uint32_t mmr_unlock_one(uint32_t *kick0, uint32_t *kick1)
{
    /* initialize the status variable */
    uint32_t status = 1;

    /* if either of the kick lock registers are locked */
    if (!(*kick0 & 0x1) | !(*kick1 & 0x1))
    {
        /* unlock the partition by writing the unlock values to the kick lock registers */
        *kick0 = KICK0_UNLOCK_VAL;
        *kick1 = KICK1_UNLOCK_VAL;
    }

    /* check to see if either of the kick registers are unlocked. */
    if (!(*kick0 & 0x1))
    {
        status = 0;
    }

    /* return the status to the calling program */
    return status;
}

uint32_t mmr_lock_one(uint32_t *kick0, uint32_t *kick1)
{
    /* create status return variable */
    uint32_t status = 1;

    /* check to see if either of the kick registers are unlocked. */
    if ((*kick0 & 0x1))
    {
        /* write the kick lock value to the kick lock registers to lock the partition */
        *kick0 = KICK_LOCK_VAL;
        *kick1 = KICK_LOCK_VAL;
    }

    /* check to see if either of the kick registers are still unlocked. */
    if ((*kick0 & 0x1))
    {
        status = 0;
    }
    /* return success or failure */
    return status;
}

uint32_t main_mmr_unlock_all()
{
    /* initialize the status variable */
    uint32_t status = 1;
    /* Unlock the 0th partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK0_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK0_KICK1));
    /* Unlock the 1st partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK1_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK1_KICK1));
    /* Unlock the 2nd partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK2_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK2_KICK1));
    /* Unlock the 3rd partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK3_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK3_KICK1));
    /* Unlock the 4th partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK4_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK4_KICK1));
    /* Unlock the 5th partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK5_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK5_KICK1));
    /* Unlock the 6th partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK6_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK6_KICK1));

    /* Unlock the 7th partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK7_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK7_KICK1));
    /* Return status to calling program */
    return status;
}

uint32_t main_mmr_lock_all()
{
    /* initialize the status variable */
    uint32_t status = 1;
    /* Unlock the 0th partition */
    status &= mmr_lock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK0_KICK0),
                           (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK0_KICK1));
    /* Unlock the 1st partition */
    status &= mmr_lock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK1_KICK0),
                           (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK1_KICK1));
    /* Unlock the 2nd partition */
    status &= mmr_lock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK2_KICK0),
                           (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK2_KICK1));
    /* Unlock the 3rd partition */
    status &= mmr_lock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK3_KICK0),
                           (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK3_KICK1));
    /* Unlock the 4th partition */
    status &= mmr_lock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK4_KICK0),
                           (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK4_KICK1));

    status &= mmr_lock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK5_KICK0),
                           (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK5_KICK1));
    status &= mmr_lock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK6_KICK0),
                           (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK6_KICK1));

    /* Unlock the 7th partition */
    status &= mmr_lock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK7_KICK0),
                           (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK7_KICK1));
    /* Return status to calling program */
    return status;
}

uint32_t serdes_init(uint8_t serdesInstance, CSL_SerdesSSCMode SSC_Mode)
{
    CSL_SerdesResult           status;
    uint32_t                   i, laneNum;
    CSL_SerdesLaneEnableParams serdesLaneEnableParams;
    CSL_SerdesLaneEnableStatus laneRetVal = CSL_SERDES_LANE_ENABLE_NO_ERR;

    if (serdesInstance > 3)
    {
        PCIE_log("Invalid Serdes!\n");
        return 1;
    }

    memset(&serdesLaneEnableParams, 0, sizeof(serdesLaneEnableParams));
    serdesLaneEnableParams.serdesInstance = (CSL_SerdesInstance)serdesInstance;

    switch (serdesInstance)
    {
    case 0:
        serdesLaneEnableParams.baseAddr = CSL_SERDES_16G0_BASE;
        break;
    case 1:
        serdesLaneEnableParams.baseAddr = CSL_SERDES_16G1_BASE;
        break;
    case 2:
        serdesLaneEnableParams.baseAddr = CSL_SERDES_16G2_BASE;
        break;
    case 3:
        serdesLaneEnableParams.baseAddr = CSL_SERDES_16G3_BASE;
        break;
    }

    serdesLaneEnableParams.refClock = CSL_SERDES_REF_CLOCK_100M;
    serdesLaneEnableParams.refClkSrc = CSL_SERDES_REF_CLOCK_INT;
    serdesLaneEnableParams.linkRate = CSL_SERDES_LINK_RATE_8G;
    serdesLaneEnableParams.numLanes = 0x2;
    serdesLaneEnableParams.laneMask = 0x3;
    serdesLaneEnableParams.SSC_mode = SSC_Mode;
    serdesLaneEnableParams.phyType = CSL_SERDES_PHY_TYPE_PCIe;
    serdesLaneEnableParams.pcieGenType = PCIE_GEN3;
    serdesLaneEnableParams.operatingMode = CSL_SERDES_FUNCTIONAL_MODE;
    serdesLaneEnableParams.phyInstanceNum = serdesInstance;
    for (i = 0; i < serdesLaneEnableParams.numLanes; i++)
    {
        serdesLaneEnableParams.laneCtrlRate[i] = CSL_SERDES_LANE_FULL_RATE;
        /* still have to change to correct loopback mode */
        serdesLaneEnableParams.loopbackMode[i] = CSL_SERDES_LOOPBACK_DISABLED;
    }

    CSL_serdesPorReset(serdesLaneEnableParams.baseAddr);

    /* Select the IP type, IP instance num, Serdes Lane Number */
    for (laneNum = 0; laneNum < serdesLaneEnableParams.numLanes; laneNum++)
    {
        CSL_serdesIPSelect(CSL_CTRL_MMR0_CFG0_BASE,
                           serdesLaneEnableParams.phyType,
                           serdesLaneEnableParams.phyInstanceNum,
                           serdesLaneEnableParams.serdesInstance,
                           laneNum);
    }

    /* selects the appropriate clocks for all serdes based on the protocol chosen */
    status = CSL_serdesRefclkSel(CSL_CTRL_MMR0_CFG0_BASE,
                                 serdesLaneEnableParams.baseAddr,
                                 serdesLaneEnableParams.refClock,
                                 serdesLaneEnableParams.refClkSrc,
                                 serdesLaneEnableParams.serdesInstance,
                                 serdesLaneEnableParams.phyType);

    if (status != CSL_SERDES_NO_ERR)
    {
        PCIE_log("Invalid SERDES Init Params, status=%d\n", status);
        return 1;
    }

    /* Assert PHY reset and disable all lanes */
    CSL_serdesDisablePllAndLanes(serdesLaneEnableParams.baseAddr,
                                 serdesLaneEnableParams.numLanes,
                                 serdesLaneEnableParams.laneMask);

    /*Load the Serdes Config File */
    status = CSL_serdesPCIeInit(&serdesLaneEnableParams); /* Use this for PCIe serdes config load */

    /* Return error if input params are invalid */
    if (status != CSL_SERDES_NO_ERR)
    {
        PCIE_log("Invalid SERDES Init Params, status=%d\n", status);
    }

    /* Set this to standard mode defined by Cadence */
    for (laneNum = 0; laneNum < serdesLaneEnableParams.numLanes; laneNum++)
    {
        CSL_serdesPCIeModeSelect(
            serdesLaneEnableParams.baseAddr, serdesLaneEnableParams.pcieGenType, laneNum);
    }

    /* Common Lane Enable API for lane enable, pll enable etc */
    laneRetVal = CSL_serdesLaneEnable(&serdesLaneEnableParams);

    if (laneRetVal != 0)
    {
        PCIE_log("Invalid Serdes Lane Enable\n");
        return 2;
    }

    PCIE_log("Serdes %d Init Complete\n", serdesInstance);
    return 0;
}

void pcie_serdes_cfg(int32_t serdes)
{
    main_mmr_unlock_all();

    serdes_init(serdes, CSL_SERDES_NO_SSC);

    /*Wait for PLL to lock (3000 CLKIN1 cycles) */
    volatile uint32_t delay = 10000;
    while (delay > 10000)
    {
    }

    pcie_set_mode(serdes, PCIE_GEN3, PCIE_RC, PCIE_X2);
    pcie_refclk_to_io(serdes, 2);
}
