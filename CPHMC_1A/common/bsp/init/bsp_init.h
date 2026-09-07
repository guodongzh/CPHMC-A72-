/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       bsp_init.h
 *@author     LiuRui
 *@date       2025.09.23
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.09.23  1.0       LiuRui
 ******************************************************************************/

#ifndef _CPHMC_1A_R0_BSP_INIT_H
#define _CPHMC_1A_R0_BSP_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>


/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define INIT_FLGA_MAGIC 0x55555555
#define FLASH_SPINK_LOCK_ID  (0x00u)
#define REPORT_SPINK_LOCK_ID (0x01u)
#define I2C0_SPINK_LOCK_ID   (0x02u)
#define I2C1_SPINK_LOCK_ID   (0x03u)
#define I2C2_SPINK_LOCK_ID   (0x04u)
#define I2C6_SPINK_LOCK_ID   (0x05u)

// 版本信息
#define HW_VER ("3.0")

#define FW_RELEASE_R0_VER   ("1.0.1.3")  // core0
#define FW_RELEASE_R1_VER   ("1.0.1.3")  // core1
#define FW_RELEASE_R2_VER   ("1.0.1.3")  // core2
#define FW_RELEASE_R3_VER   ("1.0.1.3")  // core3
#define FW_RELEASE_C6X0_VER ("1.0.1.3")  // core4
#define FW_RELEASE_C6X1_VER ("1.0.1.3")  // core5
#define FW_RELEASE_C7X0_VER ("1.0.1.3")  // core6
#define FW_RELEASE_MCU1_VER ("1.0.1.3")  // core7

#if defined(BUILD_MCU2_0)
#define MAINCTRL_CORE0_PROG_VER_H     1					// 手写版本高
#define MAINCTRL_CORE0_PROG_VER_L     1					// 手写版本低

#elif defined(BUILD_MCU2_1)
#define MAINCTRL_CORE1_PROG_VER_H     1
#define MAINCTRL_CORE1_PROG_VER_L     1

#elif defined(BUILD_MCU3_0)
#define MAINCTRL_CORE2_PROG_VER_H     1
#define MAINCTRL_CORE2_PROG_VER_L     1

#elif defined(BUILD_MCU3_1)
#define MAINCTRL_CORE3_PROG_VER_H     1
#define MAINCTRL_CORE3_PROG_VER_L     1

#elif defined(BUILD_C66X_1)
#define MAINCTRL_CORE4_PROG_VER_H     1
#define MAINCTRL_CORE4_PROG_VER_L     1

#elif defined(BUILD_C66X_2)
#define MAINCTRL_CORE5_PROG_VER_H     1
#define MAINCTRL_CORE5_PROG_VER_L     1

#elif defined(BUILD_C7X_1)
#define MAINCTRL_CORE6_PROG_VER_H     1
#define MAINCTRL_CORE6_PROG_VER_L     1

#elif defined(BUILD_MCU1_1)
#define MAINCTRL_CORE7_PROG_VER_H     1
#define MAINCTRL_CORE7_PROG_VER_L     1
#endif
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/
typedef struct _init_flag
{
    volatile uint32_t flash_init_ok;
    uint8_t resv[104];
}init_flag_t;

typedef struct _fpga_version
{
    uint16_t main_soft_ver;     // FPGA软件版本
    uint8_t  main_fun_ver;      // FPGA功能版本
    uint8_t  main_pcie_ver;     // pcie应用协议内部接口版本
    uint32_t main_soft_code;    // 软件校验码
    uint16_t slot2_soft_ver;
    uint8_t  slot2_fun_ver;
    uint8_t  slot2_pcie_ver;
    uint32_t slot2_soft_code;
    uint16_t slot3_soft_ver;
    uint8_t  slot3_fun_ver;
    uint8_t  slot3_pcie_ver;
    uint32_t slot3_soft_code;
} fpga_version_t;

typedef struct _cpu_version
{
    uint32_t low_lib_ver;     // lib git version
    uint32_t app_ver[8u];     // git version
} cpu_version_t;

/**
 * @brief: pscode fireware version
 */
typedef struct _cpu_fw_ver
{
    uint8_t major[2];        // 大版本
    uint8_t minor[2];        // 小版本
} cpu_fw_ver_t;

typedef struct _cpu_Date
{
    uint8_t year;
    uint8_t month;
    uint8_t day;
} cpu_Date_t;

typedef struct _core_ver
{
    uint32_t    ver_high;
    uint32_t    ver_low;
} core_ver_t;

typedef struct _platform_ver
{
    uint32_t              versionflag;             // 版本号初始化标志
    uint8_t               hw_ver[2];               // CPU硬件版本:手写
    cpu_version_t         cpu_version;             // CPU git版本号
    cpu_fw_ver_t          cpu_fw_ver[8];           // CPU可视化固件发布版本:手写
    fpga_version_t        fpga_version;
    uint16_t              core_image_crc[8];       // 程序镜像CRC
    core_ver_t            core_ver[8];             // 程序总版本:手写
    uint8_t               ver_asc[8][32];          // 程序非标版本：手写
    cpu_Date_t            buildtime[8];            // 程序的编译时间：年月日
    uint16_t              cfg_crc[8];              // 程序配置文件CRC
} platform_ver_t;

extern init_flag_t init_flag;
extern platform_ver_t g_platform_ver_shm;
extern platform_ver_t g_platform_ver;



/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void bsp_init_ok();
void init_core_softver(void);
void get_softver_all(void);

#ifdef __cplusplus
}
#endif

#endif  //_CPHMC_1A_R0_BSP_INIT_H
