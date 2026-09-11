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
 *
 */

/**
 *
 * \file    board_flash.c
 *
 * \brief   This contains board flash common APIs.
 *
 ******************************************************************************/

#include "board/board_cfg.h"
#include "board/src/flash/include/board_flash.h"
#include "ti/csl/soc/j721e/src/cslr_soc_baseaddress.h"
#include "ti/csl/src/ip/spinlock/V1/csl_spinlock.h"
#include "ti/csl/src/ip/ospi/V0/csl_ospi.h"
#include "ti/drv/spi/src/v0/OSPI_v0.h"
#ifdef BUILD_MCU1_0
#define FLASH_SPINK_LOCK_ID  (0x00u)
#else
#include "bsp_init.h"
#endif
/* This structure holds information about the flash devices on the board */
Board_FlashInfo Board_flashInfo[MAX_BOARD_FLASH_INSTANCE_NUM] = {
    {
        0,
    },
};
/******************************************************************************
 * BOARD_flashOpen
 ******************************************************************************/
Board_flashHandle Board_flashOpen(uint32_t portNum, void *params)
{
    uint32_t count;
    Board_FlashInfo *flashInfo;
    uint32_t flashIntf;

    /* spin till lock is acquired */
    while (1)
    {
        if (SPINLOCKLockStatusSet(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID) ==
            CSL_SPINLOCK_VAL_FREE)
        {
            break;
        }
    }

    for (count = 0U; count < MAX_BOARD_FLASH_INSTANCE_NUM; count++)
    {
        if (0U == Board_flashInfo[count].flashHandle)
        {
            flashInfo = &Board_flashInfo[count];
            break;
        }
    }

    if (MAX_BOARD_FLASH_INSTANCE_NUM == count)
    {
        goto out;
    }

    flashIntf = BOARD_FLASH_NOR_QSPI;

    NOR_HANDLE flashHandle;
    NOR_Obj *norFlashObj;

    /* Open the NOR flash */
    flashHandle = NOR_open(flashIntf, portNum, params);
    if (!flashHandle)
    {
        goto out;
    }

    norFlashObj = (NOR_Obj *)flashHandle;
    flashInfo->flashHandle = flashHandle;
    flashInfo->manufacturer_id = norFlashObj->info.manufacturerId;
    flashInfo->device_id = norFlashObj->info.deviceId;
    flashInfo->type = BOARD_FLASH_NOR;
    flashInfo->block_count = norFlashObj->info.blockCnt;
    flashInfo->page_count = norFlashObj->info.pageCnt;
    flashInfo->page_size = norFlashObj->info.pageSize;
    flashInfo->sector_size = norFlashObj->info.sectorSize;
    flashInfo->sector_cnt = norFlashObj->info.sectorCnt;

    SPINLOCKLockStatusFree(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID);
    return (Board_flashHandle)flashInfo;
out:
    SPINLOCKLockStatusFree(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID);
    return 0;
}

/******************************************************************************
 * BOARD_flashClose
 ******************************************************************************/
Board_flash_STATUS Board_flashClose(Board_flashHandle handle)
{
    Board_FlashInfo *flashInfo;
    Board_flash_STATUS revt;

    /* spin till lock is acquired */
    while (1)
    {
        if (SPINLOCKLockStatusSet(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID) ==
            CSL_SPINLOCK_VAL_FREE)
        {
            break;
        }
    }

    if (!handle)
    {
        revt = BOARD_FLASH_EFAIL;
        goto out;
    }

    flashInfo = (Board_FlashInfo *)handle;
    if (!flashInfo->flashHandle)
    {
        revt = BOARD_FLASH_EFAIL;
        goto out;
    }

    NOR_close(flashInfo->flashHandle);
    flashInfo->flashHandle = 0;
    revt = BOARD_FLASH_EOK;
out:
    SPINLOCKLockStatusFree(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID);
    return revt;
}

/******************************************************************************
 * BOARD_flashRead
 ******************************************************************************/
Board_flash_STATUS
Board_flashRead(Board_flashHandle handle, uint32_t offset, const uint8_t *buf, uint32_t len)
{
    Board_FlashInfo *flashInfo;
    Board_flash_STATUS revt;

    /* spin till lock is acquired */
    while (1)
    {
        if (SPINLOCKLockStatusSet(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID) ==
            CSL_SPINLOCK_VAL_FREE)
        {
            break;
        }
    }

    if (!handle)
    {
        revt = BOARD_FLASH_EFAIL;
        goto out;
    }

    flashInfo = (Board_FlashInfo *)handle;
    if (!flashInfo->flashHandle)
    {
        revt = BOARD_FLASH_EFAIL;
        goto out;
    }

    if (0U == len)
    {
        revt = BOARD_FLASH_EINVALID;
        goto out;
    }

    if (NOR_PASS != NOR_read(flashInfo->flashHandle, offset, len, buf))
    {
        revt = BOARD_FLASH_EFAIL;
    }
    else
    {
        revt = BOARD_FLASH_EOK;
    }
out:
    SPINLOCKLockStatusFree(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID);
    return revt;
}

