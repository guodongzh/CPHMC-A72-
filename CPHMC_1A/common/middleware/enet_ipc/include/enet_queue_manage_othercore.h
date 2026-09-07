/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       enet_queue_manage_othercore.h
*@author     wenjunf
*@date       2024.07.01
*@brief      以太网核间通信底层其他核接口
*@par        History
*Date        Version   Author     Description
2024.07.01   1.0       wenjunf    以太网核间通信底层其他核接口
******************************************************************************/
#ifndef _ENET_QUEUE_MANAGE_OTHERCORE_H_
#define _ENET_QUEUE_MANAGE_OTHERCORE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "platform.h"

// 核间通讯共享内存监视
typedef struct
{
    // 以太网接收监视
    uint32_t Enet_rec_FPGA_2_Core0_Index_train_OK_number;     // 以太网接收Core0接收FPGA的索引车帧数
    uint32_t Enet_rec_FPGA_2_Core0_valid_Meth_Car_OK_number;  // 以太网接收Core0接收FPGA的100M以太网列车帧数
    uint32_t Enet_rec_FPGA_2_Core0_valid_Lan_Meth_Car_OK_number;  // 以太网接收Core0接收FPGA的内网口100M以太网列车帧数
    uint32_t Enet_rec_Core0_2_ShmRam_queue_OK_number;  // 以太网接收Core0将收到100M以太网发送到共享内存帧数
    uint32_t Enet_rec_Core_2_Class_queue_OK_number;  // 以太网接收各核填入分类队列帧数
    uint32_t Enet_rec_Core_2_callback_OK_number;     // 以太网接收各Core调用回调函数次数
    uint32_t Enet_rec_err_number;                    // 以太网接收PCIe以太网接收出错帧数
    uint32_t Enet_rec_err_location;                  // 以太网接收PCIe以太网接收出错定位

    // 以太网发送监视
    uint32_t Enet_send_Core_run_sendto_raw_socket_prog_number;  // 以太网发送各Core调用sendto_raw_socket_prog次数
    uint32_t Enet_send_Core_2_ShmRam_queue_OK_number;  // 以太网发送其它核发送给核间共享内存队列次数
    uint32_t Enet_send_Core0_2_Core0_enet_PCIe_queue_OK_number;  // 以太网发送Core0到Core0的PCIe发送队列帧数
    uint32_t Enet_send_Core1_2_Core0_enet_PCIe_queue_OK_number;  // 以太网发送Core1到Core0的PCIe发送队列帧数
    uint32_t Enet_send_Core2_2_Core0_enet_PCIe_queue_OK_number;  // 以太网发送Core2到Core0的PCIe发送队列帧数
    uint32_t Enet_send_Core3_2_Core0_enet_PCIe_queue_OK_number;  // 以太网发送Core3到Core0的PCIe发送队列帧数
    uint32_t Enet_send_CoreA53_2_Core0_enet_PCIe_queue_OK_number;  // 以太网发送CoreA53到Core0的PCIe发送队列帧数
    uint32_t Enet_send_Core0_enqueue_pcie_enet_frame_OK_number;  // 以太网发送入PCIe队列帧数
    uint32_t Enet_send_Core0_2_FPGA_OK_number;                   // 从PCIe的队列中发送给FPGA以太网帧数
    uint32_t Enet_send_err_number;                               // 以太网发送出错次数
    uint32_t Enet_send_err_location;                             // 以太网发送出错定位

} CORE_ENET_QUEUE_MONITOR_INF_STRUCT;

extern CORE_ENET_QUEUE_MONITOR_INF_STRUCT Core_enet_queue_monitor_inf;
void ipc_prog_othercore_rawpkg_to_shm_txque(enet_car_tx_t *p_frame_src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _ENET_GOOSE_H_ */