/******************************************************************************
 *@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
 *@file       tcpip.c
 *@author     xuesen
 *@date       2026.05.06
 *@brief      TCP/IP原始报文分发与协议栈初始化实现。
 *@par        History
 *Date        Version   Author     Description
 *2026.05.06  1.0       xuesen     Initial version
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "net_all_include.h"
#define __NET_C_
#include "tcpip.h"
#ifdef __cplusplus
extern "C" {

#endif

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

ipaddr_t g_EthIpAddr[INET_MAC_COUNT];
ipaddr_t g_EthMask[INET_MAC_COUNT];
ipaddr_t g_EthGateway[INET_MAC_COUNT];

const ipaddr_t MultiIPTbl[CFG_MULTIIP_NUM] =  // CFG_MULTIIP_NUM=3
    {CFG_MANAGE_MULTIIP, all_zeroes_ipaddr};

net_protocol_debug_t net_protocol_debug = {0};

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 处理接收到的以太网原始报文。
 * @param pInfo 原始报文信息
 * @return RTN_OK表示处理成功，RTN_ERR表示处理失败
 */

bool tcpip_process(NET_RAW_PKG_INFO *pInfo)
{
    bool result = false;

    if ((pInfo == NULL) || (pInfo->pRawPkg == NULL))
    {
        NET_PROTOCOL_DEBUG_LOG("[NET][DROP] stage=input info_null=%u raw_null=%u\r\n",
                               (pInfo == NULL) ? 1U : 0U,
                               ((pInfo == NULL) || (pInfo->pRawPkg == NULL)) ? 1U : 0U);
        return false;
    }

    // 由平台适配层根据报文来源选择当前本地网口地址
    if (!net_port_get_current_enet_info(pInfo))
    {
        // 当前端口地址无效时丢弃报文，避免沿用上一次查询结果
        net_protocol_debug.err_location = 0x01;
        net_protocol_debug.err_number++;
        NET_PROTOCOL_DEBUG_LOG("[NET][DROP] stage=local_addr len=%u type=0x%04X slot=%u port=%u port_type=0x%02X\r\n",
                               pInfo->nLength,
                               pInfo->nEtherType,
                               pInfo->Src_Slot_ID,
                               pInfo->Src_Port_ID,
                               pInfo->PortType);
        return false;
    }

    // 仅组网平台加载独立组网地址，单网口平台不得构造组网地址别名
    if (net_profile_enable_networking() && !net_port_get_networking_enet_info(pInfo))
    {
        // 组网地址查询失败时不继续处理，防止使用残留地址
        net_protocol_debug.err_location = 0x02;
        net_protocol_debug.err_number++;
        NET_PROTOCOL_DEBUG_LOG("[NET][DROP] stage=networking_addr len=%u type=0x%04X slot=%u port=%u port_type=0x%02X\r\n",
                               pInfo->nLength,
                               pInfo->nEtherType,
                               pInfo->Src_Slot_ID,
                               pInfo->Src_Port_ID,
                               pInfo->PortType);
        return false;
    }
    switch (pInfo->nEtherType)
    {
    case CFG_ETHTYPE_IP:
        if (pInfo->nLength >= NET_IP_HEAD_LEN)
        {
            result = (ip_recv(pInfo) == RTN_OK);
        }
        break;
    case CFG_ETHTYPE_ARP:
        if (pInfo->nLength >= 28)
        {
            result = (arp_recv(pInfo) == RTN_OK);
        }
        break;
    default:
        break;
    }
    return result;
}

/**
 * @brief 初始化网络协议栈。
 */
void protocol_stack_init(void)
{
    memset(&udp_recv_que, 0, sizeof(udp_recv_que));
    ip_reass_init();
    inet_init();
}