/**
 *  \brief	This function computes the sector and page based on an offset
 *              This function also sets the blkErase_flag to BFALSE to support
 *              Sector erase. This flag is used by the Board_flashErase()
 *              to determine the erase type
 *
 *	\param	handle		    [IN]  Pointer to Board_FlashHandle.
 *              offset		    [IN]  Flash Offset to be computed to Sector
 *              sector              [IN]  Computed Sector number
 *              page                [IN]  Computed page number
 *
 *  \return		Returns Board_flash_STATUS
 *
 */
Board_flash_STATUS Board_flashOffsetToSectorPage(Board_flashHandle handle,
                                                 uint32_t offset,
                                                 uint32_t *sector,
                                                 uint32_t *page)
{
    uint32_t leftover, sector_count, page_size, page_count;
    Board_FlashInfo *flashInfo;

    if (!handle)
    {
        return BOARD_FLASH_EFAIL;
    }

    flashInfo = (Board_FlashInfo *)handle;
    if (!flashInfo->flashHandle)
    {
        return BOARD_FLASH_EFAIL;
    }

    page_size = flashInfo->page_size;
    page_count = flashInfo->page_count;
    sector_count = flashInfo->sector_cnt;

    *sector = offset / flashInfo->sector_size;
    leftover = offset % flashInfo->sector_size;
    *page = leftover / page_size;
    if (leftover % page_size)
    {
        /* All writes must be page aligned for now */
        return BOARD_FLASH_EUNSUPPORTED;
    }
    if (*sector > sector_count)
    {
        return BOARD_FLASH_EINVALID;
    }
    if (*page > page_count)
    {
        return BOARD_FLASH_EINVALID;
    }

    return BOARD_FLASH_EOK;
}

/**
 *  \brief		This function computes the block and page number based on the
 *              offset. This function also sets the blkErase_flag to BTRUE to
 *              support Block erase. This flag is used by the
 *              Board_flashErase() to determine the erase type.
 *
 *	\param		handle		        [IN]   	Pointer to Board_FlashHandle.
 *              offset		      [IN]   	Flash Offset to be computed to Sector
 *              page            [IN]    Computed Page number
 *              page            [IN]    Computed page number
 *
 *  \return		Returns Board_flash_STATUS
 *
 */
Board_flash_STATUS Board_flashOffsetToBlkPage(Board_flashHandle handle,
                                              uint32_t offset,
                                              uint32_t *block,
                                              uint32_t *page)
{
    uint32_t leftover, block_size, block_count, page_size, page_count;
    Board_FlashInfo *flashInfo;

    if (!handle)
    {
        return BOARD_FLASH_EFAIL;
    }

    flashInfo = (Board_FlashInfo *)handle;
    if (!flashInfo->flashHandle)
    {
        return BOARD_FLASH_EFAIL;
    }

    block_count = flashInfo->block_count;
    page_size = flashInfo->page_size;
    page_count = flashInfo->page_count;
    block_size = (page_count * page_size);

    *block = offset / block_size;
    leftover = offset % block_size;
    *page = leftover / page_size;
    if (leftover % page_size)
    {
        /* All writes must be page aligned for now */
        return BOARD_FLASH_EUNSUPPORTED;
    }
    if (*block > block_count)
    {
        return BOARD_FLASH_EINVALID;
    }
    if (*page > page_count)
    {
        return BOARD_FLASH_EINVALID;
    }

    return BOARD_FLASH_EOK;
}

/******************************************************************************
 * Computes a block and page based on an offset
 ******************************************************************************/
Board_flash_STATUS Board_flashBlkPageToOffset(Board_flashHandle handle,
                                              uint32_t *offset,
                                              uint32_t block,
                                              uint32_t page)
{
    uint32_t block_count, page_size, page_count;
    Board_FlashInfo *flashInfo;

    if (!handle)
    {
        return BOARD_FLASH_EFAIL;
    }

    flashInfo = (Board_FlashInfo *)handle;
    if (!flashInfo->flashHandle)
    {
        return BOARD_FLASH_EFAIL;
    }

    block_count = flashInfo->block_count;
    page_size = flashInfo->page_size;
    page_count = flashInfo->page_count;
    if ((block > block_count) || (page > page_count))
    {
        return BOARD_FLASH_EINVALID;
    }

    *offset = (block * (page_count * page_size)) + (page * page_size);

    return BOARD_FLASH_EOK;
}

/******************************************************************************
 * BOARD_flashWrite
 ******************************************************************************/
