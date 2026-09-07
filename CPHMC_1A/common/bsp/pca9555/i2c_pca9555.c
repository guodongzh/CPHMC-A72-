/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       i2c_pca9555.c
 *@author     xs
 *@date       2024.12.24
 *@brief      i2c read pca9555 test source.c file
 *@par        History
 *Date        Version   Author     Description
 *2024.12.24  1.0       xs         example
 *2025.11.18  1.0       LiuRui     refactor
 ******************************************************************************/
#include <bsp/i2c_bus_init/i2c_bus_init.h>
#include "i2c_pca9555.h"
#include <ti/csl/arch/r5/csl_arm_r5.h>

board_info_t all_board_info[5] = {
    {false, PCA9555_SLOTA_I2C0_ADDR, 0, 0}, //槽位1板卡ID
    {false, PCA9555_SLOTB_I2C0_ADDR, 1, 0}, //槽位2板卡ID
    {false, PCA9555_SLOTC_I2C0_ADDR, 2, 0}, //槽位3板卡ID
    {false, PCA9555_SLOTD_I2C0_ADDR, 3, 0}, //槽位4板卡ID
    {false, PCA9555_MAIN_I2C1_ADDR, 4, 0},  //主控板卡ID
};

static uint8_t led_state[2] = {0xff, 0xff};

int32_t get_csx_board_id(I2C_Handle handle, board_info_t *board_info)
{
    uint8_t txBuffer[3] = {0, 0, 0};
    uint8_t rxBuffer[2] = {0, 0};
    int32_t status;
    uint8_t deviceAddress;
    uint16_t boardid = 0x0;
    I2C_Transaction i2cTransaction;

    /* device addr*/
    deviceAddress = board_info->i2c_addr;
    /* Determine if I2C sensor is present */
    status = I2C_control(handle, I2C_CMD_PROBE, &deviceAddress);
    if (status == I2C_STATUS_SUCCESS)
    {
        // PCA9555 default mode is input mode
        I2C_transactionInit(&i2cTransaction);
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 1;
        i2cTransaction.slaveAddress = deviceAddress;
        txBuffer[0] = PCA9555_CMD_BYTE_REG0;
        status = I2C_transfer(handle, &i2cTransaction);

        if (status == I2C_STS_SUCCESS)
        {
            I2C_transactionInit(&i2cTransaction);
            i2cTransaction.readBuf = rxBuffer;
            i2cTransaction.readCount = 2;
            i2cTransaction.slaveAddress = deviceAddress;

            rxBuffer[0] = rxBuffer[1] = 0;
            status = I2C_transfer(handle, &i2cTransaction);
            if (status == I2C_STS_SUCCESS)
            {
                boardid = ((uint16_t)rxBuffer[1] << 8) | rxBuffer[0];
                board_info->is_online = true;
                board_info->board_id = boardid;
            }
            else
            {
                Debug_logTag("PCA9555 SLOT%d Board ID read failed!\r\n", board_info->slot_num);
            }
        }
    }
    return status;
}

int32_t pca9555_led_enable_output()
{
    uint8_t txBuffer[3] = {0, 0, 0};
    uint8_t rxBuffer[2] = {0, 0};
    int32_t status;
    uint8_t deviceAddress;
    I2C_Transaction i2cTransaction;

    deviceAddress = PCA9555_SD_I2C6_ADDR;
//    /* Determine if I2C sensor is present */
//    i2c6_lock();
//    status = I2C_control(i2c6_handle, I2C_CMD_PROBE, &deviceAddress);
//    i2c6_unlock();
//    if (status == I2C_STATUS_SUCCESS)
//    {
//        Debug_logTag("[I2C6] PCA9555 sd found at device address 0x%02x \r\n", deviceAddress);
//    }
//    else
//    {
//        Debug_logTag("[I2C6] PCA9555 sd not found at device address 0x%02x \r\n", deviceAddress);
//    }
//    if (status == I2C_STATUS_SUCCESS)
//    {
        /* Config PCA9555 P0.0~P1.7 as output mode */
        I2C_transactionInit(&i2cTransaction);
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 3;
        i2cTransaction.slaveAddress = deviceAddress;
        txBuffer[0] = PCA9555_CMD_BYTE_REG6;
        txBuffer[1] = 0x00;  // P0.0~P0.7 output mode
        txBuffer[2] = 0x00;  // P1.0~P1.7 output mode
        status = I2C_transfer(i2c6_handle, &i2cTransaction);
//    }
    return status;
}

