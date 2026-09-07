/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       not_flash.h
 *@author     LiuRui
 *@date       2026.05.06
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.05.06  1.0       LiuRui
 ******************************************************************************/

#ifndef __NOT_FLASH_H
#define __NOT_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

#define FLASH_MANF_ID_ISSI          0x9Du
#define FLASH_DEVICE_ID_IS25LP128F  0x6018u /**< ISSI 16MB QSPI NOR Flash */
#define FLASH_DEVICE_ID_IS25WP128F  0x7018u /**< ISSI 16MB QSPI NOR Flash */
#define FLASH_DEVICE_ID_IS25LP256D  0x6019u /**< ISSI 32MB QSPI NOR Flash */
#define FLASH_DEVICE_ID_IS25WP256D  0x7019u /**< ISSI 32MB QSPI NOR Flash */
#define FLASH_DEVICE_ID_IS25LP01GJ  0x6021u /**< ISSI 128MB QSPI NOR Flash */
#define FLASH_DEVICE_ID_IS25WP01GJ  0x7021u /**< ISSI 128MB QSPI NOR Flash */

#define FLASH_MANF_ID_MICRON        0x20
#define FLASH_DEVICE_ID_MT25QL128A  0xBA18u /**< Micro 16MB QSPI NOR Flash */
#define FLASH_DEVICE_ID_MT25QL256A  0xBA19u /**< Micro 32MB QSPI NOR Flash */
#define FLASH_DEVICE_ID_MT25QL01GB  0xBA21u /**< Micro 128MB QSPI NOR Flash */

/** Flash device commands */
#define NOR_BE_SECTOR_NUM           (-1U)
#define NOR_CMD_WREN                (0x06U)
#define NOR_CMD_WRSR                (0x01U)
#define NOR_CMD_RDSR                (0x05U)
#define NOR_CMD_RDID                (0x9FU)
#define NOR_CMD_QPIEN               (0x35u)

#define NOR_CMD_BULK_ERASE          (0xD8U)
#define NOR_CMD_BLOCK_ERASE         (0x52U)
#define NOR_CMD_SECTOR_ERASE        (0x20U)
#define NOR_CMD_QUAD_FAST_RD        (0x6BU)
#define NOR_CMD_QUAD_FAST_PROG      (0x32U)

/** Status Register, Write-in-Progress bit */
#define NOR_SR_WIP                  (1U << 0U)


/** In Micro seconds */
#define NOR_PAGE_PROG_TIMEOUT       (400U)
#define NOR_SECTOR_ERASE_TIMEOUT    (600U * 1000U)
#define NOR_WRSR_WRITE_TIMEOUT      (600U * 1000U)
#define NOR_BULK_ERASE_TIMEOUT      (110U * 1000U * 1000U)


/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern NOR_Info nor_device_info[];
extern NOR_CmdTable nor_flash_cmd_table;
extern NOR_cmdTime nor_flash_cmd_time;


#ifdef __cplusplus
}
#endif

#endif  //__NOT_FLASH_H