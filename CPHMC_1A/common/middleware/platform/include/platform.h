/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       platform .h
 *@author     jinyangh
 *@date       2025.10.30
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.10.30  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ti/csl/src/ip/spinlock/V1/csl_spinlock.h"
#include "board/src/flash/include/board_flash.h"
#include "board/src/flash/nor/nor.h"
#include "flash.h"
#include "bsp_init.h"
#include "debug_config.h"
#include "pcie_fpga.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
/*************************************** system *************************************/
#if defined(BUILD_MCU2_0)
#define CORE_R5F0         BUILD_MCU2_0
#elif defined(BUILD_MCU1_1)
#define MCU_R5F1         BUILD_MCU1_1
#elif defined(BUILD_MCU2_1)
#define CORE_R5F1         BUILD_MCU2_1
#elif defined(BUILD_MCU3_0)
#define CORE_R5F2         BUILD_MCU3_0
#elif defined(BUILD_MCU3_1)
#define CORE_R5F3         BUILD_MCU3_1
#endif
#define SystemP_SUCCESS   ((int32_t )0)
#define SystemP_FAILURE   ((int32_t)-1)

/*************************************** 共享内存 *************************************/
#define ENET_RX_SHARE             __attribute__((section(".rx_enetque")))
#define IPC_INFO_SHARE            __attribute__((section(".ipc_shm_info")))
#define ENET_R1_TX_R0_SHARE       __attribute__((section(".r1_tx_r0_enetque")))
#define ENET_R2_TX_R0_SHARE       __attribute__((section(".r2_tx_r0_enetque")))
#define ENET_R3_TX_R0_SHARE       __attribute__((section(".r3_tx_r0_enetque")))
#define ENET_A_TX_R0_SHARE        __attribute__((section(".a72_tx_r0_enetque")))
#define ENET_R0_MULTICAST_SHARE   __attribute__((section(".r0_multicast_que")))
#define ENET_R0_TX_R1_SHARE       __attribute__((section(".r0_tx_r1_enetque")))
#define ENET_R2_TX_R1_SHARE       __attribute__((section(".r2_tx_r1_enetque")))
#define ENET_R3_TX_R1_SHARE       __attribute__((section(".r3_tx_r1_enetque")))
#define ENET_R1_MULTICAST_SHARE   __attribute__((section(".r1_multicast_que")))
#define ENET_R0_TX_R2_SHARE       __attribute__((section(".r0_tx_r2_enetque")))
#define ENET_R1_TX_R2_SHARE       __attribute__((section(".r1_tx_r2_enetque")))
#define ENET_R3_TX_R2_SHARE       __attribute__((section(".r3_tx_r2_enetque")))
#define ENET_R2_MULTICAST_SHARE   __attribute__((section(".r2_multicast_que")))
#define ENET_R0_TX_R3_SHARE       __attribute__((section(".r0_tx_r3_enetque")))
#define ENET_R1_TX_R3_SHARE       __attribute__((section(".r1_tx_r3_enetque")))
#define ENET_R2_TX_R3_SHARE       __attribute__((section(".r2_tx_r3_enetque")))
#define ENET_R3_MULTICAST_SHARE   __attribute__((section(".r3_multicast_que")))

/*************************************** cache ***************************************/
#define CacheP_TYPE_ALL           0
#define CacheP_TYPE_ALLD          0

/*************************************** pcie ****************************************/
#define SLOW_CAR_ID               (9)

