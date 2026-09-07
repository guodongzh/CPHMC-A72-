/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       enet_queue_common.c
 *@author     wenjunf
 *@date       2024.07.01
 *@brief      以太网核间通信底层共用接口
 *@par        History
 *Date        Version   Author     Description
 *2024.07.01  1.0       wenjunf    以太网核间通信底层共用接口
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "net_all_include.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
enet_car_rx_t    g_rxethpkg[MAX_METH_SOCKET_ID] = {0};
NET_RAW_PKG_INFO usrrawpkg[MAX_METH_SOCKET_ID]  = {0};

static uint16_t port_frame_cnt = 0;
#if defined(NET_PROTOCOL_DEBUG_LOG_ENABLE) && (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
static uint32_t s_raw_queue_full_count        = 0U;
static uint32_t s_network_queue_full_count    = 0U;
#endif

RX_METH_SOCKET_MANAGE_STRUCT Rx_Meth_Socket_Manager;
TX_METH_SOCKET_MANAGE_STRUCT Tx_Meth_Socket_Manager;
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// 初始化全局Socket管理器
void initSocketManager(void)
{
    EACH_RX_SOCKET_CONFIG_STRUCT *p_rx_socket_config;
    EACH_TX_SOCKET_CONFIG_STRUCT *p_tx_socket_config;

    memset(&Rx_Meth_Socket_Manager, 0, sizeof(Rx_Meth_Socket_Manager));
    memset(&Tx_Meth_Socket_Manager, 0, sizeof(Tx_Meth_Socket_Manager));
    memset(&Core_enet_queue_monitor_inf, 0, sizeof(Core_enet_queue_monitor_inf));
    // 设置Socket_ID=0 为arp、ICMP 和UDP
    p_rx_socket_config = &Rx_Meth_Socket_Manager.rx_socket_configs[0];
    // 配置底层需处理的以太网报文类型
    p_rx_socket_config->cfg_flag        = true;
    p_rx_socket_config->reportTypeCount = 2;
    p_rx_socket_config->reportTypes[0]  = 0x0806;  // arp报文
    p_rx_socket_config->reportTypes[1]  = 0x0800;  // IP报文

    // 设置Socket_ID=0 为arp、ICMP 和UDP
    p_tx_socket_config = &Tx_Meth_Socket_Manager.tx_socket_configs[0];
    // 配置底层需处理的以太网报文类型
    p_tx_socket_config->cfg_flag        = true;
    p_tx_socket_config->reportTypeCount = 2;
    p_tx_socket_config->reportTypes[0]  = 0x0806;  // arp报文
    p_tx_socket_config->reportTypes[1]  = 0x0800;  // IP报文
}

/**
 * 接收Socket_ID初始化函数
 * 功能：用于在socket_ID中添加需要归在该类socket_ID的以太网报文类型
 * @param socket_ID:Socket_ID序号
 * @param report_type_number:需要归类的报文类型个数
 * @param p_report_type:报文类型缓存区指针
 */
// socket_ID_app --从1开始，0 为ping 和UDP 底层的以太网类型
int32_t init_Rx_Socket(uint32_t socket_ID, uint16_t report_type_number, uint16_t *p_report_type)
{
    EACH_RX_SOCKET_CONFIG_STRUCT *p_rx_socket_config;

    if (socket_ID == 0)
    {
        Core_enet_queue_monitor_inf.Enet_rec_err_number++;
        Core_enet_queue_monitor_inf.Enet_rec_err_location = 1;
        //       DebugP_log("ERROR: socket_ID:%d invalid![Rx_Meth_Socket_Manager]\r\n", socket_ID);
        return -1;  // 无效的socket_ID
    }
    if (socket_ID >= MAX_METH_SOCKET_ID)
    {
        Core_enet_queue_monitor_inf.Enet_rec_err_number++;
        Core_enet_queue_monitor_inf.Enet_rec_err_location = 2;
        //       DebugP_log("ERROR: socket_ID:%d invalid![Rx_Meth_Socket_Manager]\r\n", socket_ID);
        return -1;  // 无效的socket_ID
    }

    if (report_type_number > MAX_REPORT_TYPES)
    {
        Core_enet_queue_monitor_inf.Enet_rec_err_number++;
        Core_enet_queue_monitor_inf.Enet_rec_err_location = 3;
        //       DebugP_log("ERROR: socket_ID:%d, report_type_number exceeds limit![Rx_Meth_Socket_Manager]\r\n",
        //       socket_ID);
        return -2;  // 报文类型数量超出限制
    }

    //
    p_rx_socket_config = &Rx_Meth_Socket_Manager.rx_socket_configs[socket_ID];
    if (p_rx_socket_config->cfg_flag == true)
    {  // 已配置
        Core_enet_queue_monitor_inf.Enet_rec_err_number++;
        Core_enet_queue_monitor_inf.Enet_rec_err_location = 4;
        return -4;  // socket_ID 已经被使用
    }

    p_rx_socket_config->cfg_flag = true;
    // 配置报文类型
    p_rx_socket_config->reportTypeCount = report_type_number;
    memcpy(p_rx_socket_config->reportTypes, p_report_type, report_type_number * sizeof(uint16_t));

    return 0;
}

/**
 * 发送Socket_ID初始化函数
 * 功能：用于在socket_ID中添加需要归在该类socket_ID的以太网报文类型
 * @param socket_ID:Socket_ID序号
 * @param report_type_number:需要归类的报文类型个数
 * @param p_report_type:报文类型缓存区指针
 */