Board_flash_STATUS
Board_flashWrite(Board_flashHandle handle, uint32_t offset, uint8_t *buf, uint32_t len)
{
    Board_FlashInfo *flashInfo;
    Board_flash_STATUS revt;

    /* spin till lock is acquired */
    while (1)
    {
        if (SPINLOCKLockStatusSet(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID) ==
            CSL_SPINLOCK_VAL_FREE)
        {
            break;
        }
    }

    if (!handle)
    {
        revt = BOARD_FLASH_EFAIL;
        goto out;
    }

    flashInfo = (Board_FlashInfo *)handle;
    if (!flashInfo->flashHandle)
    {
        revt = BOARD_FLASH_EFAIL;
        goto out;
    }

    if ((NULL == buf) || (0U == len))
    {
        revt = BOARD_FLASH_EINVALID;
        goto out;
    }

    if (NOR_PASS != NOR_write(flashInfo->flashHandle, offset, len, buf))
    {
        revt = BOARD_FLASH_EFAIL;
    }
    else
    {
        revt = BOARD_FLASH_EOK;
    }

out:
    SPINLOCKLockStatusFree(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID);
    return revt;
}

/**
 *  \brief  platform_device_erase_block
 *
 *  Board_flashErase supports Block and Sector erase based on the blkErase_flag
 *  This API expects blkErase_flag to be set to the appropriate Boolean value to
 *  determine the type of erase.
 *
 *  blkErase_flag is set in one of the Board_flashOffsetToSectorPage() or
 *  Board_flashOffsetToBlkPage() APIs for sector or block erase respectively.
 *
 *  \param
 *  handle  - Handle to the flash device
 *  blk_num - Block number to be erased when blkErase_flag=BTRUE
 *            Sector number to be erased when blkErase_flag=BFALSE
 *
 *
 ********************************************************************************/
Board_flash_STATUS Board_flashErase(Board_flashHandle handle, uint32_t blk_num, bool blkErase_flag)
{
    Board_FlashInfo *flashInfo;
    Board_flash_STATUS revt = BOARD_FLASH_EOK;

    /* spin till lock is acquired */
    while (1)
    {
        if (SPINLOCKLockStatusSet(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID) ==
            CSL_SPINLOCK_VAL_FREE)
        {
            break;
        }
    }

    if (!handle)
    {
        revt = BOARD_FLASH_EFAIL;
        goto out;
    }

    flashInfo = (Board_FlashInfo *)handle;
    if (!flashInfo->flashHandle)
    {
        revt = BOARD_FLASH_EFAIL;
        goto out;
    }

    if (NOR_PASS != NOR_erase(flashInfo->flashHandle, (int32_t)blk_num, blkErase_flag))
    {
        revt = BOARD_FLASH_EFAIL;
    }
out:
    SPINLOCKLockStatusFree(CSL_NAVSS0_SPINLOCK_BASE, FLASH_SPINK_LOCK_ID);
    return revt;
}

Board_flash_STATUS Board_flashEraseSector(Board_flashHandle handle, uint32_t sector_num)
{
    return Board_flashErase(handle, sector_num, 0);
}

Board_flash_STATUS Board_flashEraseBlk(Board_flashHandle handle, uint32_t blk_num)
{
    return Board_flashErase(handle, blk_num, 1);
}

/* Add a chip select function */
Board_flash_STATUS Board_flashChipSelect(Board_flashHandle handle, uint8_t flash_chipSelect)
{
    const CSL_ospi_flash_cfgRegs *baseAddr = (const CSL_ospi_flash_cfgRegs *)(CSL_MCU_FSS0_OSPI0_CTRL_BASE);
    OSPI_v0_HwAttrs *hwAttrs;
    OSPI_Handle spiHandle;
    NOR_Obj *norFlashObj;
    Board_FlashInfo *flashInfo;
    Board_flash_STATUS revt;

    if (flash_chipSelect > 4)
        flash_chipSelect = 0;

    if (!handle)
    {
        revt = BOARD_FLASH_EINVALID;
        goto out;
    }

    flashInfo = (Board_FlashInfo *)handle;
    if (!flashInfo->flashHandle)
    {
        revt = BOARD_FLASH_EFAIL;
        goto out;
    }

    norFlashObj = (NOR_Obj *)flashInfo->flashHandle;
    if (!norFlashObj->hwHandle)
    {
        revt = BOARD_FLASH_EINVALID;
        goto out;
    }

    spiHandle = (OSPI_Handle)norFlashObj->hwHandle;
    hwAttrs = (OSPI_v0_HwAttrs *)spiHandle->hwAttrs;
    hwAttrs->chipSelect = flash_chipSelect;
    CSL_ospiSetChipSelect(baseAddr, flash_chipSelect, 0);

    revt = BOARD_FLASH_EOK;
out:
    return revt;
}
