/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_profile.h
*@author     xuesen
*@date       2026.07.08
*@brief      网络协议栈装置策略接口定义。
*@par        History
*Date        Version   Author     Description
*2026.07.08  1.0       xuesen     Initial version
******************************************************************************/

#ifndef __NET_PROFILE_H__
#define __NET_PROFILE_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

#include "net_other.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

#ifndef NET_PROFILE_ENABLE_CAN_FORWARD
// CAN升级转发仅由AM64x的Core0承担，其他平台仅保留CAN ID计算能力
#if defined(NET_PROFILE_AM64X_BUILD) && defined(SOC_AM64X) && defined(CORE_R5F0)
#define NET_PROFILE_ENABLE_CAN_FORWARD 1
#else
#define NET_PROFILE_ENABLE_CAN_FORWARD 0
#endif
#endif

#ifndef NET_PROFILE_ENABLE_TFTP_CAN_PROXY
// TFTP CAN升级代理仅由AM64x的Core0承担
#if defined(NET_PROFILE_AM64X_BUILD) && defined(SOC_AM64X) && defined(CORE_R5F0)
#define NET_PROFILE_ENABLE_TFTP_CAN_PROXY 1
#else
#define NET_PROFILE_ENABLE_TFTP_CAN_PROXY 0
#endif
#endif

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

typedef enum
{
    NET_PROFILE_G31RD = 0,
    NET_PROFILE_C6X0,
    NET_PROFILE_AM64X,
    NET_PROFILE_J721E,
} net_profile_id_t;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

/**
 * @brief 查询当前平台是否提供独立组网地址和发送通道。
 * @return true表示启用组网功能，false表示仅使用本机地址
 */
bool net_profile_enable_networking(void);

/**
 * @brief 查询当前装置是否启用组网TFTP通道。
 * @return true表示启用，false表示禁用
 */
bool net_profile_enable_networking_tftp(void);

/**
 * @brief 查询当前装置是否允许通过CAN远程升级。
 * @return true表示允许，false表示禁止
 */
bool net_profile_enable_remote_can_upgrade(void);

/**
 * @brief 判断当前核心是否接收组网地址的ARP和ICMP报文。
 * @return true表示接收，false表示丢弃
 */
bool net_profile_should_accept_net_ip_icmp(void);

/**
 * @brief 计算当前装置及核心对应的主控CAN ID。
 * @return 主控CAN ID
 */
uint8_t net_profile_calc_master_canid(void);

/**
 * @brief 查询当前构建是否启用CAN数据转发。
 * @return true表示启用，false表示禁用
 */
bool net_profile_enable_can_forward(void);

/**
 * @brief 查询当前装置是否允许R5F1核心使用CAN通道。
 * @return true表示允许，false表示禁止
 */
bool net_profile_enable_core_r5f1_can(void);

#ifdef __cplusplus
}
#endif

#endif /* __NET_PROFILE_H__ */