// socket_ID_app --从1开始，0 为ping 和UDP 底层的以太网类型
int32_t init_Tx_Socket(uint32_t socket_ID, uint16_t report_type_number, uint16_t *p_report_type)
{
    EACH_TX_SOCKET_CONFIG_STRUCT *p_tx_socket_config;

    if (socket_ID == 0)
    {
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 1;
        //       DebugP_log("ERROR: socket_ID:%d invalid![Rx_Meth_Socket_Manager]\r\n", socket_ID);
        return -1;  // 无效的socket_ID
    }

    if (socket_ID >= MAX_METH_SOCKET_ID)
    {
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 2;
        //       DebugP_log("ERROR: socket_ID:%d invalid![Rx_Meth_Socket_Manager]\r\n", socket_ID);
        return -1;  // 无效的socket_ID
    }

    if (report_type_number > MAX_REPORT_TYPES)
    {
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 3;
        //       DebugP_log("ERROR: socket_ID:%d, report_type_number exceeds limit![Rx_Meth_Socket_Manager]\r\n",
        //       socket_ID);
        return -2;  // 报文类型数量超出限制
    }

    //
    p_tx_socket_config = &Tx_Meth_Socket_Manager.tx_socket_configs[socket_ID];
    if (p_tx_socket_config->cfg_flag == true)
    {  // 已配置
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 4;
        return -4;  // socket_ID 已经被使用
    }

    p_tx_socket_config->cfg_flag  = true;
    p_tx_socket_config->send_mode = 0;
    // 配置报文类型
    p_tx_socket_config->reportTypeCount = report_type_number;
    memcpy(p_tx_socket_config->reportTypes, p_report_type, report_type_number * sizeof(uint16_t));

    return 0;
}

int32_t init_Tx_Socket_with_sendmode(uint32_t socket_ID, uint16_t report_type_number, uint16_t *p_report_type, uint16_t send_mode)
{
    int32_t                       ret_val;
    EACH_TX_SOCKET_CONFIG_STRUCT *p_tx_socket_config;

    ret_val = init_Tx_Socket(socket_ID, report_type_number, p_report_type);
    if (ret_val != 0)
    {
        return ret_val;
    }
    p_tx_socket_config            = &Tx_Meth_Socket_Manager.tx_socket_configs[socket_ID];
    p_tx_socket_config->send_mode = send_mode;
    return 0;
}

// ####################################################################################################
//						接收程序区
// ####################################################################################################
// 接收以太网报文填入本Core的归类队列中
int8_t enqueueRxPacket(uint32_t socket_ID, const enet_car_rx_t *p_packet)
{
    EACH_RX_SOCKET_CONFIG_STRUCT *p_rx_socket_config;
    EACH_RX_QUEUE_STRUCT         *p_rx_queue;

    if (Rx_Meth_Socket_Manager.rx_socket_configs[socket_ID].cfg_flag != true)
    {  // 无效的socket_ID
        Core_enet_queue_monitor_inf.Enet_rec_err_number++;
        Core_enet_queue_monitor_inf.Enet_rec_err_location = 10;
        return false;  // 无效的socket_ID
    }

    p_rx_socket_config = &Rx_Meth_Socket_Manager.rx_socket_configs[socket_ID];
    p_rx_queue         = &p_rx_socket_config->queue;

    if (((p_rx_queue->wr_point + 1) & RX_QUEUE_SIZE) == p_rx_queue->rd_point)
    {  // 已经到末尾，则丢弃本报文
        Core_enet_queue_monitor_inf.Enet_rec_err_number++;
        Core_enet_queue_monitor_inf.Enet_rec_err_location = 11;
        return false;  // 无效的socket_ID
    }

    // 负载数据16字节对齐
    uint16_t length = p_packet->length + CAR_HEADER_LEN + 4;  // 包含车头、 payload、 校验和
    // 16字节对齐
    length = (length + ALIGNED_SIZE - 1) / ALIGNED_SIZE;
    length = length * ALIGNED_SIZE;

    memcpy(&p_rx_queue->packets[p_rx_queue->wr_point], p_packet, length);
    p_rx_queue->wr_point = (p_rx_queue->wr_point + 1) & RX_QUEUE_SIZE;  // wr_point--写指针
    return true;
}

// 从本核的归类队列中出队列
int8_t dequeueRxPacket(uint32_t socket_ID, enet_car_rx_t *p_dest_packet)
{
    uint32_t                      cal_enet_sum;
    uint32_t                      report_enet_sum;
    enet_car_rx_t                *p_src_enet_car_rx_t;
    uint32_t                     *p_uint32;
    uint32_t                      length_byte;
    uint32_t                      length_16 = 0;  // 16字节个数
    uint32_t                      length_4  = 0;  // 4字节个数
    EACH_RX_SOCKET_CONFIG_STRUCT *p_socket_config;

    p_socket_config = &Rx_Meth_Socket_Manager.rx_socket_configs[socket_ID];
    if (p_socket_config->cfg_flag != true)
    {  // 没有配置
        Core_enet_queue_monitor_inf.Enet_rec_err_number++;
        Core_enet_queue_monitor_inf.Enet_rec_err_location = 21;
        return false;  // 无效的socket_ID
    }

    EACH_RX_QUEUE_STRUCT *p_rx_queue = &p_socket_config->queue;

    if (p_rx_queue->rd_point != p_rx_queue->wr_point)
    {
        // 检查校验和是否正确
        p_src_enet_car_rx_t = &p_rx_queue->packets[p_rx_queue->rd_point];
        cal_enet_sum        = cal_enet_rec_checksum_prog(p_src_enet_car_rx_t);
        // 16字节对齐
        length_byte = CAR_HEADER_LEN + p_src_enet_car_rx_t->length + 4;  // 包含ipc_frame_cnt的长度和校验和
        length_16   = (length_byte + ALIGNED_SIZE - 1) / ALIGNED_SIZE;   // 包含校验和 +4
        length_4    = (length_16 * 4) - 1;
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
        p_uint32    = (uint32_t *)&p_src_enet_car_rx_t->fream_head;
        #pragma GCC diagnostic pop

        report_enet_sum = p_uint32[length_4];

        if (report_enet_sum != cal_enet_sum)
        {                                                                       // 校验和不正确，则丢弃
            p_rx_queue->rd_point = (p_rx_queue->rd_point + 1) & RX_QUEUE_SIZE;  // rd_point--读指针
            Core_enet_queue_monitor_inf.Enet_rec_err_number++;
            Core_enet_queue_monitor_inf.Enet_rec_err_location = 22;
            return ENET_SUM_ERR;
        }
        memcpy(p_dest_packet, p_src_enet_car_rx_t, (length_4 * 4));

        p_rx_queue->rd_point = (p_rx_queue->rd_point + 1) & RX_QUEUE_SIZE;  // rd_point--读指针
        return true;
    }
    else  // 队列空
    {
        // Queue is empty
        return false;
    }
}

