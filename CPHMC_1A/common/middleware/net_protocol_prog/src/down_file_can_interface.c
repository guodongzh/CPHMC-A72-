/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       down_file_can_interface.c
*@author     xuesen
*@date       2026.05.06
*@brief      一键下载CAN转发处理实现。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/

#include "net_all_include.h"

// 仅为装置策略允许的目标编译实际CAN升级队列和转发任务
#if NET_PROFILE_ENABLE_CAN_FORWARD
#if defined(__has_include)
#if __has_include("can_ipc.h")
#include "can_ipc.h"
#endif
#endif

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

// IO板卡一键下载使用CAN1网，面板一键下载使用CAN2网
DOWN_FILE_ALL_APP_BYTE_SEND_SOCKET_STRUCT            down_all_app_loop_send_socket_can1;   // can1网应用发送队列
DOWN_FILE_ALL_CAN_REC_SOCKET_REPORT_DOWN_FILE_STRUCT down_all_can1_loop_rec_socket_report; // CAN1接收循环整帧报文
DOWN_FILE_ALL_APP_BYTE_SEND_SOCKET_STRUCT            down_all_app_loop_send_socket_can2;   // CAN2网应用发送队列
DOWN_FILE_ALL_CAN_REC_SOCKET_REPORT_DOWN_FILE_STRUCT down_all_can2_loop_rec_socket_report; // CAN2接收循环整帧报文
DOWN_FILE_STATE_MONITOR_CAN_INF_STRUCT               down_file_state_monitor_can_inf;

DOWN_FILE_ALL_CAN_RECEIVE_QUEUE_STRUCT down_file_all_can1_loop_receive_queue; // CAN1接收帧处理队列
DOWN_FILE_ALL_CAN_SEND_QUEUE_STRUCT    down_file_all_can1_int_send_queue;
DOWN_FILE_ALL_CAN_RECEIVE_QUEUE_STRUCT down_file_all_can2_loop_receive_queue; // CAN2接收帧处理队列
DOWN_FILE_ALL_CAN_SEND_QUEUE_STRUCT    down_file_all_can2_int_send_queue;

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */


static bool down_file_is_valid_response_source(uint32_t can_perip, uint8_t can_src_id);
static bool down_file_push_rec_queue_int(each_can_report_t *p_can_Info_src,
                                         DOWN_FILE_ALL_CAN_RECEIVE_QUEUE_STRUCT *p_receive_queue);
static int8_t down_file_app_rec_socket_put_together_loop(
    uint32_t can_perip,
    DOWN_FILE_ALL_CAN_RECEIVE_QUEUE_STRUCT *p_receive_queue,
    DOWN_FILE_ALL_CAN_REC_SOCKET_REPORT_DOWN_FILE_STRUCT *p_socket_report);
static int8_t down_file_app_handle_loop_rec_can_socket(
    uint32_t can_perip,
    DOWN_FILE_ALL_CAN_REC_SOCKET_REPORT_DOWN_FILE_STRUCT *p_socket_report);
static void down_file_can_send_soft_int_task(uint32_t can_perip,
                                             DOWN_FILE_ALL_APP_BYTE_SEND_SOCKET_STRUCT *p_send_socket,
                                             DOWN_FILE_ALL_CAN_SEND_QUEUE_STRUCT *p_send_queue);
static int8_t down_file_input_can_int_queue_from_app_socket(
    DOWN_FILE_ALL_APP_BYTE_SEND_SOCKET_STRUCT *p_send_socket,
    DOWN_FILE_ALL_CAN_SEND_QUEUE_STRUCT *p_send_queue);


/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 初始化一键下载CAN1和CAN2转发参数。
 */


