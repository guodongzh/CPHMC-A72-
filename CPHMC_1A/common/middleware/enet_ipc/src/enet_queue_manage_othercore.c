/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       enet_queue_manage_othercore.c
 *@author     wenjunf
 *@date       2024.07.01
 *@brief      以太网核间通信底层其他核接口
 *@par        History
 *Date        Version   Author     Description
 *2024.07.01  1.0       wenjunf    以太网核间通信底层其他核接口
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "net_all_include.h"

CORE_ENET_QUEUE_MONITOR_INF_STRUCT Core_enet_queue_monitor_inf;

extern tx_ctrl_t tx_ctrls[SHM_DIR_NUM];
#ifndef CORE_R5F0
// 获取对应共享内存指针，各核的报文发送到共享队列中
// Src_Core_ID --报文来源的Core序号
static enet_car_tx_t *get_send_shm_to_txfifo(uint8_t Src_Core_ID)
{

    if (Src_Core_ID == CORE1_ID)
    {
        return r1_tx_r0_que[0];
    }
    else if (Src_Core_ID == CORE2_ID)
    {
        return r2_tx_r0_que[0];
    }
    else if (Src_Core_ID == CORE3_ID)
    {
        return r3_tx_r0_que[0];
    }
    else if (Src_Core_ID == CORE53_ID)
    {
        return a_tx_r0_que;
    }
    else
    {
        return NULL;
    }
}

// 各Core->R0:核间通信发送：将应用报文写入Core0共享发送缓存区
void ipc_prog_othercore_rawpkg_to_shm_txque(enet_car_tx_t *p_frame_src)
{
    // 获取发送给对应核心的共享内存指针
    tx_ctrl_t                    *p_ctrl_dest;
    uint8_t                       buf_idx_dest;
    enet_car_tx_t                *p_shm_dest;  // 读取当前共享队列指针
    enet_car_tx_t                *p_frame_dest;
    static uint32_t               ipc_shmque_frame_cnt = 0;
    uint32_t                      length_byte;
    uint32_t                      length_16 = 0;  // 16字节个数
    uint32_t                      length_4  = 0;  // 4字节个数
    uint8_t                       Core_ID;

    Core_ID   = get_Current_Core_ID();  // 获得当前Core序号

    p_shm_dest = get_send_shm_to_txfifo(Core_ID);  // 读取当前共享队列指针
    if (p_shm_dest == NULL)
    {
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 10;
        return;
    }
    p_ctrl_dest  = &tx_ctrls[Core_ID];
    buf_idx_dest = p_ctrl_dest->active_buf_idx;
    p_frame_dest = &p_shm_dest[buf_idx_dest];

    ipc_shmque_frame_cnt++;  // 从1开始
    if (ipc_shmque_frame_cnt == 0)
    {  // 防止反转 为0
        ipc_shmque_frame_cnt = 1;
    }
    p_frame_src->ipc_frame_cnt = ipc_shmque_frame_cnt;  // 核间通信帧计数

    length_byte = CAR_HEADER_LEN + p_frame_src->length + 4;  /// 包含车头、 payload、 校验和
    // 16字节对齐
    length_16 = (length_byte + ALIGNED_SIZE - 1) / ALIGNED_SIZE;
    length_4  = length_16 * 4;

    length_4    = length_4 + 1;     // ipc_frame_cnt
    length_byte = length_4 * 4;  // 转换为Byte

    // 将一帧报文写入共享内存
    memcpy(p_frame_dest, p_frame_src, length_byte);
    cache_wb_com(p_frame_dest, length_byte, CacheP_TYPE_ALL);  // 刷新Cache

    // 切换缓冲区
    p_ctrl_dest->active_buf_idx = ((p_ctrl_dest->active_buf_idx + 1) & (p_ctrl_dest->frame_num - 1));

    Core_enet_queue_monitor_inf.Enet_send_Core_2_ShmRam_queue_OK_number++;
}
#endif