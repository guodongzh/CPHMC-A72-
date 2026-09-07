/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       bsp_init.c
 *@author     LiuRui
 *@date       2025.09.23
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.09.23  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#ifndef BUILD_MCU1_0

#include <ti/osal/CacheP.h>
#include <platform.h>
#include <bsp_lib_version.h>
#include <squ_mod.h>
#include "bsp_init.h"
#include "ft3_data.h"
#include "app_version.h"
#include "file_system.h"
#include "ipc_data_drv.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
init_flag_t init_flag __attribute__((aligned(128), section(".init_flag")));
platform_ver_t g_platform_ver_shm __attribute__((aligned(128), section(".version"), used));
platform_ver_t g_platform_ver = {0};

const char MAINCTRL_CORE0_PROG_VER_ASC[32] = "D";			 // core0非标版本
const char MAINCTRL_CORE1_PROG_VER_ASC[32] = "D";			 // core1非标版本
const char MAINCTRL_CORE2_PROG_VER_ASC[32] = "D";			 // core2非标版本
const char MAINCTRL_CORE3_PROG_VER_ASC[32] = "D";			 // core3非标版本
const char MAINCTRL_CORE4_PROG_VER_ASC[32] = "D";			 // core4非标版本
const char MAINCTRL_CORE5_PROG_VER_ASC[32] = "D";			 // core5非标版本
const char MAINCTRL_CORE6_PROG_VER_ASC[32] = "D";			 // core6非标版本
const char MAINCTRL_CORE7_PROG_VER_ASC[32] = "D";			 // core7非标版本
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
static void load_image_and_cfg_crc(void)
{
    const char image_name[8][32] = {
        {"R5F0.appimage.hs_fs"},
        {"R5F1.appimage.hs_fs"},
        {"R5F2.appimage.hs_fs"},
        {"R5F3.appimage.hs_fs"},
        {"DSPC6X0.appimage.hs_fs"},
        {"DSPC6X1.appimage.hs_fs"},
        {"DSPC7X0.appimage.hs_fs"},
        {"MCU1.appimage.hs_fs"},
    };
    int32_t fileHandle = 0;
    uint8_t retval = 0;

    fileHandle = file_open("irigb_ip.cfg", FAT_MODE_R_OPEN);
    FILE_INFO r0_cfg_info = {0};
    retval = file_info_read(fileHandle, &r0_cfg_info);
    if (retval == 1)
    {
        g_platform_ver.cfg_crc[0] = r0_cfg_info.File_Sum;
    }
    else
    {
        g_platform_ver.cfg_crc[0] = 0xFFFF;
    }
    file_close(fileHandle, 0);

    for (size_t i = 0; i < sizeof(image_name)/sizeof(image_name[0]); i++)
    {
        fileHandle = file_open(image_name[i], FAT_MODE_R_OPEN);
        FILE_INFO image_info = {0};
        retval = file_info_read(fileHandle, &image_info);
        if (retval == 1)
        {
            g_platform_ver.core_image_crc[i] = image_info.File_Sum;
        }
        else
        {
            g_platform_ver.core_image_crc[i] = 0xFFFF;
        }
        file_close(fileHandle, 0);
    }
}

