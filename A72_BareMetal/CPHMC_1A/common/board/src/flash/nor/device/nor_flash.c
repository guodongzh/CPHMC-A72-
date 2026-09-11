/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       not_flash.c
 *@author     LiuRui
 *@date       2026.05.06
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.05.06  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "board/src/flash/nor/nor.h"
#include "nor_flash.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

NOR_Info nor_device_info[] = {
    /*is25lp128f*/
    {.manufacturerId = FLASH_MANF_ID_ISSI,
     .deviceId = FLASH_DEVICE_ID_IS25LP128F,
     .idNumByte = 0x3U,
     .addrWidth = 0x3U,

     .totalSize = (16U * 1024U * 1024U),
     .blockSize = (32U * 1024U),
     .blockCnt = (16U * 1024U * 1024U) / (32U * 1024U),
     .pageSize = 256,
     .pageCnt = (16U * 1024U * 1024U) / 256,
     .sectorSize = 4 * 1024,
     .sectorCnt = (16U * 1024U * 1024U) / (4 * 1024),
     .rdDummyClk = 8,
     .srWip = (1U << 0U)},
    /*is25wp128f*/
    {.manufacturerId = FLASH_MANF_ID_ISSI,
     .deviceId = FLASH_DEVICE_ID_IS25WP128F,
     .idNumByte = 0x3U,
     .addrWidth = 0x3U,

     .totalSize = (16U * 1024U * 1024U),
     .blockSize = (32U * 1024U),
     .blockCnt = (16U * 1024U * 1024U) / (32U * 1024U),
     .pageSize = 256,
     .pageCnt = (16U * 1024U * 1024U) / 256,
     .sectorSize = 4 * 1024,
     .sectorCnt = (16U * 1024U * 1024U) / (4 * 1024),
     .rdDummyClk = 8,
     .srWip = (1U << 0U)},
    /*IS25LP256D*/
    {.manufacturerId = FLASH_MANF_ID_ISSI,
     .deviceId = FLASH_DEVICE_ID_IS25LP256D,
     .idNumByte = 0x3U,
     .addrWidth = 0x3U,

     .totalSize = (32U * 1024U * 1024U),
     .blockSize = (32U * 1024U),
     .blockCnt = (32U * 1024U * 1024U) / (32U * 1024U),
     .pageSize = 256,
     .pageCnt = (32U * 1024U * 1024U) / 256,
     .sectorSize = 4 * 1024,
     .sectorCnt = (32U * 1024U * 1024U) / (4 * 1024),

     .rdDummyClk = 8,
     .srWip = (1U << 0U)},
    /*IS25WP256D*/
    {.manufacturerId = FLASH_MANF_ID_ISSI,
     .deviceId = FLASH_DEVICE_ID_IS25WP256D,
     .idNumByte = 0x3U,
     .addrWidth = 0x3U,

     .totalSize = (32U * 1024U * 1024U),
     .blockSize = (32U * 1024U),
     .blockCnt = (32U * 1024U * 1024U) / (32U * 1024U),
     .pageSize = 256,
     .pageCnt = (32U * 1024U * 1024U) / 256,
     .sectorSize = 4 * 1024,
     .sectorCnt = (32U * 1024U * 1024U) / (4 * 1024),

     .rdDummyClk = 8,
     .srWip = (1U << 0U)},
    /*IS25LP01GJ*/
    {.manufacturerId = FLASH_MANF_ID_ISSI,
     .deviceId = FLASH_DEVICE_ID_IS25LP01GJ,
     .idNumByte = 0x3U,
     .addrWidth = 0x3U,

     .totalSize = (128U * 1024U * 1024U),
     .blockSize = (32U * 1024U),
     .blockCnt = (128U * 1024U * 1024U) / (128U * 1024U),
     .pageSize = 256,
     .pageCnt = (128U * 1024U * 1024U) / 256,
     .sectorSize = 4 * 1024,
     .sectorCnt = (128U * 1024U * 1024U) / (4 * 1024),

     .rdDummyClk = 8,
     .srWip = (1U << 0U)},

    /*IS25WP01G*/
    {.manufacturerId = FLASH_MANF_ID_ISSI,
     .deviceId = FLASH_DEVICE_ID_IS25WP01GJ,
     .idNumByte = 0x3U,
     .addrWidth = 0x3U,

     .totalSize = (128U * 1024U * 1024U),
     .blockSize = (32U * 1024U),
     .blockCnt = (128U * 1024U * 1024U) / (128U * 1024U),
     .pageSize = 256,
     .pageCnt = (128U * 1024U * 1024U) / 256,
     .sectorSize = 4 * 1024,
     .sectorCnt = (128U * 1024U * 1024U) / (4 * 1024),

     .rdDummyClk = 8,
     .srWip = (1U << 0U)},

    /*mt25ql128*/
    {.manufacturerId = FLASH_MANF_ID_MICRON,
     .deviceId = FLASH_DEVICE_ID_MT25QL128A,
     .idNumByte = 0x3U,
     .addrWidth = 0x3U,

     .totalSize = (16U * 1024U * 1024U),
     .blockSize = (16U * 1024U),
     .blockCnt = (16U * 1024U * 1024U) / (32U * 1024U),
     .pageSize = 256,
     .pageCnt = (16U * 1024U * 1024U) / 256,
     .sectorSize = 4 * 1024,
     .sectorCnt = (16U * 1024U * 1024U) / (4 * 1024),
     .rdDummyClk = 8,
     .srWip = (1U << 0U)
    },

    /*mt25ql256*/
    {.manufacturerId = FLASH_MANF_ID_MICRON,
     .deviceId = FLASH_DEVICE_ID_MT25QL256A,
     .idNumByte = 0x3U,
     .addrWidth = 0x3U,

     .totalSize = (32U * 1024U * 1024U),
     .blockSize = (32U * 1024U),
     .blockCnt = (32U * 1024U * 1024U) / (32U * 1024U),
     .pageSize = 256,
     .pageCnt = (32U * 1024U * 1024U) / 256,
     .sectorSize = 4 * 1024,
     .sectorCnt = (32U * 1024U * 1024U) / (4 * 1024),
     .rdDummyClk = 8,
     .srWip = (1U << 0U)

    },
    /*mt25ql01g*/
    {.manufacturerId = FLASH_MANF_ID_MICRON,
     .deviceId = FLASH_DEVICE_ID_MT25QL01GB,
     .idNumByte = 0x3U,
     .addrWidth = 0x3U,

     .totalSize = (128U * 1024U * 1024U),
     .blockSize = (32U * 1024U),
     .blockCnt = (128U * 1024U * 1024U) / (32U * 1024U),
     .pageSize = 256,
     .pageCnt = (128U * 1024U * 1024U) / 256,
     .sectorSize = 4 * 1024,
     .sectorCnt = (128U * 1024U * 1024U) / (4 * 1024),
     .rdDummyClk = 8,
     .srWip = (1U << 0U)

    },

    /*end*/
    {.manufacturerId = 0,
     .deviceId = 0,
     .idNumByte = 0,
     .addrWidth = 0,

     .totalSize = 0,
     .blockSize = 0,
     .blockCnt = 0,
     .pageSize = 0,
     .pageCnt = 0,
     .sectorSize = 0,
     .sectorCnt = 0,
     .rdDummyClk = 0,
     .srWip = (1U << 0U)},
};

NOR_CmdTable nor_flash_cmd_table = {
    .wrEn = NOR_CMD_WREN,
    .wrSr = NOR_CMD_WRSR,
    .rdSr = NOR_CMD_RDSR,
    .rdId = NOR_CMD_RDID,
    .qpien = NOR_CMD_QPIEN,
    .blockErase = NOR_CMD_BLOCK_ERASE,
    .sectorErase = NOR_CMD_SECTOR_ERASE,
    .rd = NOR_CMD_QUAD_FAST_RD,
    .prog = NOR_CMD_QUAD_FAST_PROG,
};

NOR_cmdTime nor_flash_cmd_time = {
    .pageProg = NOR_PAGE_PROG_TIMEOUT,
    .sectorErase = NOR_SECTOR_ERASE_TIMEOUT,
    .wrsr = NOR_WRSR_WRITE_TIMEOUT,
    .bulkErase = NOR_BULK_ERASE_TIMEOUT,
};
