/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       tcpipinit.c
*@author     xuesen
*@date       2026.05.06
*@brief      TCP/IP协议栈配置与初始化实现。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/

#include "net_all_include.h"

#ifdef __cplusplus
extern "C" {



#endif

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

extern callback_recvfrom_udp * m_pCallBackUdp;
extern callback_recvfrom_udp * m_pCallBackRpc;
extern callback_tcpip_process *m_pCallBackSntp;

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */


void tcpip_cfg_ex(uint8_t cabinet_id);


/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 初始化TCP/IP发送队列和协议表。
 * @param pTxFifo 发送FIFO句柄
 * @param pCallbackUdp UDP接收回调函数
 */
void tcpip_init(acp_buf_t *pTxFifo, callback_recvfrom_udp *pCallbackUdp)
{
    m_pCallBackUdp  = pCallbackUdp;
    m_pCallBackRpc  = NULL;
    m_pCallBackSntp = NULL;

    arp_init();    // arp
    socket_init(); // sock
}

/**
 * @brief 设置指定逻辑网口的IP参数。
 * @param nEthLogNo 逻辑网口号
 * @param ipaddr IP地址
 * @param mask 子网掩码
 * @param gateway 默认网关
 * @return true表示设置成功，false表示参数无效
 */
bool tcpip_set_ipaddr(uint32_t nEthLogNo, ipaddr_t ipaddr, ipaddr_t mask, ipaddr_t gateway)
{
    uint32_t nEthNo;

    if (g_nInetMacCount <= 0)
        return false;
    if (nEthLogNo >= g_nInetMacCount)
        return false;
    nEthNo               = nEthLogNo;
    g_EthIpAddr[nEthNo]  = ipaddr;
    g_EthMask[nEthNo]    = mask;
    g_EthGateway[nEthNo] = gateway;
    return true;
}

/**
 * @brief 获取指定逻辑网口的IP地址。
 * @param nEthLogNo 逻辑网口号
 * @return IP地址，网口无效时返回0
 */
ipaddr_t tcpip_get_ipaddr(uint32_t nEthLogNo)
{
    //	if ((nEthLogNo >= g_nInetMacCount) || (nEthLogNo < 0))
    if (nEthLogNo >= g_nInetMacCount)
        return 0;
    return g_EthIpAddr[nEthLogNo];
}

/**
 * @brief 使用默认机柜号配置网口IP参数。
 */
void tcpip_cfg(void)
{
    tcpip_cfg_ex(0);
}

/**
 * @brief 按机柜号配置网口IP参数。
 * @param cabinet_id 机柜号
 */
void tcpip_cfg_ex(uint8_t cabinet_id)
{
    uint32_t i;
    uint32_t ip_base_addr;
    uint32_t ip_addr_set[32];
    uint8_t  nEthPhyNo;
    uint8_t  nInDspPlus;

    /*
            if (g_nInetMacCount <= 0)	// 没有分配以太网口
                    return;

            ip_base_addr = CFG_IPBASE;

            switch (BdGetCoreTag())
            {
            case CORE_0_TAG:
                    nInDspPlus = 0;
                    break;
            case CORE_1_TAG:
                    nInDspPlus = 20;
                    break;
            case CORE_2_TAG:
                    nInDspPlus = 10;
                    break;
            case CORE_3_TAG:
                    nInDspPlus = 30;
                    break;
            default:
                    nInDspPlus = 100;
                    break;
            }

            if (BdGetCpuTag() == CPU_1_TAG)
                    nZynq1Plus = 40;            // 按扩展槽处理
            else
                    nZynq1Plus = 0;

            // IP规则
            // 内网
            ip_addr_set[0] = ip_base_addr + GetBoardNo() + nInDspPlus + nZynq1Plus;	 //共享内网, 管理网
            if (IsExShareSlot() && ((GetBoardNo() & 7) == 1)) //J2插槽（IP=201）及内网互联母差从机J2插槽(IP=209）的特殊处理
            {
                    NetSwitchToE3();
                    ip_addr_set[0] += 200;
            }
    */
    ip_base_addr   = CFG_IPBASE;
    ip_addr_set[0] = CFG_IPBASE;
    nInDspPlus     = 1;
    cabinet_id     = 2;

    // 外网
    if (cabinet_id == 0)
    {
        for (i = 1; i < 16; i++)
            ip_addr_set[i] = (((uint32_t)192 << 24) + ((uint32_t)(168 - 10 * (i - 1)) << 16) + ((uint32_t)130 << 8) + 44 + nInDspPlus);
    }
    else
    {
        for (i = 1; i < 16; i++)
            ip_addr_set[i] = (((uint32_t)192 << 24) + ((uint32_t)(168 - 10 * (i - 1)) << 16) + ((uint32_t)(100 + cabinet_id) << 8) + 44 + nInDspPlus);
    }

    ip_addr_set[16] = ip_base_addr + (((uint32_t)20) << 16) + GetBoardNo();      // 内网3,LVDS网 //192.198.*.*
    ip_addr_set[17] = ip_base_addr + (((uint32_t)20) << 16) + GetBoardNo() + 20; // 内网3,LVDS网 //192.198.*.*

    // 根据物理位置确定IP地址
    for (i = 0; i < g_nInetMacCount; i++)
    {
        //		nEthPhyNo = g_FpgaLog2Phy[i];
        nEthPhyNo = 0;
        if (nEthPhyNo >= FPGA_MAC_COUNT)
        {
            // 物理网口号越界
            return;
        }
        g_EthIpAddr[i]  = ip_addr_set[nEthPhyNo];
        g_EthMask[i]    = 0xffffff00;
        g_EthGateway[i] = (g_EthIpAddr[i] & g_EthMask[i]) + 254;
    }
}

#ifdef __cplusplus
}
#endif