int32_t recvfrom(uint32_t socket_ID, enet_car_rx_t *packet, uint32_t timeoutMs)
{
    int8_t ret_val;

    if (Rx_Meth_Socket_Manager.rx_socket_configs[socket_ID].cfg_flag != true)
    {
        Core_enet_queue_monitor_inf.Enet_rec_err_number++;
        Core_enet_queue_monitor_inf.Enet_rec_err_location = 31;
        return -1;  // 无效的socket_ID
    }
    ret_val = dequeueRxPacket(socket_ID, packet);
    if (ret_val == true)  // 有报文
    {
        return packet->length;  // 返回实际接收的数据包长度
    }
    else if (ret_val == false)
    {
        return ENET_QUE_EMPTY;  // Queue is empty
    }
    else
    {
        return ENET_SUM_ERR;  // 校验和出错
    }
}

// 根据接收到的以太网报文类型归类入队
int8_t inet_fpga_to_rxfifo(void)
{
    static uint8_t                read_idx_local = 0;
    uint8_t                      *shm_write_pos  = &ipc_shm_info.rx_ctrl_info.enet_write_pos;
    uint32_t                      loop_i;
    enet_car_rx_t                *p_src_frame;
    EACH_RX_SOCKET_CONFIG_STRUCT *p_rx_socket_config;

    cache_inv_com(&ipc_shm_info.rx_ctrl_info.enet_write_pos,
                  sizeof(ipc_shm_info.rx_ctrl_info.enet_write_pos),
                  CacheP_TYPE_ALL);
    for (loop_i = 0; loop_i < 16; loop_i++)
    {
        // 最多处理16帧
        if (read_idx_local == *shm_write_pos)
        {  // 队列空
            break;
        }
        p_src_frame = &enet_rx_shm_que[read_idx_local];
        cache_inv_com(p_src_frame, sizeof(enet_car_rx_t), CacheP_TYPE_ALL);

        read_idx_local++;
        read_idx_local &= (RX_CAR_MAX_NUM - 1);
        for (uint32_t socket_ID = 0; socket_ID < MAX_METH_SOCKET_ID; socket_ID++)
        {
            p_rx_socket_config = &Rx_Meth_Socket_Manager.rx_socket_configs[socket_ID];
            if (p_rx_socket_config->cfg_flag != true)
            {
                continue;
            }
            for (uint16_t i = 0; i < p_rx_socket_config->reportTypeCount; i++)
            {
                if (p_src_frame->ethtype == p_rx_socket_config->reportTypes[i])
                {
                    if (enqueueRxPacket(socket_ID, p_src_frame) == true)
                    {
                        Core_enet_queue_monitor_inf.Enet_rec_Core_2_Class_queue_OK_number++;
                    }
                    break;
                }
            }
        }
    }
    return TRUE;
}

/**
 *
 * @param socket_ID Socket序号，从哪个以太网接收缓存区取报文，从0开始
 * @param pCallback 回调函数
 * @param nCount 回调函数重复调用个数,读空立即退出
 * @param pEnv 一个预留的指针，传参用。默认可赋NULL。
 * @return 实际处理的包数 ，0-没有处理报数，-1--输入参数出错 -4--校验和出错
 */