/*************************************** flash ***************************************/
/************************************************************************************
 * Flash 0
 ************************************************************************************
 * Region Type	               Start Address           End Address	Size
 * SBL          	       0x0000 0000             0x0008 0000	0x08 0000 （512K）
 * TIFS                        0x0008 0000             0x0010 0000	0x08 0000 （512K）
 * tbl                         0x0010 0000             0x0018 0000	0x08 0000 （512K）
 * atf_spl                     0x0018 0000             0x0028 0000	0x10 0000 （1MB）
 * Core0_App_Image             0x0028 0000             0x0038 0000	0x10 0000 （1MB）
 * Core1_App_Image             0x0038 0000             0x0048 0000	0x10 0000 （1MB）
 * Core2_App_Image             0x0048 0000             0x0058 0000	0x10 0000 （1MB）
 * Core3_App_Image             0x0058 0000             0x0068 0000	0x10 0000 （1MB）
 * Core4_App_Image             0x0068 0000             0x0078 0000	0x10 0000 （1MB）
 * Core5_App_Image             0x0078 0000             0x0088 0000	0x10 0000 （1MB）
 * Core6_App_Image             0x0088 0000             0x0098 0000	0x10 0000 （1MB）
 * Core7_App_Image             0x0098 0000             0x00a8 0000	0x10 0000 （1MB）
 * backup                      0x00a8 0000             0x00e8 0000	0x40 0000 （4MB）
 * Reserved                    0x00e8 0000             0x00e9 0000	0x01 0000 （64KB）
 * FAT_FILE_INFO               0x00e9 0000             0x00f9 0000	0x10 0000 （1MB）//记录FAT表中的文件的信息，每个文件信息大小为一个扇区4096Byte(4096 * 32 * 1 = 128k)
 * **********************************************************************************
 */
/************************************************************************************
 * Flash 1
 ************************************************************************************
 * Region Type	               Start Address           End Address	Size
 * R0_Config_file              0x0000 0000             0x0020 0000	0x20 0000 （2MB）//R0的前两个配置文件用于存放特殊的共享配置文件，所以R0的其他配置文件从第三个编号开始存储
 * R1_Config_file              0x0020 0000	       0x0040 0000	0x20 0000 （2MB）
 * R2_Config_file              0x0040 0000	       0x0060 0000	0x20 0000 （2MB）
 * R3_Config_file              0x0060 0000	       0x0080 0000	0x20 0000 （2MB）
 * C66x_0_Config_file          0x0080 0000	       0x00a0 0000	0x20 0000 （2MB）
 * C66x_1_Config_file          0x00a0 0000	       0x00c0 0000	0x20 0000 （2MB）
 * C71x_Config_file            0x00c0 0000	       0x00e0 0000	0x20 0000 （2MB）
 * FAT_FILE_INFO               0x00e0 0000	       0x00F0 0000	0x10 0000 （1MB）//记录FAT表中的文件的信息，每个文件信息大小为一个扇区4096Byte(4096 * 32 * 7 = 896k)
 * R0_Report_Manage            0x00F0 0000             0x00F8 0000      0x08 0000  (512K)
 * R1_Report_Manage            0x00F8 0000             0x0100 0000      0x08 0000  (512K)
 * R2_Report_Manage            0x0100 0000             0x0108 0000      0x08 0000  (512K)
 * R3_Report_Manage            0x0108 0000             0x0110 0000      0x08 0000  (512K)
 * C66x_0_Report_Manage        0x0110 0000             0x0118 0000      0x08 0000  (512K)
 * C66x_1_Report_Manage        0x0118 0000             0x0120 0000      0x08 0000  (512K)
 * C71x_Report_Manage          0x0120 0000             0x0128 0000      0x08 0000  (512K)
 * R0_Setting_Manage           0x0128 0000             0x0129 8000      0x01 8000  (96K)
 * R1_Setting_Manage           0x0129 8000             0x012B 0000      0x01 8000  (96K)
 * R2_Setting_Manage           0x012B 0000             0x012C 8000      0x01 8000  (96K)
 * R3_Setting_Manage           0x012C 8000             0x012E 0000      0x01 8000  (96K)
 * C66x_0_Setting_Manage       0x012E 0000             0x012F 8000      0x01 8000  (96K)
 * C66x_1_Setting_Manage       0x012F 8000             0x0131 0000      0x01 8000  (96K)
 * C71x_Setting_Manage         0x0131 0000             0x0132 8000      0x01 8000  (96K)
 * Reserved
 * **********************************************************************************
 */
/************************************************************************************
 * Flash 2
 ************************************************************************************
 * Region Type	               Start Address           End Address	Size
 * FPGA_Image                  0x0000 0000             0x00F0 0000	0xF0 0000 （15MB）
 * FAT_FILE_INFO               0x00F0 0000	       0x0100 0000	0x10 0000 （1MB）//记录FAT表中的文件的信息，每个文件信息大小为一个扇区4096Byte(4096 * 32 * 1 = 128k)
 * **********************************************************************************
 */
#define FLASH_0                   (0)
#define FLASH_1                   (0)
#define FLASH_2                   (1)

