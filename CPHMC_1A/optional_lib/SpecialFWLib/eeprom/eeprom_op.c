/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       eeprom_op.c
 *@author     jinyangh
 *@date       2024.06.15
 *@brief      EEPROM algorithm block source file
 *@par        History
 *Date        Version   Author     Description
 *2024.06.15  1.0       jinyangh   Initial version
 *2024.07.24  2.0       jinyangh   Change the read data location and Adds a write status bit
 *2024.09.03  2.1       wenjunf    fix eeprom_flush_prog bug and remove e2_wr_* function block
 *                                 detection change logic
 *2024.09.26  2.2       jinyangh   fix writing algorithm fast address overflow problem and code specification.
 *2026.03.27  3.0       jinyangh   Simplify the data type of the E2 function and add CRC verification.
 ******************************************************************************/
#ifdef BUILD_MCU
/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/

#include <stdlib.h>
#include "eeprom_op.h"
#include "debug_config.h"
#include "platform.h"
#include "eeprom_at24c.h"
#include "i2c_bus_init.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
// 定义全局数组用于一次性烧写
E2_DATA_TYPE     gEepromDataWr[EEPROM_BUF_NUM]    = {0u};//存放写数据的数组,数组大小为32*1024=32KB
E2_DATA_TYPE     gEepromDataWrBak[EEPROM_BUF_NUM] = {0u};//存放上一次的数据,数组大小为32*1024=32KB
E2_DATA_RD_TYPE  gEepromDataRd[EEPROM_BUF_NUM]    = {0u};//存放读数据的数组,数组大小为32*1024=32KB
E2_DATA_RD_TYPE  gEepromDataRd_4B[4] = {0};//存放读到的4Byte数据

uint8_t  E2_states[EEPROM_BUF_NUM];//用于表示EEPROM是否写成功,每个元素表示一个E2_DATA_TYPE数据写操作的标志
static uint32_t crc32_table[256];
static uint8_t  crc32_table_inited = 0;
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */
uint32_t CRC32_Calc(const uint8_t *data, uint32_t len);
/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief : 32bit CRC校验，查表法
 */
void CRC32_InitTable(void)
{
    uint32_t crc;
    uint32_t poly = 0xEDB88320;   // 反射多项式
    uint32_t i, j;

    for (i = 0; i < 256; i++)
    {
        crc = i;
        for (j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ poly;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }

    crc32_table_inited = 1;
}

/**
 * @brief : 32bit CRC校验，查表法
 */
uint32_t CRC32_Calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    if (!crc32_table_inited)
    {
        CRC32_InitTable();
    }

    while (len--)
    {
        crc = (crc >> 8) ^ crc32_table[(crc ^ *data++) & 0xFF];
    }

    return crc ^ 0xFFFFFFFF;
}


/**
 * @param region : EEPROM的数据区域编号(0 ~ 1),共分为2个区域，每个区域32KB/2=16KB
 * @param offset : EEPROM数据区域中的偏移(0 ~ (16*1024/8)-1)
 * @param data   : EEPROM待写入的数据(4字节写入)
 * @return       : 0：写入失败，1：写入成功
 */
uint8_t *eeprom_wr_1dw(uint8_t region, uint16_t offset, uint32_t data)
{
    if ((region > EEPROM_MAX_REGION_NUM - 1) || (offset > (OFS - 1)))
    {
        return NULL;
    }

    gEepromDataWr[region * OFS + offset].data = data;

    return E2_states;
}

/**
 * @param region : EEPROM的数据区域编号(0 ~ 1),共分为2个区域，每个区域32KB/2=16KB
 * @param offset : EEPROM数据区域中的偏移(0 ~ (16*1024/8)-4)，-4是为了不跨区
 * @param data   : EEPROM待写入的数据(4字节写入)
 * @return       : 0：写入失败，1：写入成功
 */
uint8_t *eeprom_wr_4dw(uint8_t  region,
                       uint16_t offset,
                       uint32_t data0,
                       uint32_t data1,
                       uint32_t data2,
                       uint32_t data3)
{
    if ((region > EEPROM_MAX_REGION_NUM - 1) || (offset > (OFS - 4)))
    {
        return NULL;
    }

    gEepromDataWr[region * OFS + offset + 0].data = data0;
    gEepromDataWr[region * OFS + offset + 1].data = data1;
    gEepromDataWr[region * OFS + offset + 2].data = data2;
    gEepromDataWr[region * OFS + offset + 3].data = data3;

    return E2_states;
}

/**
 * @param region : EEPROM的数据区域编号(0 ~ 1),共分为2个区域，每个区域32KB/2=16KB
 * @param offset : EEPROM数据区域中的偏移(0 ~ (16*1024/8)-1)
 * @return       : NULL：读出失败，读Buffer的地址：读出成功
 */
