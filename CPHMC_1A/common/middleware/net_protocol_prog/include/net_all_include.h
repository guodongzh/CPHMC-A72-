/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_all_include.h
*@author     xuesen
*@date       2026.05.06
*@brief      网络协议模块公共头文件汇总。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

#ifndef __NET_ALL_INCLUDE_H__
#define __NET_ALL_INCLUDE_H__ 1


/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#if defined(NET_PORT_C6X0)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#if defined(NET_PORT_AM64X)
#include "can_ipc.h"
#endif

#include "net_other.h"
#include "net_port.h"
#include "net_profile.h"
#include "tcpip.h"
#include "netlib.h"
#include "net_arp.h"
#include "net_ip.h"
#include "net_tftp.h"
#include "net_private.h"
#include "net_tftp_err.h"
#include "crc16.h"

#if NET_PROFILE_ENABLE_CAN_FORWARD
#include "down_file_can_interface.h"
#endif


/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

#if defined(NET_PORT_C6X0)
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#endif

#ifndef NET_PROTOCOL_DEBUG_LOG_ENABLE
#define NET_PROTOCOL_DEBUG_LOG_ENABLE    (0U) // 协议栈调试打印默认关闭
#endif

#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
// 调试输出由平台适配层实现，协议代码不直接依赖底层日志组件
#define NET_PROTOCOL_DEBUG_LOG(...)      net_port_debug_log(__VA_ARGS__)
#else
#define NET_PROTOCOL_DEBUG_LOG(...)      ((void)0)
#endif

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

#endif