int32_t inet_rxfifo_to_app(uint32_t socket_ID_app, callback_recvfrom_raw pCallback, int32_t nCount, void *pEnv)
{
    int32_t  retval;
    int32_t  validpkgnum   = 0;
    bool     callback_flag = false;
    
    if ((pCallback != NULL) && (nCount > 0) && (socket_ID_app < MAX_METH_SOCKET_ID))
    {
        while (nCount--)
        {
            retval = recvfrom(socket_ID_app, &g_rxethpkg[socket_ID_app], 10);  // 先从循环buffer中取出一包数据
            if (retval > 0)
            {
                usrrawpkg[socket_ID_app].nEtherType     = g_rxethpkg[socket_ID_app].ethtype;
                usrrawpkg[socket_ID_app].nLength        = g_rxethpkg[socket_ID_app].length;
                usrrawpkg[socket_ID_app].pRawPkg        = g_rxethpkg[socket_ID_app].payload + 14;
                usrrawpkg[socket_ID_app].pMacPkg        = g_rxethpkg[socket_ID_app].payload;
                usrrawpkg[socket_ID_app].PortType       = g_rxethpkg[socket_ID_app].port_type;
                usrrawpkg[socket_ID_app].Src_Slot_ID    = g_rxethpkg[socket_ID_app].src_slot;
                usrrawpkg[socket_ID_app].Src_Port_ID    = g_rxethpkg[socket_ID_app].src_port - 1;
                usrrawpkg[socket_ID_app].NetStorm_State = g_rxethpkg[socket_ID_app].net_storm_state;

                memcpy(usrrawpkg[socket_ID_app].rmtMac, &g_rxethpkg[socket_ID_app].payload[6], ENET_MAC_ADDR_LEN);
                memcpy(usrrawpkg[socket_ID_app].dstMac, g_rxethpkg[socket_ID_app].dstmacaddr, ENET_MAC_ADDR_LEN);
                callback_flag = pCallback(&usrrawpkg[socket_ID_app]);
                Core_enet_queue_monitor_inf.Enet_rec_Core_2_callback_OK_number++;  // Core调用接收回调函数次数
                if (!callback_flag)
                {
                    // 若返回值为false则表示应用层未该帧报文处理，底层需判以太网类型根据需要交由协议栈处理
                }
                validpkgnum++;
            }
            else if (retval == ENET_SUM_ERR)
            {  // 校验和出错
                return ENET_SUM_ERR;
            }
        }
    }
    else
    {
        Core_enet_queue_monitor_inf.Enet_rec_err_number++;
        Core_enet_queue_monitor_inf.Enet_rec_err_location = 32;
        validpkgnum                                       = -1;
    }

    return validpkgnum;
}

// 计算以太网列车接收校验和
// 返回校验和
uint32_t cal_enet_rec_checksum_prog(enet_car_rx_t *rx_frame)
{
    uint32_t        calc_checksum;
    uint32_t        length_byte;
    uint32_t        length_16 = 0;  // 16字节个数
    uint32_t        length_4  = 0;  // 4字节个数
    const uint32_t *p_uint32;

    // 计算32位加和
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    p_uint32    = (const uint32_t *)&rx_frame->fream_head;
    #pragma GCC diagnostic pop
    length_byte = CAR_HEADER_LEN + rx_frame->length + 4;  // 包含车头、 payload、 校验和

    // 16字节对齐
    length_16 = (length_byte + ALIGNED_SIZE - 1) / ALIGNED_SIZE;

    // 包含校验和的16字节对齐
    length_4 = (length_16 * 4) - 1;  // 去掉校验和

    calc_checksum = 0;
    for (uint32_t i = 0; i < length_4; i++)
    {
        calc_checksum += p_uint32[i];
    }

    return calc_checksum;
}

// ###################################################################################################
//				发送程序区
//	##################################################################################################

// 计算以太网列车发送校验和
// 返回校验和
uint32_t cal_enet_send_checksum_prog(enet_car_tx_t *tx_frame)
{
    uint32_t        calc_checksum;
    uint32_t        length_byte;
    uint32_t        length_16 = 0;  // 16字节个数
    uint32_t        length_4  = 0;  // 4字节个数
    const uint32_t *p_uint32;

    // 计算32位加和
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    p_uint32 = (const uint32_t *)&tx_frame->fream_head;
    #pragma GCC diagnostic pop

    length_byte = CAR_HEADER_LEN + tx_frame->length + 4;  // 包含车头、 payload、 校验和

    length_16 = (length_byte + ALIGNED_SIZE - 1) / ALIGNED_SIZE;
    length_4  = (length_16 * 4) - 1;

    calc_checksum = 0;
    for (uint32_t i = 0; i < length_4; i++)
    {
        calc_checksum += p_uint32[i];
    }

    return calc_checksum;
}

/**
 * 以太网发送函数
 * @param frame_pri 报文优先级(1:高优先级,0:低优先级)
 * @param socket_ID:Socket序号，填写到哪个以太网缓存区
 * @param pRawPkg:报文类型后面的内容
 * @param nLength:报文长度
 * @param nFlag:发送标志
 * @param enet_send_port_def:端口类型，发送的端口,按bit位区分,1个网口占个2Bit
 * @param pDstMac:目的MAC地址
 * @param nEtherType:报文类型，按大端方式传参
 * @param nVLAN:VLAN
 * @return 返回成功发送的个数
 */