void init_down_file_app_can1_param_prog(void)
{
    memset((char *)&down_file_all_can1_loop_receive_queue, 0, sizeof(down_file_all_can1_loop_receive_queue));
    memset((char *)&down_file_all_can1_int_send_queue, 0, sizeof(down_file_all_can1_int_send_queue));
    memset((char *)&down_file_all_can2_loop_receive_queue, 0, sizeof(down_file_all_can2_loop_receive_queue));
    memset((char *)&down_file_all_can2_int_send_queue, 0, sizeof(down_file_all_can2_int_send_queue));
    memset((char *)&down_file_state_monitor_can_inf, 0, sizeof(down_file_state_monitor_can_inf));
    memset((char *)&down_all_can1_loop_rec_socket_report, 0, sizeof(down_all_can1_loop_rec_socket_report));
    memset((char *)&down_all_can2_loop_rec_socket_report, 0, sizeof(down_all_can2_loop_rec_socket_report));
    memset((char *)&down_all_app_loop_send_socket_can1, 0, sizeof(down_all_app_loop_send_socket_can1));
    memset((char *)&down_all_app_loop_send_socket_can2, 0, sizeof(down_all_app_loop_send_socket_can2));
}

/**
 * @brief 将一键下载接收报文压入指定CAN处理队列。
 * @param p_can_Info_src CAN网原始报文
 * @param p_receive_queue CAN接收处理队列
 * @return true表示已接管处理，false表示非一键下载报文
 */
static bool down_file_push_rec_queue_int(each_can_report_t *p_can_Info_src,
                                         DOWN_FILE_ALL_CAN_RECEIVE_QUEUE_STRUCT *p_receive_queue)
{
    DOWN_FILE_CAN_EXTENDER_ID_STRUCT *p_can_extender_ID_src;
    each_can_report_t *               p_each_can_report_dest;
    uint32_t                          loop_i;
    uint32_t                          next_wr_p;

    if ((p_can_Info_src == NULL) || (p_receive_queue == NULL))
    {
        return false;
    }
    p_can_extender_ID_src = (DOWN_FILE_CAN_EXTENDER_ID_STRUCT *)&p_can_Info_src->can_efid;
    // 只接管一键下载上行命令
    if (p_can_extender_ID_src->order_code != DOWN_FILE_CAN_CODE_LOOP_UPGRADE_CMD) // 不是上行命令
    {
        return false;
    }
    next_wr_p = (p_receive_queue->can_queue_wr_p + 1) & DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER;
    if (next_wr_p == p_receive_queue->can_queue_loop_rd_p)
    {
        down_file_state_monitor_can_inf.rec_socket_err_number++;
        down_file_state_monitor_can_inf.rec_err_locate = 9;
        return true;
    }
    p_receive_queue->can_queue_wr_p &= DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER;
    p_each_can_report_dest = &p_receive_queue->can_rec_queue_buff[p_receive_queue->can_queue_wr_p];

    p_each_can_report_dest->can_efid = p_can_Info_src->can_efid;
    p_each_can_report_dest->can_dlen = p_can_Info_src->can_dlen;
    for (loop_i = 0; loop_i < 8; loop_i++)
    {
        p_each_can_report_dest->can_data[loop_i] = p_can_Info_src->can_data[loop_i];
    }

    p_receive_queue->can_queue_wr_p = next_wr_p;

    return true;
}

/**
 * @brief 将一键下载CAN1接收报文压入处理队列。
 * @param p_can_Info_src CAN网原始报文
 * @return true表示已接管处理，false表示非一键下载报文
 */
bool down_file_push_rec_can1_queue_int(each_can_report_t *p_can_Info_src)
{
    return down_file_push_rec_queue_int(p_can_Info_src, &down_file_all_can1_loop_receive_queue);
}

/**
 * @brief 将一键下载CAN2接收报文压入面板升级处理队列。
 * @param p_can_Info_src CAN网原始报文
 * @return true表示已接管处理，false表示非一键下载报文
 */
bool down_file_push_rec_can2_queue_int(each_can_report_t *p_can_Info_src)
{
    return down_file_push_rec_queue_int(p_can_Info_src, &down_file_all_can2_loop_receive_queue);
}

/**
 * @brief 校验升级响应源地址是否与CAN通道匹配。
 * @param can_perip CAN通道序号
 * @param can_src_id CAN源地址
 * @return true表示地址与通道匹配，false表示不匹配
 */
static bool down_file_is_valid_response_source(uint32_t can_perip, uint8_t can_src_id)
{
    if (can_perip == CAN2_APP_ID)
    {
        return (can_src_id == DOWN_FILE_PANEL_CAN_ID);
    }

    return ((can_perip == CAN1_APP_ID) && (can_src_id < DOWN_FILE_CPU_2_DIO_CAN_GROUP_ID));
}