#define FLASH_PAGE_SIZE           (256)
#define FLASH_SECTOR_SIZE         (4096)

//FPGA_FLASH_SEL
#define M_FPGA_FLASH_SEL_A_BASE_ADDR  (0)
#define M_FPGA_FLASH_SEL_A_PIN        (0)
#define M_FPGA_FLASH_SEL_A_DIR        (0)
#define M_FPGA_FLASH_SEL_B_BASE_ADDR  (0)
#define M_FPGA_FLASH_SEL_B_PIN        (0)
#define M_FPGA_FLASH_SEL_B_DIR        (0)
#define FPGA_FLASH_GPIO_BASS_A        (0)
#define FPGA_FLASH_GPIO_PIN_A         (0)
#define FPGA_FLASH_GPIO_BASS_B        (0)
#define FPGA_FLASH_GPIO_PIN_B         (0)

/*************************************** file system *********************************/
#define FILE_KEY_NUM                (14)

// FLASH偏移按1M为单位，都是页对齐的地址，1M大小的空间可分为256个扇区，所以相对扇区ID为0-255
#if defined(BUILD_MCU2_0) || defined(BUILD_MCU1_0)
#define CONFIG_FILE_OFFSET_BASE      (0x000000U)  // R0的其他配置文件扇区ID为2-511
#define CONFIG_FILE_INFO_OFFSET_BASE (0xe00000U)

#elif defined(BUILD_MCU2_1)
#define CONFIG_FILE_OFFSET_BASE      (0x200000U)  // R1的配置文件扇区ID为0-511
#define CONFIG_FILE_INFO_OFFSET_BASE (0xe20000U)

#elif defined(BUILD_MCU3_0)
#define CONFIG_FILE_OFFSET_BASE      (0x400000U)  // R2的配置文件扇区ID为0-511
#define CONFIG_FILE_INFO_OFFSET_BASE (0xe40000U)

#elif defined(BUILD_MCU3_1)
#define CONFIG_FILE_OFFSET_BASE      (0x600000U)  // R3的配置文件扇区ID为0-511
#define CONFIG_FILE_INFO_OFFSET_BASE (0xe60000U)

#elif defined(BUILD_C66X_1)
#define CONFIG_FILE_OFFSET_BASE      (0x800000U)  // c66x_0的配置文件扇区ID为0-511
#define CONFIG_FILE_INFO_OFFSET_BASE (0xe80000U)

#elif defined(BUILD_C66X_2)
#define CONFIG_FILE_OFFSET_BASE      (0xa00000U)  // c66x_1的配置文件扇区ID为0-511
#define CONFIG_FILE_INFO_OFFSET_BASE (0xea0000U)

#elif defined(BUILD_C7X_1)
#define CONFIG_FILE_OFFSET_BASE      (0xc00000U)  // c71x的配置文件扇区ID为0-511
#define CONFIG_FILE_INFO_OFFSET_BASE (0xec0000U)
#endif
//sbl、app image、spl、fpga image都是只能由R0下载，所以偏移都是0开始
#define APP_IMAGE_OFFSET_BASE       (0x000000U)
#define APP_IMAGE_INFO_OFFSET_BASE  (0xe90000U)
#define FPGA_IMAGE_OFFSET_BASE      (0x000000U)
#define FPGA_IMAGE_INFO_OFFSET_BASE (0xF00000U)