int32_t pca9555_led_init()
{
    uint8_t txBuffer[3] = {0, 0, 0};
    uint8_t rxBuffer[2] = {0, 0};
    int32_t status;
    uint8_t deviceAddress;
    I2C_Transaction i2cTransaction;

    status = pca9555_led_enable_output();
    if (status == I2C_STS_SUCCESS)
    {
        deviceAddress = PCA9555_SD_I2C6_ADDR;
        I2C_transactionInit(&i2cTransaction);
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 3;
        i2cTransaction.slaveAddress = deviceAddress;
        txBuffer[0] = PCA9555_CMD_BYTE_REG2;
        txBuffer[1] = led_state[0];  // P0.0~P0.7 hight level
        txBuffer[2] = led_state[1];  // P1.0~P1.7 hight level
        status = I2C_transfer(i2c6_handle, &i2cTransaction);
    }

    return status;
}

void pca9555_ctrl_led(uint16_t led_num_mask, bool is_on)
{
    uint8_t txBuffer[3] = {0, 0, 0};
    uint8_t rxBuffer[2] = {0, 0};
    int32_t status;

    /* Config PCA9555 P0.0~P1.7 according to param */
    I2C_Transaction i2cTransaction;
    I2C_transactionInit(&i2cTransaction);
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 3;
    i2cTransaction.slaveAddress = PCA9555_SD_I2C6_ADDR;
    txBuffer[0] = PCA9555_CMD_BYTE_REG2;
    if (is_on)
    {
        txBuffer[1] = (led_state[0] & ((~led_num_mask) & 0xff));
        txBuffer[2] = (led_state[1] & ((~led_num_mask) >> 8));
    }
    else
    {
        txBuffer[1] = (led_state[0] | (led_num_mask & 0xff));
        txBuffer[2] = (led_state[1] | (led_num_mask >> 8));
    }
    led_state[0] = txBuffer[1];
    led_state[1] = txBuffer[2];
    status = I2C_transfer(i2c6_handle, &i2cTransaction);
    if (status != I2C_STS_SUCCESS)
    {
        Debug_logTag("[I2C6] PCA9555 write failed!\r\n");
        return;
    }
}

/*
 *  ======== test function ========
 */
void pca9555_get_all_board_id(void)
{
    uint32_t state;

    /* slot 1 - 4 */
    for (int i = 0; i < 4; i++)
    {
        state = get_csx_board_id(i2c0_handle, &all_board_info[i]);
        if (state != I2C_STS_SUCCESS)
        {
            Debug_logTag("[I2C0] PCA9555 SLOT%d not found at device address 0x%02x \r\n",
                      i,
                      all_board_info[i].i2c_addr);
        }
        else
        {
            Debug_logTag("[I2C0] pca9555 SLOT%d Board ID: 0x%x\r\n", i, all_board_info[i].board_id);
        }
    }
    state = get_csx_board_id(i2c1_handle, &all_board_info[4]);
    if (state != I2C_STS_SUCCESS)
    {
        Debug_logTag("[I2C1] PCA9555 MainCtrBoard not found at device address 0x%02x \r\n",
                  all_board_info[4].i2c_addr);
    }
    else
    {
        Debug_logTag("[I2C1] pca9555 MainCtrBoard Board ID: 0x%x\r\n", all_board_info[4].board_id);
    }
}
