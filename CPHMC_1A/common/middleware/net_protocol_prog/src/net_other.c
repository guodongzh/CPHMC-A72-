/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_other.c
*@author     xuesen
*@date       2026.05.06
*@brief      网络协议模块公共状态与辅助函数实现。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/

#include "net_all_include.h"


/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

// 本网口信息
ethaddr_t self_SrcMacAddr; // 本网口MAC源地址
ipaddr_t  self_ipaddr;     // 本网口IP地址

ethaddr_t net_SrcMacAddr; // 组网 MAC 源地址
ipaddr_t  net_ipaddr;     // 组网 IP 地址

ethaddr_t rec_Remote_MacAddr; // 接收的远端MAC源地址
ipaddr_t  rec_Remote_ipaddr;  // 接收的远端IP地址

uint8_t self_canid; // 本地CAN地址
uint8_t dst_canid = 0;

uint32_t  g_nInetMacCount;
ethaddr_t g_EthAddr[INET_MAC_COUNT];
ethaddr_t g_SrcMacAddr[INET_MAC_COUNT];
static uint32_t gPingSm;
static uint32_t nPingPort;
static uint32_t gPingCnt, gPingOkCnt, gPingFailCnt;
static uint32_t gPingDstIp;
static uint32_t gPingTx, gPingRx;
static uint32_t gPingTmStart;

extern FILE_FAT_TABLE_STRUCT File_Fat_Table;

uint32_t sys_clock = 0;

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

uint32_t timer_count(void);
bool     ustep(uint32_t usec, uint32_t start);
void     icmp_echo_request(uint16_t nEthNo, const ipaddr_t *d_ipaddr, int len);


/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 从平台适配层初始化本机网络地址。
 */


void init_net_prog(void)
{
    get_self_Mac_IP(&self_SrcMacAddr, &self_ipaddr);
}

/**
 * @brief 获取平台初始化流程登记的本机MAC和IP地址。
 * @param p_self_mac 输出的本地MAC地址
 * @param p_self_IP 输出的本地IP地址
 */
void get_self_Mac_IP(ethaddr_t *p_self_mac, ipaddr_t *p_self_IP)
{
    if ((p_self_mac == NULL) || (p_self_IP == NULL))
    {
        return;
    }

    // 本机地址由平台初始化流程登记，协议层不再保存平台默认值
    (void)net_port_get_local_addr(p_self_mac, p_self_IP);
}

/**
 * @brief 获取当前板卡号。
 * @return 板卡号
 */
uint32_t GetBoardNo(void)
{
    return 1;
}

/**
 * @brief 处理收到的ICMP Echo Reply报文。
 * @param pInfo 原始报文信息
 * @param nEthNo 逻辑网口号
 */
void ping_recv(NET_RAW_PKG_INFO *pInfo, uint16_t nEthNo)
{
}

/**
 * @brief 执行Ping状态机周期处理。
 */
