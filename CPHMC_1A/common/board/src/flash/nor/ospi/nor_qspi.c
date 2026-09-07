/*
 * Copyright (c) 2018 - 2022, Texas Instruments Incorporated
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

#include "board/src/flash/nor/ospi/nor_qspi.h"
#include "board/src/flash/nor/nor.h"
#include "board/src/flash/include/board_flash.h"
#include <ti/drv/spi/soc/SPI_soc.h>
#include <ti/csl/soc.h>
#include <stdlib.h>
#include "board/src/flash/nor/device/nor_flash.h"

static NOR_HANDLE Nor_qspiOpen(uint32_t portNum, void *params);
static void Nor_qspiClose(NOR_HANDLE handle);
static NOR_STATUS Nor_qspiRead(NOR_HANDLE handle,
                               uint32_t addr,
                               uint32_t len,
                               uint8_t *buf);
static NOR_STATUS Nor_qspiWrite(NOR_HANDLE handle,
                                uint32_t addr,
                                uint32_t len,
                                uint8_t *buf);
static NOR_STATUS Nor_qspiErase(NOR_HANDLE handle, int32_t eraseIndex, bool blkErase);

static NOR_STATUS
Nor_qspiCmdWrite(OSPI_Handle handle, uint8_t *cmdBuf, uint32_t cmdLen, uint32_t dataLen);
static NOR_STATUS Nor_qspiWaitReady(NOR_Obj *nor, OSPI_Handle handle, uint32_t timeOut);


/* NOR function table for NOR OSPI interface implementation */
const NOR_FxnTable Nor_qspiFxnTable = {
    &Nor_qspiOpen,
    &Nor_qspiClose,
    &Nor_qspiRead,
    &Nor_qspiWrite,
    &Nor_qspiErase,
};

static NOR_STATUS NOR_qspiCmdRead(OSPI_Handle handle,
                                  uint8_t *cmdBuf,
                                  uint32_t cmdLen,
                                  uint8_t *rxBuf,
                                  uint32_t rxLen)
{
    OSPI_Transaction transaction;
    uint32_t transferType = SPI_TRANSACTION_TYPE_READ;
    bool ret;

    /* Update the mode and transfer type with the required values */
    OSPI_control(handle, OSPI_V0_CMD_SET_CFG_MODE, NULL);
    OSPI_control(handle, OSPI_V0_CMD_XFER_MODE_RW, (void *)&transferType);

    transaction.txBuf = (void *)cmdBuf;
    transaction.rxBuf = (void *)rxBuf;
    transaction.count = cmdLen + rxLen;

    ret = OSPI_transfer(handle, &transaction);
    if (BTRUE == ret)
    {
        return NOR_PASS;
    }
    else
    {
        return NOR_FAIL;
    }
}

static NOR_STATUS
Nor_qspiCmdWrite(OSPI_Handle handle, uint8_t *cmdBuf, uint32_t cmdLen, uint32_t dataLen)
{
    OSPI_Transaction transaction;

    uint32_t transferType = SPI_TRANSACTION_TYPE_WRITE;
    bool ret;

    /* Update the mode and transfer type with the required values */
    OSPI_control(handle, OSPI_V0_CMD_SET_CFG_MODE, NULL);
    OSPI_control(handle, OSPI_V0_CMD_XFER_MODE_RW, (void *)&transferType);

    transaction.txBuf = (void *)cmdBuf; /* Buffer includes command and write data */
    transaction.count = cmdLen + dataLen;
    transaction.rxBuf = NULL;
    transaction.arg = (void *)(uintptr_t)dataLen;

    ret = OSPI_transfer(handle, &transaction);
    if (BTRUE == ret)
    {
        return NOR_PASS;
    }
    else
    {
        return NOR_FAIL;
    }
}