E2_DATA_RD_TYPE *eeprom_rd_1dw(uint8_t region, uint16_t offset)
{
    if ((region > EEPROM_MAX_REGION_NUM - 1) || (offset > (OFS - 1)))
    {
        return NULL;
    }

    gEepromDataRd_4B[0] = gEepromDataRd[region * OFS + offset];

    return gEepromDataRd_4B;
}

/**
 * @param region : EEPROM的数据区域编号(0 ~ 1),共分为2个区域，每个区域32KB/2=16KB
 * @param offset : EEPROM数据区域中的偏移(0 ~ (16*1024/8)-4)，-4是为了不跨区
 * @return       : NULL：读出失败，读Buffer的地址：读出成功
 */
E2_DATA_RD_TYPE *eeprom_rd_4dw(uint8_t region, uint16_t offset)
{
    if ((region > EEPROM_MAX_REGION_NUM - 1) || (offset > (OFS - 4)))
    {
        return NULL;
    }

    gEepromDataRd_4B[0] = gEepromDataRd[region * OFS + offset + 0];
    gEepromDataRd_4B[1] = gEepromDataRd[region * OFS + offset + 1];
    gEepromDataRd_4B[2] = gEepromDataRd[region * OFS + offset + 2];
    gEepromDataRd_4B[3] = gEepromDataRd[region * OFS + offset + 3];

    return gEepromDataRd_4B;
}

void eeprom_recover_data_init(void)
{
    // 先将所有数据读到一个buffer中
    int32_t status;

    status = EEPROM_AT24C_open();
    if (status != I2C_STATUS_SUCCESS)
    {
        Debug_logError("EEPROM DEVICE Open Failed\r\n");
    }

    status = EEPROM_AT24C_read(i2c1_handle, 0, (uint8_t *)gEepromDataRd, EEPROM_BUF_NUM * 8);  // 读32KByte
    if (status != I2C_STS_SUCCESS)
    {
        Debug_logError("EEPROM Read of %d bytes failed !!!\n", EEPROM_BUF_NUM * 8);
    }

    memcpy(gEepromDataWr, gEepromDataRd, EEPROM_BUF_NUM * 8);
    memcpy(gEepromDataWrBak, gEepromDataRd, EEPROM_BUF_NUM * 8);
    memset(E2_states, WR_STATES_BUSY, EEPROM_BUF_NUM);  // 初始状态全为待写状态

    // 判断读出数据的CRC校验是否正确
    for(uint32_t i = 0; i < EEPROM_BUF_NUM; i++)
    {
        // 从E2中读出的rd_state实际是data的CRC校验值
        if (gEepromDataRd[i].rd_state == CRC32_Calc((uint8_t *)&gEepromDataRd[i].data, 4))
        {
            gEepromDataRd[i].rd_state = 1;
        }
        else
        {
            gEepromDataRd[i].rd_state = 0;
        }
    }
}

void eeprom_flush_prog(void)
{
    int32_t         status = I2C_STS_SUCCESS;
    static uint32_t ofs    = 0;  // ofs是循环指向gEepromDataWr的下标，范围0 ~ ((32*1024)/8)-1

    if (gEepromDataWr[ofs].data != gEepromDataWrBak[ofs].data)
    {
        EEPROM_log("[BW-1] gEepromDataWrBak[%d] = %d\n", ofs, gEepromDataWrBak[ofs].data);
        EEPROM_log("[BW-1] gEepromDataWr[%d] = %d\n", ofs, gEepromDataWr[ofs].data);
        gEepromDataWr[ofs].crc = CRC32_Calc((uint8_t *)&gEepromDataWr[ofs].data, 4);
        status = EEPROM_AT24C_write(i2c1_handle, ofs * 8, (uint8_t *)&gEepromDataWr[ofs], 8);  // 8Byte写入
        if (status != I2C_STS_SUCCESS)
        {
            Debug_logError("[R1] EEPROM Write of %d bytes failed at 0x%X offset !!!\n", 8, ofs);
            E2_states[ofs] = WR_STATES_ERR;
        }
        else
        {
            gEepromDataWrBak[ofs] = gEepromDataWr[ofs];
            EEPROM_log("[BW-2] gEepromDataWrBak[%d] = %d\n", ofs, gEepromDataWrBak[ofs].data);
            EEPROM_log("[BW-2] gEepromDataWr[%d] = %d\n", ofs, gEepromDataWr[ofs].data);
            E2_states[ofs] = WR_STATES_OK;
        }
    }

    ofs = (ofs + 1) % EEPROM_BUF_NUM;
}
#endif