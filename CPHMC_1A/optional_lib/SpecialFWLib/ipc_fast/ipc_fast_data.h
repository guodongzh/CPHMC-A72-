/**
 *************************************************************************
 * @file      ipc_fast_data.h
 * @author    zht
 * @date      2023/11/08
 * @brief     核间快速数据接口
 * @attention None
 *************************************************************************
 */
#ifndef IPC_FAST_DATA_H_
#define IPC_FAST_DATA_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <ti/osal/osal.h>

uint8_t ipc_wr_4dw(uint8_t blkNr, uint32_t in1, uint32_t in2, uint32_t in3, uint32_t in4);
uint8_t ipc_wr_8dw(uint8_t blkNr, uint32_t in1, uint32_t in2, uint32_t in3, uint32_t in4,
                   uint32_t in5, uint32_t in6, uint32_t in7, uint32_t in8);
uint8_t ipc_wr_8w(uint8_t blkNr,
                 uint16_t in1,
                 uint16_t in2,
                 uint16_t in3,
                 uint16_t in4,
                 uint16_t in5,
                 uint16_t in6,
                 uint16_t in7,
                 uint16_t in8);
uint8_t  ipc_wr_real(uint8_t blkNr, float in1, float in2, float in3, float in4);
uint8_t* ipc_rd_blk(uint8_t coreNr, uint8_t blkNr);
void ipc_wr_update();
void ipc_rd_update(uint8_t coreNr);

#ifdef __cplusplus
}
#endif

#endif