static NOR_STATUS
Nor_qspiDataWrite(OSPI_Handle handle, uint32_t addr, uint8_t *buf, uint32_t data_len)
{
    OSPI_Transaction transaction;

    uint32_t transferType = SPI_TRANSACTION_TYPE_WRITE;
    bool ret;

    /* Set the transfer mode, write op code and tx lines */
    OSPI_control(handle, OSPI_V0_CMD_SET_XFER_MODE, NULL);
    OSPI_control(handle, OSPI_V0_CMD_XFER_MODE_RW, (void *)&transferType);
    /* Send Page Program command */
    transaction.arg = (void *)(uintptr_t)addr;
    transaction.txBuf = (void *)buf;
    transaction.rxBuf = NULL;
    transaction.count = data_len;

    ret = OSPI_transfer(handle, &transaction);
    if (BFALSE == ret)
    {
        return NOR_FAIL;
    }
    else
    {
        return NOR_PASS;
    }
}

static int32_t Nor_qspiReadId(OSPI_Handle handle, uint32_t *manfID, uint32_t *devID)
{
    NOR_STATUS retVal;
    uint8_t idCode[10] = {0};
    uint8_t cmd = 0x9FU; /*Jedec standard command*/
    uint32_t norIdNumByte = 3; /*Jedec ID len*/

    retVal = NOR_qspiCmdRead(handle, &cmd, 1, idCode, norIdNumByte);
    if (NOR_PASS == retVal)
    {
        *manfID = (uint32_t)idCode[0];
        *devID = ((uint32_t)idCode[1] << 8) | ((uint32_t)idCode[2]);
    }

    return retVal;
}

static NOR_Obj *Nor_qspiGetDevice(uint32_t manfID, uint32_t devID)
{
    NOR_Obj *norFlashObj =  NULL;

    for (int i = 0; nor_device_info[i].manufacturerId != 0; ++i)
    {
        if (manfID == nor_device_info[i].manufacturerId &&
            devID == nor_device_info[i].deviceId)
        {
            norFlashObj = (NOR_Obj *)malloc(sizeof(NOR_Obj));
            if (NULL == norFlashObj)
            {
                return NULL;
            }
            memset(norFlashObj, 0, sizeof(NOR_Obj));
            memcpy(&norFlashObj->info, &nor_device_info[i], sizeof(NOR_Info));
            memcpy(&norFlashObj->cmdTable, &nor_flash_cmd_table, sizeof(NOR_CmdTable));
            memcpy(&norFlashObj->cmdTime, &nor_flash_cmd_time, sizeof(NOR_cmdTime));
            break;
        }
    }
    return norFlashObj;
}

static NOR_STATUS Nor_qspiDisableWP(NOR_Obj *nor, OSPI_Handle handle)
{
    uint8_t cmdWren = nor->cmdTable.wrEn;
    uint8_t cmd = nor->cmdTable.rdSr;
    uint8_t sr = 0;
    uint8_t sr_mask = 0x43;
    if (NOR_qspiCmdRead(handle, &cmd, 1, &sr, 1))
    {
        return NOR_FAIL;
    }

    if (Nor_qspiCmdWrite(handle, &cmdWren, 1, 0))
    {
        return NOR_FAIL;
    }
    Nor_qspiWaitReady(nor, handle, 100);
    sr &= sr_mask;
    uint8_t data[2] = {nor->cmdTable.wrSr, sr};
    if (Nor_qspiCmdWrite(handle, data, 1, 1))
    {
        return NOR_FAIL;
    }
    Nor_qspiWaitReady(nor, handle, 100);
    if (NOR_qspiCmdRead(handle, &cmd, 1, &sr, 1))
    {
        return NOR_FAIL;
    }

    return NOR_PASS;
}

