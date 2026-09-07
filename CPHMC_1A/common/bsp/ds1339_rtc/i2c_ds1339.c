/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       i2c_ds1339.c
 *@author     xqb
 *@date       2024.07.08
 *@brief      i2c read ds1339RTC test source.c file
 *@par        History
 *Date        Version   Author     Description
 *2024.07.08  1.0       xqb        example
 *2024.12.18  1.1       LiuRui     refactor
 ******************************************************************************/
#include <bsp/i2c_bus_init/i2c_bus_init.h>
#include "i2c_ds1339.h"


/**********************************************************************
 ************************** Macros ************************************
 **********************************************************************/
#define I2C_TRANSACTION_TIMEOUT       (10000U)

/* Interrupt reserved for Core ID 1. 128 is based on RM */
#define I2C_INST_WKUP_I2C0_INT_OFFSET (128U)
const char *weekdays[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
ds1339_time_t g_rtc_time;
/* Check and clear the OSF flag in STATUS REGISTER */
int32_t rtc_ds1339_check_status(void)
{
    uint8_t txBuffer[2] = {REGADDR_STATUS};
    uint8_t status_reg = 0;
    int16_t status;
    I2C_Transaction i2cTransaction;

    I2C_transactionInit(&i2cTransaction);
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = &status_reg;
    i2cTransaction.readCount = 1;
    i2cTransaction.slaveAddress = DS1339_ADDR;
    i2cTransaction.timeout = I2C_TRANSACTION_TIMEOUT;
    status = I2C_transfer(i2c2_handle, &i2cTransaction);

    if (status != I2C_STS_SUCCESS)
    {
        Debug_logTag("OSF flag Transfer failed\r\n");
        return status;
    }

    /* Check if the OSF bit is set */
    if (status_reg & 0x80)
    {
        txBuffer[1] = (status_reg & ~0x80);

        I2C_transactionInit(&i2cTransaction);
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 2;
        i2cTransaction.readBuf = NULL;
        i2cTransaction.readCount = 0;
        i2cTransaction.slaveAddress = DS1339_ADDR;
        i2cTransaction.timeout = I2C_TRANSACTION_TIMEOUT;
        status = I2C_transfer(i2c2_handle, &i2cTransaction);

        if (status != I2C_STS_SUCCESS)
        {
            Debug_logTag("OSF bit set Transfer failed\r\n");
        }
    }
    return status;
}

/* Initialize DS1339 - Ensure oscillator is running */
int32_t rtc_s1339_init(void)
{
    uint8_t control_reg;
    int32_t status;
    I2C_Transaction i2cTransaction;
    uint8_t deviceAddress = DS1339_ADDR;
    uint8_t txBuffer[2] = {REGADDR_CONTROL};

    /* Determine if I2C sensor is present */
    status = I2C_control(i2c2_handle, I2C_CMD_PROBE, &deviceAddress);
    if (I2C_STATUS_SUCCESS == status)
    {
        /* Read the control register */
        I2C_transactionInit(&i2cTransaction);
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 1;
        i2cTransaction.readBuf = &control_reg;
        i2cTransaction.readCount = 1;
        i2cTransaction.slaveAddress = DS1339_ADDR;
        i2cTransaction.timeout = I2C_TRANSACTION_TIMEOUT;
        status = I2C_transfer(i2c2_handle, &i2cTransaction);

        if (status != I2C_STS_SUCCESS)
        {
            Debug_logTag("DS1339RTC_init set Transfer failed\r\n");
            return I2C_STATUS_ERROR;
        }

        /* Ensure the oscillator is running (clear EOSC bit) */
        if (control_reg & 0x80)
        { /* If EOSC bit is set Clear the EOSC bit */
            txBuffer[1] = (control_reg & (~0x80));
            I2C_transactionInit(&i2cTransaction);
            i2cTransaction.writeBuf = txBuffer;
            i2cTransaction.writeCount = 1;
            i2cTransaction.slaveAddress = DS1339_ADDR;
            i2cTransaction.timeout = I2C_TRANSACTION_TIMEOUT;
            status = I2C_transfer(i2c2_handle, &i2cTransaction);

            if (status != I2C_STS_SUCCESS)
            {
                Debug_logTag("DS1339RTC_init set Transfer failed\r\n");
                return I2C_STATUS_ERROR;
            }
        }
        return I2C_STATUS_SUCCESS;
    }
    else
    {
        Debug_logTag("[I2C2] DS1339RTC sensor not found at device address 0x%02x \r\n", deviceAddress);
    }
    return status;
}

/* DS1339 set time */
int32_t rtc_ds1339_set_time(ds1339_time_t *time)
{
    uint8_t i;
    uint8_t txBuffer[8];
    int32_t status;
    uint8_t deviceAddress = DS1339_ADDR;
    I2C_Params i2cparams;
    I2C_Transaction i2cTransaction;

    /* Determine if I2C sensor is present */
    status = I2C_control(i2c2_handle, I2C_CMD_PROBE, &deviceAddress);
    if (I2C_STATUS_SUCCESS == status)
    {
        txBuffer[0] = REGADDR_SECONDS;
        for (i = 0; i < 7; i++)
        {
            txBuffer[i + 1] = ConvertDataToSet(time, REGADDR_SECONDS + i);
        }
        I2C_transactionInit(&i2cTransaction);
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 8;
        i2cTransaction.readBuf = NULL;
        i2cTransaction.readCount = 0;
        i2cTransaction.slaveAddress = DS1339_ADDR;
        status = I2C_transfer(i2c2_handle, &i2cTransaction);

        if (I2C_STS_SUCCESS != status)
        {
            Debug_logTag("initData Transfer failed\r\n");
            return I2C_STATUS_ERROR;
        }
        return I2C_STATUS_SUCCESS;
    }
    else
    {
        Debug_logTag("[I2C2] DS1339RTC sensor not found at device address 0x%02x \r\n", deviceAddress);
    }
    return status;
}

/* DS1339 get time */
int32_t rtc_ds1339_get_time(ds1339_time_t *time)
{
    int32_t status;
    uint8_t i;
    uint8_t receiveData[7];
    I2C_Transaction i2cTransaction;
    uint8_t txBuffer[1] = {REGADDR_SECONDS};
    uint8_t deviceAddress = DS1339_ADDR;
    status = I2C_control(i2c2_handle, I2C_CMD_PROBE, &deviceAddress);
    if (I2C_STATUS_SUCCESS == status)
    {
        /* Found DS1339 RTC sensor */
        I2C_transactionInit(&i2cTransaction);
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 1;
        i2cTransaction.readBuf = receiveData;
        i2cTransaction.readCount = 7;
        i2cTransaction.slaveAddress = DS1339_ADDR;
        status = I2C_transfer(i2c2_handle, &i2cTransaction);

        if (status == I2C_STS_SUCCESS)
        {
            for (i = 0; i < 7; i++)
            {
                ConvertDataToGet(receiveData[i], time, REGADDR_SECONDS + i);
            }
            return I2C_STATUS_SUCCESS;
        }
        else
        {
            Debug_logTag("DS1339RTC_init get Transfer failed\r\n");
            return I2C_STATUS_ERROR;
        }
    }

    return status;
}

///* BCD ConvertData (dec-bin)*/
uint8_t ConvertDataToSet(const ds1339_time_t *time, uint8_t regType)
{
    uint8_t sendData = 0;

    switch (regType)
    {
    case REGADDR_SECONDS:
        if (time->second > 10)
        {
            sendData = ((time->second / 10) << 4) & 0x7F;
            sendData |= time->second % 10;
        }
        else
        {
            sendData = time->second;
        }
        break;

    case REGADDR_MINUTES:
        if (time->minute > 10)
        {
            sendData = ((time->minute / 10) << 4) & 0x7F;
            sendData |= time->minute % 10;
        }
        else
        {
            sendData = time->minute;
        }
        break;

    case REGADDR_HOURS:
        if (time->hour > 10)
        {
            sendData = ((time->hour / 10) << 4) & 0x1F;
            sendData |= time->hour % 10;
        }
        else
        {
            sendData = time->hour;
        }
        break;

    case REGADDR_DAY_OF_WEEK:
        sendData = time->weekday & 0x0E;
        break;

    case REGADDR_DATE:
        if (time->day > 10)
        {
            sendData = ((time->day / 10) << 4) & 0x3F;
            sendData |= time->day % 10;
        }
        else
        {
            sendData = time->day;
        }
        break;

    case REGADDR_MONTH_CENTURY:
        if (time->month > 10)
        {
            sendData = ((time->month / 10) << 4) & 0x1F;
            sendData |= time->month % 10;
        }
        else
        {
            sendData = time->month;
        }
        break;
    case REGADDR_YEAR:
        if (time->year > 10)
        {
            sendData = ((time->year / 10) << 4) & 0xF0;
            sendData |= time->year % 10;
        }
        else
        {
            sendData = time->year;
        }
        break;
    default:;
    }
    return sendData;
}

/* BCD ConvertData (bin-dec)*/
void ConvertDataToGet(uint8_t rData, ds1339_time_t *time, uint8_t regType)
{
    switch (regType)
    {
    case REGADDR_SECONDS:
        time->second = (rData >> 4 & 0x07) * 10 + (rData & 0x0F);
        break;

    case REGADDR_MINUTES:
        time->minute = (rData >> 4 & 0x07) * 10 + (rData & 0x0F);
        break;

    case REGADDR_HOURS:
        time->hour = (rData >> 4 & 0x01) * 10 + (rData & 0x0F);
        break;

    case REGADDR_DAY_OF_WEEK:
        time->weekday = rData & 0x07;
        break;

    case REGADDR_DATE:
        time->day = (rData >> 4 & 0x03) * 10 + (rData & 0x0F);
        break;

    case REGADDR_MONTH_CENTURY:
        time->month = (rData >> 4 & 0x01) * 10 + (rData & 0x0F);
        break;

    case REGADDR_YEAR:
        time->year = (rData >> 4 & 0x0F) * 10 + (rData & 0x0F);
        break;

    default:;
    }
}

/*
 *  ======== test function ========
 */
void rtc_ds1339_test(void)
{
    int32_t status;

    status = rtc_s1339_init();
    if (status != I2C_STATUS_SUCCESS)
    {
        Debug_logTag("Initialize DS1339 failed\r\n");
        return;
    }

    /* set configure time params --- 2024.7.13 10:30:00*/
    g_rtc_time.second = 20;
    g_rtc_time.minute = 55;
    g_rtc_time.hour = 17;
    g_rtc_time.weekday = 2;
    g_rtc_time.day = 19;
    g_rtc_time.month = 8;
    g_rtc_time.year = 25;  // 两位数

    /*use ds1339_set_time() to set g_time */
    status = rtc_ds1339_set_time(&g_rtc_time);

    if (status != I2C_STATUS_SUCCESS)
    {
        Debug_logTag("failed to set time\r\n");
    }
    for (uint8_t loop = 0; loop < 2; loop++)
    {
        status = rtc_ds1339_get_time(&g_rtc_time);
        if (status == I2C_STATUS_SUCCESS)
        {
            Debug_logTag("Current Date: 20%02d,%02d,%02d\n",
                      g_rtc_time.year,
                      g_rtc_time.month,
                      g_rtc_time.day);
            Debug_logTag("Current Time: %s:%02d:%02d:%02d\n", weekdays[g_rtc_time.weekday],
                      g_rtc_time.hour,
                      g_rtc_time.minute,
                      g_rtc_time.second);
        }
        else
        {
            Debug_logTag("Failed to get time, status: %d\n", status);
        }

        /*delay 5s*/
        Osal_delay(5000);
    }
}
