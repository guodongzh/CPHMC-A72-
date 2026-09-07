/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       enet_queue_manage_core0.c
 *@author     wenjunf
 *@date       2025.02.17
 *@brief      以太网核间通信底层管理核接口
 *@par        History
 *Date        Version   Author     Description
 *2025.02.17  1.0       wenjunf    以太网核间通信底层管理核接口
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "net_all_include.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
// 以太网报文的核间共享内存定义，这些二维数组队列第一个数组是给外网使用的，第二个数组是给内网使用的
enet_car_tx_t a_tx_r0_que[B_CAR_MAX_NUM] ENET_A_TX_R0_SHARE = {0};       // A核发送队列，32个队列
                                                                         // //Core0发送队列，2个队列
enet_car_tx_t r1_tx_r0_que[2][S_CAR_MAX_NUM] ENET_R1_TX_R0_SHARE = {0};  // Core1发送队列
enet_car_tx_t r2_tx_r0_que[2][S_CAR_MAX_NUM] ENET_R2_TX_R0_SHARE = {0};  // Core2发送队列
enet_car_tx_t r3_tx_r0_que[2][S_CAR_MAX_NUM] ENET_R3_TX_R0_SHARE = {0};  // Core2发送队列
// Core0发送队列
enet_car_rx_t enet_rx_shm_que[RX_CAR_MAX_NUM] ENET_RX_SHARE = {0};  // 以太网接收队列

pcie_enet_tx_queue_t pcie_r0_tx_enetque = {0};  // 向PCIe发送以太网队列

tx_ctrl_t tx_ctrls[SHM_DIR_NUM] = {
    [0].frame_num = S_CAR_MAX_NUM,
    [1].frame_num = S_CAR_MAX_NUM,
    [2].frame_num = S_CAR_MAX_NUM,
    [3].frame_num = S_CAR_MAX_NUM,
    [4].frame_num = B_CAR_MAX_NUM,
};  // 发送R0/R2/R3/A53方向

rx_ctrl_t rx_ctrls[SHM_DIR_NUM] = {
    [0].frame_num = S_CAR_MAX_NUM,
    [1].frame_num = S_CAR_MAX_NUM,
    [2].frame_num = S_CAR_MAX_NUM,
    [3].frame_num = S_CAR_MAX_NUM,
    [4].frame_num = B_CAR_MAX_NUM,
};  // 接收R0/R2/R3/A53方向
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */
static enet_car_tx_t *Core0_get_recv_shm_from_txfifo(uint8_t Core_ID);
static int8_t enqueue_pcie_enet_frame(enet_car_tx_t *frame);
static bool dequeue_pcie_enet_frame(enet_car_tx_t **p_frame_dest,
                                    uint16_t *p_length,
                                    const uint32_t used_ports[4],
                                    bool *has_sent_inet_frame);
/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

#ifdef CORE_R5F0
// 从各Core中取队列，填写到PCIe的发送队列中
// Src_Core_ID --报文来源的Core序号
static enet_car_tx_t *Core0_get_recv_shm_from_txfifo(uint8_t Src_Core_ID)
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

// 扫描Core0发送队列
void ipc_scan_core0_enet_txque_to_pcie_txque(enet_car_tx_t *p_frame_src)
{
    if (enqueue_pcie_enet_frame(p_frame_src) == true)
    {  // 将各核的以太网发送报文，填入PCIe发送队列
        Core_enet_queue_monitor_inf.Enet_send_Core0_2_Core0_enet_PCIe_queue_OK_number++;
    }
}