static NOR_STATUS Nor_qspiCfgFlash(NOR_Obj *nor, OSPI_Handle handle)
{
    OSPI_v0_HwAttrs const *hwAttrs = (OSPI_v0_HwAttrs const *)handle->hwAttrs;
    CSL_ospi_flash_cfgRegs *regAddr = (CSL_ospi_flash_cfgRegs *)(hwAttrs->baseAddr);

    NOR_STATUS retVal = NOR_PASS;
    uint8_t cmdWren = nor->cmdTable.wrEn;
    uint32_t opCode[3];
    uint32_t dummyCycles, rx_lines;
    uint8_t cmdRdSr = nor->cmdTable.rdSr;
    uint8_t sr = 0;

    dummyCycles = nor->info.rdDummyClk;
    rx_lines = OSPI_XFER_LINES_QUAD;
    opCode[0] = nor->cmdTable.rd;
    opCode[1] = nor->cmdTable.prog;
    opCode[2] = nor->cmdTable.rdSr;

    if (nor->info.manufacturerId == FLASH_MANF_ID_ISSI)
    {
        if (nor->info.deviceId == FLASH_DEVICE_ID_IS25LP128F ||
            nor->info.deviceId == FLASH_DEVICE_ID_IS25WP128F ||
            nor->info.deviceId == FLASH_DEVICE_ID_IS25LP256D ||
            nor->info.deviceId == FLASH_DEVICE_ID_IS25WP256D ||
            nor->info.deviceId == FLASH_DEVICE_ID_IS25LP01GJ ||
            nor->info.deviceId == FLASH_DEVICE_ID_IS25WP01GJ)
        {
            uint8_t qe_bit_pos = (uint8_t)(1 << 6);
            if (NOR_qspiCmdRead(handle, &cmdRdSr, 1, &sr, 1))
            {
                return NOR_FAIL;
            }

            if ((sr & qe_bit_pos) != 0)
            {
                /* QE is already set */
            }
            else
            {
                /*enable 1s-1s-4s read and write*/
                if (Nor_qspiCmdWrite(handle, &cmdWren, 1, 0))
                {
                    return NOR_FAIL;
                }
                Nor_qspiWaitReady(nor, handle, 100);
                sr |= qe_bit_pos;
                uint8_t data[2] = {nor->cmdTable.wrSr, sr};
                if (Nor_qspiCmdWrite(handle, data, 1, 1))
                {
                    return NOR_FAIL;
                }
                Nor_qspiWaitReady(nor, handle, 100);
                if (NOR_qspiCmdRead(handle, &cmdRdSr, 1, &sr, 1))
                {
                    return NOR_FAIL;
                }
            }
        }
    }
    else if (nor->info.manufacturerId == FLASH_MANF_ID_MICRON)
    {
        if (nor->info.deviceId == FLASH_DEVICE_ID_MT25QL128A ||
            nor->info.deviceId == FLASH_DEVICE_ID_MT25QL256A ||
            nor->info.deviceId == FLASH_DEVICE_ID_MT25QL01GB)
        {
            uint8_t bit_mask = 0x5C;
            if (NOR_qspiCmdRead(handle, &cmdRdSr, 1, &sr, 1))
            {
                return NOR_FAIL;
            }

            uint8_t cmdRdFlagSr = 0x70;
            uint8_t flagSr = 0;
            if (NOR_qspiCmdRead(handle, &cmdRdFlagSr, 1, &flagSr, 1))
            {
                return NOR_FAIL;
            }

            uint8_t cmdRdNonSr = 0xb5;
            uint8_t nonSr[2] = {0};
            if (NOR_qspiCmdRead(handle, &cmdRdNonSr, 1, nonSr, 2))
            {
                return NOR_FAIL;
            }

            //        if (Nor_qspiCmdWrite(handle, &cmdWren, 1, 0))
            //        {
            //            return NOR_FAIL;
            //        }
            //        Nor_qspiWaitReady(nor, handle, 10000);
            //
            //        nonSr[0] = 0xef;
            //        nonSr[1] = 0x8f;
            //        uint8_t data[3] = {0xb1, nonSr[0], nonSr[1]};
            //        if (Nor_qspiCmdWrite(handle, data, 1, 2))
            //        {
            //            return NOR_FAIL;
            //        }
            //        Nor_qspiWaitReady(nor, handle, 10000);
            //        nonSr[0] = 0;
            //        nonSr[1] = 0;
            //        if (NOR_qspiCmdRead(handle, &cmdRdNonSr, 1, nonSr, 2))
            //        {
            //            return NOR_FAIL;
            //        }

            if (nor->info.addrWidth == 4 && (nonSr[0] & 0x01))
            {
                if (Nor_qspiCmdWrite(handle, &cmdWren, 1, 0))
                {
                    return NOR_FAIL;
                }
                Nor_qspiWaitReady(nor, handle, 100);

                nonSr[0] &= 0xfe;
                uint8_t data[3] = {0xb1, nonSr[0], nonSr[1]};
                if (Nor_qspiCmdWrite(handle, data, 1, 2))
                {
                    return NOR_FAIL;
                }
                Nor_qspiWaitReady(nor, handle, 100);
                nonSr[0] = 0;
                if (NOR_qspiCmdRead(handle, &cmdRdNonSr, 1, nonSr, 2))
                {
                    return NOR_FAIL;
                }
            }
            else if (nor->info.addrWidth == 3 && !(nonSr[0] & 0x01))
            {
                if (Nor_qspiCmdWrite(handle, &cmdWren, 1, 0))
                {
                    return NOR_FAIL;
                }
                Nor_qspiWaitReady(nor, handle, 100);

                nonSr[0] |= 0xff;
                uint8_t data[3] = {0xb1, nonSr[0], nonSr[1]};
                if (Nor_qspiCmdWrite(handle, data, 1, 2))
                {
                    return NOR_FAIL;
                }
                Nor_qspiWaitReady(nor, handle, 100);
                nonSr[0] = 0;
                if (NOR_qspiCmdRead(handle, &cmdRdNonSr, 1, nonSr, 2))
                {
                    return NOR_FAIL;
                }
            }

            if ((sr & bit_mask) != 0)
            {
                if (Nor_qspiCmdWrite(handle, &cmdWren, 1, 0))
                {
                    return NOR_FAIL;
                }
                Nor_qspiWaitReady(nor, handle, 100);
                sr &= (~bit_mask);
                uint8_t data[2] = {nor->cmdTable.wrSr, sr};
                if (Nor_qspiCmdWrite(handle, data, 1, 1))
                {
                    return NOR_FAIL;
                }
                Nor_qspiWaitReady(nor, handle, 100);
                if (NOR_qspiCmdRead(handle, &cmdRdSr, 1, &sr, 1))
                {
                    return NOR_FAIL;
                }
            }
        }
    }

    OSPI_control(handle, OSPI_V0_CMD_RD_DUMMY_CLKS, (void *)&dummyCycles);
    OSPI_control(handle, OSPI_V0_CMD_SET_XFER_LINES, (void *)&rx_lines);
    OSPI_control(handle, OSPI_V0_CMD_XFER_OPCODE, (void *)opCode);


    uint32_t numAddrBytes;

    if (nor->info.addrWidth == 4)
    {
        numAddrBytes = CSL_OSPI_MEM_MAP_NUM_ADDR_BYTES_4;
    }
    else
    {
        numAddrBytes = CSL_OSPI_MEM_MAP_NUM_ADDR_BYTES_3;
    }

    /* Set device size cofigurations */
    CSL_ospiSetDevSize((const CSL_ospi_flash_cfgRegs *)(hwAttrs->baseAddr),
                       numAddrBytes,
                       nor->info.pageSize,
                       nor->info.blockSize);

    /* 1s-1s-4s read and write*/
    /* Transfer lines for Read */
    /* Set transfer lines for sending command */
    CSL_REG32_FINS(&regAddr->DEV_INSTR_RD_CONFIG_REG,
                   OSPI_FLASH_CFG_DEV_INSTR_RD_CONFIG_REG_INSTR_TYPE_FLD,
                   CSL_OSPI_CFG_XFER_LINES_SINGLE);
    /* Set transfer lines for sending address */
    CSL_REG32_FINS(&regAddr->DEV_INSTR_RD_CONFIG_REG,
                   OSPI_FLASH_CFG_DEV_INSTR_RD_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_FLD,
                   CSL_OSPI_CFG_XFER_LINES_SINGLE);
    /* Set transfer lines for sending data */
    CSL_REG32_FINS(&regAddr->DEV_INSTR_RD_CONFIG_REG,
                   OSPI_FLASH_CFG_DEV_INSTR_RD_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_FLD,
                   CSL_OSPI_CFG_XFER_LINES_QUAD);

    /* Transfer lines for Write */
    /* Set transfer lines for sending address */
    CSL_REG32_FINS(&regAddr->DEV_INSTR_WR_CONFIG_REG,
                   OSPI_FLASH_CFG_DEV_INSTR_WR_CONFIG_REG_ADDR_XFER_TYPE_STD_MODE_FLD,
                   CSL_OSPI_CFG_XFER_LINES_SINGLE);
    /* Set transfer lines for sending data */
    CSL_REG32_FINS(&regAddr->DEV_INSTR_WR_CONFIG_REG,
                   OSPI_FLASH_CFG_DEV_INSTR_WR_CONFIG_REG_DATA_XFER_TYPE_EXT_MODE_FLD,
                   CSL_OSPI_CFG_XFER_LINES_QUAD);

    return retVal;
}