void init_core_softver(void)
{
#if defined(BUILD_MCU2_0)
    sscanf(HW_VER, "%hhu.%hhu", &g_platform_ver.hw_ver[0], &g_platform_ver.hw_ver[1]);
    g_platform_ver.cpu_version.low_lib_ver = APP_VSC_VERSION;
    g_platform_ver.cpu_version.app_ver[0]  = APP_VSC_VERSION;
    g_platform_ver.core_ver[0].ver_high    = MAINCTRL_CORE0_PROG_VER_H;
    g_platform_ver.core_ver[0].ver_low     = MAINCTRL_CORE0_PROG_VER_L;
    memcpy(g_platform_ver.ver_asc[0], MAINCTRL_CORE0_PROG_VER_ASC, sizeof(MAINCTRL_CORE0_PROG_VER_ASC));
    g_platform_ver.buildtime[0].year       = APP_BUILD_YEAR;
    g_platform_ver.buildtime[0].month      = APP_BUILD_MONTH;
    g_platform_ver.buildtime[0].day        = APP_BUILD_DAY;

    load_image_and_cfg_crc();

    sscanf(FW_RELEASE_R0_VER, "%hhu.%hhu.%hhu.%hhu", &g_platform_ver.cpu_fw_ver[0].major[0], &g_platform_ver.cpu_fw_ver[0].major[1],
           &g_platform_ver.cpu_fw_ver[0].minor[0], &g_platform_ver.cpu_fw_ver[0].minor[1]);
    sscanf(FW_RELEASE_R1_VER, "%hhu.%hhu.%hhu.%hhu", &g_platform_ver.cpu_fw_ver[1].major[0], &g_platform_ver.cpu_fw_ver[1].major[1],
           &g_platform_ver.cpu_fw_ver[1].minor[0], &g_platform_ver.cpu_fw_ver[1].minor[1]);
    sscanf(FW_RELEASE_R2_VER, "%hhu.%hhu.%hhu.%hhu", &g_platform_ver.cpu_fw_ver[2].major[0], &g_platform_ver.cpu_fw_ver[2].major[1],
           &g_platform_ver.cpu_fw_ver[2].minor[0], &g_platform_ver.cpu_fw_ver[2].minor[1]);
    sscanf(FW_RELEASE_R3_VER, "%hhu.%hhu.%hhu.%hhu", &g_platform_ver.cpu_fw_ver[3].major[0], &g_platform_ver.cpu_fw_ver[3].major[1],
           &g_platform_ver.cpu_fw_ver[3].minor[0], &g_platform_ver.cpu_fw_ver[3].minor[1]);
    sscanf(FW_RELEASE_C6X0_VER, "%hhu.%hhu.%hhu.%hhu", &g_platform_ver.cpu_fw_ver[4].major[0], &g_platform_ver.cpu_fw_ver[4].major[1],
           &g_platform_ver.cpu_fw_ver[4].minor[0], &g_platform_ver.cpu_fw_ver[4].minor[1]);
    sscanf(FW_RELEASE_C6X1_VER, "%hhu.%hhu.%hhu.%hhu", &g_platform_ver.cpu_fw_ver[5].major[0], &g_platform_ver.cpu_fw_ver[5].major[1],
           &g_platform_ver.cpu_fw_ver[5].minor[0], &g_platform_ver.cpu_fw_ver[5].minor[1]);
    sscanf(FW_RELEASE_C7X0_VER, "%hhu.%hhu.%hhu.%hhu", &g_platform_ver.cpu_fw_ver[6].major[0], &g_platform_ver.cpu_fw_ver[6].major[1],
           &g_platform_ver.cpu_fw_ver[6].minor[0], &g_platform_ver.cpu_fw_ver[6].minor[1]);
    sscanf(FW_RELEASE_MCU1_VER, "%hhu.%hhu.%hhu.%hhu", &g_platform_ver.cpu_fw_ver[7].major[0], &g_platform_ver.cpu_fw_ver[7].major[1],
           &g_platform_ver.cpu_fw_ver[7].minor[0], &g_platform_ver.cpu_fw_ver[7].minor[1]);
    g_platform_ver.versionflag |= 0x01;

#elif defined(BUILD_MCU2_1)
    g_platform_ver.cpu_version.app_ver[1]  = APP_VSC_VERSION;
    g_platform_ver.core_ver[1].ver_high    = MAINCTRL_CORE1_PROG_VER_H;
    g_platform_ver.core_ver[1].ver_low     = MAINCTRL_CORE1_PROG_VER_L;
    memcpy(g_platform_ver.ver_asc[1], MAINCTRL_CORE1_PROG_VER_ASC, sizeof(MAINCTRL_CORE1_PROG_VER_ASC));
    g_platform_ver.buildtime[1].year       = APP_BUILD_YEAR;
    g_platform_ver.buildtime[1].month      = APP_BUILD_MONTH;
    g_platform_ver.buildtime[1].day        = APP_BUILD_DAY;
    g_platform_ver.versionflag            |= 0x02;
    ipc_data_drv_write(256, (uint8_t *)&g_platform_ver, sizeof(g_platform_ver));

#elif defined(BUILD_MCU3_0)
    g_platform_ver.cpu_version.app_ver[2]  = APP_VSC_VERSION;
    g_platform_ver.core_ver[2].ver_high    = MAINCTRL_CORE2_PROG_VER_H;
    g_platform_ver.core_ver[2].ver_low     = MAINCTRL_CORE2_PROG_VER_L;
    memcpy(g_platform_ver.ver_asc[2], MAINCTRL_CORE2_PROG_VER_ASC, sizeof(MAINCTRL_CORE2_PROG_VER_ASC));
    g_platform_ver.buildtime[2].year       = APP_BUILD_YEAR;
    g_platform_ver.buildtime[2].month      = APP_BUILD_MONTH;
    g_platform_ver.buildtime[2].day        = APP_BUILD_DAY;
    g_platform_ver.versionflag            |= 0x04;
    ipc_data_drv_write(256, (uint8_t *)&g_platform_ver, sizeof(g_platform_ver));

#elif defined(BUILD_MCU3_1)
    g_platform_ver.cpu_version.app_ver[3]  = APP_VSC_VERSION;
    g_platform_ver.core_ver[3].ver_high    = MAINCTRL_CORE3_PROG_VER_H;
    g_platform_ver.core_ver[3].ver_low     = MAINCTRL_CORE3_PROG_VER_L;
    memcpy(g_platform_ver.ver_asc[3], MAINCTRL_CORE3_PROG_VER_ASC, sizeof(MAINCTRL_CORE3_PROG_VER_ASC));
    g_platform_ver.buildtime[3].year       = APP_BUILD_YEAR;
    g_platform_ver.buildtime[3].month      = APP_BUILD_MONTH;
    g_platform_ver.buildtime[3].day        = APP_BUILD_DAY;
    g_platform_ver.versionflag            |= 0x08;
    ipc_data_drv_write(256, (uint8_t *)&g_platform_ver, sizeof(g_platform_ver));

#elif defined(BUILD_C66X_1)
    g_platform_ver.cpu_version.app_ver[4]  = APP_VSC_VERSION;
    g_platform_ver.core_ver[4].ver_high    = MAINCTRL_CORE4_PROG_VER_H;
    g_platform_ver.core_ver[4].ver_low     = MAINCTRL_CORE4_PROG_VER_L;
    memcpy(g_platform_ver.ver_asc[4], MAINCTRL_CORE4_PROG_VER_ASC, sizeof(MAINCTRL_CORE4_PROG_VER_ASC));
    g_platform_ver.buildtime[4].year       = APP_BUILD_YEAR;
    g_platform_ver.buildtime[4].month      = APP_BUILD_MONTH;
    g_platform_ver.buildtime[4].day        = APP_BUILD_DAY;
    g_platform_ver.versionflag            |= 0x10;
    ipc_data_drv_write(256, (uint8_t *)&g_platform_ver, sizeof(g_platform_ver));

#elif defined(BUILD_C66X_2)
    g_platform_ver.cpu_version.app_ver[5]  = APP_VSC_VERSION;
    g_platform_ver.core_ver[5].ver_high    = MAINCTRL_CORE5_PROG_VER_H;
    g_platform_ver.core_ver[5].ver_low     = MAINCTRL_CORE5_PROG_VER_L;
    memcpy(g_platform_ver.ver_asc[5], MAINCTRL_CORE5_PROG_VER_ASC, sizeof(MAINCTRL_CORE5_PROG_VER_ASC));
    g_platform_ver.buildtime[5].year       = APP_BUILD_YEAR;
    g_platform_ver.buildtime[5].month      = APP_BUILD_MONTH;
    g_platform_ver.buildtime[5].day        = APP_BUILD_DAY;
    g_platform_ver.versionflag            |= 0x20;
    ipc_data_drv_write(256, (uint8_t *)&g_platform_ver, sizeof(g_platform_ver));

#elif defined(BUILD_C7X_1)
    g_platform_ver.cpu_version.app_ver[6]  = APP_VSC_VERSION;
    g_platform_ver.core_ver[6].ver_high    = MAINCTRL_CORE6_PROG_VER_H;
    g_platform_ver.core_ver[6].ver_low     = MAINCTRL_CORE6_PROG_VER_L;
    memcpy(g_platform_ver.ver_asc[6], MAINCTRL_CORE6_PROG_VER_ASC, sizeof(MAINCTRL_CORE6_PROG_VER_ASC));
    g_platform_ver.buildtime[6].year       = APP_BUILD_YEAR;
    g_platform_ver.buildtime[6].month      = APP_BUILD_MONTH;
    g_platform_ver.buildtime[6].day        = APP_BUILD_DAY;
    g_platform_ver.versionflag            |= 0x40;
    ipc_data_drv_write(256, (uint8_t *)&g_platform_ver, sizeof(g_platform_ver));

#elif defined(BUILD_MCU1_1)
    g_platform_ver.cpu_version.app_ver[7]  = APP_VSC_VERSION;
    g_platform_ver.core_ver[7].ver_high    = MAINCTRL_CORE7_PROG_VER_H;
    g_platform_ver.core_ver[7].ver_low     = MAINCTRL_CORE7_PROG_VER_L;
    memcpy(g_platform_ver.ver_asc[7], MAINCTRL_CORE7_PROG_VER_ASC, sizeof(MAINCTRL_CORE7_PROG_VER_ASC));
    g_platform_ver.buildtime[7].year       = APP_BUILD_YEAR;
    g_platform_ver.buildtime[7].month      = APP_BUILD_MONTH;
    g_platform_ver.buildtime[7].day        = APP_BUILD_DAY;
    g_platform_ver.versionflag            |= 0x80;
    ipc_data_drv_write(256, (uint8_t *)&g_platform_ver, sizeof(g_platform_ver));
#endif
}

