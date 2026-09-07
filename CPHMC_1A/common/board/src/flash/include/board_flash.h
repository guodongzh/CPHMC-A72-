/*
 * Copyright (c) 2016-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BOARD_FLASH_H_
#define BOARD_FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <ti/csl/tistdtypes.h>

#if defined(BOARD_NOR_FLASH_IN)
#include "board/src/flash/nor/nor.h"
#endif
#if defined(BOARD_NAND_FLASH_IN)
#include <ti/board/src/flash/nand/nand.h>
#endif

/**
 *
 *  \ingroup BOARD_LIB_MODULE
 *
 *  \defgroup BOARD_LIB_FLASH Board Flash Library
 *
 *  Provides access to different flash devices.
 *
 *  @{
 *
 */
/* @} */

/**
 * \ingroup BOARD_LIB_FLASH
 * @defgroup  BOARD_LIB_FLASH_TYPES Data Types
 */
/*@{*/

/**
 *  @brief 	This type defines the opaque handle returned to a flash device that is opened.
 *  		The handle must be used in all subsequent operations.
 *
 */
typedef uintptr_t Board_flashHandle;

/**
 * Error codes used by Board flash functions. Negative values are errors,
 * while positive values indicate success.
 */
typedef int32_t Board_flash_STATUS; /** Board Flash API return type */

#define BOARD_FLASH_EINVALID        (-3) /**< Error code for invalid parameters */
#define BOARD_FLASH_EUNSUPPORTED    (-2) /**< Error code for unsupported feature */
#define BOARD_FLASH_EFAIL           (-1) /**< General failure code */
#define BOARD_FLASH_EOK             0    /**< General success code */


/**
 * @brief Indicates the type of NOR flash peripheral
 *
 */
typedef enum
{
    BOARD_FLASH_NOR_SPI = 0,
    /**<NOR SPI peripheral interface */
    BOARD_FLASH_NOR_QSPI,
    /**<NOR QSPI peripheral interface */
    BOARD_FLASH_NOR_GPMC,
    /**<NOR GPMC peripheral interface */
    BOARD_FLASH_NOR_OSPI,
    /**<NOR OSPI peripheral interface */
    BOARD_FLASH_NOR_HPF,
    /**<NOR HyperBus peripheral interface */
    BOARD_FLASH_NOR_INTF_MAX
    /**<End of NOR peripheral interface */
} Board_flashNorPeriType;

/**
 * @brief Indicates the type of NAND flash peripheral
 *
 */
typedef enum
{
    BOARD_FLASH_NAND_GPMC = 0,
    /**<NAND GPMC peripheral interface */
    BOARD_FLASH_NAND_EMIF16,
    /**<NAND EMIF16 peripheral interface */
    BOARD_FLASH_NAND_OSPI,
    /**<NAND OSPI peripheral interface */
    BOARD_FLASH_NAND_INTF_MAX
    /**<End of NAND peripheral interface */
} Board_flashNandPeriType;

/**
 * @brief Indicates the type of device
 *
 */
typedef enum
{
    BOARD_FLASH_NAND,
    /**<NAND Flash*/
    BOARD_FLASH_NOR,
    /**<NOR Flash*/
    BOARD_FLASH_EEPROM,
    /**<EEPROM */
    BOARD_FLASH_SD,
    /**<SD Card*/
    BOARD_FLASH_EMMC,
    /**<eMMC Card*/
    BOARD_FLASH_QSPI_FLASH,
    /**<QSPI flash */
    BOARD_FLASH_MAX
    /**<End of devices*/
} Board_flashType;

/**
 * @brief Options to set qspi flash read/write access mode
 */
typedef enum
{
    BOARD_FLASH_QSPI_IO_MODE_SINGLE,
    /**< QSPI flash read/write access on single I/O line */
    BOARD_FLASH_QSPI_IO_MODE_DUAL,
    /**< QSPI flash read/write access on two I/O lines */
    BOARD_FLASH_QSPI_IO_MODE_QUAD
    /**< QSPI flash read/write access on four I/O lines */
} Board_flashQspiIoMode;

/**
 *  @brief This structure contains information about the flash device on the board
 *
 *			The bblist points to an array of bytes where each position represents a
 *			block on the device. If the block is good it is marked as 0xFF. If the block
 *			is bad, it is marked as 0x00. For devices that do not support a bad block
 *list this value will be NULL. The number of blocks in the bblist is determined by the block_count
 *field.
 */
typedef struct
{
    uint32_t flashHandle;
    /**<Handle to the flash interface as returned by flash init function */
    uint32_t manufacturer_id;
    /**<manufacturer ID*/
    uint32_t device_id;
    /**<Manufacturers device ID*/
    Board_flashType type;
    /**<Type of device */
    uint32_t width;
    /**<Width in bits*/
    uint32_t block_count;
    /**<Total blocks. First block starts at 0. */
    uint32_t page_count;
    /**<Page count per block*/
    uint32_t page_size;
    /**<Number of bytes in a page */
    uint32_t spare_size;
    /**<Spare area size in bytes*/
    uint32_t bboffset;
    /**<Offset into spare area to check for a bad block */
    uint32_t column;
    /**<Column for a NAND device */
    uint8_t *bblist;
    /** <Bad Block list or NULL if device does not support one  */
    uint32_t sector_size;
    /**<Number of bytes in a sector */
    uint32_t sector_cnt;
    /*! Total sectors. First sector starts at 0. */
} Board_FlashInfo;