static NOR_STATUS Nor_qspiXipEnable(OSPI_Handle handle) { return NOR_PASS; }

NOR_HANDLE Nor_qspiOpen(uint32_t portNum, void *params)
{
    OSPI_Params spiParams; /* SPI params structure */
    OSPI_Handle hwHandle;  /* SPI handle */
    NOR_Obj *norFlashObj;
    NOR_HANDLE norHandle = 0;
    uint32_t delay;
    uint32_t readCnt = 0;
    uint32_t readStart = 0;
    uint32_t readCntPrv = 0;
    uint32_t readStartPrv = 0;
    OSPI_v0_HwAttrs ospiCfg;
    uint32_t manfID, devID;

    if (params)
    {
        memcpy(&spiParams, params, sizeof(OSPI_Params));
    }
    else
    {
        /* Use default SPI config params if no params provided */
        OSPI_Params_init(&spiParams);
    }
    hwHandle = OSPI_open(SPI_OSPI_DOMAIN_MCU, portNum, &spiParams);

    if (hwHandle)
    {
        OSPI_socGetInitCfg(SPI_OSPI_DOMAIN_MCU, portNum, &ospiCfg);
        if (BTRUE == ospiCfg.phyEnable)
        {
            /* set initial PHY DLL delay */
            delay = 0U;
            OSPI_control(hwHandle, OSPI_V0_CMD_CFG_PHY, (void *)(&delay));

            /* calibrate PHY */
            for (uint32_t i = 0; i < 128U; i++)
            {
                if (NOR_PASS == Nor_qspiReadId(hwHandle, &manfID, &devID))
                {
                    /* Iterate flash reads, find the start index and successful read ID count */
                    if (0U == readCnt)
                        readStart = i;
                    readCnt++;
                }
                else
                {
                    if ((0U != readCnt) && (readCnt > readCntPrv))
                    {
                        /* save the start index and most successful read ID count */
                        readCntPrv = readCnt;
                        readStartPrv = readStart;
                        readCnt = 0;
                        readStart = 0;
                    }
                }

                /* Increment DLL delay */
                OSPI_control(hwHandle, OSPI_V0_CMD_CFG_PHY, NULL);
            }

            if (readCnt > readCntPrv)
            {
                readCntPrv = readCnt;
                readStartPrv = readStart;
            }

            if (0U != readCntPrv)
            {
                if (NOR_PASS == Nor_qspiReadId(hwHandle, &manfID, &devID))
                {
                    if (NOR_PASS == Nor_qspiReadId(hwHandle, &manfID, &devID))
                    {
                        norFlashObj  = Nor_qspiGetDevice(manfID, devID);
                        if (norFlashObj)
                        {
                            norFlashObj->hwHandle = (uintptr_t)hwHandle;
                            norHandle = (NOR_HANDLE)norFlashObj;
                        }
                        else
                        {
                            return norHandle;
                        }
                    }
                    else
                    {
                        OSPI_close(hwHandle);
                        return norHandle;
                    }
                }
            }
            else
            {
                OSPI_close(hwHandle);
                return norHandle;
            }
        }
        else /* ospi_cfg->phyEnable == BFALSE */
        {
            if (NOR_PASS == Nor_qspiReadId(hwHandle, &manfID, &devID))
            {
                norFlashObj  = Nor_qspiGetDevice(manfID, devID);
                if (norFlashObj)
                {
                    norFlashObj->hwHandle = (uintptr_t)hwHandle;
                    norHandle = (NOR_HANDLE)norFlashObj;
                    Nor_qspiCfgFlash(norFlashObj, hwHandle);
                }
                else
                {
                    return norHandle;
                }
            }
            else
            {
                OSPI_close(hwHandle);
                return norHandle;
            }
        }

        if (BTRUE == ospiCfg.xipEnable)
        {
            Nor_qspiXipEnable(hwHandle);
        }
    }

    return norHandle;
}

