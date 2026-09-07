/************************************************* *****************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       eeprom_at24c.c
 *@author     XuQuanbing
 *@date       2024.10.28
 *@brief      EEPROM function source file
 *@par        History
 *Date        Version   Author       Description
 *2024.10.28  1.0       XuQuanbing   Initial version
 ******************************************************************************/
/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "i2c_bus_init.h"
#include <string.h>
#include <stdint.h>
#include "debug_config.h"
#include "eeprom_at24c.h"
#include <ti/osal/SemaphoreP.h>
#include "ti/csl/src/ip/spinlock/V1/csl_spinlock.h"
#include "ti/csl/soc/j721e/src/cslr_soc.h"
#include "ti/osal/CacheP.h"
#include "bsp_init.h"
#include "ti/osal/osal.h"

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
/* For offset more than this, the I2C address needs to be incremented */

const uint32_t eeprom_pageSize = 64U;
const uint32_t eeprom_size = 32768;

#define TEST_DATA_VALUE     0x45
#define EEPROM_TEST_ADDRESS 0x20 /* test read data address*/
#define TEST_LENGTH         64
static uint8_t readData[TEST_LENGTH], writeData[TEST_LENGTH];

eeprom_diag_t eeprom_diag = {0};

// Eeprom open flag bit
uint16_t gEepromOpenFlag __attribute__((aligned(4), section(".bss.flag_shared_eeprom"), used));

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */
int16_t EEPROM_AT24C_open(void)
{
    int16_t  status;
    uint32_t i2cAddress;

    i2cAddress = EEPROM_Address;
    /* Determine if I2C sensor is present */
    status = I2C_control(i2c1_handle, I2C_CMD_PROBE, &i2cAddress);

    if (status == I2C_STATUS_SUCCESS)
    {
        gEepromOpenFlag = 0x55AA;
        CacheP_wb(&gEepromOpenFlag, sizeof(gEepromOpenFlag));
    }
    else
    {
        EEPROM_log("EEPROM DEVICE found not found at device address 0x%02x \r\n", EEPROM_Address);
    }

    return status;
}

void EEPROM_AT24C_close(I2C_Handle handle)
{
    gEepromOpenFlag = 0x0000;
    CacheP_wb(&gEepromOpenFlag, sizeof(gEepromOpenFlag));

    if (NULL != handle)
    {
        I2C_close(handle);
    }
}


int16_t EEPROM_AT24C_read(I2C_Handle handle,
                          uint32_t   offset,
                          uint8_t   *buf,
                          uint32_t   len)
{
    int16_t  status = I2C_STS_SUCCESS;
    uint32_t i2cAddress;
    uint32_t readLen, remainderLen;
    uint32_t readOffset, remainderOffset;
    uint8_t  offsetBuf[2U];

    I2C_Transaction i2ctransaction;

    if ((NULL == handle) || (NULL == buf))
    {
        status = I2C_STS_ERR;
    }

    /* Validate address input */
    if ((offset + len) > eeprom_size)
    {
        status = I2C_STS_ERR;
    }

    if (status == I2C_STS_SUCCESS)
    {
        i2cAddress = EEPROM_Address;

        /* Calculate params for 64KB boundary crossing */
        /* Length to be read this time */
        readLen = 0U;
        /* offset need to read this time */
        readOffset = 0U;
        /* initialization offset after remainder offset */
        remainderOffset = offset;

        if (offset < EEPROM_AT24C_WRAP_OFFSET)
        {
            readLen = len;
            /* For crossing wrap page boundary to readdata */
            if ((readLen + offset) > EEPROM_AT24C_WRAP_OFFSET)
            {
                readLen = (EEPROM_AT24C_WRAP_OFFSET - offset);
            }
            readOffset = offset;
            remainderOffset = EEPROM_AT24C_WRAP_OFFSET;
        }
        remainderLen = len - readLen;

        /* Handle reads for less than 64KB boundary - if any */
        if (readLen != 0U)
        {
            /* Perform dummy write to set the right offset and read data */
            offsetBuf[0U] = (readOffset & 0xFF00U) >> 8U;
            offsetBuf[1U] = (readOffset & 0x00FFU);
            I2C_transactionInit(&i2ctransaction);
            i2ctransaction.slaveAddress = i2cAddress;
            i2ctransaction.writeBuf = &offsetBuf[0U];
            i2ctransaction.writeCount = 2;
            i2ctransaction.readBuf = buf;
            i2ctransaction.readCount = readLen;
            i2ctransaction.timeout = 1000U;
            status = I2C_transfer(handle, &i2ctransaction);
        }

        /* Handle reads for more than 64KB boundary - if any */
        if ((status == I2C_STS_SUCCESS) && (remainderLen != 0U))
        {
            /* For offset more than 64KB we need to perform seperate read
             * operation with I2C device address as +1 -> Bit 1 (A16) set to 1 */
            i2cAddress++;

            /* Perform dummy write operation to set the right offset */
            offsetBuf[0U] = (remainderOffset & 0x0000FF00U) >> 8U;
            offsetBuf[1U] = (remainderOffset & 0x000000FFU);
            I2C_transactionInit(&i2ctransaction);
            i2ctransaction.writeBuf = &offsetBuf[0U];
            i2ctransaction.writeCount = 2U;
            i2ctransaction.slaveAddress = i2cAddress;
            i2ctransaction.readBuf = buf + readLen;
            i2ctransaction.readCount = remainderLen;
            i2ctransaction.timeout = 10000U;
            status = I2C_transfer(handle, &i2ctransaction);
        }
    }
    return (status);
}