/**
 * @brief 将指定CAN通道的短帧拼接为一键下载完整帧。
 * @param can_perip CAN通道序号
 * @param p_receive_queue CAN接收处理队列
 * @param p_socket_report CAN完整帧组包上下文
 * @return true表示收到完整帧，false表示尚未完成
 */
static int8_t down_file_app_rec_socket_put_together_loop(
    uint32_t can_perip,
    DOWN_FILE_ALL_CAN_RECEIVE_QUEUE_STRUCT *p_receive_queue,
    DOWN_FILE_ALL_CAN_REC_SOCKET_REPORT_DOWN_FILE_STRUCT *p_socket_report)
{
    each_can_report_t *               p_each_can_report;
    DOWN_FILE_CAN_EXTENDER_ID_STRUCT *p_can_extender_ID;
    uint32_t                          loop_i;
    uint32_t                          loop_j;

    for (loop_i = 0; loop_i < DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER; loop_i++)
    {
        if (p_receive_queue->can_queue_wr_p == p_receive_queue->can_queue_loop_rd_p)
        {
            break;
        }
        p_each_can_report = &p_receive_queue->can_rec_queue_buff[p_receive_queue->can_queue_loop_rd_p];

        p_can_extender_ID = (DOWN_FILE_CAN_EXTENDER_ID_STRUCT *)&p_each_can_report->can_efid;
        if (!down_file_is_valid_response_source(can_perip, p_can_extender_ID->Src_can_addr))
        {
            // 源地址与接收通道不匹配
            p_receive_queue->can_queue_loop_rd_p++;
            p_receive_queue->can_queue_loop_rd_p &= DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER;
            down_file_state_monitor_can_inf.rec_socket_err_number++;
            down_file_state_monitor_can_inf.rec_err_locate = 10;
            continue;
        }

        if (p_can_extender_ID->socket_ID == 0)
        {
            // 如为第0帧
            p_socket_report->rec_report_len       = 0;
            p_socket_report->expect_rec_socket_ID = 1;
            p_socket_report->Src_can_addr         = p_can_extender_ID->Src_can_addr;
        }
        else
        {
            // 其它帧
            if (p_socket_report->Src_can_addr != p_can_extender_ID->Src_can_addr)
            {
                // 源地址不正确
                p_receive_queue->can_queue_loop_rd_p++;
                p_receive_queue->can_queue_loop_rd_p &= DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER;
                down_file_state_monitor_can_inf.rec_socket_err_number++;
                down_file_state_monitor_can_inf.rec_err_locate = 11;
                continue;
            }

            if (p_socket_report->expect_rec_socket_ID != p_can_extender_ID->socket_ID)
            {
                // SOCKET_ID 不连续
                p_receive_queue->can_queue_loop_rd_p++;
                p_receive_queue->can_queue_loop_rd_p &= DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER;
                down_file_state_monitor_can_inf.rec_socket_err_number++;
                down_file_state_monitor_can_inf.rec_err_locate = 12;
                continue;
            }
            p_socket_report->expect_rec_socket_ID++;
        }
        if (p_each_can_report->can_dlen > 8)
        {
            p_receive_queue->can_queue_loop_rd_p++;
            p_receive_queue->can_queue_loop_rd_p &= DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER;
            down_file_state_monitor_can_inf.rec_socket_err_number++;
            down_file_state_monitor_can_inf.rec_err_locate = 13;
            continue;
        }
        if (p_socket_report->rec_report_len >= DOWN_FILE_MAX_CAN_SOCKET_LENGTH)
        {
            // 超出长度
            p_socket_report->rec_report_len = 0;
            down_file_state_monitor_can_inf.rec_socket_err_number++;
            down_file_state_monitor_can_inf.rec_err_locate = 14;
            p_receive_queue->can_queue_loop_rd_p++;
            p_receive_queue->can_queue_loop_rd_p &= DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER;
            continue;
        }

        for (loop_j = 0; loop_j < p_each_can_report->can_dlen; loop_j++)
        {
            if (p_socket_report->rec_report_len >= DOWN_FILE_MAX_CAN_SOCKET_LENGTH)
            {
                p_socket_report->rec_report_len = 0;
                down_file_state_monitor_can_inf.rec_socket_err_number++;
                down_file_state_monitor_can_inf.rec_err_locate = 15;
                break;
            }
            p_socket_report->can_buff[p_socket_report->rec_report_len] = p_each_can_report->can_data[loop_j];
            p_socket_report->rec_report_len++;
        }

        p_receive_queue->can_queue_loop_rd_p++;
        p_receive_queue->can_queue_loop_rd_p &= DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER;
        if (p_can_extender_ID->continue_flag == 1) // 该帧为末尾帧
        {
            int8_t handle_ret;

            p_socket_report->can_extender_ID = p_each_can_report->can_efid;
            handle_ret = down_file_app_handle_loop_rec_can_socket(can_perip, p_socket_report);
            p_socket_report->rec_report_len = 0;
            return handle_ret;
        }
    }

    return false;
}