/**
 *  @brief Maximum Board flash instance number that can be opened.
 */
#define MAX_BOARD_FLASH_INSTANCE_NUM 8

/* @} */

/**
 * \ingroup BOARD_LIB_FLASH
 * @defgroup  BOARD_LIB_FLASH_API  Application Interfaces
 */
/*@{*/
/**************************************************************************
 **                      API function Prototypes
 **************************************************************************/

/**
 *  @brief       Opens a flash device for use
 *
 *  @param[in]	 portNum   Peripheral port number attached to the flash device
 *  @param[in]   params    configuration parameters for the peripheral interface
 *
 *  @retval      NULL or Board_flashHandle.
 *
 *  @remark
 *               On success a handle is returned in which should be used in
 *				 all subsequent calls. As of now, the devices are not virtualized
 *and only one open may exist at a time for a particular device.
 *
 */
Board_flashHandle Board_flashOpen(uint32_t portNum, void *params);

/**
 *  @brief       Closes the device
 *
 *  @param[in]   handle  Handle to the device as returned in the open call.
 *
 *  @retval      BOARD_FLASH_EOK on Success
 *
 */
Board_flash_STATUS Board_flashClose(Board_flashHandle handle);

/**
 *  @brief       Reads a page from the device
 *
 *  @param[in]   handle Flash device handle from the open
 *  @param[in]   offset Offset to start the read from
 *  @param[in]   buf	Pointer to a buffer to read the data into
 *  @param[in] 	 len    Amount of data to read
 *
 *  @retval      BOARD_FLASH_EOK on Success
 *
 *  @remark      The buffer size should be page_size + spare_size
 *               The application should not write into the spare area
 *
 */
Board_flash_STATUS Board_flashRead(Board_flashHandle handle,
                                   uint32_t          offset,
                                   const uint8_t    *buf,
                                   uint32_t          len,
                                   uint8_t flash_chipSelect);

/**
 *  @brief      Write the data to the device
 *
 *  @param[in]  handle  Handle to the device as returned by open
 *  @param[in]  offset  Offset to start writing the data at.
 *  @param[in] 	buf     Pointer to  data to write
 *  @param[in] 	len     Length of the data pointed to by buf
 *
 *  @retval     BOARD_FLASH_EOK on Success
 *
 */
Board_flash_STATUS Board_flashWrite(Board_flashHandle handle,
                                    uint32_t          offset,
                                    uint8_t          *buf,
                                    uint32_t          len,
                                    uint8_t flash_chipSelect);

/**
 *  @brief      Convert the block and page number to offset
 *
 *  @param[in]  handle  Handle to the device as returned by open
 *  @param[in]  offset 		Offset to start writing the data at.
 *  @param[in] 	block       Block number
 *  @param[in] 	page        Page number
 *
 *  @retval     BOARD_FLASH_EOK on Success
 *
 *
 */
Board_flash_STATUS Board_flashBlkPageToOffset(Board_flashHandle handle,
                                              uint32_t         *offset,
                                              uint32_t          block,
                                              uint32_t          page);

/**
 *  @brief      Convert the offset to block and page number
 *
 *  @param[in]  handle  Handle to the device as returned by open
 *
 *  @param[in]  offset 		Offset to start writing the data at.
 *
 *  @param[in] 	block       Pointer to the block number
 *
 *  @param[in] 	page        Pointer to the Page number
 *
 *  @retval     BOARD_FLASH_EOK on Success
 *
 *
 */
Board_flash_STATUS Board_flashOffsetToBlkPage(Board_flashHandle handle,
                                              uint32_t          offset,
                                              uint32_t         *block,
                                              uint32_t         *page);

/**
 *  @brief      Convert the offset to sector and page number
 *
 *  @param[in]  handle              Handle to the device as returned by open
 *
 *  @param[in]  offset 		        Offset to start writing the data at.
 *
 *  @param[in] 	sector              Pointer to the sector number
 *
 *  @param[in] 	page                Pointer to the Page number
 *
 *  @param[in]  hybridSector_flag   Flag to enable Hybrid Sector Erase
 *
 *  @retval     BOARD_FLASH_EOK on Success
 *
 *
 */
Board_flash_STATUS Board_flashOffsetToSectorPage(Board_flashHandle handle,
                                                 uint32_t          offset,
                                                 uint32_t         *sector,
                                                 uint32_t         *page);

/**
 *  @brief       erase a block on the flash block
 *
 *  @param[in]   handle  Flash device handle from the open
 *
 *  @param[in]   blk_num Block ID to erase
 *
 *  @retval      BOARD_FLASH_EOK on Success
 *
 */
Board_flash_STATUS Board_flashErase(Board_flashHandle handle, uint32_t blk_num, bool blkErase_flag, uint8_t flash_chipSelect);

Board_flash_STATUS Board_flashEraseSector(Board_flashHandle handle, uint32_t sector_num, uint8_t flash_chipSelect);
Board_flash_STATUS Board_flashEraseBlk(Board_flashHandle handle, uint32_t blk_num, uint8_t flash_chipSelect);
Board_flash_STATUS Board_flashChipSelect(Board_flashHandle handle, uint8_t flash_chipSelect);

#ifdef __cplusplus
}
#endif

#endif

/* @} */
