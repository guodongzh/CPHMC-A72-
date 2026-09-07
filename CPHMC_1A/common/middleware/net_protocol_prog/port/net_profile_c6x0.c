/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_profile_c6x0.c
*@author     xuesen
*@date       2026.07.08
*@brief      C6X0 装置网络策略实现。
*@par        History
*Date        Version   Author     Description
*2026.07.08  1.0       xuesen     Initial version
******************************************************************************/

#include "net_all_include.h"

#if defined(NET_PROFILE_C6X0_BUILD)

/**
 * @brief 查询C6X0装置是否启用组网功能。
 * @return 固定返回false
 */
bool net_profile_enable_networking(void)
{
    return false;
}

/**
 * @brief 查询C6X0装置是否启用组网TFTP通道。
 * @return 固定返回false
 */
bool net_profile_enable_networking_tftp(void)
{
    return false;
}

/**
 * @brief 查询C6X0装置是否允许通过CAN远程升级。
 * @return 固定返回false
 */
bool net_profile_enable_remote_can_upgrade(void)
{
    return false;
}

/**
 * @brief 判断当前核心是否接收组网地址的ARP和ICMP报文。
 * @return 固定返回false
 */
bool net_profile_should_accept_net_ip_icmp(void)
{
    return false;
}

/**
 * @brief 根据当前板号计算C6X0主控CAN ID。
 * @return 当前主控对应的CAN ID
 */
uint8_t net_profile_calc_master_canid(void)
{
    return (uint8_t)(0x40U + (net_port_get_board_no() & 0x2FU));
}

/**
 * @brief 查询C6X0构建是否启用CAN升级转发。
 * @return 固定返回false
 */
bool net_profile_enable_can_forward(void)
{
    return false;
}

/**
 * @brief 查询C6X0装置是否允许R5F1核心使用CAN升级通道。
 * @return 固定返回false
 */
bool net_profile_enable_core_r5f1_can(void)
{
    return false;
}

#endif