void Nor_qspiClose(NOR_HANDLE handle)
{
    NOR_Obj *norFlashObj;
    OSPI_Handle spiHandle;

    if (handle)
    {
        norFlashObj = (NOR_Obj *)handle;
        spiHandle = (OSPI_Handle)norFlashObj->hwHandle;

        if (spiHandle)
        {
            OSPI_close(spiHandle);
        }
        free(norFlashObj);
    }
}

static NOR_STATUS Nor_qspiWaitReady(NOR_Obj *nor, OSPI_Handle handle, uint32_t timeOut)
{
    uint8_t status;
    uint8_t cmd = nor->cmdTable.rdSr;

    do
    {
        if (NOR_qspiCmdRead(handle, &cmd, 1, &status, 1))
        {
            return NOR_FAIL;
        }
        if (0U == (status & nor->info.srWip))
        {
            return NOR_PASS;
        }

        timeOut--;
        if (!timeOut)
        {
            /* Timed out */
            return NOR_FAIL;
        }
    } while (1);
}

NOR_STATUS Nor_qspiRead(NOR_HANDLE handle,
                        uint32_t addr,
                        uint32_t len,
                        uint8_t *buf)
{
    OSPI_Transaction transaction;
    NOR_Obj *norFlashObj;
    OSPI_Handle spiHandle;
    bool ret;
    uint32_t transferType = SPI_TRANSACTION_TYPE_READ;

    if (!handle)
    {
        return NOR_FAIL;
    }

    norFlashObj = (NOR_Obj *)handle;
    if (!norFlashObj->hwHandle)
    {
        return NOR_FAIL;
    }
    spiHandle = (OSPI_Handle)norFlashObj->hwHandle;

    /* Validate address input */
    if (norFlashObj->info.totalSize < (addr + len))
    {
        return NOR_FAIL;
    }

    if (Nor_qspiWaitReady(norFlashObj, spiHandle, norFlashObj->cmdTime.wrsr))
    {
        return NOR_FAIL;
    }

    /* Set transfer mode and read type */
    OSPI_control(spiHandle, OSPI_V0_CMD_SET_XFER_MODE, NULL);
    OSPI_control(spiHandle, OSPI_V0_CMD_XFER_MODE_RW, (void *)&transferType);

    transaction.arg = (void *)(uintptr_t)addr;
    transaction.txBuf = NULL;
    transaction.rxBuf = (void *)buf;
    transaction.count = len;

    ret = OSPI_transfer(spiHandle, &transaction);
    if (BTRUE == ret)
    {
        return NOR_PASS;
    }
    else
    {
        return NOR_FAIL;
    }
}

