/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       inet_queue_common.c
 *@author     jinyangh
 *@date       2025.11.25
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.11.25  1.0       jinyangh    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "net_all_include.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
uint8_t        inet_macaddr[INET_CORE_NUM][6];
uint8_t        broadcast_macaddr[6]  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t        rec_inet_port_flag[2] = {0, 0}; //这两个字节分别表示单播和组播的接收使能标志位

enet_car_tx_t  r0_multicast_que[S_CAR_MAX_NUM] ENET_R0_MULTICAST_SHARE = {0};

enet_car_tx_t  r0_tx_r1_que[S_CAR_MAX_NUM] ENET_R0_TX_R1_SHARE = {0};
enet_car_tx_t  r2_tx_r1_que[S_CAR_MAX_NUM] ENET_R2_TX_R1_SHARE = {0};
enet_car_tx_t  r3_tx_r1_que[S_CAR_MAX_NUM] ENET_R3_TX_R1_SHARE = {0};
enet_car_tx_t  r1_multicast_que[S_CAR_MAX_NUM] ENET_R1_MULTICAST_SHARE = {0};

enet_car_tx_t  r0_tx_r2_que[S_CAR_MAX_NUM] ENET_R0_TX_R2_SHARE = {0};
enet_car_tx_t  r1_tx_r2_que[S_CAR_MAX_NUM] ENET_R1_TX_R2_SHARE = {0};
enet_car_tx_t  r3_tx_r2_que[S_CAR_MAX_NUM] ENET_R3_TX_R2_SHARE = {0};
enet_car_tx_t  r2_multicast_que[S_CAR_MAX_NUM] ENET_R2_MULTICAST_SHARE = {0};

enet_car_tx_t  r0_tx_r3_que[S_CAR_MAX_NUM] ENET_R0_TX_R3_SHARE = {0};
enet_car_tx_t  r1_tx_r3_que[S_CAR_MAX_NUM] ENET_R1_TX_R3_SHARE = {0};
enet_car_tx_t  r2_tx_r3_que[S_CAR_MAX_NUM] ENET_R2_TX_R3_SHARE = {0};
enet_car_tx_t  r3_multicast_que[S_CAR_MAX_NUM] ENET_R3_MULTICAST_SHARE = {0};

// 为本核的每个接收共享队列，分配单独的接收信息变量（包括自己给自己发，这个只是方便写代码，实际不用）
// 比如R0的接收队列有：r0_tx_r0_que,     r1_tx_r0_que[1],  r2_tx_r0_que[1],  r3_tx_r0_que[1]
//                 r0_multicast_que, r1_multicast_que, r2_multicast_que, r3_multicast_que
rx_ctrl_t      inet_rx_ctrls[INET_CORE_NUM][INET_TX_MODE_NUM] ={
    [0][0].frame_num = S_CAR_MAX_NUM, [0][1].frame_num = S_CAR_MAX_NUM,
    [1][0].frame_num = S_CAR_MAX_NUM, [1][1].frame_num = S_CAR_MAX_NUM,
    [2][0].frame_num = S_CAR_MAX_NUM, [2][1].frame_num = S_CAR_MAX_NUM,
    [3][0].frame_num = S_CAR_MAX_NUM, [3][1].frame_num = S_CAR_MAX_NUM,
};
// 为本核的每个发送共享队列，分配单独的发送信息变量（包括自己给自己发，这个只是方便写代码，实际不用）
// 比如R0的发送队列有：r0_tx_r0_que, r0_tx_r1_que, r0_tx_r2_que, r0_tx_r3_que, r0_multicast_que
tx_ctrl_t      inet_tx_ctrls[INET_CORE_NUM + 1] ={
    [0].frame_num = S_CAR_MAX_NUM,
    [1].frame_num = S_CAR_MAX_NUM,
    [2].frame_num = S_CAR_MAX_NUM,
    [3].frame_num = S_CAR_MAX_NUM,
    [4].frame_num = S_CAR_MAX_NUM, // 本核组播共享队列
};
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

extern bool verify_addr_checksum(const addr_info_t *info);
/**
 * @brief          : 内网初始化
 */