/*************************************** report manage *******************************/
#define REPORT_CORE_SIZE               0x80000
// 此地址用于存放不同类型报文区域中最后一个被擦除的扇区号
// 占用了 report 区域之前、FAT_FILE_INFO 区域末尾的 7 个扇区（每个核一个扇区）
#ifdef BUILD_MCU2_0
#define RECORD_ERASED_SECTOR_ADD      (0xf00000 - 4096 * 7)
#define OTHER_REPORT_OFFSET           (0xf00000 + REPORT_CORE_SIZE * 0)
#define ALM_REPORT_OFFSET             (0xf10000 + REPORT_CORE_SIZE * 0)
#define OPERATE_REPORT_OFFSET         (0xf20000 + REPORT_CORE_SIZE * 0)
#define RESULT_REPORT_OFFSET          (0xf30000 + REPORT_CORE_SIZE * 0)
#define ACTIVE_REPORT_OFFSET          (0xf40000 + REPORT_CORE_SIZE * 0)
#define SOE_REPORT_OFFSET             (0xf60000 + REPORT_CORE_SIZE * 0)
#elif defined(BUILD_MCU2_1)
#define RECORD_ERASED_SECTOR_ADD      (0xf00000 - 4096 * 6)
#define OTHER_REPORT_OFFSET           (0xf00000 + REPORT_CORE_SIZE * 1)
#define ALM_REPORT_OFFSET             (0xf10000 + REPORT_CORE_SIZE * 1)
#define OPERATE_REPORT_OFFSET         (0xf20000 + REPORT_CORE_SIZE * 1)
#define RESULT_REPORT_OFFSET          (0xf30000 + REPORT_CORE_SIZE * 1)
#define ACTIVE_REPORT_OFFSET          (0xf40000 + REPORT_CORE_SIZE * 1)
#define SOE_REPORT_OFFSET             (0xf60000 + REPORT_CORE_SIZE * 1)
#elif defined(BUILD_MCU3_0)
#define RECORD_ERASED_SECTOR_ADD      (0xf00000 - 4096 * 5)
#define OTHER_REPORT_OFFSET           (0xf00000 + REPORT_CORE_SIZE * 2)
#define ALM_REPORT_OFFSET             (0xf10000 + REPORT_CORE_SIZE * 2)
#define OPERATE_REPORT_OFFSET         (0xf20000 + REPORT_CORE_SIZE * 2)
#define RESULT_REPORT_OFFSET          (0xf30000 + REPORT_CORE_SIZE * 2)
#define ACTIVE_REPORT_OFFSET          (0xf40000 + REPORT_CORE_SIZE * 2)
#define SOE_REPORT_OFFSET             (0xf60000 + REPORT_CORE_SIZE * 2)
#elif defined(BUILD_MCU3_1)
#define RECORD_ERASED_SECTOR_ADD      (0xf00000 - 4096 * 4)
#define OTHER_REPORT_OFFSET           (0xf00000 + REPORT_CORE_SIZE * 3)
#define ALM_REPORT_OFFSET             (0xf10000 + REPORT_CORE_SIZE * 3)
#define OPERATE_REPORT_OFFSET         (0xf20000 + REPORT_CORE_SIZE * 3)
#define RESULT_REPORT_OFFSET          (0xf30000 + REPORT_CORE_SIZE * 3)
#define ACTIVE_REPORT_OFFSET          (0xf40000 + REPORT_CORE_SIZE * 3)
#define SOE_REPORT_OFFSET             (0xf60000 + REPORT_CORE_SIZE * 3)
#elif defined(BUILD_C66X_1)
#define RECORD_ERASED_SECTOR_ADD      (0xf00000 - 4096 * 3)
#define OTHER_REPORT_OFFSET           (0xf00000 + REPORT_CORE_SIZE * 4)
#define ALM_REPORT_OFFSET             (0xf10000 + REPORT_CORE_SIZE * 4)
#define OPERATE_REPORT_OFFSET         (0xf20000 + REPORT_CORE_SIZE * 4)
#define RESULT_REPORT_OFFSET          (0xf30000 + REPORT_CORE_SIZE * 4)
#define ACTIVE_REPORT_OFFSET          (0xf40000 + REPORT_CORE_SIZE * 4)
#define SOE_REPORT_OFFSET             (0xf60000 + REPORT_CORE_SIZE * 4)
#elif defined(BUILD_C66X_2)
#define RECORD_ERASED_SECTOR_ADD      (0xf00000 - 4096 * 2)
#define OTHER_REPORT_OFFSET           (0xf00000 + REPORT_CORE_SIZE * 5)
#define ALM_REPORT_OFFSET             (0xf10000 + REPORT_CORE_SIZE * 5)
#define OPERATE_REPORT_OFFSET         (0xf20000 + REPORT_CORE_SIZE * 5)
#define RESULT_REPORT_OFFSET          (0xf30000 + REPORT_CORE_SIZE * 5)
#define ACTIVE_REPORT_OFFSET          (0xf40000 + REPORT_CORE_SIZE * 5)
#define SOE_REPORT_OFFSET             (0xf60000 + REPORT_CORE_SIZE * 5)
#elif defined(BUILD_C7X_1)
#define RECORD_ERASED_SECTOR_ADD      (0xf00000 - 4096 * 1)
#define OTHER_REPORT_OFFSET           (0xf00000 + REPORT_CORE_SIZE * 6)
#define ALM_REPORT_OFFSET             (0xf10000 + REPORT_CORE_SIZE * 6)
#define OPERATE_REPORT_OFFSET         (0xf20000 + REPORT_CORE_SIZE * 6)
#define RESULT_REPORT_OFFSET          (0xf30000 + REPORT_CORE_SIZE * 6)
#define ACTIVE_REPORT_OFFSET          (0xf40000 + REPORT_CORE_SIZE * 6)
#define SOE_REPORT_OFFSET             (0xf60000 + REPORT_CORE_SIZE * 6)
#endif