// 扫描其它Core的以太网报文报文 填到PCIe发送队列
// 将共享内存的发送队列读空为止
void ipc_scan_other_core_enet_txque_to_pcie_txque(void)
{
    enet_car_tx_t *p_frame_src = NULL;
    enet_car_tx_t *p_shm_src = NULL;
    uint8_t buf_idx;             // 读共享队列指针
    uint32_t ipc_frame_cnt_new;  // 共享队列的新的CNT值
    uint32_t last_recv_cnt;      // 本Core上次的CNT值

    // 按顺序扫描各核共享内存队列
    // TODO 目前只扫描A53->R0核的以太网报文
    for (uint8_t loop_core = CORE1_ID; loop_core < CORE53_ID + 1; ++loop_core)
    {
        p_shm_src = Core0_get_recv_shm_from_txfifo(loop_core);
        if (p_shm_src == NULL)
        {
            Core_enet_queue_monitor_inf.Enet_send_err_number++;
            Core_enet_queue_monitor_inf.Enet_send_err_location = 30;
            continue;
        }
        rx_ctrl_t *p_ctrl_src = &rx_ctrls[loop_core];  // 只转发各Core发送的报文

        for (uint8_t i = 0; i < p_ctrl_src->frame_num; ++i)  // 遍历缓存区读空为止
        {
            buf_idx = p_ctrl_src->active_buf_idx;  // 读共享队列指针
            p_frame_src = &p_shm_src[buf_idx];
            cache_inv_com(p_frame_src, sizeof(enet_car_tx_t), CacheP_TYPE_ALL);  // 刷新Cache
            // 此处可以选择不判断是否是核内报文，因为从此处的共享队列中拿出的数据一定是往网口发送的

            ipc_frame_cnt_new = p_shm_src[buf_idx].ipc_frame_cnt;  // 共享队列的CNT值
            last_recv_cnt = p_ctrl_src->last_recv_cnt;             // 上次读的CNT值

            // 判断是否为新数据?
            bool is_new_frame = check_frame_u32_cnt_add(ipc_frame_cnt_new,
                                                        last_recv_cnt);  // 通过CNT判断是否有新数据
            if (is_new_frame)
            {
                if (enqueue_pcie_enet_frame(p_frame_src) == false)
                {  // 将各核的以太网发送报文，填入PCIe发送队列，如队列满了，则退出，不再填PCIe队列
                    return;
                }
                p_ctrl_src->active_buf_idx =
                    ((p_ctrl_src->active_buf_idx + 1) &
                     (p_ctrl_src->frame_num - 1));  // 各Core共享队列读取指针+1，指向下一个队列
                if (loop_core == CORE1_ID)
                {
                    Core_enet_queue_monitor_inf.Enet_send_Core1_2_Core0_enet_PCIe_queue_OK_number++;
                }
                else if (loop_core == CORE2_ID)
                {
                    Core_enet_queue_monitor_inf.Enet_send_Core2_2_Core0_enet_PCIe_queue_OK_number++;
                }
                else if (loop_core == CORE3_ID)
                {
                    Core_enet_queue_monitor_inf.Enet_send_Core3_2_Core0_enet_PCIe_queue_OK_number++;
                }
                else if (loop_core == CORE53_ID)
                {
                    Core_enet_queue_monitor_inf
                        .Enet_send_CoreA53_2_Core0_enet_PCIe_queue_OK_number++;
                }
            }
            else  // 该队列不是新数据，认为共享队列读空
            {
                if (p_shm_src[buf_idx].ipc_frame_cnt == 0 &&
                    p_shm_src[((buf_idx + 1) & (p_ctrl_src->frame_num - 1))].ipc_frame_cnt == 0 &&
                    p_shm_src[((buf_idx - 1) & (p_ctrl_src->frame_num - 1))].ipc_frame_cnt == 0)
                {                                    // cnt均为0，为初始化状态
                    p_ctrl_src->active_buf_idx = 0;  // 当前队列为0
                }
                p_ctrl_src->last_recv_cnt = ipc_frame_cnt_new;  // 更新本地的CNT值为共享内存的CNT值
                break;                                          // 读空，读下一个Core
            }

            p_ctrl_src->last_recv_cnt = ipc_frame_cnt_new;  // 更新本地的CNT值为共享内存的CNT值
        }
    }
}

// Core0 扫描各核的共享队列，填到发送PCIe的发送总队列中
/**
 * 核间通信接收：顺序扫描除本核以外的用于核间通信的报文发送缓冲区
 * 当前仅用作转发A53核心的以太网报文到PCIE
 */
void ipc_scan_all_core_enet_txque_to_pcie_txque(void)
{
    send_to_ethernet();                              // 扫描Core0发送队列
    ipc_scan_other_core_enet_txque_to_pcie_txque();  // 扫描其它Core发送队列
}

/**
 * 将以太网报文用PCIE发出(R0->PCIE->FPGA)
 * @param enet_car_tx_t *packet
 */