void inet_init(void)
{
    uint32_t sec_cnt = 0u;

    //cache_inv_com(&ipc_shm_info.addr_info, sizeof(ipc_shm_info.addr_info), CacheP_TYPE_ALL);

    //chengenmin 2026-08-19
    //必须保证 inet_macaddr 完全正确，因为VDO核间通信通过mac来决定是哪个核发来的
    //while (0x12345678 != ipc_shm_info.addr_info.magic_flag)
    while (!verify_addr_checksum(&ipc_shm_info.addr_info))
    {
        Osal_delay(1000);
        sec_cnt++;
        if (sec_cnt >= 5u)  // 超过5秒即认为配置异常情况
        {
            memset(&inet_macaddr, 0, sizeof(inet_macaddr));
            DebugP_log("[R%d][Error] inet mac err!!\n", getCoreNr());
            return;
        }
        //cache_inv_com(&ipc_shm_info.addr_info, sizeof(ipc_shm_info.addr_info), CacheP_TYPE_ALL);
    }

    for (int core = 0; core < INET_CORE_NUM; core++)
    {
        memcpy(inet_macaddr[core], &ipc_shm_info.addr_info.cores[core].lan_mac, 6);
    }
}

/**
 * @brief          : 初始化内网接收使能标志位
 * @param rec_port : 接收信息
 *                   bit0  ~ bit7 ：sCore序号
 *                   bit8  ~ bit15：模式（00 –为单播，FF —为核间组播）
 *                   bit16 ~ bit31：备用
 *                   eg: 0x0001  单播，核0
 *                       0xFF04  组播，核2
 * @attention      : 底层会判断内网接收使能标志位，如果有，才去扫接收队列中的报文
 */
void rec_net_raw_core_2_core_port_enable(uint32_t rec_port)
{
    uint8_t core_num = (uint8_t)(rec_port & 0xFF);
    uint8_t mode     = (uint8_t)((rec_port >> 8) & 0xFF);

    if (mode == 0x00) // 单播
    {
        rec_inet_port_flag[0] |= core_num;
    }
    else if (mode == 0xFF) // 组播
    {
        rec_inet_port_flag[1] |= core_num;
    }
}

/**
 * @brief          : 获取其他核发给本核的内网共享队列或其他核的组播共享队列
 * @param core_id  : 核心ID
 *                   R0：0
 *                   R1：1
 *                   R2：2
 *                   R3：3
 *                   A ：4
 * @param mode     : 接收报文模式（单播或组播）
 * @return         : 共享队列指针
 */