/**
 * @brief 将指定CAN通道的完整响应封装回TFTP发送通道。
 * @param can_perip CAN通道序号
 * @param p_socket_report CAN完整帧组包上下文
 * @return true表示处理成功，false表示报文无效
 */
static int8_t down_file_app_handle_loop_rec_can_socket(
    uint32_t can_perip,
    DOWN_FILE_ALL_CAN_REC_SOCKET_REPORT_DOWN_FILE_STRUCT *p_socket_report)
{
    DOWN_FILE_CAN_EXTENDER_ID_STRUCT *p_can_extender_ID;
    uint8_t                           can_src_ID;
    uint16_t                          opcode = 0;

    // 解析CAN扩展ID
    p_can_extender_ID = (DOWN_FILE_CAN_EXTENDER_ID_STRUCT *)&p_socket_report->can_extender_ID;
    can_src_ID        = p_can_extender_ID->Src_can_addr;

    // 仅接收与升级通道匹配的板卡响应
    if (!down_file_is_valid_response_source(can_perip, can_src_ID))
    {
        return false;
    }

    // 一键下载响应转发

    if (p_can_extender_ID->order_code != DOWN_FILE_CAN_CODE_LOOP_UPGRADE_CMD) // 不是一键下载上行命令，则退出
    {
        return false;
    }

    tftp_can_send_packet(p_socket_report->can_buff, p_socket_report->rec_report_len);

    /*memcpy(&opcode, &down_all_can1_loop_rec_socket_report.can_buff[4], 2);
    opcode = htons(opcode);
    if (opcode == OP_TFTP_ACK)
    {
        tftp_can_send_ack(down_all_can1_loop_rec_socket_report.can_buff, down_all_can1_loop_rec_socket_report.rec_report_len);
    }
    else if (opcode == OP_TFTP_OK)
    {
        tftp_can_send_ok(down_all_can1_loop_rec_socket_report.can_buff, down_all_can1_loop_rec_socket_report.rec_report_len);
    }
    else if (opcode == OP_TFTP_ERROR)
    {
        tftp_can_send_error(down_all_can1_loop_rec_socket_report.can_buff, down_all_can1_loop_rec_socket_report.rec_report_len);
    }
    else if (opcode == OP_TFTP_ROK)
    {
        tftp_can_send_rok(down_all_can1_loop_rec_socket_report.can_buff, down_all_can1_loop_rec_socket_report.rec_report_len);
    }
    else
    {
        return false;
    }*/

    (void)opcode;
    return true;
}

/**
 * @brief 将以太网升级载荷写入CAN发送socket队列。
 * @param pRawData 以太网TFTP载荷
 * @param dst_can_id 目标CAN ID
 * @param send_len 载荷长度
 * @return 0表示写入成功
 */