void push_enet_frm_to_pcie(void)
{
    car_tx_t *car;
    uint32_t new_ports[4] = {0};
    uint8_t car_index = 0;
    train_tx_t *tx_cfg = enet_tx_cfg;  // 以太网数据进慢速大车厢
    enet_car_tx_t *p_frame;
    uint16_t length;
    uint32_t used_ports[4] = {0};
    bool has_sent_inet_frame = false;  // 标记是否已发送内网报文

    // 每个周期同一端口只发一帧报文，不同端口，可发多帧
    // 检测当前帧的目的端口是否为同一端口？
    // TODO 后续考虑若同一帧报文对多个端口，如何发送？
    while (dequeue_pcie_enet_frame(&p_frame, &length, used_ports, &has_sent_inet_frame))
    {
        if (p_frame->dstport_en_slot1 != 0)
        {
            new_ports[0] = p_frame->dstport_en_slot1;
            used_ports[0] |= new_ports[0];
        }
#ifdef SOC_J721E
        if (p_frame->dstport_en_slot2 != 0)
        {
            new_ports[1] = p_frame->dstport_en_slot2;
            used_ports[1] |= new_ports[1];
            tx_cfg = &slot0_1_train_tx[9];
        }
        else if (p_frame->dstport_en_slot3 != 0)
        {
            new_ports[2] = p_frame->dstport_en_slot3;
            used_ports[2] |= new_ports[2];
            tx_cfg = &slot2_train_tx[9];
        }
        else if (p_frame->dstport_en_slot4 != 0)
        {
            new_ports[3] = p_frame->dstport_en_slot4;
            used_ports[3] |= new_ports[3];
            tx_cfg = &slot3_train_tx[9];
        }
#endif
        car_index = train_tx_get_buff(tx_cfg, CAR_TYPE_B, &car);  // 向PCIe发送列车序号
        if (car != NULL)                                          // 没有空车厢
        {
            memcpy(car, &p_frame->fream_head, length);
            tx_cfg->car_len_b[car_index] = length;
            Core_enet_queue_monitor_inf
                .Enet_send_Core0_2_FPGA_OK_number++;  // 从PCIe的队列中发送给FPGA以太网帧数
        }
        else
        {
            Core_enet_queue_monitor_inf.Enet_send_err_number++;
            Core_enet_queue_monitor_inf.Enet_send_err_location = 31;
            return;
        }
    }
}

/**
 * 管理核R0：负责将FPGA->CPU的PCIE列车数据拷贝到核间共享接收缓冲区
 * 原封不动的将车厢数据拷贝到共享内存
 * 接收端若需要校验加和，需自己就算payload补零的长度，得出加和校验的位置
 */
void pcie_to_enet_rxque(uint8_t *car_addr, uint8_t car_idx, uint8_t effect_car_num)
{
    static uint8_t write_idx_local = 0;
    uint8_t *shm_write_pos = &ipc_shm_info.rx_ctrl_info.enet_write_pos;
    enet_car_rx_t *dst_frame = &enet_rx_shm_que[write_idx_local];

    // 负载数据16字节对齐, 包含车头、 payload、 校验和
    uint16_t length = ((car_rx_t *)car_addr)->payload_len + CAR_HEADER_LEN + CHECK_SUM_LEN;
    // 16字节对齐
    length = (length + ALIGNED_SIZE - 1) / ALIGNED_SIZE;
    length = length * ALIGNED_SIZE;

    memcpy(dst_frame, car_addr, length);

    /*The Ethernet frame contains 4 bytes of CRC,
     * which need to be removed
     * */
    uint32_t *sum_data = (uint32_t *)dst_frame;
    uint16_t len_pakg, len_sum, sum_index;
    dst_frame->length -= 4;
    len_pakg = dst_frame->length + CAR_HEADER_LEN + CHECK_SUM_LEN;
    uint16_t diff = len_pakg % ALIGNED_SIZE;
    if (diff != 0)
    {
        diff = ALIGNED_SIZE - diff;
    }
    len_sum = len_pakg + diff;
    sum_index = (len_sum / 4) - 1;

    /*Recalculate check sum*/
    uint32_t check_sum = 0;
    for (int j = 0; j < sum_index; ++j)
    {
        check_sum += sum_data[j];
    }
    sum_data[sum_index] = check_sum;

    cache_wb_com(dst_frame, len_sum, CacheP_TYPE_ALL);

    write_idx_local = (write_idx_local + 1) & 0x1F;
    Core_enet_queue_monitor_inf.Enet_rec_Core0_2_ShmRam_queue_OK_number++;
    // 最后一帧时共享写索引
    if (car_idx == effect_car_num - 1)
    {
        *shm_write_pos = write_idx_local;
        cache_wb_com(&ipc_shm_info.rx_ctrl_info.enet_write_pos,
                     sizeof(ipc_shm_info.rx_ctrl_info.enet_write_pos),
                     CacheP_TYPE_ALL);
    }
}

