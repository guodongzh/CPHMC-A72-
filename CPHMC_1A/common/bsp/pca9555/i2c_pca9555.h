/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       i2c_pca9555.h
*@author     xs
*@date       2024.12.24
*@brief      i2c read pca9555 test source.h file
*@par        History
*Date        Version   Author     Description
2024.12.24   1.0       xs         example
******************************************************************************/
#ifndef I2C_PCA9555_I2C_PCA9555_H_
#define I2C_PCA9555_I2C_PCA9555_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <ti/osal/osal.h>
#include <ti/drv/i2c/I2C.h>
#include <ti/drv/i2c/soc/I2C_soc.h>
#include <ti/csl/soc.h>
#include "board/board.h"
#include "debug_config.h"

/**
 * PCA9555(I2C to IO)
 */
#define PCA9555_I2C1_ADDR       (0x20)
#define PCA9555_I2C0_ADDR       (0x21)

#define PCA9555_CMD_BYTE_REG0   (0x00U) //input port0
#define PCA9555_CMD_BYTE_REG1   (0x01U) //input port1
#define PCA9555_CMD_BYTE_REG2   (0x02U) //ouput port0
#define PCA9555_CMD_BYTE_REG3   (0x03U) //ouput port1
#define PCA9555_CMD_BYTE_REG4   (0x04U) //Polarity Inversion Port 0
#define PCA9555_CMD_BYTE_REG5   (0x05U) //Polarity Inversion Port 1
#define PCA9555_CMD_BYTE_REG6   (0x06U) //config port0
#define PCA9555_CMD_BYTE_REG7   (0x07U) //config port1

#define PCA9555_SLOTA_I2C0_ADDR (0x20)
#define PCA9555_SLOTB_I2C0_ADDR (0x21)
#define PCA9555_SLOTC_I2C0_ADDR (0x22)
#define PCA9555_SLOTD_I2C0_ADDR (0x23)
#define PCA9555_MAIN_I2C1_ADDR  (0x21)
#define PCA9555_SD_I2C6_ADDR    (0x20)

#define CSA_BOARD_ID            0x1160
#define CSB_BOARD_ID            0x1260
#define CSD_BOARD_ID            0x1460

enum PIN_NUM_MASK
{
    PIN_NUM_0 = (0x1 << 0),
    PIN_NUM_1 = (0x1 << 1),
    PIN_NUM_2 = (0x1 << 2),
    PIN_NUM_3 = (0x1 << 3),
    PIN_NUM_4 = (0x1 << 4),
    PIN_NUM_5 = (0x1 << 5),
    PIN_NUM_6 = (0x1 << 6),
    PIN_NUM_7 = (0x1 << 7),
    PIN_NUM_8 = (0x1 << 8),
    PIN_NUM_9 = (0x1 << 9),
    PIN_NUM_10 = (0x1 << 10),
    PIN_NUM_11 = (0x1 << 11),
    PIN_NUM_12 = (0x1 << 12),
    PIN_NUM_13 = (0x1 << 13),
    PIN_NUM_14 = (0x1 << 14),
    PIN_NUM_15 = (0x1 << 15),
};

typedef struct _board_info
{
    bool is_online;
    uint8_t i2c_addr;
    uint8_t slot_num;
    uint16_t board_id;
} board_info_t;

extern board_info_t all_board_info[5];

int32_t get_csx_board_id(I2C_Handle handle, board_info_t *board_info);
int32_t pca9555_led_enable_output();
int32_t pca9555_led_init();
void pca9555_ctrl_led(uint16_t led_num_mask, bool is_on);
void pca9555_get_all_board_id(void);

#endif /* I2C_PCA9555_I2C_PCA9555_H_ */
