/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       ipc_data_drv.c
*@author     jinyangh
*@date       2026.07.09
*@brief
*@par        History
*Date        Version   Author     Description
*2026.07.09  1.0       jinyangh    example
******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "ipc_data_drv.h"

#include <stdbool.h>
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
static uint8_t  gIpcDataDrv_R0[IPC_DATA_DRV_NUM] __attribute__((aligned(128), section(".r0_ipc_data_drv_mem")));
static uint8_t  gIpcDataDrv_R1[IPC_DATA_DRV_NUM] __attribute__((aligned(128), section(".r1_ipc_data_drv_mem")));
static uint8_t  gIpcDataDrv_R2[IPC_DATA_DRV_NUM] __attribute__((aligned(128), section(".r2_ipc_data_drv_mem")));
static uint8_t  gIpcDataDrv_R3[IPC_DATA_DRV_NUM] __attribute__((aligned(128), section(".r3_ipc_data_drv_mem")));
static uint8_t  gIpcDataDrv_C60[IPC_DATA_DRV_NUM] __attribute__((aligned(128), section(".c60_ipc_data_drv_mem")));
static uint8_t  gIpcDataDrv_C61[IPC_DATA_DRV_NUM] __attribute__((aligned(128), section(".c61_ipc_data_drv_mem")));
static uint8_t  gIpcDataDrv_C70[IPC_DATA_DRV_NUM] __attribute__((aligned(128), section(".c70_ipc_data_drv_mem")));
static uint8_t  gIpcDataDrv_MCU1[IPC_DATA_DRV_NUM] __attribute__((aligned(128), section(".mcu1_ipc_data_drv_mem")));

static uint8_t *gIpcDataDrv_buff[core_type_num] = {
    gIpcDataDrv_R0,
    gIpcDataDrv_R1,
    gIpcDataDrv_R2,
    gIpcDataDrv_R3,
    gIpcDataDrv_C60,
    gIpcDataDrv_C61,
    gIpcDataDrv_C70,
    gIpcDataDrv_MCU1,
};
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
/**
* @brief 向本核 IPC 数据驱动共享区写入数据
* @param[in]  position  写入起始位置(字节偏移)，范围 0 ~ (IPC_DATA_DRV_NUM - 1)
* @param[in]  buf       待写入数据的源缓冲区指针
* @param[in]  num       待写入的字节数
* @return     true: 写入成功，false: buf为空或越界
*/
bool ipc_data_drv_write(uint32_t position, const uint8_t *buf, uint32_t num)
{
   if ((buf == NULL) || (num == 0U) || ((position + num) > IPC_DATA_DRV_NUM))
   {
       return false;
   }
   uint8_t *pDst = gIpcDataDrv_buff[getCoreNr()] + position;
   memcpy(pDst, buf, num);
   CacheP_wb(pDst, num);
   return true;
}

/**
* @brief 读取指定核的 IPC 数据驱动共享区数据到本地 buf
* @param[in]  core_id   目标核 ID，范围 0~3(R5F0 ~ R5F3)
* @param[in]  position  读取起始位置(字节偏移)，范围 0 ~ (IPC_DATA_DRV_NUM - 1)
* @param[out] buf       存放读取数据的目标缓冲区指针
* @param[in]  num       需要读取的字节数
* @return     true: 读取成功，false: 参数非法或越界
*/
bool ipc_data_drv_read(core_type core_id, uint32_t position, uint8_t *buf, uint32_t num)
{
   if ((core_id >= core_type_num) || (buf == NULL) || (num == 0U) || ((position + num) > IPC_DATA_DRV_NUM))
   {
       return false;
   }
   uint8_t *pSrc = gIpcDataDrv_buff[core_id] + position;
   CacheP_Inv(pSrc, num);
   memcpy(buf, pSrc, num);
   return true;
}
