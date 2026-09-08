/**
 *  \file   sbl_main.c
 *
 *  \brief  This file contain main function, call the Board Initialization
 *          functions & slave core boot-up functions in sequence.
 *
 */

 /* TI RTOS header files */
#include "sbl_main.h"
#include <ti/csl/cslr_gtc.h>
#include "my_board_init.h"
#include "ecc_ddr.h"
#include "app_version.h"
#include "j721e_init.h"
#include "ti/boot/soc/k3/sbl_qos.h"
/**********************************************************************
 ************************** Global Variables **************************
 **********************************************************************/
extern sblProfileInfo_t sblProfileLog[MAX_PROFILE_LOG_ENTRIES];
extern uint32_t sblProfileLogIndx;
extern uint32_t sblProfileLogOvrFlw;

volatile sblProfileInfo_t * sblProfileLogAddr __attribute__((section(".sbl_profile_info")));
volatile uint32_t *sblProfileLogIndxAddr __attribute__((section(".sbl_profile_info")));
volatile uint32_t *sblProfileLogOvrFlwAddr __attribute__((section(".sbl_profile_info")));

sblEntryPoint_t k3xx_evmEntry;
const CSL_ArmR5MpuRegionCfg gCslR5MpuCfg[CSL_ARM_R5F_MPU_REGIONS_MAX] =
{
    {
        /* Region 0 configuration: complete 32 bit address space = 4Gbits */
        .regionId         = 0U,
        .enable           = 1U,
        .baseAddr         = 0x0U,
        .size             = CSL_ARM_R5_MPU_REGION_SIZE_4GB,
        .subRegionEnable  = CSL_ARM_R5_MPU_SUB_REGION_ENABLE_ALL,
        .exeNeverControl  = 1U,
        .accessPermission = CSL_ARM_R5_ACC_PERM_PRIV_USR_RD_WR,
        .shareable        = 0U,
        .cacheable        = (uint32_t)FALSE,
        .cachePolicy      = 0U,
        .memAttr          = 0U,
    },
    {
        /* Region 1 configuration: 128 bytes memory for exception vector execution */
        .regionId         = 1U,
        .enable           = 1U,
        .baseAddr         = 0x0U,
        .size             = CSL_ARM_R5_MPU_REGION_SIZE_32KB,
        .subRegionEnable  = CSL_ARM_R5_MPU_SUB_REGION_ENABLE_ALL,
        .exeNeverControl  = 0U,
        .accessPermission = CSL_ARM_R5_ACC_PERM_PRIV_USR_RD_WR,
        .shareable        = 0U,
        .cacheable        = (uint32_t)TRUE,
        .cachePolicy      = CSL_ARM_R5_CACHE_POLICY_NON_CACHEABLE,
        .memAttr          = 0U,
    },
    {
        /* Region 2 configuration: 1 MB OCMS RAM - Covers RAM sizes for multiple SoCs */
        .regionId         = 2U,
        .enable           = 1U,
        .baseAddr         = 0x41C00000,
        .size             = CSL_ARM_R5_MPU_REGION_SIZE_1MB,
        .subRegionEnable  = CSL_ARM_R5_MPU_SUB_REGION_ENABLE_ALL,
        .exeNeverControl  = 0U,
        .accessPermission = CSL_ARM_R5_ACC_PERM_PRIV_USR_RD_WR,
        .shareable        = 0U,
        .cacheable        = (uint32_t)TRUE,
        .cachePolicy      = CSL_ARM_R5_MEM_ATTR_CACHED_WT_NO_WA,
        .memAttr          = 0U,
    },
    {
        /* Region 3 configuration: MCMS3 RAM */
        .regionId         = 3U,
        .enable           = 1U,
        .baseAddr         = 0x70000000,
        .size             = CSL_ARM_R5_MPU_REGION_SIZE_8MB,
        .subRegionEnable  = CSL_ARM_R5_MPU_SUB_REGION_ENABLE_ALL,
        .exeNeverControl  = 0U,
        .accessPermission = CSL_ARM_R5_ACC_PERM_PRIV_USR_RD_WR,
        .shareable        = 0U,
        .cacheable        = (uint32_t)TRUE,
        .cachePolicy      = CSL_ARM_R5_MEM_ATTR_CACHED_WT_NO_WA,
        .memAttr          = 0U,
    },
    {
        /* Region 4 configuration: 2 GB DDR RAM */
        .regionId         = 4U,
        .enable           = 1U,
        .baseAddr         = 0x80000000,
        .size             = CSL_ARM_R5_MPU_REGION_SIZE_2GB,
        .subRegionEnable  = CSL_ARM_R5_MPU_SUB_REGION_ENABLE_ALL,
        .exeNeverControl  = 0U,
        .accessPermission = CSL_ARM_R5_ACC_PERM_PRIV_USR_RD_WR,
        .shareable        = 0U,
        .cacheable        = (uint32_t)TRUE,
        .cachePolicy      = CSL_ARM_R5_MEM_ATTR_CACHED_WT_NO_WA,
        .memAttr          = 0U,
    },
    {
        /* Region 5 configuration: 64 KB BTCM */
        .regionId         = 5U,
        .enable           = 1U,
        .baseAddr         = 0x41010000,
        .size             = CSL_ARM_R5_MPU_REGION_SIZE_32KB,
        .subRegionEnable  = CSL_ARM_R5_MPU_SUB_REGION_ENABLE_ALL,
        .exeNeverControl  = 0U,
        .accessPermission = CSL_ARM_R5_ACC_PERM_PRIV_USR_RD_WR,
        .shareable        = 0U,
        .cacheable        = (uint32_t)TRUE,
        .cachePolicy      = CSL_ARM_R5_CACHE_POLICY_NON_CACHEABLE,
        .memAttr          = 0U,
    },
    {
        /* Region 6 configuration: Covers first 64MB of EVM Flash (FSS DAT0) */
        .regionId         = 6U,
        .enable           = 1U,
        .baseAddr         = 0x50000000,
        .size             = CSL_ARM_R5_MPU_REGION_SIZE_64MB,
        .subRegionEnable  = CSL_ARM_R5_MPU_SUB_REGION_ENABLE_ALL,
        .exeNeverControl  = 0U,
        .accessPermission = CSL_ARM_R5_ACC_PERM_PRIV_USR_RD_WR,
        .shareable        = 0U,
        .cacheable        = (uint32_t)TRUE,
        .cachePolicy      = CSL_ARM_R5_CACHE_POLICY_WB_WA,
        .memAttr          = 0U,
    },
    {
        /* Region 14 configuration (Non-cached for PHY tuning data): Covers last 256KB of EVM Flash (FSS DAT0) */
        .regionId         = 7U,
        .enable           = 1U,
        .baseAddr         = 0x53FE0000,
        .size             = CSL_ARM_R5_MPU_REGION_SIZE_128KB,
        .subRegionEnable  = CSL_ARM_R5_MPU_SUB_REGION_ENABLE_ALL,
        .exeNeverControl  = 0U,
        .accessPermission = CSL_ARM_R5_ACC_PERM_PRIV_USR_RD_WR,
        .shareable        = 0U,
        /* OSPI PHY tuning algorithm which runs in DAC mode needs
         * cache to be disabled for this section of FSS data region.
         */
        .cacheable        = (uint32_t)FALSE,
        .cachePolicy      = 0U,
        .memAttr          = 0U,
    },
    {
        /* Region 15 configuration: 128 MB FSS DAT1 */
        .regionId         = 8U,
        .enable           = 1U,
        .baseAddr         = 0x58000000,
        .size             = CSL_ARM_R5_MPU_REGION_SIZE_128MB,
        .subRegionEnable  = CSL_ARM_R5_MPU_SUB_REGION_ENABLE_ALL,
        .exeNeverControl  = 0U,
        .accessPermission = CSL_ARM_R5_ACC_PERM_PRIV_USR_RD_WR,
        .shareable        = 0U,
        .cacheable        = (uint32_t)TRUE,
        .cachePolicy      = CSL_ARM_R5_MEM_ATTR_CACHED_WT_NO_WA,
        .memAttr          = 0U,
    },
    {
        /* Region 9 configuration: 512KB Main OCMRAM - no need for mapping */
        .regionId         = 9U,
        .enable           = 1U,
        .baseAddr         = 0x03600000,
        .size             = CSL_ARM_R5_MPU_REGION_SIZE_512KB,
        .subRegionEnable  = CSL_ARM_R5_MPU_SUB_REGION_ENABLE_ALL,
        .exeNeverControl  = 0U,
        .accessPermission = CSL_ARM_R5_ACC_PERM_PRIV_USR_RD_WR,
        .shareable        = 0U,
        .cacheable        = (uint32_t)TRUE,
        .cachePolicy      = CSL_ARM_R5_MEM_ATTR_CACHED_WT_NO_WA,
        .memAttr          = 0U,
    },
};

