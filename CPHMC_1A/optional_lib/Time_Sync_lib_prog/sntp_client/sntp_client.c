#include "sntp_client.h"
#include "net_all_include.h"
#include "cfg_prase_app.h"
#include "irig_b_interface.h"
#include "sync_logic_prog.h"
#include "irigb_codeelement.h"

// 常量定义
#define SNTP_POLL_INTERVAL_SEC 20

// 全局变量
static uint32_t sntp_tick_counter = 0;
#if defined(SVG_DEV_FUNC)
static uint32_t sntp_server_ip = TOIPADDR(192, 168, 10, 111); // 构网SVG项目：默认服务器IP，IP必须与内网网段做区分，让A核能够识别到从外网转发
#else
static uint32_t sntp_server_ip = TOIPADDR(192, 168, 10, 111); // 智能终端项目：默认服务器IP，直接走外网通讯
#endif
static uint32_t sntp_valid_countdown = 0; // 有效性倒计时

// 防抖动变量
static uint32_t sntp_stable_counter = 0; // 稳定偏差计数器
#define SNTP_STABLE_THRESHOLD 5          // 需要连续检测到的次数
#define SNTP_DIFF_THRESHOLD 2            // 时间偏差阈值（秒）

// 外部全局变量
extern CLK_TIME_EDGE Clk_Time_Edge;
// extern callback_tcpip_process *m_pCallBackSntp; // 移除这行，因为 callback_tcpip_process 未定义

// 前向声明
void sntp_send_request(void);
void sntp_recv_callback(int32_t nEthNo, NET_RAW_PKG_INFO *pInfo);

// 初始化
void sntp_init(void)
{
    uint8_t ip_array[4] = {0};

    // 从配置文件中解析 SNTP 服务器 IP
    if (ip_string_to_array((uint8_t *)IRIGB_IP_Cfg.irigb_cfg.Sntp_Serve_IP, (uint8_t *)ip_array) == TRUE)
    {
        sntp_server_ip = TOIPADDR(ip_array[0], ip_array[1], ip_array[2], ip_array[3]);
    }

    // 注册回调函数
    udp_register_sntp(sntp_recv_callback);
}

// 轮询任务 (每秒调用一次)
void sntp_poll(void)
{
    // 自愈机制：每次都尝试重新注册，确保万无一失
    udp_register_sntp(sntp_recv_callback);

    // 检查同步状态 (B码优先级)
    // 仅当B码处于失步状态 (eSCHS_Lose) 时才继续
    if (sync_logic_allow_sntp_update() == false)
    {
        sntp_tick_counter = 0;
        return;
    }

    if (sntp_valid_countdown > 0)
    {
        sntp_valid_countdown--;
    }

    sntp_tick_counter++;
    if (sntp_tick_counter >= SNTP_POLL_INTERVAL_SEC)
    {
        sntp_tick_counter = 0;
        sntp_send_request();
    }
}

// 检查 SNTP 是否有效
bool sntp_is_valid(void)
{
    return (sntp_valid_countdown > 0);
}

// 发送请求
void sntp_send_request(void)
{
    sntp_msg_t sntp_msg;
    udp_socket_t udp_socket;

    memset(&sntp_msg, 0, sizeof(sntp_msg));
    memset(&udp_socket, 0, sizeof(udp_socket));

    // LI=0, VN=3, Mode=3 (客户端)
    sntp_msg.li_vn_mode = (0 << 6) | (3 << 3) | SNTP_MODE_CLIENT;

    // 设置 Socket
    udp_socket.rmt_ipaddr = sntp_server_ip;
    udp_socket.rmt_port = SNTP_PORT;
    udp_socket.lcl_port = SNTP_PORT; 

    // 发送
    udp_send((const bystrm *)&sntp_msg, sizeof(sntp_msg), &udp_socket);
}

// 接收回调
void sntp_recv_callback(int32_t nEthNo, NET_RAW_PKG_INFO *pInfo)
{
    if (sync_logic_allow_sntp_update() == false)
    {
        return;
    }

    // pInfo->pRawPkg 指向 IP 头。
    // UDP 数据偏移 = IP 头 (20) + UDP 头 (8) = 28 字节。
    // 注意：IP 头长度可能变化 (IHL)，但根据 net_udp.c 逻辑暂时假设为标准的 20 字节。
    
    uint8_t *pRawData = (uint8_t *)pInfo->pRawPkg;
    sntp_msg_t *pMsg = (sntp_msg_t *)(pRawData + 28);
    
    // 检查模式 (服务器 = 4)
    uint8_t mode = pMsg->li_vn_mode & 0x07;
    if (mode != SNTP_MODE_SERVER)
    {
        return;
    }
    
    // 提取发送时间戳 (服务器发送数据包的时间)
    uint32_t trans_ts_sec = ntohl(pMsg->trans_ts_sec);
    // uint32_t trans_ts_frac = ntohl(pMsg->trans_ts_frac);
    
    // 转换为 Unix 时间 (1970年以来的秒数)
    if (trans_ts_sec > NTP_TIMESTAMP_DELTA)
    {
        uint32_t unix_sec = trans_ts_sec - NTP_TIMESTAMP_DELTA;
        
        // 防抖动逻辑
        uint32_t current_cpu_time = Clk_Time_Edge.nUTC;
        uint32_t diff = (unix_sec > current_cpu_time) ? (unix_sec - current_cpu_time) : (current_cpu_time - unix_sec);

        // 如果偏差较小 (<= 2s)，立即更新
        if (diff <= SNTP_DIFF_THRESHOLD)
        {
            Clk_Time_Edge.nUTC = unix_sec;
            sntp_stable_counter = 0; // 成功的小幅更新后重置计数器
        }
        else
        {
            // 检测到大偏差，检查稳定性
            sntp_stable_counter++;
            if (sntp_stable_counter >= SNTP_STABLE_THRESHOLD)
            {
                // 确认稳定偏差，强制更新
                Clk_Time_Edge.nUTC = unix_sec;
                sntp_stable_counter = 0;
            }
            else
            {
                // 忽略此次更新，等待确认
                // 但仍标记为有效源以保持“活跃”状态
            }
        }
        
        // 设置时间质量
        Clk_Time_Edge.nTimeQuality = 0x00;
        
        Clk_Time_Edge.nTimeState = SYNCLK_SYNC_SRC_OK_MASK | SYNCLK_SYNC_FOLLOW_MASK;

        // 重置有效倒计时 (2个轮询周期)
        sntp_valid_countdown = SNTP_POLL_INTERVAL_SEC * 2;
    }
}