int16_t EEPROM_AT24C_write(I2C_Handle     handle,
                           uint32_t       offset,
                           const uint8_t *buf,
                           uint32_t       len)
{
    uint32_t i2cAddress;
    uint32_t curWriteLen, curOffset, bytesWritten;
    uint8_t  pageWrBuf[EEPROM_WR_BUF_SIZE];
    uint8_t  dummyRead;
    int16_t  status = I2C_STS_SUCCESS;
    int16_t  writeStatus;

    I2C_Transaction i2ctransaction;
    if ((NULL == handle) || (NULL == buf))
    {
        status = I2C_STS_ERR;
    }

    /* Validate address input */
    if ((offset + len) > eeprom_size)
    {
        status = I2C_STS_ERR;
    }

    if (status == I2C_STS_SUCCESS)
    {
        /* Perform page by page write operation */
        curOffset = offset;
        bytesWritten = 0U;
        while (len > 0)
        {
            /* For offset more than 64KB we need to perform seperate write
             * operation with I2C device address as +1 -> Bit 1 (A16) set to 1 */

            i2cAddress = EEPROM_Address;

            if (curOffset >= EEPROM_AT24C_WRAP_OFFSET) /* AT24C256 no need */
            {
                i2cAddress++;
            }

            /* Calculate num bytes to write for this iteration */
            curWriteLen = eeprom_pageSize;
            if (curOffset % eeprom_pageSize)
            {
                /* For first non-page boundary writes */
                curWriteLen = eeprom_pageSize - (curOffset % eeprom_pageSize);
            }
            /* Adjust for last chunk */
            if (curWriteLen > len)
            {
                curWriteLen = len;
            }

            /* Perform dummy write operation to set the right offset */
            pageWrBuf[0U] = (curOffset & 0xFF00U) >> 8U;
            pageWrBuf[1U] = (curOffset & 0x00FFU);
            memcpy(&pageWrBuf[2U], buf + bytesWritten, curWriteLen);
            I2C_transactionInit(&i2ctransaction);
            i2ctransaction.writeBuf = pageWrBuf;
            i2ctransaction.writeCount = 2U + curWriteLen; /* 2 --- datatoffset  start */
            i2ctransaction.slaveAddress = i2cAddress;
            i2ctransaction.timeout = 1000U;
            /* Perform write operation */
            status = I2C_transfer(handle, &i2ctransaction);
            if (I2C_STS_SUCCESS != status)
            {
                /* return if the write operation is not successful. */
                break;
            }

            /* After write operation flash will not respond for write cycle time.
             * This is approximately 4ms (min). makesure write data successfully */
            Osal_delay(5);

            /* Wait for write to finish */
            I2C_transactionInit(&i2ctransaction);
            i2ctransaction.writeBuf = pageWrBuf;
            i2ctransaction.writeCount = 2U;
            i2ctransaction.readBuf = &dummyRead;
            i2ctransaction.readCount = 1U;
            i2ctransaction.slaveAddress = i2cAddress;
            i2ctransaction.timeout = 1000U;
            while (1U)
            {
                writeStatus = I2C_transfer(handle, &i2ctransaction);
                if (I2C_STS_SUCCESS == writeStatus)
                {
                    break;
                }
            }

            len -= curWriteLen;
            curOffset += curWriteLen;
            bytesWritten += curWriteLen;
        }
    }
    return (status);
}

void eeprom_test(void)
{
    int16_t status;
    eeprom_diag.wr_rd_running = 1;
    eeprom_diag.status = 2;

    for (uint32_t i = 0; i < TEST_LENGTH; i++)
    {
        writeData[i] = TEST_DATA_VALUE;
    }

    status = EEPROM_AT24C_open();
    if (status != I2C_STATUS_SUCCESS)
    {
        EEPROM_AT24C_close(i2c1_handle);
        EEPROM_log("EEPROM DEVICE Open Failed\r\n");
    }

    /* write data */
    status = EEPROM_AT24C_write(i2c1_handle, EEPROM_TEST_ADDRESS, writeData, TEST_LENGTH);
    if (status != I2C_STS_SUCCESS)
    {
        EEPROM_log("write data failed, statusW = %d\n", status);
    }
    Osal_delay(10);

    /* read data */
    status = EEPROM_AT24C_read(i2c1_handle, EEPROM_TEST_ADDRESS, readData, TEST_LENGTH);
    if (status != I2C_STS_SUCCESS)
    {
        EEPROM_log("read data failed, statusR = %d\n", status);
    }

    for (uint32_t i = 0; i < TEST_LENGTH; i++)
    {
        if (readData[i] != writeData[i])
        {
            EEPROM_log("Data mismatch at index %d: expected 0x%02X, got 0x%02X\n", i, writeData[i], readData[i]);
            eeprom_diag.wr_rd_err++;
            eeprom_diag.status = 4;
        }
        else
        {
            EEPROM_log("[EEPROM]:data match!\n");
            eeprom_diag.wr_rd_oks++;
            eeprom_diag.status = 3;
        }
    }
    Osal_delay(2000);
    eeprom_diag.wr_rd_running = 0;
}