#if defined(BUILD_MCU2_0)
/**
 * @brief R0收集所有核的版本信息，并放到共享内存
 */
void get_softver_all(void)
{
    platform_ver_t    temp          = {0};
    static uint32_t   all_ok_flag   = 0x1FF;

    if(all_ok_flag)
    {
        if (all_ok_flag & 0x101)
        {
            if (g_platform_ver.versionflag & 0x101)
            {
                // 肯定是R0先执行拷贝逻辑
                memcpy((uint8_t *)&g_platform_ver_shm, (uint8_t *)&g_platform_ver, sizeof(g_platform_ver));
                g_platform_ver_shm.versionflag |= 0x101;
                all_ok_flag &= ~0x101;
            }
        }

        if (all_ok_flag & 0x02)
        {
            memset(&temp, 0, sizeof(temp));
            ipc_data_drv_read(r1, 256, (uint8_t *)&temp, sizeof (temp));
            if (temp.versionflag & 0x02)
            {
                g_platform_ver_shm.cpu_version.app_ver[1]  = temp.cpu_version.app_ver[1];
                g_platform_ver_shm.core_ver[1].ver_high    = temp.core_ver[1].ver_high;
                g_platform_ver_shm.core_ver[1].ver_low     = temp.core_ver[1].ver_low;
                memcpy(g_platform_ver_shm.ver_asc[1], temp.ver_asc[1], sizeof(temp.ver_asc[1]));
                g_platform_ver_shm.buildtime[1].year       = temp.buildtime[1].year;
                g_platform_ver_shm.buildtime[1].month      = temp.buildtime[1].month;
                g_platform_ver_shm.buildtime[1].day        = temp.buildtime[1].day;
                g_platform_ver_shm.versionflag |= 0x02;
                all_ok_flag &= ~0x02;
            }
        }

        if (all_ok_flag & 0x04)
        {
            memset(&temp, 0, sizeof(temp));
            ipc_data_drv_read(r2, 256, (uint8_t *)&temp, sizeof (temp));
            if (temp.versionflag & 0x04)
            {
                g_platform_ver_shm.cpu_version.app_ver[2]  = temp.cpu_version.app_ver[2];
                g_platform_ver_shm.core_ver[2].ver_high    = temp.core_ver[2].ver_high;
                g_platform_ver_shm.core_ver[2].ver_low     = temp.core_ver[2].ver_low;
                memcpy(g_platform_ver_shm.ver_asc[2], temp.ver_asc[2], sizeof(temp.ver_asc[2]));
                g_platform_ver_shm.buildtime[2].year       = temp.buildtime[2].year;
                g_platform_ver_shm.buildtime[2].month      = temp.buildtime[2].month;
                g_platform_ver_shm.buildtime[2].day        = temp.buildtime[2].day;
                g_platform_ver_shm.versionflag |= 0x04;
                all_ok_flag &= ~0x04;
            }
        }

        if (all_ok_flag & 0x08)
        {
            memset(&temp, 0, sizeof(temp));
            ipc_data_drv_read(r3, 256, (uint8_t *)&temp, sizeof (temp));
            if (temp.versionflag & 0x08)
            {
                g_platform_ver_shm.cpu_version.app_ver[3]  = temp.cpu_version.app_ver[3];
                g_platform_ver_shm.core_ver[3].ver_high    = temp.core_ver[3].ver_high;
                g_platform_ver_shm.core_ver[3].ver_low     = temp.core_ver[3].ver_low;
                memcpy(g_platform_ver_shm.ver_asc[3], temp.ver_asc[3], sizeof(temp.ver_asc[3]));
                g_platform_ver_shm.buildtime[3].year       = temp.buildtime[3].year;
                g_platform_ver_shm.buildtime[3].month      = temp.buildtime[3].month;
                g_platform_ver_shm.buildtime[3].day        = temp.buildtime[3].day;
                g_platform_ver_shm.versionflag |= 0x08;
                all_ok_flag &= ~0x08;
            }
        }

        if (all_ok_flag & 0x10)
        {
            memset(&temp, 0, sizeof(temp));
            ipc_data_drv_read(c60, 256, (uint8_t *)&temp, sizeof (temp));
            if (temp.versionflag & 0x10)
            {
                g_platform_ver_shm.cpu_version.app_ver[4]  = temp.cpu_version.app_ver[4];
                g_platform_ver_shm.core_ver[4].ver_high    = temp.core_ver[4].ver_high;
                g_platform_ver_shm.core_ver[4].ver_low     = temp.core_ver[4].ver_low;
                memcpy(g_platform_ver_shm.ver_asc[4], temp.ver_asc[4], sizeof(temp.ver_asc[4]));
                g_platform_ver_shm.buildtime[4].year       = temp.buildtime[4].year;
                g_platform_ver_shm.buildtime[4].month      = temp.buildtime[4].month;
                g_platform_ver_shm.buildtime[4].day        = temp.buildtime[4].day;
                g_platform_ver_shm.versionflag |= 0x10;
                all_ok_flag &= ~0x10;
            }
        }

        if (all_ok_flag & 0x20)
        {
            memset(&temp, 0, sizeof(temp));
            ipc_data_drv_read(c61, 256, (uint8_t *)&temp, sizeof (temp));
            if (temp.versionflag & 0x20)
            {
                g_platform_ver_shm.cpu_version.app_ver[5]  = temp.cpu_version.app_ver[5];
                g_platform_ver_shm.core_ver[5].ver_high    = temp.core_ver[5].ver_high;
                g_platform_ver_shm.core_ver[5].ver_low     = temp.core_ver[5].ver_low;
                memcpy(g_platform_ver_shm.ver_asc[5], temp.ver_asc[5], sizeof(temp.ver_asc[5]));
                g_platform_ver_shm.buildtime[5].year       = temp.buildtime[5].year;
                g_platform_ver_shm.buildtime[5].month      = temp.buildtime[5].month;
                g_platform_ver_shm.buildtime[5].day        = temp.buildtime[5].day;
                g_platform_ver_shm.versionflag |= 0x20;
                all_ok_flag &= ~0x20;
            }
        }

        if (all_ok_flag & 0x40)
        {
            memset(&temp, 0, sizeof(temp));
            ipc_data_drv_read(c70, 256, (uint8_t *)&temp, sizeof (temp));
            if (temp.versionflag & 0x40)
            {
                g_platform_ver_shm.cpu_version.app_ver[6]  = temp.cpu_version.app_ver[6];
                g_platform_ver_shm.core_ver[6].ver_high    = temp.core_ver[6].ver_high;
                g_platform_ver_shm.core_ver[6].ver_low     = temp.core_ver[6].ver_low;
                memcpy(g_platform_ver_shm.ver_asc[6], temp.ver_asc[6], sizeof(temp.ver_asc[6]));
                g_platform_ver_shm.buildtime[6].year       = temp.buildtime[6].year;
                g_platform_ver_shm.buildtime[6].month      = temp.buildtime[6].month;
                g_platform_ver_shm.buildtime[6].day        = temp.buildtime[6].day;
                g_platform_ver_shm.versionflag |= 0x40;
                all_ok_flag &= ~0x40;
            }
        }

        if (all_ok_flag & 0x80)
        {
            memset(&temp, 0, sizeof(temp));
            ipc_data_drv_read(mcu1, 256, (uint8_t *)&temp, sizeof (temp));
            if (temp.versionflag & 0x80)
            {
                g_platform_ver_shm.cpu_version.app_ver[7]  = temp.cpu_version.app_ver[7];
                g_platform_ver_shm.core_ver[7].ver_high    = temp.core_ver[7].ver_high;
                g_platform_ver_shm.core_ver[7].ver_low     = temp.core_ver[7].ver_low;
                memcpy(g_platform_ver_shm.ver_asc[7], temp.ver_asc[7], sizeof(temp.ver_asc[7]));
                g_platform_ver_shm.buildtime[7].year       = temp.buildtime[7].year;
                g_platform_ver_shm.buildtime[7].month      = temp.buildtime[7].month;
                g_platform_ver_shm.buildtime[7].day        = temp.buildtime[7].day;
                g_platform_ver_shm.versionflag |= 0x80;
                all_ok_flag &= ~0x80;
            }
        }

        CacheP_wb(&g_platform_ver_shm, sizeof(g_platform_ver_shm));
    }
}
#endif