int32_t sendto_raw_socket(uint8_t                   frame_pri,
                          uint32_t                  socket_ID,
                          const uint8_t            *pRawPkg,
                          uint32_t                  nLength,
                          int32_t                   nFlag,
                          ENET_SEND_PORT_DEF_STRUCT enet_send_port_def,
                          const uint8_t            *pDstMac,
                          uint16_t                  nEtherType,
                          uint32_t                  nVLAN)
{
    CURRENT_ENET_INF_STRUCT             Current_enet_inf;  // 当前enet信息
    CURRENT_CORE_ENET_MAC_IP_INF_STRUCT Current_core_enet_Mac_ip_inf;
    EACH_TX_QUEUE_STRUCT               *p_each_tx_queue;
    EthFrame                           *txFrame;      // 以太网帧不带vlan
    EthVlanFrame                       *txVlanFrame;  // 以太网帧带vlan
    uint16_t                            Vlan_tpid;
    enet_car_tx_t                      *p_ennet_car_tx_t;
    uint32_t                           *p_uint32;
    uint32_t                            length_byte;
    uint32_t                            length_16 = 0;  // 16字节个数
    uint32_t                            length_4  = 0;  // 4字节个数
    uint32_t                            bitmap;
    uint32_t                            loop_slot;
    uint32_t                            loop_net;
    static uint16_t                     port_frame_cnt = 0;

    if (Tx_Meth_Socket_Manager.tx_socket_configs[socket_ID].cfg_flag != true)
    {
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 20;
        return FALSE;
    }
    p_each_tx_queue = &Tx_Meth_Socket_Manager.tx_socket_configs[socket_ID].queue;
    if (((p_each_tx_queue->wr_point + 1) & TX_QUEUE_SIZE) == p_each_tx_queue->rd_point)
    {  // 缓存区满，不填队列
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 21;
#if defined(NET_PROTOCOL_DEBUG_LOG_ENABLE) && (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
        s_raw_queue_full_count++;
        if ((s_raw_queue_full_count == 1U) || ((s_raw_queue_full_count & 0x0FU) == 0U))
        {
            NET_PROTOCOL_DEBUG_LOG("[ENET][TXQ_FULL][RAW] count=%u socket=%u wr=%u rd=%u len=%u type=0x%02X\r\n",
                                   s_raw_queue_full_count,
                                   socket_ID,
                                   p_each_tx_queue->wr_point,
                                   p_each_tx_queue->rd_point,
                                   nLength,
                                   enet_send_port_def.enet_send_Port_type);
        }
#endif
        return false;
    }

    if (nLength > ETH_PAYLOAD_LEN)
    {  // 以太网长度超范围
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 22;
        return false;
    }

    Current_enet_inf.Core_ID = get_Current_Core_ID();                    // 当前Core序号
    Current_enet_inf.Net_type = enet_send_port_def.enet_send_Port_type;  // 以太网类型
    Current_enet_inf.Net_ID = 0xff;
#ifdef SOC_J721E
    for (uint8_t slot_id = 2; slot_id < MAX_ENET_SLOT_NUMBER; slot_id++)
    {
#else
    for (uint8_t slot_id = 0; slot_id < MAX_ENET_SLOT_NUMBER; slot_id++)
#endif
        bitmap = 0x03;  // 2位为1个网口, bit1为收，bit0为发
        for (uint8_t port_id = 0; port_id < MAX_PORT_ID; port_id++)
        {
            if ((enet_send_port_def.nEthMask[slot_id] & bitmap) != 0)
            {
                Current_enet_inf.Net_ID = port_id;
                Current_enet_inf.Slot_ID = slot_id;
                break;
            }
            bitmap = bitmap << 2;
        }
        if (Current_enet_inf.Net_ID != 0xff)
        {
            break;
        }
    }

    if ((Current_enet_inf.Net_ID == 0xff) && (enet_send_port_def.enet_send_Port_type != 0x41))  // 没有找到网口
    {
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 23;
        return false;
    }

    port_frame_cnt++;

    get_locate_enet_inf(&Current_core_enet_Mac_ip_inf, Current_enet_inf);

    p_ennet_car_tx_t                 = &p_each_tx_queue->packets[p_each_tx_queue->wr_point];
    p_ennet_car_tx_t->fream_head     = TXQUEUE_FREAM_HEAD;                      // 帧头: 0x1234：CPU->FPGA
    p_ennet_car_tx_t->port_frame_cnt = port_frame_cnt;                          // 对应端口帧计数
    p_ennet_car_tx_t->msg_type       = enet_send_port_def.enet_send_Port_type;  // 报文类型 (1：FT3,2：百兆口,3：千兆口,4：CAN口,0x41-内网)
    p_ennet_car_tx_t->send_mode1     = nFlag & 0x0f;                            // 发送模式1
    p_ennet_car_tx_t->send_mode2     = (nFlag >> 4) & 0x0f;                     // 发送模式2
    p_ennet_car_tx_t->length         = nLength;

    p_ennet_car_tx_t->dstport_en_slot1 = enet_send_port_def.nEthMask[0];  // 槽位1目的端口标识
    p_ennet_car_tx_t->dstport_en_slot2 = enet_send_port_def.nEthMask[1];  // 槽位2目的端口标识
    p_ennet_car_tx_t->dstport_en_slot3 = enet_send_port_def.nEthMask[2];  // 槽位3目的端口标识
    p_ennet_car_tx_t->dstport_en_slot4 = enet_send_port_def.nEthMask[3];  // 槽位4目的端口标识

    p_ennet_car_tx_t->irq_num   = Current_core_enet_Mac_ip_inf.send_enet_int_ID;  // 中断号
    p_ennet_car_tx_t->resv1     = 0;
    p_ennet_car_tx_t->frame_pri = frame_pri;  // 报文优先级(1:高优先级,0:低优先级)

    // 判Vlan-Tag
    Vlan_tpid = (Enet_htonl(nVLAN) >> 16) & 0xFFFF;  // 取出Vlan-Tag高位2字节Type
    if (Vlan_tpid == ETHERTYPE_VLAN_TAG)             // 包含Vlan
    {

        txVlanFrame = (EthVlanFrame *)p_ennet_car_tx_t->payload;
        memcpy(txVlanFrame->hdr.dstMac, pDstMac, ENET_MAC_ADDR_LEN);
        memcpy(txVlanFrame->hdr.srcMac, &Current_core_enet_Mac_ip_inf.srcMac[0], ENET_MAC_ADDR_LEN);
        memcpy(&txVlanFrame->hdr.tpid, &nVLAN, sizeof(uint32_t));
        txVlanFrame->hdr.etherType = nEtherType;
        memcpy(&txVlanFrame->payload[0U], pRawPkg, nLength);
        p_ennet_car_tx_t->length = sizeof(EthVlanFrameHeader) + nLength;  // 帧头长度+数据包有效载荷长度
    }
    else
    {
        txFrame = (EthFrame *)p_ennet_car_tx_t->payload;
        memcpy(txFrame->hdr.dstMac, pDstMac, ENET_MAC_ADDR_LEN);
        memcpy(txFrame->hdr.srcMac, &Current_core_enet_Mac_ip_inf.srcMac[0], ENET_MAC_ADDR_LEN);
        txFrame->hdr.etherType = nEtherType;
        memcpy(&txFrame->payload[0U], pRawPkg, nLength);
        p_ennet_car_tx_t->length = sizeof(EthFrameHeader) + nLength;  // 帧头长度+数据包有效载荷长度
    }

    // 16字节对齐
    length_byte = CAR_HEADER_LEN + p_ennet_car_tx_t->length + 4;  // //包含车头、 payload、 校验和
    length_16   = (length_byte + ALIGNED_SIZE - 1) / ALIGNED_SIZE;
    length_4    = (length_16 * 4) - 1;
    // 填写校验和
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    p_uint32           = (uint32_t *)&p_ennet_car_tx_t->fream_head;
    #pragma GCC diagnostic pop
    p_uint32[length_4] = cal_enet_send_checksum_prog(p_ennet_car_tx_t);

    p_each_tx_queue->wr_point = (p_each_tx_queue->wr_point + 1) & TX_QUEUE_SIZE;
    Core_enet_queue_monitor_inf.Enet_send_Core_run_sendto_raw_socket_prog_number++;

    return 1;
}