int main()
{
#if defined(SBL_ENABLE_HLOS_BOOT) && (defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4))
    cpu_core_id_t core_id;
#endif
    uint32_t atcm_size;
    SBL_ADD_PROFILE_POINT;

    uint32_t devGroup;
    devGroup = DEVGRP_ALL;
    Board_STATUS status;

    #define SBL_PLL_INIT                (BOARD_INIT_PLL)
    #define SBL_CLOCK_INIT              (BOARD_INIT_MODULE_CLOCK)

    /* Any SoC specific Init. */
    SBL_SocEarlyInit();

    if (SBL_LOG_LEVEL > SBL_LOG_NONE)
    {
        /* Configure UART Tx pinmux. */
        Board_uartTxPinmuxConfig();
    }

    if (SBL_LOG_LEVEL > SBL_LOG_NONE)
    {
        UART_HwAttrs uart_cfg;

        UART_socGetInitCfg(BOARD_UART_INSTANCE, &uart_cfg);
        /* Use UART fclk freq setup by ROM */
        uart_cfg.baseAddr = CSL_WKUP_UART0_BASE;
        uart_cfg.frequency = SBL_SYSFW_UART_MODULE_INPUT_CLK;
        uart_cfg.frequency = SBL_ROM_UART_MODULE_INPUT_CLK;
        /* Disable the UART interrupt */
        uart_cfg.enableInterrupt = FALSE;
        UART_socSetInitCfg(BOARD_UART_INSTANCE, &uart_cfg);
        /* Init UART for logging. */
        UART_stdioInit(BOARD_UART_INSTANCE);
        putchar_ = UART_putc;
    }

    /* Initialize the ATCM */
    atcm_size = sblAtcmSize();
    memset((void *)SBL_MCU_ATCM_BASE, 0xFF, atcm_size);

    /* Relocate CSL Vectors to ATCM*/
    memcpy((void *)SBL_MCU_ATCM_BASE, (void *)_resetvectors, 0x100);

#if defined(SBL_OCM_MAIN_DOMAIN_RAT)
    /* Setup RAT to load data into MCU2_0 OCM RAM for MCU1_0 */
    /* This is mapping the OCM RAM for MCU2_0 (a 40 bit address) Main domain to 0xD0000000 */
    SBL_log(SBL_LOG_MAX, "Initializing RAT ...");
#define RAT_BASE (0x40F90000)
#define REGION_ID (0x0)
    *(unsigned int *)(RAT_BASE + 0x44 + (REGION_ID*0x10)) = 0xD0000000; //IN ADDRESS
    *(unsigned int *)(RAT_BASE + 0x48 + (REGION_ID*0x10)) = 0x02000000;
    *(unsigned int *)(RAT_BASE + 0x4C + (REGION_ID*0x10)) = 0x0000004F; //Upper 16 bits of the real physical address.
    *(unsigned int *)(RAT_BASE + 0x40 + (REGION_ID*0x10)) = 0x80000013;
    SBL_log(SBL_LOG_MAX, "done.\n");
#endif

    // j721e_early_init();

    /* Load SYSFW. */
    SBL_SciClientInit(devGroup);
    printf_("\n%s build at 20%d-%d-%d - %s, APP VSC VER = 0x%08x\n",
            SBL_VERSION_STR, APP_BUILD_YEAR, APP_BUILD_MONTH, APP_BUILD_DAY,
            __TIME__, APP_VSC_VERSION);

#if !defined(SBL_SKIP_PINMUX_ENABLE)
    /* Board pinmux. */
    SBL_ADD_PROFILE_POINT;
    SBL_log(SBL_LOG_NONE, "Initlialzing PINMUX ...");
    status = Board_init(BOARD_INIT_PINMUX_CONFIG);
    if (status == BOARD_SOK)
    {
        SBL_log(SBL_LOG_NONE, "done.\n");
    }
    else
    {
        SBL_log(SBL_LOG_NONE, "faild.\n");
    }
#endif

#if !defined(SBL_SKIP_LATE_INIT)
    /* Any SoC specific Init. */
    SBL_SocLateInit();
#endif

#if defined(SBL_ENABLE_PLL) && !defined(SBL_SKIP_SYSFW_INIT)
    SBL_ADD_PROFILE_POINT;
    SBL_log(SBL_LOG_NONE, "Initlialzing PLLs ...");
    status = Board_init(SBL_PLL_INIT);
    if (status == BOARD_SOK)
    {
        SBL_log(SBL_LOG_NONE, "done.\n");
    }
    else
    {
        SBL_log(SBL_LOG_NONE, "faild.\n");
    }
#endif

#if defined(SBL_ENABLE_CLOCKS) && !defined(SBL_SKIP_SYSFW_INIT)
    SBL_log(SBL_LOG_NONE, "Initlialzing Clocks ...");
    SBL_ADD_PROFILE_POINT;
    status = Board_init(SBL_CLOCK_INIT);
    if (status == BOARD_SOK)
    {
        SBL_log(SBL_LOG_NONE, "done.\n");
    }
    else
    {
        SBL_log(SBL_LOG_NONE, "faild.\n");
    }
#endif

#if defined(SBL_ENABLE_DDR) && defined(SBL_ENABLE_PLL) && defined(SBL_ENABLE_CLOCKS)  && !defined(SBL_SKIP_SYSFW_INIT)
    SBL_log(SBL_LOG_NONE, "Initlialzing DDR @ %u MT/s ...", BOARD_DDR_RATE_MT_S);
    status = Board_init(BOARD_INIT_DDR | BOARD_INIT_DDR_ECC);
//    ecc_test();
    if (status == BOARD_SOK)
    {
        SBL_log(SBL_LOG_NONE, "done.\n");
#if defined(SOC_J721E) && !defined(SBL_USE_MCU_DOMAIN_ONLY)
        /*
         * MSMC_CFG (0x6e00xxxx) is not accessible from the MCU R5F SBL on
         * this platform even after DDR initialization; a direct access
         * stalls the boot CPU.  Keep this opt-in and apply MSMC starvation
         * bounds later from an authorized Linux context by default.
         */
#if defined(SBL_ENABLE_MSMC_QOS_FROM_MCU_R5)
        SBL_SetMSMCQoS();
#endif
#endif
    }
    else
    {
        SBL_log(SBL_LOG_NONE, "faild.\n");
    }
#endif

#if defined(SBL_ENABLE_SERDES)
    SBL_log(SBL_LOG_NONE, "Initlialzing Serdes ...");
    main_mmr_unlock_all();
    pcie_set_mode(0, PCIE_GEN3, PCIE_RC, PCIE_X2);
    pcie_set_mode(1, PCIE_GEN3, PCIE_RC, PCIE_X2);
    pcie_set_mode(2, PCIE_GEN3, PCIE_RC, PCIE_X2);
    pcie_set_mode(3, PCIE_GEN3, PCIE_RC, PCIE_X2);
    pcie_refclk_to_io(0, 2);
    pcie_refclk_to_io(1, 2);
    pcie_refclk_to_io(2, 2);
    pcie_refclk_to_io(3, 2);
    status = Board_init(BOARD_INIT_SERDES_PHY);
    if (status == BOARD_SOK)
    {
        SBL_log(SBL_LOG_NONE, "done.\n");
    }
    else
    {
        SBL_log(SBL_LOG_NONE, "faild.\n");
    }
#endif

    /* Enable GTC */
    SBL_log(SBL_LOG_NONE, "Initializing GTC ... done.\n");
    volatile uint32_t *gtcRegister = (uint32_t *) CSL_GTC0_GTC_CFG1_BASE;
    *gtcRegister = *gtcRegister | CSL_GTC_CFG1_CNTCR_EN_MASK | CSL_GTC_CFG1_CNTCR_HDBG_MASK;

    enable_sd_power();
    // gpio_intr_init();
    intr_router_cfg();
    SBL_log(SBL_LOG_NONE, "Begin parsing user application\n");

    /* Boot all non-SBL cores in multi-core app image */
    SBL_BootImage(&k3xx_evmEntry);
    SBL_ADD_PROFILE_POINT;
    /* Export SBL logs */
    sblProfileLogAddr = sblProfileLog;
    sblProfileLogIndxAddr = &sblProfileLogIndx;
    sblProfileLogOvrFlwAddr = &sblProfileLogOvrFlw;

#if defined(SBL_ENABLE_HLOS_BOOT) && (defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4))
    /* For J721E/J7200/J721S2/J784S4 we have to manage all core boots at the end, to load mcu1_0 sciserver app */
    /* Boot the HLOS on the Cortex-A cores towards the end */
    for(core_id = MPU1_CPU0_ID; core_id <= MPU2_CPU3_ID; core_id ++)
    {
        /* Try booting all cores other than the cluster running the SBL */
        if (k3xx_evmEntry.CpuEntryPoint[core_id] != SBL_INVALID_ENTRY_ADDR)
            SBL_SlaveCoreBoot(core_id, (uint32_t)NULL, &k3xx_evmEntry, SBL_REQUEST_CORE);
    }

    for(core_id = MCU2_CPU0_ID; core_id <= SBL_LAST_CORE_ID; core_id ++)
    {
        /* Try booting all MAIN domain cores except the Cortex-A cores */
        if (k3xx_evmEntry.CpuEntryPoint[core_id] != SBL_INVALID_ENTRY_ADDR)
            SBL_SlaveCoreBoot(core_id, (uint32_t)NULL, &k3xx_evmEntry, SBL_REQUEST_CORE);
    }

#endif

    SBL_ADD_PROFILE_POINT;

    /* Boot the core running SBL in the end */
    SBL_log(SBL_LOG_NONE, "booting mcu1_0 and mcu1_1\n");
    if ((k3xx_evmEntry.CpuEntryPoint[MCU1_CPU1_ID] != SBL_INVALID_ENTRY_ADDR) ||
        (k3xx_evmEntry.CpuEntryPoint[MCU1_CPU0_ID] < SBL_INVALID_ENTRY_ADDR))
    {
        SBL_SlaveCoreBoot(MCU1_CPU0_ID, 0, &k3xx_evmEntry, SBL_REQUEST_CORE);
        SBL_SlaveCoreBoot(MCU1_CPU1_ID, 0, &k3xx_evmEntry, SBL_REQUEST_CORE);
    }

    /* Execute a WFI */
    asm volatile (" wfi");

    return 0;
}