/*************************************** setting manage ******************************/
// Flash配置参数
#define FLASH_BASE_OFFSET 0x1280000
#define SECTORS_PER_CORE  24
#define FLASH_CORE_SIZE   (SECTORS_PER_CORE * FLASH_SECTOR_SIZE)  // 96KB

#ifdef BUILD_MCU2_0
#define CURRENT_CORE             0
#define CURRENT_CORE_BASE_OFFSET (FLASH_BASE_OFFSET + CURRENT_CORE * FLASH_CORE_SIZE)
#elif defined(BUILD_MCU2_1)
#define CURRENT_CORE             1
#define CURRENT_CORE_BASE_OFFSET (FLASH_BASE_OFFSET + CURRENT_CORE * FLASH_CORE_SIZE)
#elif defined(BUILD_MCU3_0)
#define CURRENT_CORE             2
#define CURRENT_CORE_BASE_OFFSET (FLASH_BASE_OFFSET + CURRENT_CORE * FLASH_CORE_SIZE)
#elif defined(BUILD_MCU3_1)
#define CURRENT_CORE             3
#define CURRENT_CORE_BASE_OFFSET (FLASH_BASE_OFFSET + CURRENT_CORE * FLASH_CORE_SIZE)
#elif defined(BUILD_C66X_1)
#define CURRENT_CORE             4
#define CURRENT_CORE_BASE_OFFSET (FLASH_BASE_OFFSET + CURRENT_CORE * FLASH_CORE_SIZE)
#elif defined(BUILD_C66X_2)
#define CURRENT_CORE             5
#define CURRENT_CORE_BASE_OFFSET (FLASH_BASE_OFFSET + CURRENT_CORE * FLASH_CORE_SIZE)
#elif defined(BUILD_C7X_1)
#define CURRENT_CORE             6
#define CURRENT_CORE_BASE_OFFSET (FLASH_BASE_OFFSET + CURRENT_CORE * FLASH_CORE_SIZE)
#endif
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
/*************************************** flash ***************************************/
typedef void* Flash_Handle;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
/*************************************** core ****************************************/
uint8_t getCoreNr(void);

/*************************************** cache ***************************************/
void cache_inv_com(void *blockPtr, uint32_t byteCnt, uint32_t type);
void cache_wb_com(void *addr, uint32_t size, uint32_t type);

/*************************************** board ***************************************/
extern uint8_t GPEMCSlotID;
extern train_tx_t  *irig_b_train_tx;
extern train_rx_t  *irig_b_train_rx;

/*************************************** flash ***************************************/
extern  Flash_Handle flash_handle0;
extern  Flash_Handle flash_handle1;
extern  Flash_Handle flash_handle2;
int32_t Flash_read_com(void *handle, uint32_t offset, uint8_t *buf, uint32_t len, uint8_t flash_chipSelect);
int32_t Flash_write_com(void *handle, uint32_t offset, uint8_t *buf, uint32_t len, uint8_t flash_chipSelect);
int32_t Flash_eraseSector_com(void *handle, uint32_t sectorNum, uint8_t flash_chipSelect);
int32_t Flash_offsetToSectorPage_com(void *handle, uint32_t offset, uint32_t *sector, uint32_t *page);

/*************************************** gpio ****************************************/
void GPIO_pinWriteLow_com(uint32_t baseAddr, uint32_t pinNum);
void GPIO_pinWriteHigh_com(uint32_t baseAddr, uint32_t pinNum);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PLATFORM_H */
