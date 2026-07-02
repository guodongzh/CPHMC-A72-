/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       app_Fat.c
 *@author     jinyangh
 *@date       06.06
 *@brief      Provide file table configuration.
 *@par        History
 *Date        Version   Author     Description
 *2025.06.06  1.0       jinyangh   first version
 *2025.09.18  1.1       liurui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "file_system.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
FILE_FAT_TABLE_STRUCT File_Fat_Table = {
    .Fat_flag = 0x87654321,        //
    .AppType = "BM_Linux_CSD601",  // 应用标识符，可区分不同系统使用的 FAT 表

    .File_Fat =
        {
            {
                .Name = "boot.cfg",
                .Sector_ID = 0,
                .Sector_Num = 8,
                .Flash_Pro = FLASH_PRO_CPU_DATA,
            },
            {
                .Name = "irigb_ip.cfg",
                .Sector_ID = 8,
                .Sector_Num = 8,
                .Flash_Pro = FLASH_PRO_CPU_DATA,
            },
            {
                // 二级boot
                .Name = "sbl.tiimage",
                .Sector_ID = 0,
                .Sector_Num = 128,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // 二级boot
                .Name = "sbl_null.tiimage",
                .Sector_ID = 0,
                .Sector_Num = 128,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // m3固件
                .Name = "ti.fs",
                .Sector_ID = 128,
                .Sector_Num = 128,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // 三级boot程序
                .Name = "boot_app.appimage.hs_fs",
                .Sector_ID = 256,
                .Sector_Num = 128,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // 三级boot程序
                .Name = "boot_app_null.appimage.hs_fs",
                .Sector_ID = 256,
                .Sector_Num = 128,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                .Name = "atf_optee_spl.appimage.hs_fs",
                .Sector_ID = 384,
                .Sector_Num = 256,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // Core0 核程序
                .Name = "R5F0.appimage.hs_fs",
                .Sector_ID = 640,
                .Sector_Num = 256,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // Core1 核程序
                .Name = "R5F1.appimage.hs_fs",
                .Sector_ID = 896,
                .Sector_Num = 256,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // Core2 核程序
                .Name = "R5F2.appimage.hs_fs",
                .Sector_ID = 1152,
                .Sector_Num = 256,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // Core3 核程序
                .Name = "R5F3.appimage.hs_fs",
                .Sector_ID = 1408,
                .Sector_Num = 256,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // Core4 核程序
                .Name = "DSPC6X0.appimage.hs_fs",
                .Sector_ID = 1664,
                .Sector_Num = 256,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // Core5 核程序
                .Name = "DSPC6X1.appimage.hs_fs",
                .Sector_ID = 1920,
                .Sector_Num = 256,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                // Core6 核程序
                .Name = "DSPC7X0.appimage.hs_fs",
                .Sector_ID = 2176,
                .Sector_Num = 256,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                .Name = "all_cores.appimage.hs_fs",
                .Sector_ID = 2432,
                .Sector_Num = 512,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                .Name = "core0_null.appimage.hs_fs",
                .Sector_ID = 3200,
                .Sector_Num = 2,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                .Name = "core1_null.appimage.hs_fs",
                .Sector_ID = 3202,
                .Sector_Num = 2,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                .Name = "core2_null.appimage.hs_fs",
                .Sector_ID = 3204,
                .Sector_Num = 2,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                .Name = "core3_null.appimage.hs_fs",
                .Sector_ID = 3206,
                .Sector_Num = 2,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                .Name = "core4_null.appimage.hs_fs",
                .Sector_ID = 3208,
                .Sector_Num = 2,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                .Name = "core5_null.appimage.hs_fs",
                .Sector_ID = 3210,
                .Sector_Num = 2,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                .Name = "core6_null.appimage.hs_fs",
                .Sector_ID = 3212,
                .Sector_Num = 2,
                .Flash_Pro = FLASH_PRO_CPU_IMAGE,
            },
            {
                .Name = "cphmc_top.bin",
                .Sector_ID = 0,
                .Sector_Num = 2560,
                .Flash_Pro = FLASH_PRO_FPGA_IMAGE,
            },
            {
                /*must be last*/
                .Name = "FAT_END",
            },
        },
};
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
