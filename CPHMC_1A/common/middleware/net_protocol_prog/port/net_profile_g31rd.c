/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_profile_g31rd.c
*@author     xuesen
*@date       2026.07.08
*@brief      G31RD 装置网络策略实现。
*@par        History
*Date        Version   Author     Description
*2026.07.08  1.0       xuesen     Initial version
******************************************************************************/

#include "net_all_include.h"

#if defined(NET_PROFILE_G31RD_BUILD) || (!defined(NET_PROFILE_C6X0_BUILD) && \
                                        !defined(NET_PROFILE_AM64X_BUILD) && \
                                         !defined(NET_PROFILE_J721E_BUILD))

/**
 * @brief 查询G31RD装置是否启用组网功能。
 * @return 固定返回false
 */
bool net_profile_enable_networking(void)
{
    return false;
}

/**
 * @brief 查询G31RD装置是否启用组网TFTP通道。
 * @return 固定返回false
 */
bool net_profile_enable_networking_tftp(void)
{
    return false;
}

/**
 * @brief 查询G31RD装置是否允许通过CAN远程升级。
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
 * @brief 获取G31RD装置使用的主控CAN ID。
 * @return 固定返回0x41
 */
uint8_t net_profile_calc_master_canid(void)
{
    return 0x41;
}

/**
 * @brief 查询G31RD构建是否启用CAN升级转发。
 * @return 固定返回false
 */
bool net_profile_enable_can_forward(void)
{
    return false;
}

/**
 * @brief 查询G31RD装置是否允许R5F1核心使用CAN通道。
 * @return 固定返回false
 */
bool net_profile_enable_core_r5f1_can(void)
{
    return false;
}

#endif