uint8_t io_transfer_upgrade_data(uint8_t *pRawData, uint8_t dst_can_id, uint32_t send_len)
{
    DOWN_FILE_EACH_APP_256BYTE_SEND_BUFF_STRUCT *p_down_file_each_app_256byte_send_buff_dest;
    DOWN_FILE_CAN_EXTENDER_ID_STRUCT *           p_can_externder_ID;
    DOWN_FILE_ALL_APP_BYTE_SEND_SOCKET_STRUCT *  p_send_socket;
    uint32_t                                     next_wr_p;

    if ((pRawData == NULL) || (send_len > DOWN_FILE_MAX_CAN_SOCKET_LENGTH))
    {
        down_file_state_monitor_can_inf.send_socket_err_number++;
        down_file_state_monitor_can_inf.send_err_locate = 20;
        return 1;
    }

    // 面板升级固定走CAN2，其余IO板卡保持走CAN1。
    p_send_socket = (dst_can_id == DOWN_FILE_PANEL_CAN_ID) ? &down_all_app_loop_send_socket_can2
                                                           : &down_all_app_loop_send_socket_can1;

    next_wr_p = (p_send_socket->wr_p + 1) & DOWN_FILE_MAX_APP_BYTE_CAN_SOCKET_NUMBER;
    if (next_wr_p == p_send_socket->rd_p)
    {
        down_file_state_monitor_can_inf.send_socket_err_number++;
        down_file_state_monitor_can_inf.send_err_locate = 21;
        return 1;
    }

    p_down_file_each_app_256byte_send_buff_dest = &p_send_socket->app_send_can_socket[p_send_socket->wr_p];

    p_can_externder_ID = (DOWN_FILE_CAN_EXTENDER_ID_STRUCT *)&p_down_file_each_app_256byte_send_buff_dest->
        can_extender_ID;

    p_can_externder_ID->order_code    = DOWN_FILE_CAN_CODE_LOOP_DOWNRADE_CMD;
    p_can_externder_ID->Src_can_addr  = DOWN_FILE_MONITOR_TOOLS_CAN_ID;
    p_can_externder_ID->Dest_can_addr = dst_can_id;

    p_down_file_each_app_256byte_send_buff_dest->send_len = send_len;
    memcpy(&p_down_file_each_app_256byte_send_buff_dest->send_buff[0], pRawData, send_len);

    p_send_socket->wr_p = next_wr_p;

    return 0;
}

/**
 * @brief 将指定CAN通道的长帧拆分为短帧并提交到底层发送。
 * @param can_perip CAN通道序号
 * @param p_send_socket 应用层发送socket队列
 * @param p_send_queue CAN短帧发送队列
 */
static void down_file_can_send_soft_int_task(uint32_t can_perip,
                                             DOWN_FILE_ALL_APP_BYTE_SEND_SOCKET_STRUCT *p_send_socket,
                                             DOWN_FILE_ALL_CAN_SEND_QUEUE_STRUCT *p_send_queue)
{
    each_can_report_t *p_can_report;
    uint8_t            send_flag;

    // 指定CAN通道发送任务
    send_flag = false;
    if (p_send_queue->can_queue_wr_p == p_send_queue->can_queue_rd_p)
    {
        // 判断队列是否空,如队列没有空，则不从应用取报文填队列
        if (down_file_input_can_int_queue_from_app_socket(p_send_socket, p_send_queue) == true)
        {
            // 应用层SOCKET有等待发送的报文
            send_flag = true;
        }
    }
    else
    {
        send_flag = true;
    }

    if (send_flag == true)
    {
        // 队列有发送任务
        p_can_report = &p_send_queue->can_send_queue[p_send_queue->can_queue_rd_p];
        if (Can_send_app(can_perip, p_can_report) == true)
        {
            // 发送成功
            p_send_queue->can_queue_rd_p++;
            p_send_queue->can_queue_rd_p &= DOWN_FILE_MAX_CAN_SEND_QUEUE_NUMBER;
        }
    }
}

/**
 * @brief 将IO板卡升级长帧拆分后通过CAN1提交到底层发送。
 */
void down_file_Can1_send_soft_int_task(void)
{
    down_file_can_send_soft_int_task(CAN1_APP_ID,
                                     &down_all_app_loop_send_socket_can1,
                                     &down_file_all_can1_int_send_queue);
}

/**
 * @brief 将面板升级长帧拆分后通过CAN2提交到底层发送。
 */
void down_file_Can2_send_soft_int_task(void)
{
    down_file_can_send_soft_int_task(CAN2_APP_ID,
                                     &down_all_app_loop_send_socket_can2,
                                     &down_file_all_can2_int_send_queue);
}

