/**
 *  \file     boot_core_defs.c
 *
 *  \brief    This file defines available main domain slave cores and order of booting
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "ti/boot/src/rprc/sbl_rprc_parse.h"
#include "ti/boot/soc/k3/sbl_soc_cfg.h"
#include "boot_core_defs.h"

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* Definition of available Main Domain cores that can be booted
 * by the sample application for J721E SOC */

const sblSlaveCoreInfo_t sbl_late_slave_core_stages_info[NUM_BOOT_STAGES][MAX_CORES_PER_STAGE] = {
    {
        /* MCU2_CPU0 info */
        {
            SBL_PROC_ID_MCU2_CPU0,
            SBL_DEV_ID_MCU2_CPU0,
            SBL_CLK_ID_MCU2_CPU0,
            SBL_MCU2_CPU0_FREQ_HZ,
        },
        /* MCU2_CPU1 info */
        {
            SBL_PROC_ID_MCU2_CPU1,
            SBL_DEV_ID_MCU2_CPU1,
            SBL_CLK_ID_MCU2_CPU1,
            SBL_MCU2_CPU1_FREQ_HZ,
        },
        /* MCU3_CPU0 info */
        {
            SBL_PROC_ID_MCU3_CPU0,
            SBL_DEV_ID_MCU3_CPU0,
            SBL_CLK_ID_MCU3_CPU0,
            SBL_MCU3_CPU0_FREQ_HZ,
        },
        /* MCU3_CPU1 info */
        {
            SBL_PROC_ID_MCU3_CPU1,
            SBL_DEV_ID_MCU3_CPU1,
            SBL_CLK_ID_MCU3_CPU1,
            SBL_MCU3_CPU1_FREQ_HZ,
        },
        /* DSP1_C66X info */
        {
            SBL_PROC_ID_DSP1_C66X,
            SBL_DEV_ID_DSP1_C66X,
            SBL_CLK_ID_DSP1_C66X,
            SBL_DSP1_C66X_FREQ_HZ,
        },
        /* DSP2_C66X info */
        {
            SBL_PROC_ID_DSP2_C66X,
            SBL_DEV_ID_DSP2_C66X,
            SBL_CLK_ID_DSP2_C66X,
            SBL_DSP2_C66X_FREQ_HZ,
        },
        /* DSP1_C7X info */
        {
            SBL_PROC_ID_DSP1_C7X,
            SBL_DEV_ID_DSP1_C7X,
            SBL_CLK_ID_DSP1_C7X,
            SBL_DSP1_C7X_FREQ_HZ,
        },
    },
    {
        /* MPU1_CPU0 info */
        {
            SBL_PROC_ID_MPU1_CPU0,
            SBL_DEV_ID_MPU1_CPU0,
            SBL_CLK_ID_MPU1_CPU0,
            SBL_MPU1_CPU0_FREQ_HZ,
        },
        {
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
        },
        {
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
        },
        {
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
        },
        {
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
        },
        {
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
        },
        {
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
            SBL_INVALID_ID,
        },
    },
};

cpu_core_id_t boot_array[6] = {
    MCU2_CPU0_ID,
    MCU2_CPU1_ID,
    MCU3_CPU0_ID,
    MCU3_CPU1_ID,
    DSP1_C7X_ID,
    MPU1_CPU0_ID,
};

/*image flash addr*/
uint32_t main_boot_flash_images[NUM_BOOT_STAGES][MAX_CORES_PER_STAGE] = {
    {
        CORE0_APPS_FLASH_ADDR,
        CORE1_APPS_FLASH_ADDR,
        CORE2_APPS_FLASH_ADDR,
        CORE3_APPS_FLASH_ADDR,
        CORE4_APPS_FLASH_ADDR,
        CORE5_APPS_FLASH_ADDR,
        CORE6_APPS_FLASH_ADDR,
    },
    {
        A72_APP_FLASH_ADDR,
        0,
        0,
        0,
        0,
        0,
        0,
    }};

uint32_t debug_boot_flash_images[NUM_BOOT_STAGES][MAX_CORES_PER_STAGE] = {
    {
        CORE0_APPS_NULL_FLASH_ADDR,
        CORE1_APPS_NULL_FLASH_ADDR,
        CORE2_APPS_NULL_FLASH_ADDR,
        CORE3_APPS_NULL_FLASH_ADDR,
        CORE4_APPS_NULL_FLASH_ADDR,
        CORE5_APPS_NULL_FLASH_ADDR,
        CORE6_APPS_NULL_FLASH_ADDR,
    },
    {
        A72_APP_FLASH_ADDR,
        0,
        0,
        0,
        0,
        0,
        0,
    }};

uint32_t back_up_boot_flash_images[NUM_BOOT_STAGES][MAX_CORES_PER_STAGE] = {
    {
        ALL_CORES_APPS_FLASH_ADDR,
        0,
        0,
        0,
        0,
        0,
        0,
    },
    {
        A72_APP_FLASH_ADDR,
        0,
        0,
        0,
        0,
        0,
        0,
    }};

/*
 * Mode 3 loads the same combined debug/NULL image used by mode 2 for the
 * non-A72 cores. Stage 1 deliberately contains no A72 image so CCS/JTAG can
 * connect, load A72_BareMetal.elf and start it independently.
 */
uint32_t a72_noos_boot_flash_images[NUM_BOOT_STAGES][MAX_CORES_PER_STAGE] = {
    {
        ALL_CORES_APPS_NUL_FLASH_ADDR,
        0,
        0,
        0,
        0,
        0,
        0,
    },
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    }};
