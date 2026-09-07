/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       i2c_tmp75q1.c
 *@author     xqb
 *@date       2024.07.09
 *@brief      i2c read tmp75q1 test source.c file
 *@par        History
 *Date        Version   Author     Description
 *2024.07.09  1.0       xqb        example
 ******************************************************************************/
#include <bsp/i2c_bus_init/i2c_bus_init.h>
#include "i2c_tmp75q1.h"
#include <math.h>

/**********************************************************************
 ************************** Macros ************************************
 **********************************************************************/
#define I2C_TRANSACTION_TIMEOUT (10000U)
float temperaturecelcius;

/* tmp75q1 temperature sensor open */
int32_t tmp75Q1_init()
{
    uint8_t txBuffer[2];
    int32_t status;
    uint8_t deviceAddress;

    I2C_Transaction i2cTransaction;

    deviceAddress = TMP75Q1_I2C2_ADDR;
    /* Determine if I2C sensor is present */
    status = I2C_control(i2c2_handle, I2C_CMD_PROBE, &deviceAddress);
    if (status != I2C_STATUS_SUCCESS)
    {
        Debug_logTag("[I2C2] Temperature sensor not found at device address 0x%02x \r\n", deviceAddress);
    }
    if (status == I2C_STATUS_SUCCESS)
    {
        I2C_transactionInit(&i2cTransaction);
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 2;
        i2cTransaction.slaveAddress = deviceAddress;
        txBuffer[0] = TMP75Q1_CONFIG_REG;
        txBuffer[1] = TMP75Q1_CFG_REG_CONFIG(TMP75Q1_SD_DISABLE_DFT,
                                             TMP75Q1_TM_DISABLE_DFT,
                                             TMP75Q1_POL_LOW_DFT,
                                             TMP75Q1_FQ_ZERO_DFT,
                                             TMP75Q1_CR_THREE,
                                             TMP75Q1_OS_DISABLE_DFT); /* TMP75Q1 Sensor Config Register Set */
        status = I2C_transfer(i2c2_handle, &i2cTransaction);

        if (I2C_STS_SUCCESS != status)
        {
            Debug_logTag("I2C Test: ");
            Debug_logTag("Data Transfer failed\r\n");
        }
    }
    return status;
}

/* tmp75q1 temperature sensor read data */
int16_t tmp72q1_read_data()
{
    uint8_t txBuffer[2];
    uint8_t rxBuffer[2];
    int16_t temperature;
    int32_t status;
    I2C_Transaction i2cTransaction;

    /* Found TMP75Q1 temperature sensor */
    /* Select result register */
    /* Set the I2C EEPROM write/read address */

    I2C_transactionInit(&i2cTransaction);
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.slaveAddress = TMP75Q1_I2C2_ADDR;
    txBuffer[0] = TMP75Q1_RESULT_REG;
    status = I2C_transfer(i2c2_handle, &i2cTransaction);

    if (status == I2C_STS_SUCCESS)
    {
        /* clear RX buffer every time we read, to make sure it does not have stale data */
        rxBuffer[0] = rxBuffer[1] = 0;
        i2cTransaction.slaveAddress = TMP75Q1_I2C2_ADDR;
        i2cTransaction.readBuf = rxBuffer;
        i2cTransaction.readCount = 2;
        i2cTransaction.timeout = I2C_TRANSACTION_TIMEOUT;
        status = I2C_transfer(i2c2_handle, &i2cTransaction);

        if (status == I2C_STS_SUCCESS)
        {
            temperature = ((uint16_t)rxBuffer[0] << 8) | (rxBuffer[1]); /* Combine temperature values to uint16_t */
            temperature = temperature >> 4;                             /* 12-bit resolution, move 4 bits to the right */
            if (rxBuffer[0] & 0x80)
            {
                temperature |= 0xF000; /* Handle negative temperature values */
            }
            return temperature;
        }
        else
        {
            Debug_logTag("[I2C2] TMP75Q1 read failed\r\n");
            return 0;
        }
    }
    else
    {
        Debug_logTag("[I2C2] TMP75Q1 open failed\r\n");
        return 0;
    }
}

/*
 *  ======== test function ========
 */
void tmp75q1_test(void)
{
    uint8_t x, y;
    char buffer[16];
    uint32_t state;

    /* Init TMP75Q1 */
    state = tmp75Q1_init();
    if (state != I2C_STS_SUCCESS)
    {
        Debug_logTag("Tmp75q1 open failed\r\n");
    }

    if (state == I2C_STS_SUCCESS)
    {
        temperaturecelcius = tmp72q1_read_data() / 16.0f;
        x = (uint8_t)floorf(temperaturecelcius);
        y = (uint8_t)roundf((temperaturecelcius - x) * 100);
        snprintf(buffer, sizeof(buffer), "%d.%02d", x, y);
        /*delay 2s*/
        Osal_delay(2000);
        Debug_logTag("[I2C2] TMP75Q1 Current Temperature = %s (celcius)\r\n", buffer);
    }
    else
    {
        Debug_logTag("[I2C2] TMP75Q1 get Temperature failed\r\n");
    }
}