/**
 * @brief 将应用层socket缓存填入指定CAN发送队列。
 * @param p_send_socket 应用层发送socket队列
 * @param p_send_queue CAN短帧发送队列
 * @return true表示填入成功，false表示无待发送数据
 */
static int8_t down_file_input_can_int_queue_from_app_socket(
    DOWN_FILE_ALL_APP_BYTE_SEND_SOCKET_STRUCT *p_send_socket,
    DOWN_FILE_ALL_CAN_SEND_QUEUE_STRUCT *p_send_queue)
{
    DOWN_FILE_EACH_APP_256BYTE_SEND_BUFF_STRUCT *p_each_app_loop_send_buff_src;
    DOWN_FILE_CAN_EXTENDER_ID_STRUCT *           p_can_extender_ID;
    uint32_t                                     loop_k;
    each_can_report_t *                          p_each_can_send_queue;
    uint32_t                                     can_send_len;
    uint32_t                                     over_flag;
    uint32_t                                     loop_i;

    if (p_send_socket->rd_p == p_send_socket->wr_p)
    {
        // SOCKET 发送缓存区空
        return false;
    }

    p_each_app_loop_send_buff_src = &p_send_socket->app_send_can_socket[p_send_socket->rd_p];

    // 填写CAN队列发送缓存区

    p_send_queue->can_queue_wr_p = 0;
    p_send_queue->can_queue_rd_p = 0;

    for (loop_k = 0; loop_k < DOWN_FILE_MAX_CAN_SEND_QUEUE_NUMBER + 1; loop_k++)
    {
        p_each_can_send_queue = &p_send_queue->can_send_queue[p_send_queue->can_queue_wr_p];
        can_send_len = 0;
        over_flag    = 0;
        for (loop_i = 0; loop_i < 8; loop_i++)
        {
            p_each_can_send_queue->can_data[loop_i] = p_each_app_loop_send_buff_src->send_buff[loop_k * 8 + loop_i];
            can_send_len++;
            if ((loop_k * 8 + loop_i + 1) >= p_each_app_loop_send_buff_src->send_len)
            {
                over_flag = true;
                break;
            }
        }
        p_each_can_send_queue->can_dlen = can_send_len; // Can网发送长度
        p_each_can_send_queue->can_efid = p_each_app_loop_send_buff_src->can_extender_ID;
        p_can_extender_ID               = (DOWN_FILE_CAN_EXTENDER_ID_STRUCT *)&p_each_can_send_queue->can_efid;
        p_can_extender_ID->socket_ID    = loop_k; // 帧序号

        if (over_flag == true)
        {
            // 发送结束
            p_can_extender_ID->continue_flag = 0x01; // 续帧标志
        }
        else
        {
            p_can_extender_ID->continue_flag = 0;
        }
        p_send_queue->can_queue_wr_p++;
        p_send_queue->can_queue_wr_p &= DOWN_FILE_MAX_CAN_SEND_QUEUE_NUMBER;
        if (over_flag == true)
        {
            // 发送结束
            break;
        }
    }
    p_send_socket->rd_p++;
    p_send_socket->rd_p &= DOWN_FILE_MAX_APP_BYTE_CAN_SOCKET_NUMBER;
    return true;
}

/**
 * @brief 将CAN1短帧拼接为IO板卡升级完整帧。
 * @return true表示收到完整帧，false表示尚未完成
 */
int8_t down_file_app_rec_can1_socket_put_together_loop(void)
{
    return down_file_app_rec_socket_put_together_loop(CAN1_APP_ID,
                                                       &down_file_all_can1_loop_receive_queue,
                                                       &down_all_can1_loop_rec_socket_report);
}

/**
 * @brief 将CAN2短帧拼接为面板升级完整帧。
 * @return true表示收到完整帧，false表示尚未完成
 */
int8_t down_file_app_rec_can2_socket_put_together_loop(void)
{
    return down_file_app_rec_socket_put_together_loop(CAN2_APP_ID,
                                                       &down_file_all_can2_loop_receive_queue,
                                                       &down_all_can2_loop_rec_socket_report);
}

#endif