enet_car_tx_t *get_inet_rx_shm_queue(uint8_t core_id, inet_tx_mode mode)
{
    if (mode == Unicast)
    {
#if defined(CORE_R5F0)
        enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {NULL, r1_tx_r0_que[1], r2_tx_r0_que[1], r3_tx_r0_que[1]};
#elif defined(CORE_R5F1)
        enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {r0_tx_r1_que, NULL, r2_tx_r1_que, r3_tx_r1_que};
#elif defined(CORE_R5F2)
        enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {r0_tx_r2_que, r1_tx_r2_que, NULL, r3_tx_r2_que};
#elif defined(CORE_R5F3)
        enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {r0_tx_r3_que, r1_tx_r3_que, r2_tx_r3_que, NULL};
#endif
        return inet_shm_queue[core_id];
    }
    else if (mode == Multicast)
    {
#if defined(CORE_R5F0)
        enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {NULL, r1_multicast_que, r2_multicast_que, r3_multicast_que};
#elif defined(CORE_R5F1)
        enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {r0_multicast_que, NULL, r2_multicast_que, r3_multicast_que};
#elif defined(CORE_R5F2)
        enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {r0_multicast_que, r1_multicast_que, NULL, r3_multicast_que};
#elif defined(CORE_R5F3)
        enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {r0_multicast_que, r1_multicast_que, r2_multicast_que, NULL};
#endif
        return inet_shm_queue[core_id];
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief          : 获取本核的组播共享队列
 * @return         : 共享队列指针
 */
enet_car_tx_t *get_inet_tx_multicast_queue(void)
{
    uint8_t        core_id                     = get_Current_Core_ID();
    enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {r0_multicast_que, r1_multicast_que,
                                                    r2_multicast_que, r3_multicast_que};
    return inet_shm_queue[core_id];
}

/**
 * @brief          : 判断是否是单播数据
 * @param p_mac    : 数据mac地址
 * @param core_id  : 单播数据的目的核ID
 * @return         : 单播共享队列
 */
enet_car_tx_t *is_tx_unicast(uint8_t *p_mac, uint8_t *core_id)
{
#if defined(CORE_R5F0)
    enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {NULL, r0_tx_r1_que, r0_tx_r2_que, r0_tx_r3_que};
#elif defined(CORE_R5F1)
    enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {r1_tx_r0_que[1], NULL, r1_tx_r2_que, r1_tx_r3_que};
#elif defined(CORE_R5F2)
    enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {r2_tx_r0_que[1], r2_tx_r1_que, NULL, r2_tx_r3_que};
#elif defined(CORE_R5F3)
    enet_car_tx_t *inet_shm_queue[INET_CORE_NUM] = {r3_tx_r0_que[1], r3_tx_r1_que, r3_tx_r2_que, NULL};
#endif
    for (uint8_t core = 0; core < INET_CORE_NUM; core++)
    {
        if (memcmp(&inet_macaddr[core][0], p_mac, 6) == 0)
        {
            *core_id = core;
            return inet_shm_queue[core];
        }
    }
    return NULL;
}

/**
 * @brief                 : 内网数据入私有队列
 * @param socket_ID       : 要填入的目的队列ID
 * @param p_frame_src     : 需要填入的报文
 * @return                : 成功：true   出错：false
 * @attention             : 此处的参数为tx类型的，需要变成rx类型才入队
 */
bool inet_rx_enqueue(uint32_t socket_ID, enet_car_tx_t *p_frame_src)
{
    EACH_RX_SOCKET_CONFIG_STRUCT     *p_rx_socket_config;
    EACH_RX_QUEUE_STRUCT             *p_rx_queue;
    enet_car_rx_t                    *p_frame_dest;

    uint32_t                         *p_uint32;
    uint32_t                          length_byte;
    uint32_t                          length_16 = 0;  // 16字节个数
    uint32_t                          length_4 = 0;   // 4字节个数

    p_rx_socket_config = &Rx_Meth_Socket_Manager.rx_socket_configs[socket_ID];
    p_rx_queue         = &p_rx_socket_config->queue;

    if (p_rx_socket_config->cfg_flag != true)
    {
        return false;
    }

    if (((p_rx_queue->wr_point + 1) & RX_QUEUE_SIZE) == p_rx_queue->rd_point)
    {
        // 已经到末尾，则丢弃本报文
        return false;
    }

    p_frame_dest                = &p_rx_queue->packets[p_rx_queue->wr_point];
    p_frame_dest->port_type     =  p_frame_src->msg_type;
    p_frame_dest->length        =  p_frame_src->length;
    p_frame_dest->ethtype       = *(uint16_t *)((uint8_t *)p_frame_src->payload + 12);
    memcpy(p_frame_dest->dstmacaddr, (uint8_t *)p_frame_src->payload, 6);
    memcpy(p_frame_dest->payload, p_frame_src->payload, p_frame_src->length);
    //chengenmin 2026-01-06
    // 16字节对齐
    length_byte = CAR_HEADER_LEN + p_frame_dest->length + 4;  // //包含车头、 payload、 校验和
    length_16   = (length_byte + ALIGNED_SIZE - 1) / ALIGNED_SIZE;
    length_4    = (length_16 * 4) - 1;
    // 填写校验和
    p_uint32           = (uint32_t *)&p_frame_dest->fream_head;
    p_uint32[length_4] = cal_enet_rec_checksum_prog(p_frame_dest);
    //chengenmin 2026-01-06
    p_rx_queue->wr_point        =  (p_rx_queue->wr_point + 1) & RX_QUEUE_SIZE;

    return true;
}

/**
 * @brief          : 内网从共享队列接收报文到私有队列
 * @return         : 成功：true   出错：false
 * @attention      : 从其他核发给本核的内网共享队列或其他核的内网组播共享队列中取出报文，并归类到私有队列中
 *                   内网接收和外网接收分为两个函数，虽然都是接收报文到私有队列，但是外网接收逻辑是从单独的接收共享区直接取报文放入私有队列
 */
bool inet_recv_to_rxfifo(void)
{
    bool                            ret_flag                       = true;
    uint8_t                         current_core_id                = get_Current_Core_ID();
    inet_tx_mode                    inet_mode[INET_TX_MODE_NUM]    = {Unicast, Multicast};
    enet_car_tx_t                  *inet_shm_queue                 = NULL;
    enet_car_tx_t                  *p_frame_src                    = NULL;
    rx_ctrl_t                      *p_ctrl_src                     = NULL;
    EACH_RX_SOCKET_CONFIG_STRUCT   *p_rx_socket_config             = NULL;

    // 分别遍历单播和组播的情况
    for (uint8_t mode_idx = 0; mode_idx < INET_TX_MODE_NUM; mode_idx++)
    {
        // 分别遍历所有核的内网数据
        for (uint8_t loop_core = 0; loop_core < INET_CORE_NUM; loop_core++)
        {
            // 判断核i是否有接收使能标志位，如果有，则说明核i会向本核发送内网报文，本核才会去扫共享内存
            if (rec_inet_port_flag[mode_idx] & (1 << loop_core))
            {
                inet_shm_queue = get_inet_rx_shm_queue(loop_core, inet_mode[mode_idx]);
                if (inet_shm_queue == NULL)
                {
                    continue;
                }

                p_ctrl_src = &inet_rx_ctrls[loop_core][mode_idx];
                // 遍历队列中所有的报文
                for (uint8_t loop_frame = 0; loop_frame < p_ctrl_src->frame_num; loop_frame++)
                {
                    uint8_t buf_idx = p_ctrl_src->active_buf_idx; // 当前要读取的报文索引
                    p_frame_src     = &inet_shm_queue[buf_idx];

                    // 获取报文CNT并判断是否是新报文
                    cache_inv_com(&p_frame_src->ipc_frame_cnt, sizeof(p_frame_src->ipc_frame_cnt), CacheP_TYPE_ALL);
                    uint32_t ipc_frame_cnt_new = p_frame_src->ipc_frame_cnt;        // 共享队列的CNT值
                    uint32_t last_recv_cnt     = p_ctrl_src->last_recv_cnt;         // 上次读的CNT值

                    // 判断是否为新数据?
                    bool is_new_frame = check_frame_u32_cnt_add(ipc_frame_cnt_new, last_recv_cnt);  // 通过CNT判断是否有新数据
                    if (is_new_frame)
                    {
                        p_ctrl_src->active_buf_idx = ((p_ctrl_src->active_buf_idx + 1) & (p_ctrl_src->frame_num - 1));
                        cache_inv_com(p_frame_src, sizeof(*p_frame_src), CacheP_TYPE_ALL);

                        if (p_frame_src->msg_type != INET_PORT_TYPE)
                        {
                            // 如果接收到的报文不是内网报文，则跳过
                            continue;
                        }
                        if (inet_mode[mode_idx] == Unicast) // 单播
                        {
                            if (memcmp((uint8_t *)p_frame_src->payload, &inet_macaddr[current_core_id], 6) != 0)
                            {
                                continue;
                            }
                        }
                        else if (inet_mode[mode_idx] == Multicast) // 组播
                        {
                            if ((*(uint8_t *)p_frame_src->payload != MULTICAST_MAC_HEAD)    &&
                                (memcmp((uint8_t *)p_frame_src->payload, broadcast_macaddr, 6) != 0))
                            {
                                continue;
                            }
                        }

                        // 将报文归类到私有队列
                        for (uint32_t socket_ID = 0; socket_ID < MAX_METH_SOCKET_ID; socket_ID++)
                        {
                            p_rx_socket_config = &Rx_Meth_Socket_Manager.rx_socket_configs[socket_ID];
                            if (p_rx_socket_config->cfg_flag != true)
                            {
                                // 此socket ID未配置
                                continue;
                            }

                            for (uint32_t socket_type = 0; socket_type < p_rx_socket_config->reportTypeCount; socket_type++)
                            {
                                // 取出报文类型
                                uint16_t frame_type = *(uint16_t *)((uint8_t *)p_frame_src->payload + 12);
                                if (p_rx_socket_config->reportTypes[socket_type] == frame_type) // 匹配目的报文类型
                                {
                                    // 填入私有队列
                                    if (inet_rx_enqueue(socket_ID, p_frame_src) == false)
                                    {
                                        ret_flag = false;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    else // 该报文不是新数据，认为共享队列读空
                    {
                        if (inet_shm_queue[buf_idx].ipc_frame_cnt == 0 &&
                            inet_shm_queue[((buf_idx + 1) & (p_ctrl_src->frame_num - 1))].ipc_frame_cnt == 0 &&
                            inet_shm_queue[((buf_idx - 1) & (p_ctrl_src->frame_num - 1))].ipc_frame_cnt == 0)
                        {   // cnt均为0，为初始化状态
                            p_ctrl_src->active_buf_idx = 0;
                        }
                        p_ctrl_src->last_recv_cnt = ipc_frame_cnt_new; // 更新本地的CNT值为共享内存的CNT值
                        break;                                         // 读空，读下一个Core
                    }

                    p_ctrl_src->last_recv_cnt = ipc_frame_cnt_new; // 更新本地的CNT值为共享内存的CNT值
                }
            }
        }
    }

    return ret_flag;
}

/**
 * @brief                     : 内网从私有队列发送报文到共享队列
 * @param         p_frame_src : 报文指针
 * @attention                 : 从本核私有队列中取出报文，根据mac地址发送到单播或组播共享队列
 */
void inet_send_to_txque(enet_car_tx_t *p_frame_src)
{
    enet_car_tx_t                *inet_shm_queue          = NULL;
    enet_car_tx_t                *p_frame_dest            = NULL;
    tx_ctrl_t                    *p_ctrl_dest             = NULL;
    uint8_t                       dest_core_id            = 0;

    // 判断数据发到什么地方
    if (p_frame_src->msg_type != INET_PORT_TYPE)
    {
        // 往板外发的数据
        return;
    }

    if (((*(uint8_t *)p_frame_src->payload) == MULTICAST_MAC_HEAD) ||
        (memcmp((uint8_t *)p_frame_src->payload, broadcast_macaddr, 6) == 0)) // 组播或广播
    {
        // 组播共享发送队列
        inet_shm_queue = get_inet_tx_multicast_queue();
        dest_core_id   = INET_CORE_NUM;
    }
    else // 单播或mac地址不对
    {
        inet_shm_queue = is_tx_unicast((uint8_t *)p_frame_src->payload, &dest_core_id);
        if (inet_shm_queue == NULL)
        {
            // 内网mac地址不对
            return;
        }
    }

    p_ctrl_dest = &inet_tx_ctrls[dest_core_id];
    p_frame_dest = &inet_shm_queue[p_ctrl_dest->active_buf_idx];
    p_ctrl_dest->last_send_cnt  = p_ctrl_dest->last_send_cnt + 1;
    if (p_ctrl_dest->last_send_cnt == 0)
    {
        p_ctrl_dest->last_send_cnt = 1;
    }
    p_frame_src->ipc_frame_cnt = p_ctrl_dest->last_send_cnt;

    uint32_t length_byte = CAR_HEADER_LEN + p_frame_src->length + 4;  // 包含车头、 payload、 校验和
    // 16字节对齐
    uint32_t length_16   = (length_byte + ALIGNED_SIZE - 1) / ALIGNED_SIZE;
    uint32_t length_4    = length_16 * 4;

    length_4             = length_4 + 1;     // ipc_frame_cnt
    length_byte          = length_4 * 4;     // 转换为Byte

    // 将一帧报文写入共享内存
    memcpy(p_frame_dest, p_frame_src, length_byte);
    cache_wb_com(p_frame_dest, length_byte, CacheP_TYPE_ALL);

    // 切换缓冲区
    p_ctrl_dest->active_buf_idx = ((p_ctrl_dest->active_buf_idx + 1) & (p_ctrl_dest->frame_num - 1));
}
