/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_sock.c
*@author     xuesen
*@date       2026.05.06
*@brief      网络socket管理接口实现。
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


static uint16_t m_nSocketCount;
static socket_t m_NetSock[CFG_NET_SOCKS];


/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 初始化socket表。
 */


void socket_init(void)
{
    m_nSocketCount = 0;
    memset(m_NetSock, 0, sizeof(m_NetSock));
    return;
}

/**
 * @brief 获取已创建socket数量。
 * @return socket数量
 */
uint16_t socket_count(void)
{
    return m_nSocketCount;
}

/**
 * @brief 创建socket并设置发送参数。
 * @param pTxFifo 发送FIFO句柄
 * @param nSendFlag 发送标志
 * @return socket句柄，-1表示创建失败
 */
sockfd_t sock_create_ex(acp_buf_t *pTxFifo, uint16_t nSendFlag)
{
    socket_t *psock;

    if (m_nSocketCount >= CFG_NET_SOCKS)
        return (-1);

    psock = &m_NetSock[m_nSocketCount];
    if (psock->flag != FALSE)
        return (-1);

    psock->sendflag   = nSendFlag;
    psock->ethno      = g_nInetMacCount + 1;
    psock->rmt_port   = 0;
    psock->lcl_port   = 4096 + m_nSocketCount;
    psock->rmt_ipaddr = 0;
    psock->lcl_ipaddr = 0;
    m_nSocketCount++;
    psock->flag = TRUE;
    return (sockfd_t)m_nSocketCount;

}

/**
 * @brief 创建默认发送标志的socket。
 * @param pTxFifo 发送FIFO句柄
 * @return socket句柄，-1表示创建失败
 */
sockfd_t sock_create(acp_buf_t *pTxFifo)
{
    return sock_create_ex(pTxFifo, 0);
}

/**
 * @brief 将socket绑定到本地IP地址和端口。
 * @param s socket句柄
 * @param plocaladdr 本地IP地址和端口
 * @return RTN_OK表示成功，RTN_ERR表示失败
 */
int32_t sock_bind(sockfd_t s, const sockaddr_t *plocaladdr)
{
    socket_t *psock;
    uint16_t  ethno;
    uint32_t  nIndex;

    if ((plocaladdr == NULL) || (s > ARRAYSIZEOF(m_NetSock)) || (s <= 0))
    {
        return RTN_ERR;
    }

    psock = &m_NetSock[s - 1];
    if (TRUE != psock->flag)
    {
        return RTN_ERR;
    }

    ethno = g_nInetMacCount;
    for (nIndex = 0; nIndex < g_nInetMacCount; nIndex++)
    {
        if (plocaladdr->ipaddr == g_EthIpAddr[nIndex])
        {
            ethno = nIndex;
            break;
        }
    }
    if (ethno >= g_nInetMacCount)
        return RTN_ERR;

    return udp_bind(psock, plocaladdr, ethno);
}

/**
 * @brief 将socket绑定到指定逻辑网口。
 * @param s socket句柄
 * @param ethno 逻辑网口号
 * @return RTN_OK表示成功，RTN_ERR表示失败
 */
int32_t sock_bind_ethno(sockfd_t s, uint16_t ethno)
{
    socket_t * psock;
    sockaddr_t localaddr;

    if ((ethno >= g_nInetMacCount) || (s > ARRAYSIZEOF(m_NetSock)) || (s <= 0))
    {
        return RTN_ERR;
    }

    psock = &m_NetSock[s - 1];
    if (TRUE != psock->flag)
    {
        return RTN_ERR;
    }
    localaddr.ipaddr = g_EthIpAddr[ethno];
    localaddr.port   = psock->lcl_port;
    return udp_bind(psock, &localaddr, ethno);
}

/**
 * @brief 通过socket发送数据到目标地址。
 * @param s socket句柄
 * @param buf 待发送数据
 * @param len 发送字节数
 * @param to 目标地址，成帧时统一处理字节序
 * @return 发送结果
 */
int32_t sock_sendto(sockfd_t s, const bystrm *buf, uint32_t len, const sockaddr_t *to)
{
    socket_t *psock;
    udp_socket_t udp_socket;

    if ((buf == NULL) || (to == NULL) || (s > ARRAYSIZEOF(m_NetSock)) || (s <= 0))
    {
        return RTN_ERR;
    }

    psock = &m_NetSock[s - 1];
    if (TRUE != psock->flag)
    {
        return RTN_ERR;
    }
    memset(&udp_socket, 0, sizeof(udp_socket));
    udp_socket.lcl_port   = psock->lcl_port;
    udp_socket.rmt_port   = to->port;
    // udp_socket.lcl_ipaddr = psock->lcl_ipaddr;
    udp_socket.rmt_ipaddr = to->ipaddr;
    return (int32_t)udp_send(buf, len, &udp_socket);
}

/**
 * @brief 按远端地址查找匹配的socket。
 * @param from 远端地址信息
 * @return 匹配的socket指针，未找到返回NULL
 */
socket_t *sock_ipin(const sockaddr_t *from)
{
    socket_t *psock;
    sockfd_t  s;

    //	aeos_assert(!(NULL == from));

    for (s = 0; s < ARRAYSIZEOF(m_NetSock); ++s)
    {
        psock = &m_NetSock[s];
        if (FALSE == psock->flag)
        {
            continue;
        }
        if (psock->rmt_port != from->port)
        {
            continue;
        }
        if (ipaddr_cmp(psock->rmt_ipaddr, all_zeroes_ipaddr))
        {
            return psock;
        }
        else if (ipaddr_cmp(psock->rmt_ipaddr, from->ipaddr))
        {
            return psock;
        }
        // 预留RxFifo匹配条件
    }
    return NULL;
}

#ifdef __cplusplus
}
#endif
