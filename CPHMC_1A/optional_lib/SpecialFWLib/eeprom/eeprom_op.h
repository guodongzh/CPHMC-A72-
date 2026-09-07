/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       eeprom_op.h
 *@author     jinyangh
 *@date       2024.07.24
 *@brief      EEPROM algorithm block header file
 *@par        History
 *Date        Version   Author     Description
 *2024.06.15  1.0       jinyangh   Initial version
 *2024.07.24  2.0       jinyangh   Change the read data location and Adds a write status bit
 *2026.03.27  3.0       jinyangh   Simplify the data type of the E2 function and add CRC verification.
 ******************************************************************************/
#ifdef BUILD_MCU
#ifndef EEPROM_OP_H_
#define EEPROM_OP_H_

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define EEPROM_BUF_NUM             ((32 * 1024) / 8u)
#define OFS                        ((16 * 1024) / 8u)  // 每个region的偏移量

// 写操作状态标志
#define WR_STATES_ERR              (0x00u)  // 0表示写失败(或者向某个地址写了一样的值)
#define WR_STATES_OK               (0x01u)  // 1表示写成功
#define WR_STATES_BUSY             (0x02u)  // 2表示待写状态

// EEPROM 1个数据区域中的偏移(0 ~ (16*1024/8)-1)
#define EEPROM_MAX_REGION_NUM      (2u)

typedef struct
{
    uint32_t data;
    uint32_t crc;
}E2_DATA_TYPE; // E2中实际存的数据类型

typedef struct
{
    uint32_t data;
    uint32_t rd_state; // 读出的数据状态，1：读出数据有效，0：读出的数据无效（CRC校验错误）
}E2_DATA_RD_TYPE;
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern E2_DATA_TYPE     gEepromDataWr[EEPROM_BUF_NUM];     // 存放写数据的数组,数组大小为32*1024=32KB
extern E2_DATA_TYPE     gEepromDataWrBak[EEPROM_BUF_NUM];  // 存放上一次的数据,数组大小为32*1024=32KB
extern E2_DATA_RD_TYPE  gEepromDataRd[EEPROM_BUF_NUM];     // 存放读数据的数组,数组大小为32*1024=32KB
extern uint8_t          E2_states[EEPROM_BUF_NUM];  // 用于表示EEPROM是否写成功,每个元素表示一个16bit数据写操作的标志

uint8_t *eeprom_wr_1dw(uint8_t region, uint16_t offset, uint32_t data);
uint8_t *eeprom_wr_4dw(uint8_t  region, uint16_t offset, uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3);
E2_DATA_RD_TYPE *eeprom_rd_1dw(uint8_t region, uint16_t offset);
E2_DATA_RD_TYPE *eeprom_rd_4dw(uint8_t region, uint16_t offset);

void eeprom_recover_data_init(void);
void eeprom_flush_prog(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* EEPROM_OP_H_ */
#endif