void bsp_init_ok()
{
    uint32_t pscode_cfg_init_ok = INIT_FLGA_MAGIC;

#if defined(BUILD_MCU3_0)
    ipc_data_drv_write(768, (uint8_t *)&pscode_cfg_init_ok, sizeof(pscode_cfg_init_ok));

#elif defined(BUILD_MCU3_1)
    ipc_data_drv_write(772, (uint8_t *)&ft3_tx_cfg_all_prv[3], sizeof(ft3_tx_cfg_all_prv[3]));
    ipc_data_drv_write(5892, (uint8_t *)&ft3_rx_cfg_all_prv[3], sizeof(ft3_rx_cfg_all_prv[3]));
    ipc_data_drv_write(768, (uint8_t *)&pscode_cfg_init_ok, sizeof(pscode_cfg_init_ok));

#elif defined(BUILD_C66X_1)
    ipc_data_drv_write(772, (uint8_t *)&ft3_tx_cfg_all_prv[2], sizeof(ft3_tx_cfg_all_prv[2]));
    ipc_data_drv_write(5892, (uint8_t *)&ft3_rx_cfg_all_prv[2], sizeof(ft3_rx_cfg_all_prv[2]));
    ipc_data_drv_write(768, (uint8_t *)&pscode_cfg_init_ok, sizeof(pscode_cfg_init_ok));

#elif defined(BUILD_C66X_2)
    ipc_data_drv_write(768, (uint8_t *)&pscode_cfg_init_ok, sizeof(pscode_cfg_init_ok));

#elif defined(BUILD_C7X_1)
    CacheP_wb(ft3_tx_cfg_all, sizeof(ft3_tx_cfg_all));
    CacheP_wb(ft3_rx_cfg_all, sizeof(ft3_rx_cfg_all));
    CacheP_wb(&g_squ_mod_cfg_all, sizeof(g_squ_mod_cfg_all));
    ipc_data_drv_write(768, (uint8_t *)&pscode_cfg_init_ok, sizeof(pscode_cfg_init_ok));

#elif defined(BUILD_MCU1_1)
    ipc_data_drv_write(768, (uint8_t *)&pscode_cfg_init_ok, sizeof(pscode_cfg_init_ok));

#endif
}

#endif