/**
 * 组网报文专用发送函数
 */
int32_t sendto_networking_socket(uint8_t                   frame_pri,
                                 uint32_t                  socket_ID,
                                 const uint8_t            *pRawPkg,
                                 uint32_t                  nLength,
                                 int32_t                   nFlag,
                                 ENET_SEND_PORT_DEF_STRUCT enet_send_port_def,
                                 const uint8_t            *pDstMac,
                                 uint16_t                  nEtherType,
                                 uint32_t                  nVLAN)
{
    CURRENT_ENET_INF_STRUCT Current_enet_inf;  // 当前enet信息
    CURRENT_CORE_ENET_MAC_IP_INF_STRUCT Current_core_enet_Mac_ip_inf;
    EACH_TX_QUEUE_STRUCT *p_each_tx_queue;
    EthFrame *txFrame;          // 以太网帧不带vlan
    EthVlanFrame *txVlanFrame;  // 以太网帧带vlan
    uint16_t Vlan_tpid;
    enet_car_tx_t *p_ennet_car_tx_t;
    uint32_t *p_uint32;
    uint32_t length_byte;
    uint32_t length_16 = 0;  // 16字节个数
    uint32_t length_4 = 0;   // 4字节个数
    uint32_t bitmap;
    uint32_t loop_slot;
    uint32_t loop_net;
    static uint16_t port_frame_cnt = 0;

    (void)loop_slot;

    if (Tx_Meth_Socket_Manager.tx_socket_configs[socket_ID].cfg_flag != true)
    {
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 20;
        return FALSE;
    }
    p_each_tx_queue = &Tx_Meth_Socket_Manager.tx_socket_configs[socket_ID].queue;
    if (((p_each_tx_queue->wr_point + 1) & TX_QUEUE_SIZE) == p_each_tx_queue->rd_point)
    {  // 缓存区满，不填队列
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 21;
#if defined(NET_PROTOCOL_DEBUG_LOG_ENABLE) && (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
        s_network_queue_full_count++;
        if ((s_network_queue_full_count == 1U) || ((s_network_queue_full_count & 0x0FU) == 0U))
        {
            NET_PROTOCOL_DEBUG_LOG("[ENET][TXQ_FULL][NETWORK] count=%u socket=%u wr=%u rd=%u len=%u type=0x%02X\r\n",
                                   s_network_queue_full_count,
                                   socket_ID,
                                   p_each_tx_queue->wr_point,
                                   p_each_tx_queue->rd_point,
                                   nLength,
                                   enet_send_port_def.enet_send_Port_type);
        }
#endif
        return false;
    }

    if (nLength > ETH_PAYLOAD_LEN)
    {  // 以太网长度超范围
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 22;
        return false;
    }

    Current_enet_inf.Slot_ID = get_Current_Slot_ID();                    // 当前插件序号
    Current_enet_inf.Core_ID = get_Current_Core_ID();                    // 当前Core序号
    Current_enet_inf.Net_type = enet_send_port_def.enet_send_Port_type;  // 以太网类型
    Current_enet_inf.Net_ID = 0xff;

#ifdef SOC_J721E
    for (uint8_t slot_id = 2; slot_id < MAX_ENET_SLOT_NUMBER; slot_id++)
    {
#else
    for (uint8_t slot_id = 0; slot_id < MAX_ENET_SLOT_NUMBER; slot_id++)
#endif
        bitmap = 0x03;  // 上面为发，下面收 //2位为1个网口
        for (loop_net = 0; loop_net < 16; loop_net++)
        {
            if ((enet_send_port_def.nEthMask[slot_id] & bitmap) != 0)
            {
                Current_enet_inf.Net_ID = loop_net;
                break;
            }
            bitmap = bitmap << 2;
        }
    }

    if ((Current_enet_inf.Net_ID == 0xff) && (enet_send_port_def.enet_send_Port_type != 0x41))  // 没有找到网口
    {
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 23;
        return false;
    }

    port_frame_cnt++;

    if (get_net_config_info(&Current_core_enet_Mac_ip_inf, Current_enet_inf) != true)
    {
        Core_enet_queue_monitor_inf.Enet_send_err_number++;
        Core_enet_queue_monitor_inf.Enet_send_err_location = 24;
        return false;
    }

    p_ennet_car_tx_t                 = &p_each_tx_queue->packets[p_each_tx_queue->wr_point];
    p_ennet_car_tx_t->fream_head     = TXQUEUE_FREAM_HEAD;  // 帧头: 0x1234：CPU->FPGA
    p_ennet_car_tx_t->port_frame_cnt = port_frame_cnt;      // 对应端口帧计数
    p_ennet_car_tx_t->msg_type =
        enet_send_port_def.enet_send_Port_type;          // 报文类型 (1：FT3,2：百兆口,3：千兆口,4：CAN口,0x41-内网)
    p_ennet_car_tx_t->send_mode1 = nFlag & 0x0f;         // 发送模式1
    p_ennet_car_tx_t->send_mode2 = (nFlag >> 4) & 0x0f;  // 发送模式2
    p_ennet_car_tx_t->length     = nLength;

    p_ennet_car_tx_t->dstport_en_slot1 = enet_send_port_def.nEthMask[0];  // 槽位1目的端口标识
    p_ennet_car_tx_t->dstport_en_slot2 = enet_send_port_def.nEthMask[1];  // 槽位2目的端口标识
    p_ennet_car_tx_t->dstport_en_slot3 = enet_send_port_def.nEthMask[2];  // 槽位3目的端口标识
    p_ennet_car_tx_t->dstport_en_slot4 = enet_send_port_def.nEthMask[3];  // 槽位4目的端口标识

    p_ennet_car_tx_t->irq_num   = Current_core_enet_Mac_ip_inf.send_enet_int_ID;  // 中断号
    p_ennet_car_tx_t->resv1     = 0;
    p_ennet_car_tx_t->frame_pri = frame_pri;  // 报文优先级(1:高优先级,0:低优先级)

    // 判Vlan-Tag
    Vlan_tpid = (Enet_htonl(nVLAN) >> 16) & 0xFFFF;  // 取出Vlan-Tag高位2字节Type
    if (Vlan_tpid == ETHERTYPE_VLAN_TAG)             // 包含Vlan
    {
        txVlanFrame = (EthVlanFrame *)p_ennet_car_tx_t->payload;
        memcpy(txVlanFrame->hdr.dstMac, pDstMac, ENET_MAC_ADDR_LEN);
        memcpy(txVlanFrame->hdr.srcMac, &Current_core_enet_Mac_ip_inf.srcMac[0], ENET_MAC_ADDR_LEN);
        memcpy(&txVlanFrame->hdr.tpid, &nVLAN, sizeof(uint32_t));
        txVlanFrame->hdr.etherType = nEtherType;
        memcpy(&txVlanFrame->payload[0U], pRawPkg, nLength);
        p_ennet_car_tx_t->length = sizeof(EthVlanFrameHeader) + nLength;  // 帧头长度+数据包有效载荷长度
    }
    else
    {
        txFrame = (EthFrame *)p_ennet_car_tx_t->payload;

        memcpy(txFrame->hdr.dstMac, pDstMac, ENET_MAC_ADDR_LEN);

        memcpy(txFrame->hdr.srcMac, &Current_core_enet_Mac_ip_inf.srcMac[0], ENET_MAC_ADDR_LEN);

        txFrame->hdr.etherType = nEtherType;

        memcpy(&txFrame->payload[0U], pRawPkg, nLength);

        p_ennet_car_tx_t->length = sizeof(EthFrameHeader) + nLength;  // 帧头长度+数据包有效载荷长度
    }

    // 16字节对齐
    length_byte = CAR_HEADER_LEN + p_ennet_car_tx_t->length + 4;  // //包含车头、 payload、 校验和
    length_16   = (length_byte + ALIGNED_SIZE - 1) / ALIGNED_SIZE;
    length_4    = (length_16 * 4) - 1;
    // 填写校验和
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    p_uint32           = (uint32_t *)&p_ennet_car_tx_t->fream_head;
    #pragma GCC diagnostic pop
    p_uint32[length_4] = cal_enet_send_checksum_prog(p_ennet_car_tx_t);

    p_each_tx_queue->wr_point = (p_each_tx_queue->wr_point + 1) & TX_QUEUE_SIZE;
    Core_enet_queue_monitor_inf.Enet_send_Core_run_sendto_raw_socket_prog_number++;

    return 1;
}

