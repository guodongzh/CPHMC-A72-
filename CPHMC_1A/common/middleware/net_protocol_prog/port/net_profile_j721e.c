/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_profile_j721e.c
*@author     xuesen
*@date       2026.07.08
*@brief      J721E平台网络策略实现。
*@par        History
*Date        Version   Author     Description
*2026.07.08  1.0       xuesen     Initial version
******************************************************************************/

#include "net_all_include.h"

#if defined(NET_PROFILE_J721E_BUILD)

/**
 * @brief 查询J721E平台是否启用组网功能。
 * @return 固定返回true
 */
bool net_profile_enable_networking(void)
{
    return true;
}

/**
 * @brief 查询J721E平台是否启用组网TFTP通道。
 * @return 固定返回true
 */
bool net_profile_enable_networking_tftp(void)
{
    return true;
}

/**
 * @brief 查询J721E平台是否允许通过CAN远程升级。
 * @return 固定返回false
 */
bool net_profile_enable_remote_can_upgrade(void)
{
    return false;
}

/**
 * @brief 判断当前核心是否接收组网地址的ARP和ICMP报文。
 * @return true表示当前为主核心，false表示非主核心
 */
bool net_profile_should_accept_net_ip_icmp(void)
{
    return (net_port_get_current_core_id() == NET_PRIMARY_CORE_ID);
}

/**
 * @brief 根据当前槽位号和核号计算主控CAN ID。
 * @return 当前主控对应的CAN ID
 */
uint8_t net_profile_calc_master_canid(void)
{
    uint8_t slot_id = (uint8_t)net_port_get_current_slot_id();
    uint8_t core_id = (uint8_t)net_port_get_current_core_nr();
    uint8_t base_id = 0x41;
    uint8_t offset  = 0;

    switch (slot_id)
    {
    case 0:
        base_id = 0x41;
        break;
    case 2:
        base_id = 0x51;
        break;
    case 4:
        base_id = 0x61;
        break;
    default:
        base_id = 0x41;
        break;
    }

    switch (core_id)
    {
    case 0:
        offset = 0;
        break;
    case 1:
        offset = 1;
        break;
    case 3:
        offset = 2;
        break;
    case 4:
        offset = 3;
        break;
    default:
        offset = 0;
        break;
    }

    return base_id + offset;
}

/**
 * @brief 查询J721E构建是否启用CAN数据转发。
 * @return 固定返回false
 */
bool net_profile_enable_can_forward(void)
{
    return false;
}

/**
 * @brief 查询J721E平台是否允许R5F1核心使用CAN通道。
 * @return 固定返回false
 */
bool net_profile_enable_core_r5f1_can(void)
{
    return false;
}

#endif