NOR_STATUS Nor_qspiWrite(NOR_HANDLE handle,
                         uint32_t addr,
                         uint32_t len,
                         uint8_t *buf)
{
    NOR_Obj *norFlashObj;
    OSPI_Handle spiHandle;
    uint32_t wrSize;
    uint32_t actual;
    uint32_t offset;
    OSPI_v0_HwAttrs *hwAttrs;

    if (!handle)
    {
        return NOR_FAIL;
    }

    norFlashObj = (NOR_Obj *)handle;
    if (!norFlashObj->hwHandle)
    {
        return NOR_FAIL;
    }

    /* Validate address input */
    if (norFlashObj->info.totalSize < (addr + len))
    {
        return NOR_FAIL;
    }

    spiHandle = (OSPI_Handle)norFlashObj->hwHandle;
    hwAttrs = (OSPI_v0_HwAttrs *)spiHandle->hwAttrs;

    if (hwAttrs->dacEnable)
    {
        /* direct access transfer mode */
        if ((hwAttrs->dmaEnable) && (hwAttrs->phyEnable))
        {
            wrSize = 16U;
        }
    }
    else
    {
        /* indirect access transfer mode */
        wrSize = norFlashObj->info.pageSize;
    }

    for (offset = 0U; offset < len; offset += actual)
    {
        actual = (len - offset) > wrSize ? wrSize : (len - offset);
        if (Nor_qspiWaitReady(norFlashObj, spiHandle, norFlashObj->cmdTime.wrsr))
        {
            return NOR_FAIL;
        }

        if (Nor_qspiCmdWrite(spiHandle, &norFlashObj->cmdTable.wrEn, 1, 0))
        {
            return NOR_FAIL;
        }

        if (Nor_qspiWaitReady(norFlashObj, spiHandle, norFlashObj->cmdTime.wrsr))
        {
            return NOR_FAIL;
        }

        if (Nor_qspiDataWrite(spiHandle, addr, buf + offset, actual))
        {
            return NOR_FAIL;
        }

        addr += actual;
    }

    return NOR_PASS;
}