void send_to_ethernet(void)
{
    enet_car_tx_t                *p_frame_src = NULL;
    EACH_TX_SOCKET_CONFIG_STRUCT *p_each_tx_socket_config;
    EACH_TX_QUEUE_STRUCT         *p_Tx_queue;
    uint32_t                      loop_socket;
    uint32_t                      find_flag;
    uint32_t                      inet_flag = false;
    uint32_t                      enet_flag = false;

    find_flag = 0;
    // 优先扫描业务SocketID队列报文，再扫描SocketID=0的底层arp/ip报文队列
    for (loop_socket = 1; loop_socket < MAX_METH_SOCKET_ID; loop_socket++)
    {
        p_each_tx_socket_config = &Tx_Meth_Socket_Manager.tx_socket_configs[loop_socket];
        if (p_each_tx_socket_config->cfg_flag != true || p_each_tx_socket_config->send_mode != 0)
        {  // 没有配置或者发送模式不为0
            continue;
        }
        p_Tx_queue = &p_each_tx_socket_config->queue;
        if (p_Tx_queue->rd_point == p_Tx_queue->wr_point)
        {  // 该队列缓存区空
            continue;
        }
        find_flag   = true;
        p_frame_src = &p_Tx_queue->packets[p_each_tx_socket_config->queue.rd_point];
        p_Tx_queue->rd_point++;
        p_Tx_queue->rd_point &= TX_QUEUE_SIZE;
        break;
    }
    if (find_flag != true)
    {  // 没有找到报文，检查IP报文
        p_each_tx_socket_config = &Tx_Meth_Socket_Manager.tx_socket_configs[0];
        p_Tx_queue              = &p_each_tx_socket_config->queue;
        if (p_Tx_queue->rd_point == p_Tx_queue->wr_point)
        {  // 该队列缓存区空
            return;
        }
        find_flag   = true;
        p_frame_src = &p_Tx_queue->packets[p_Tx_queue->rd_point];
        p_Tx_queue->rd_point++;
        p_Tx_queue->rd_point &= TX_QUEUE_SIZE;
    }

    if (find_flag != true)
    {  // 所有队列均空
        return;
    }

    if (p_frame_src->msg_type == INET_PORT_TYPE)
    {
        if (is_inet_frame((uint8_t *)p_frame_src->payload) == true)
        {
            // 发往内网的单播或组播数据
            inet_flag = true;
        }
        else if (is_broadcast_frame((uint8_t *)p_frame_src->payload) == true)
        {
            // 广播数据，内外网都发
            inet_flag = true;
            enet_flag = true;
        }
    }
    else
    {
        // 发往外网的数据
        enet_flag = true;
    }

    if (enet_flag == true)
    {
#ifdef CORE_R5F0
        ipc_scan_core0_enet_txque_to_pcie_txque(p_frame_src);
#else
        ipc_prog_othercore_rawpkg_to_shm_txque(p_frame_src);
#endif
    }
    if (inet_flag == true)
    {
        inet_send_to_txque(p_frame_src);
    }
}