void ping_mainloop_task(void)
{
    ethaddr_t d_ethaddr;

    static uint32_t tm_cnt = 0;
    static uint32_t tm_1s  = 0;

    switch (gPingSm)
    {
    // 空闲
    case E_PING_IDLE:
        break;
    // 检查IP对应的MAC是否已知
    case E_PING_CHK_MAC:
        tm_1s = timer_count();
        gPingOkCnt   = 0;
        gPingFailCnt = 0;
        gPingRx      = 0;
        gPingTx      = 0;
        tm_cnt       = 0;
        // 检查IP对应的MAC地址是否存在
        if (!arp_check_ex(&d_ethaddr, &gPingDstIp, nPingPort))
        {
            arp_request(&gPingDstIp, nPingPort);
            arp_request(&gPingDstIp, nPingPort);
            gPingSm = E_PING_WAIT_ARP;
        }
        else
        {
            //				printk("\r\nPing %u.%u.%u.%u @ Port%u @ %u\r\n", 0xff&(gPingDstIp>>24),
            // 0xff&(gPingDstIp>>16),
            // 0xff&(gPingDstIp>>8), 0xff&(gPingDstIp>>0),
            // nPingPort, gPingCnt);
            gPingSm = E_PING_PING;
        }
        break;
    // 等待ARP应答
    case E_PING_WAIT_ARP:
        if (ustep(1000 * 1000, tm_1s))
        {
            tm_1s = timer_count();

            if (!arp_check_ex(&d_ethaddr, &gPingDstIp, nPingPort))
            {
                // 超过1秒钟没有收到ARP应答
                if (++tm_cnt >= 10)
                {
                    //						printk("Can't find %u.%u.%u.%u 's MAC\r\n",
                    // 0xff&(gPingDstIp>>24),
                    // 0xff&(gPingDstIp>>16), 0xff&(gPingDstIp>>8), 0xff&(gPingDstIp>>0));

                    // 结束这次的ICMP操作
                    gPingCnt = 0;
                    gPingSm  = E_PING_IDLE;
                }
            }
            else
            {
                gPingSm = E_PING_PING;
            }
        }
        break;
    // Ping 测试
    case E_PING_PING:
        if (ustep(1000 * 1000, tm_1s))
        {
            tm_1s = timer_count();

            if (gPingTx != gPingRx)
            {
                //					printk("Ping Time Out\r\n");
                gPingRx = gPingTx;
                gPingFailCnt++;
            }
            gPingTmStart = timer_count();
            // 发送请求
            icmp_echo_request(nPingPort, (const ipaddr_t *)&gPingDstIp, 64);
            gPingTx++;
            gPingCnt--;
            if (gPingCnt == 0)
            {
                gPingSm = E_PING_END;
            }
        }
        break;
    // 完毕
    case E_PING_END:
        if (ustep(1000 * 1000, tm_1s))
        {
            tm_1s = timer_count();
            if (gPingTx != gPingRx)
            {
                //					printk("Ping Time Out\r\n");
                gPingRx = gPingTx;
                gPingFailCnt++;
            }
            //				printk("\r\nPing Send %u, Recv %u, Lost %u\r\n", gPingTx, gPingOkCnt,
            // gPingFailCnt);
            gPingSm = E_PING_IDLE;
        }
        break;
    default:
        gPingSm = E_PING_IDLE;
        break;
    }
}

/**
 * @brief 获取系统时钟计数。
 * @return 当前系统时钟计数
 */
uint32_t timer_count(void)
{
    return sys_clock;
}

/**
 * @brief 判断指定微秒周期是否到期。
 * @param usec 延时时间，单位为微秒
 * @param start 起始时钟计数
 * @return true表示已到期，false表示未到期
 */
bool ustep(uint32_t usec, uint32_t start)
{
    uint32_t tm_period;

    tm_period = usec;

    if (tm_period > (uint32_t)(start - timer_count()))
    {
        return false;
    }
    else
    {
        return true;
    }
}

/**
 * @brief 按文件名查询FAT文件属性。
 * @param fileName 文件名
 * @param pResult 输出的文件属性
 * @return true表示找到文件，false表示未找到
 */
bool GetFileAttrByName(const char *fileName, EACH_FLASHFAT_STRUCT *pResult)
{
    if (fileName == NULL || pResult == NULL)
        return false;

    for (size_t i = 0; i < File_Fat_Table.FileNum; i++)
    {
        if (compare_filename(fileName, File_Fat_Table.File_Fat[i].Name, File_Fat_Table.File_Fat[i].Flash_Pro) == 1)
        {
            return true;
        }
    }

    return false; // 未找到
}

/**
 * @brief 计算32位累加校验和。
 * @param data 数据起始地址
 * @param length 数据长度
 * @return 32位累加校验和
 */
uint32_t calculate_checksum(const uint8_t *data, uint32_t length)
{
    uint32_t sum = 0;

    // 累加完整32位字
    while (length >= 4)
    {
        uint32_t word = (uint32_t)data[0] |
                        ((uint32_t)data[1] << 8) |
                        ((uint32_t)data[2] << 16) |
                        ((uint32_t)data[3] << 24);
        sum    += word;
        data   += 4;
        length -= 4;
    }

    // 剩余字节按低位字节序累加
    uint32_t last_word = 0;
    for (uint32_t i = 0; i < length; i++)
    {
        last_word |= (uint32_t)data[i] << (i * 8);
    }
    sum += last_word;

    return sum;
}