NOR_STATUS Nor_qspiErase(NOR_HANDLE handle, int32_t eraseIndex, bool blkErase)
{
    uint8_t cmd[5];
    uint32_t cmdLen;
    uint32_t address = 0;
    uint8_t cmdWren;
    NOR_Obj *norFlashObj;
    OSPI_Handle spiHandle;

    OSPI_v0_HwAttrs const *hwAttrs;

    if (!handle)
    {
        return NOR_FAIL;
    }

    norFlashObj = (NOR_Obj *)handle;
    if (!norFlashObj->hwHandle)
    {
        return NOR_FAIL;
    }
    cmdWren = norFlashObj->cmdTable.wrEn;
    spiHandle = (OSPI_Handle)norFlashObj->hwHandle;
    hwAttrs = (OSPI_v0_HwAttrs const *)spiHandle->hwAttrs;

    if (BTRUE == blkErase)
    {
        if (norFlashObj->info.blockCnt <= eraseIndex)
        {
            return NOR_FAIL;
        }
        address = eraseIndex * norFlashObj->info.blockSize;
        cmd[0] = norFlashObj->cmdTable.blockErase;
    }
    else
    {
        if (norFlashObj->info.sectorCnt <= eraseIndex)
        {
            return NOR_FAIL;
        }
        address = eraseIndex * norFlashObj->info.sectorSize;
        cmd[0] = norFlashObj->cmdTable.sectorErase;
    }

    if (norFlashObj->info.addrWidth == 4)
    {
        cmd[1] = (address >> 24) & 0xFF; /* 4 address bytes */
        cmd[2] = (address >> 16) & 0xFF;
        cmd[3] = (address >> 8) & 0xFF;
        cmd[4] = (address >> 0) & 0xFF;
        cmdLen = 5U;
    }
    else
    {
        cmd[1] = (address >> 16) & 0xFF;
        cmd[2] = (address >> 8) & 0xFF;
        cmd[3] = (address >> 0) & 0xFF;
        cmdLen = 4U;
    }

    if (Nor_qspiCmdWrite(spiHandle, &cmdWren, 1, 0))
    {
        return NOR_FAIL;
    }

    if (Nor_qspiWaitReady(norFlashObj, spiHandle, norFlashObj->cmdTime.wrsr))
    {
        return NOR_FAIL;
    }

    if (Nor_qspiCmdWrite(spiHandle, cmd, cmdLen, 0))
    {
        return NOR_FAIL;
    }

    if (Nor_qspiWaitReady(norFlashObj, spiHandle, norFlashObj->cmdTime.bulkErase))
    {
        return NOR_FAIL;
    }

    return NOR_PASS;
}