// 将各核的以太网发送报文填入给PCIe的发送队列
static int8_t enqueue_pcie_enet_frame(enet_car_tx_t *frame)
{
    pcie_enet_tx_queue_t *p_queue = &pcie_r0_tx_enetque;

    uint8_t wr_point_new = (p_queue->wr_point + 1) & ENET_TX_MAX_NUM;
    if (wr_point_new == p_queue->rd_point)
    {
        // 队列满,则不填写
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 32;
        return false;
    }

    uint16_t length = CAR_HEADER_LEN + frame->length + 4;  // 包含车头、 payload、 校验和

    // 16字节对齐
    length = (length + ALIGNED_SIZE - 1) / ALIGNED_SIZE;
    length = length * ALIGNED_SIZE;

    memcpy(&p_queue->packets[p_queue->wr_point], frame, length + 4);
    p_queue->length[p_queue->wr_point] = length;
    p_queue->wr_point = wr_point_new;

    // Core填PCIe的发送队列以太网帧数
    Core_enet_queue_monitor_inf.Enet_send_Core0_enqueue_pcie_enet_frame_OK_number++;
    return true;
}

// 从发送队列中取出报文，向PCIe发送以太网报文
static bool dequeue_pcie_enet_frame(enet_car_tx_t **p_frame_dest,
                                    uint16_t *p_length,
                                    const uint32_t used_ports[4],
                                    bool *has_sent_inet_frame)
{
    uint32_t new_ports;
    uint32_t conflict_port;
    enet_car_tx_t *p_frame_src;

    pcie_enet_tx_queue_t *p_queue_src = &pcie_r0_tx_enetque;

    if (p_queue_src->rd_point == p_queue_src->wr_point)
    {
        return false;  // 队列为空
    }

    p_frame_src = &p_queue_src->packets[p_queue_src->rd_point];
    // 判断报文类型
    if (p_frame_src->msg_type == FRAME_TYPE_LAN)  // 内网报文
    {
        if (*has_sent_inet_frame)
        {  // 本周期已经发送过内网报文，不再发送
            return false;
        }
        // 标记已发送内网报文
        *has_sent_inet_frame = true;
    }
    else  // 外网报文
    {
        if (p_frame_src->dstport_en_slot1 != 0)
        {
            new_ports = p_frame_src->dstport_en_slot1;  // 获得该报文的发送网口
            conflict_port = used_ports[0] & new_ports;
        }
        else if (p_frame_src->dstport_en_slot2 != 0)
        {
            new_ports = p_frame_src->dstport_en_slot2;  // 获得该报文的发送网口
            conflict_port = used_ports[1] & new_ports;
        }
        else if (p_frame_src->dstport_en_slot3 != 0)
        {
            new_ports = p_frame_src->dstport_en_slot3;  // 获得该报文的发送网口
            conflict_port = used_ports[2] & new_ports;
        }
        else
        {
            new_ports = p_frame_src->dstport_en_slot4;  // 获得该报文的发送网口
            conflict_port = used_ports[3] & new_ports;
        }
        if (conflict_port != 0)
        {  // 网口已用,该报文不发送
            return false;
        }
    }
    *p_frame_dest = p_frame_src;
    *p_length = p_queue_src->length[p_queue_src->rd_point];
    p_queue_src->rd_point = (p_queue_src->rd_point + 1) & ENET_TX_MAX_NUM;  // 读指针+1
    return true;
}

#endif