/**
 * @brief          : 判断是否是内网数据（此内网指的是核与核之间的单播，和组播数据）
 * @param d_mac    : 目的mac地址
 * @return         : true - 内网数据，false - 非内网数据（外网口、广播）
 */
bool is_inet_frame(uint8_t *d_mac)
{
    if (*d_mac == MULTICAST_MAC_HEAD)
    {
        return true;
    }

    for (uint8_t core = 0; core < INET_CORE_NUM; core++)
    {
        if (memcmp(&inet_macaddr[core][0], d_mac, 6) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief          : 判断是否是广播数据
 * @param d_mac    : 目的mac地址
 * @return         : true - 广播数据，false - 非广播数据
 */
bool is_broadcast_frame(uint8_t *d_mac)
{
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (memcmp(d_mac, broadcast, 6) == 0)
    {
        return true;
    }

    return false;
}

void pcie_enet_shareram_clear(void)
{
    memset(r1_tx_r0_que, 0u, sizeof(r1_tx_r0_que));
    cache_wb_com(r1_tx_r0_que, sizeof(r1_tx_r0_que), CacheP_TYPE_ALLD);

    memset(r2_tx_r0_que, 0u, sizeof(r2_tx_r0_que));
    cache_wb_com(r2_tx_r0_que, sizeof(r2_tx_r0_que), CacheP_TYPE_ALLD);

    memset(r3_tx_r0_que, 0u, sizeof(r3_tx_r0_que));
    cache_wb_com(r3_tx_r0_que, sizeof(r3_tx_r0_que), CacheP_TYPE_ALLD);

    memset(r0_multicast_que, 0u, sizeof(r0_multicast_que));
    cache_wb_com(r0_multicast_que, sizeof(r0_multicast_que), CacheP_TYPE_ALLD);

    memset(r0_tx_r1_que, 0u, sizeof(r0_tx_r1_que));
    cache_wb_com(r0_tx_r1_que, sizeof(r0_tx_r1_que), CacheP_TYPE_ALLD);

    memset(r2_tx_r1_que, 0u, sizeof(r2_tx_r1_que));
    cache_wb_com(r2_tx_r1_que, sizeof(r2_tx_r1_que), CacheP_TYPE_ALLD);

    memset(r3_tx_r1_que, 0u, sizeof(r3_tx_r1_que));
    cache_wb_com(r3_tx_r1_que, sizeof(r3_tx_r1_que), CacheP_TYPE_ALLD);

    memset(r1_multicast_que, 0u, sizeof(r1_multicast_que));
    cache_wb_com(r1_multicast_que, sizeof(r1_multicast_que), CacheP_TYPE_ALLD);

    memset(r0_tx_r2_que, 0u, sizeof(r0_tx_r2_que));
    cache_wb_com(r0_tx_r2_que, sizeof(r0_tx_r2_que), CacheP_TYPE_ALLD);

    memset(r1_tx_r2_que, 0u, sizeof(r1_tx_r2_que));
    cache_wb_com(r1_tx_r2_que, sizeof(r1_tx_r2_que), CacheP_TYPE_ALLD);

    memset(r3_tx_r2_que, 0u, sizeof(r3_tx_r2_que));
    cache_wb_com(r3_tx_r2_que, sizeof(r3_tx_r2_que), CacheP_TYPE_ALLD);

    memset(r2_multicast_que, 0u, sizeof(r2_multicast_que));
    cache_wb_com(r2_multicast_que, sizeof(r2_multicast_que), CacheP_TYPE_ALLD);

    memset(r0_tx_r3_que, 0u, sizeof(r0_tx_r3_que));
    cache_wb_com(r0_tx_r3_que, sizeof(r0_tx_r3_que), CacheP_TYPE_ALLD);

    memset(r1_tx_r3_que, 0u, sizeof(r1_tx_r3_que));
    cache_wb_com(r1_tx_r3_que, sizeof(r1_tx_r3_que), CacheP_TYPE_ALLD);

    memset(r2_tx_r3_que, 0u, sizeof(r2_tx_r3_que));
    cache_wb_com(r2_tx_r3_que, sizeof(r2_tx_r3_que), CacheP_TYPE_ALLD);

    memset(r3_multicast_que, 0u, sizeof(r3_multicast_que));
    cache_wb_com(r3_multicast_que, sizeof(r3_multicast_que), CacheP_TYPE_ALLD);
}
