/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       ipc_common.c
 *@author     wenjunf
 *@date       2025.04.07
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.04.07  1.0       wenjunf    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "ipc_common.h"
#include "cfg_prase_app.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
ipc_shm_info_t ipc_shm_info IPC_INFO_SHARE;

/* 默认组网 IP/MAC 的末尾字节 YY，可被配置文件解析模块修改 */
static uint8_t g_net_addr_yy = 10;

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */
static uint8_t get_config_dev_addr(void);
static uint8_t get_plugin_no(void);
static uint8_t calc_port_yy(uint8_t port_id);
static void
make_network_addr_by_rule(uint8_t port_id, uint8_t dev_addr, uint8_t mac[6], uint8_t ip[4]);
/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// 判断新收到的CNT是否满足要求
// report_cnt_new--新的CNT, report_cnt_old-旧的CNT
// 返回：TRUE--CNT递增 ，FALSE--不递增
bool check_frame_u32_cnt_add(uint32_t report_cnt_new, uint32_t report_cnt_old)
{
    if (report_cnt_new == 0 || report_cnt_new == report_cnt_old)
    {
        // 帧内新计数值序号为0则认为对方第一帧报文，则不接收，判重启
        // 重复报文，则不接收
        return false;
    }
    if ((report_cnt_new >= 0x80000000) && (report_cnt_old >= 0x80000000))
    {
        // 在同一个半区比较大小，没有翻转
        if (report_cnt_new > report_cnt_old)
        {
            return true;
        }
        return false;
    }
    if ((report_cnt_new <= 0x80000000) && (report_cnt_old <= 0x80000000))
    {
        // 在同一个半区比较大小，没有翻转
        if (report_cnt_new > report_cnt_old)
        {
            return true;
        }
        return false;
    }
    if ((report_cnt_new < 0x10) && (report_cnt_old > 0xFFFFFFF0))
    {
        // CNT翻转
        // 当旧值接近最大值（0xFFFFFFFF）时，新值很小，说明计数器循环发生溢出翻转
        return true;
    }
    if ((report_cnt_new > 0x10) && (report_cnt_new < 0xFFFFFFF0))
    {
        if ((report_cnt_old > 0x10) && (report_cnt_old < 0xFFFFFFF0))
        {
            if (report_cnt_new > report_cnt_old)
            {
                return true;
            }
        }
    }
    return false;
}

bool check_frame_u16_cnt_add(uint16_t report_cnt_new, uint16_t report_cnt_old)
{
    if (report_cnt_new == 0 || report_cnt_new == report_cnt_old)
    {
        // 帧内新计数值序号为0则认为对方第一帧报文，则不接收，判重启
        // 重复报文，则不接收
        return false;
    }
    if ((report_cnt_new >= 0x8000) && (report_cnt_old >= 0x8000))
    {
        // 在同一个半区比较大小，没有翻转
        if (report_cnt_new > report_cnt_old)
        {
            return true;
        }
        return false;
    }
    if ((report_cnt_new <= 0x8000) && (report_cnt_old <= 0x8000))
    {
        // 在同一个半区比较大小，没有翻转
        if (report_cnt_new > report_cnt_old)
        {
            return true;
        }
        return false;
    }
    if ((report_cnt_new < 0x10) && (report_cnt_old > 0xFFF0))
    {
        // CNT翻转
        // 当旧值接近最大值（0xFFFFFFFF）时，新值很小，说明计数器循环发生溢出翻转
        return true;
    }
    if ((report_cnt_new > 0x10) && (report_cnt_new < 0xFFF0))
    {
        if ((report_cnt_old > 0x10) && (report_cnt_old < 0xFFF0))
        {
            if (report_cnt_new > report_cnt_old)
            {
                return true;
            }
        }
    }
    return false;
}

// #####################################################################################
//			公用子程序
// #####################################################################################
// 获的当前插件序号
// 返回：插件序号 从0开始

// TODO 获取槽位号和核号
uint8_t get_Current_Slot_ID(void) { return GPEMCSlotID; }

// 获得当前Core序号
uint8_t get_Current_Core_ID(void) { return getCoreNr(); }

static uint32_t calc_addr_checksum(const addr_info_t *info)
{
    const uint8_t *data = (const uint8_t *)info->cores;
    size_t len = sizeof(addr_info_t) - sizeof(info->magic_flag) - sizeof(info->checksum);
    uint32_t sum = 0;

    for (size_t i = 0; i < len; i += 4)
    {
        uint32_t word = 0;
        for (int j = 0; j < 4; j++)
        {
            size_t idx = i + j;
            if (idx >= len)
                break;  // 不足4字节补0

            word |= ((uint32_t)data[idx]) << (8 * j);  // 小端拼装
        }
        sum += word;
    }
    return sum;
}

static void update_addr_checksum(addr_info_t *info)
{
    info->checksum = calc_addr_checksum(info);
}

bool verify_addr_checksum(const addr_info_t *info)
{
    cache_inv_com(info, sizeof(addr_info_t), CacheP_TYPE_ALL);

    if (info->magic_flag != 0x12345678)
        return false;
    return (info->checksum == calc_addr_checksum(info));
}

/**
 * @brief 提供给配置模块的接口：更新组网 YY 值
 */
void set_net_addr_yy(uint8_t yy)
{
    g_net_addr_yy = yy;
}

static uint8_t get_config_dev_addr(void)
{
    if (IRIGB_IP_Cfg.device_addr_cfg.valid_flag &&
        IRIGB_IP_Cfg.device_addr_cfg.dev_addr > 0 &&
        IRIGB_IP_Cfg.device_addr_cfg.dev_addr <= 255)
    {
        return (uint8_t)IRIGB_IP_Cfg.device_addr_cfg.dev_addr;
    }

    return g_net_addr_yy;
}

static uint8_t get_plugin_no(void)
{
    return (uint8_t)(get_Current_Slot_ID() / 2 + 1);
}

static uint8_t calc_port_yy(uint8_t port_id)
{
    return (uint8_t)(178 - (port_id + 1) * 10);
}

static void make_network_addr_by_rule(uint8_t port_id, uint8_t dev_addr, uint8_t mac[6], uint8_t ip[4])
{
    uint8_t plugin_no = get_plugin_no();
    uint8_t yy = calc_port_yy(port_id);
    uint8_t xf = (uint8_t)(plugin_no * 0x10 + 0x0F);

    mac[0] = 0x00;
    mac[1] = 0x58;
    mac[2] = 0xC0;
    mac[3] = yy;
    mac[4] = xf;
    mac[5] = dev_addr;

    ip[0] = 192;
    ip[1] = yy;
    ip[2] = xf;
    ip[3] = dev_addr;
}

static void make_lan_network_addr_by_rule(uint8_t dev_addr, uint8_t mac[6], uint8_t ip[4])
{
    uint8_t plugin_no = get_plugin_no();
    uint8_t xf = (uint8_t)(plugin_no * 0x10 + 0x0F);

    mac[0] = 0x00;
    mac[1] = 0x58;
    mac[2] = 0xC0;
    mac[3] = 0xB2;
    mac[4] = dev_addr;
    mac[5] = xf;

    ip[0] = 192;
    ip[1] = 178;
    ip[2] = dev_addr;
    ip[3] = xf;
}

/**
 * @brief 初始化所有核的内网/外网 IP 与 MAC 地址，以及板卡组网地址
 *
 * 功能：
 *  - 为每个核分配固定的内网 MAC/IP
 *  - 为每个核的所有 portid 分配外网 MAC/IP
 *  - 设置各核对应的以太网发送中断号
 *
 * 内网地址分配规则：
 *   MAC: 00:58:C0:B2:6F:XY   (X = 插件序号，从1开始；Y = Core序号，从1开始)
 *   IP : 192.178.111.XY      (XY 为十六进制数，写入IP末段时转换为十进制数)
 *   中断号: Core0=1, Core1=2, ...
 *
 * 外网地址分配规则：
 *   MAC: 00:58:C0:YY:ZZ:0A    (YY = 178-(port+1)*10, ZZ = (slot_id+1)*0x10+(core+1))
 *   IP : 192.YY.ZZ.0A         (YY,ZZ 同上)
 *
 * 组网地址分配规则 ：
 *   MAC: 00:58:C0:A8:XF:Z    (X = slot_id, F = 0x0F, Z = 装置地址，默认 10)
 *   IP : 192.168.XF.Z        (XF 的十进制值)
 */
void enet_addr_init(void)
{
    uint8_t slot_id = get_Current_Slot_ID() / 2;
    uint8_t core_isr_id[CORE_MAX] = {0};
    uint8_t device_addr = get_config_dev_addr();

    /* 初始化默认内网口地址与中断号 */
    for (int core = 0; core < CORE_MAX; core++)
    {
        core_addrs_t *c = &ipc_shm_info.addr_info.cores[core];
        uint8_t xy = (uint8_t)((slot_id + 1) * 0x10 + (core + 1));

        /* 内网 MAC: 00:58:C0:B2:6F:XY */
        uint8_t lan_mac_prefix[4] = {0x00, 0x58, 0xC0, 0xB2};
        memcpy(c->lan_mac, lan_mac_prefix, sizeof(lan_mac_prefix));
        c->lan_mac[4] = 0x6F;
        c->lan_mac[5] = xy;

        /* 内网 IP: 192.178.111.XY */
        c->lan_ip[0] = 192;
        c->lan_ip[1] = 178;
        c->lan_ip[2] = 111;
        c->lan_ip[3] = xy;

        /* 设置发送中断号: Core0=1, Core1=2, ... */
        c->send_enet_int_ID = 1;
    }

    /* 初始化外网地址 */
    for (int core = 0; core < CORE_MAX; core++)
    {
        core_addrs_t *c = &ipc_shm_info.addr_info.cores[core];

        for (int port = 0; port < MAX_PORT_ID; port++)
        {
            /* 外网 MAC: 00:58:C0:YY:ZZ:0A */
            c->mac[port][0] = 0x00;
            c->mac[port][1] = 0x58;
            c->mac[port][2] = 0xC0;
            c->mac[port][3] = 178 - (port + 1) * 10;
            c->mac[port][4] = ((slot_id + 1) * 0x10) + (core + 1);
            c->mac[port][5] = device_addr;

            /* 外网 IP: 192.YY.ZZ.0A */
            c->ip[port][0] = 192;
            c->ip[port][1] = 178 - (port + 1) * 10;
            c->ip[port][2] = (slot_id + 1) * 0x10 + (core + 1);
            c->ip[port][3] = device_addr;
        }
    }
    
    // 设置内网口默认组网MAC和IP
    make_lan_network_addr_by_rule(device_addr,
                                  ipc_shm_info.addr_info.lan_mac,
                                  ipc_shm_info.addr_info.lan_ip);

    // 设置外网口默认组网 MAC 地址和 IP 地址
    for (uint8_t port = 0; port < MAX_PORT_ID; port++)
    {
        make_network_addr_by_rule(port,
                                  device_addr,
                                  ipc_shm_info.addr_info.net_mac[port],
                                  ipc_shm_info.addr_info.net_ip[port]);
    }

    /* 设置标志位与校验和 (校验和计算会自动包含新加入的 net_mac 和 net_ip 字段) */
    ipc_shm_info.addr_info.magic_flag = 0x12345678;
    update_addr_checksum(&ipc_shm_info.addr_info);

    cache_wb_com(&ipc_shm_info, sizeof(ipc_shm_info), CacheP_TYPE_ALLD);
}

/**
 * @brief 获取组网 IP/MAC 地址信息
 * * @param core_enet_inf 指向当前核以太网信息结构体的指针（用于输出）
 */
uint8_t get_net_config_info(CURRENT_CORE_ENET_MAC_IP_INF_STRUCT *core_enet_inf,
                            CURRENT_ENET_INF_STRUCT Current_enet_inf)
{
    uint8_t port_id = Current_enet_inf.Net_ID;
    uint32_t port_type = Current_enet_inf.Net_type;

    if (core_enet_inf == NULL || port_id >= MAX_PORT_ID)
    {
        return false;
    }

    if (port_type == FRAME_TYPE_LAN)
    {
        /* 1. 从共享内存中获取板卡端口唯一的组网 MAC 地址 */
        memcpy(core_enet_inf->srcMac, ipc_shm_info.addr_info.lan_mac, 6);
        /* 2. 从共享内存中获取板卡端口唯一的组网 IP 地址 */
        memcpy(core_enet_inf->src_IP, ipc_shm_info.addr_info.lan_ip, 4);
    }
    else if (port_type == FRAME_TYPE_ETH)
    {
        /* 1. 从共享内存中获取板卡端口唯一的组网 MAC 地址 */
        memcpy(core_enet_inf->srcMac, ipc_shm_info.addr_info.net_mac[port_id], 6);
        /* 2. 从共享内存中获取板卡端口唯一的组网 IP 地址 */
        memcpy(core_enet_inf->src_IP, ipc_shm_info.addr_info.net_ip[port_id], 4);
    }
    else
    {
        return false;
    }
    core_enet_inf->send_enet_int_ID = 1;
    return true;
}

/**
 * @brief 获取指定核/端口的以太网配置信息
 *
 * 根据 Current_enet_inf 中的 Core_ID / Net_ID / Net_type，
 * 从全局共享内存 ipc_shm_info 中查找对应的 MAC/IP/中断号，
 * 并填充到 p_core_enet_inf 中。
 *
 * @param[out] p_core_enet_inf   输出结构体，填充该核对应的 MAC/IP/中断号
 * @param[in]  Current_enet_inf  输入结构体，包含 Core_ID、Net_ID、Net_type 等
 *
 * @retval true   成功获取
 * @retval false  参数错误或越界
 */
uint8_t get_locate_enet_inf(CURRENT_CORE_ENET_MAC_IP_INF_STRUCT *p_core_enet_inf,
                            CURRENT_ENET_INF_STRUCT Current_enet_inf)
{
    if (!p_core_enet_inf)
    {
        return false;
    }

    //    if (!verify_addr_checksum(&ipc_shm_info.addr_info))
    //    {
    //        return false;
    //    }

    uint8_t  core_id   = Current_enet_inf.Core_ID;
    uint8_t  port_id   = Current_enet_inf.Net_ID;
    uint16_t send_mode = Current_enet_inf.send_mode;

#ifdef SOC_J721E
    if (Current_enet_inf.Slot_ID == 3)
    {
        port_id += 8;
    }
#endif

    if (core_id >= CORE_MAX || port_id >= MAX_PORT_ID)
    {
        return false; /* 参数非法 */
    }

    core_addrs_t *c = &ipc_shm_info.addr_info.cores[core_id];

    if (send_mode == 0)
    {
        // 默认由核0管理转发
        p_core_enet_inf->send_enet_int_ID = ipc_shm_info.addr_info.cores[0].send_enet_int_ID;
    }
    else
    {
        // 由本核自己发送
        p_core_enet_inf->send_enet_int_ID = c->send_enet_int_ID;
    }
    if (Current_enet_inf.Net_type == 0x41)
    {
        /* 内网：取 cores[core_id].lan_mac / lan_ip */
        memcpy(p_core_enet_inf->srcMac, c->lan_mac, 6);
        memcpy(p_core_enet_inf->src_IP, c->lan_ip, 4);
    }
    else
    {
        /* 外网：取 cores[core_id].mac[port_id] / ip[port_id] */
        memcpy(p_core_enet_inf->srcMac, c->mac[port_id], 6);
        memcpy(p_core_enet_inf->src_IP, c->ip[port_id], 4);
    }

    return true;
}

/**
 * @brief 设置某个核某个端口的外网 MAC 地址
 *
 * 根据 core_id 和 port_id，修改共享内存中对应的外网 MAC 地址。
 * 内网 MAC 地址不可修改。
 *
 * @param[in] core_id  核号 (范围: 0 .. CORE_MAX-1)
 * @param[in] port_id  网口号 (范围: 0 .. MAX_PORT_ID-1)
 * @param[in] mac      指向 6 字节的 MAC 地址数组
 *
 * @retval true   设置成功
 * @retval false  参数错误 (越界或指针为空)
 */
uint8_t enet_set_mac(uint8_t core_id, uint8_t port_id, const uint8_t mac[6])
{
    if (core_id >= CORE_MAX || port_id >= MAX_PORT_ID)
    {
        return false; /* 参数非法 */
    }

    core_addrs_t *c = &ipc_shm_info.addr_info.cores[core_id];
    memcpy(c->mac[port_id], mac, 6);

    update_addr_checksum(&ipc_shm_info.addr_info);
    /* 同步到共享内存，确保其他核可见 */
    cache_wb_com(&ipc_shm_info, sizeof(ipc_shm_info), CacheP_TYPE_ALLD);

    return true;
}

/**
 * @brief 设置某个核某个端口的外网 IP 地址
 *
 * 根据 core_id 和 port_id，修改共享内存中对应的外网 IP 地址。
 * 内网 IP 地址不可修改。
 *
 * @param[in] core_id  核号 (范围: 0 .. CORE_MAX-1)
 * @param[in] port_id  网口号 (范围: 0 .. MAX_PORT_ID-1)
 * @param[in] ip       指向 4 字节的 IP 地址数组
 *
 * @retval true   设置成功
 * @retval false  参数错误 (越界或指针为空)
 */
void set_all_port_ip(void)
{
    uint8_t ip_array[4] = {0};

    if (!IRIGB_IP_Cfg.all_port_ip_cfg.valid_flag)
    {
        return;
    }

    for (uint32_t i = 0; i < IRIGB_IP_Cfg.all_port_ip_cfg.set_ip_number; i++)
    {
        PORT_IP_CFG_STRUCT *port_ip_cfg = &IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg[i];
        uint8_t             port_id     = port_ip_cfg->eth_num - 1;
        uint8_t             core_id      = port_ip_cfg->core_num - 1;

        if (port_id >= MAX_PORT_ID || core_id >= CORE_MAX)
        {
            continue;
        }

        if (ip_string_to_array((uint8_t *)&port_ip_cfg->ip_addr, ip_array) != TRUE)
        {
            continue;
        }

        memcpy(ipc_shm_info.addr_info.cores[core_id].ip[port_id], ip_array, sizeof(ip_array));
    }

    update_addr_checksum(&ipc_shm_info.addr_info);
    cache_wb_com(&ipc_shm_info, sizeof(ipc_shm_info), CacheP_TYPE_ALLD);
}

void printf_borad_ip_mac(void)
{
    uint8_t slot_id = get_Current_Slot_ID() / 2;
    // 内网
    for (int core = 0; core < CORE_MAX; core++)
    {
        core_addrs_t *c = &ipc_shm_info.addr_info.cores[core];
        DebugP_log("[R5F%d]: Slot %d, LAN MAC: %02X:%02X:%02X:%02X:%02X:%02X, IP: %d.%d.%d.%d\r\n",
                   core,
                   slot_id,
                   c->lan_mac[0],
                   c->lan_mac[1],
                   c->lan_mac[2],
                   c->lan_mac[3],
                   c->lan_mac[4],
                   c->lan_mac[5],
                   c->lan_ip[0],
                   c->lan_ip[1],
                   c->lan_ip[2],
                   c->lan_ip[3]);
    }
    DebugP_log("--------------------------------------------------------------------------------------------\r\n");

    // 外网
    for (int core = 0; core < CORE_MAX; core++)
    {
        core_addrs_t *c = &ipc_shm_info.addr_info.cores[core];

        for (int port = 0; port < MAX_PORT_ID; port++)
        {
            DebugP_log("[R5F%d]: Slot %d, Port %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X, IP: %d.%d.%d.%d\r\n",
                       core,
                       slot_id,
                       port,
                       c->mac[port][0],
                       c->mac[port][1],
                       c->mac[port][2],
                       c->mac[port][3],
                       c->mac[port][4],
                       c->mac[port][5],
                       c->ip[port][0],
                       c->ip[port][1],
                       c->ip[port][2],
                       c->ip[port][3]);
        }
        DebugP_log("--------------------------------------------------------------------------------------------\r\n");
    }
    // 内网口组网
    DebugP_log("[Board Lan Net]: Slot %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X, IP: %d.%d.%d.%d\r\n",
               slot_id,
               ipc_shm_info.addr_info.lan_mac[0],
               ipc_shm_info.addr_info.lan_mac[1],
               ipc_shm_info.addr_info.lan_mac[2],
               ipc_shm_info.addr_info.lan_mac[3],
               ipc_shm_info.addr_info.lan_mac[4],
               ipc_shm_info.addr_info.lan_mac[5],
               ipc_shm_info.addr_info.lan_ip[0],
               ipc_shm_info.addr_info.lan_ip[1],
               ipc_shm_info.addr_info.lan_ip[2],
               ipc_shm_info.addr_info.lan_ip[3]);

    // 外网口组网
    for (int port = 0; port < MAX_PORT_ID; port++)
    {
        DebugP_log("[Board WLan Net]: Slot %d, Port %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X, IP: %d.%d.%d.%d\r\n",
                   slot_id,
                   port,
                   ipc_shm_info.addr_info.net_mac[port][0],
                   ipc_shm_info.addr_info.net_mac[port][1],
                   ipc_shm_info.addr_info.net_mac[port][2],
                   ipc_shm_info.addr_info.net_mac[port][3],
                   ipc_shm_info.addr_info.net_mac[port][4],
                   ipc_shm_info.addr_info.net_mac[port][5],
                   ipc_shm_info.addr_info.net_ip[port][0],
                   ipc_shm_info.addr_info.net_ip[port][1],
                   ipc_shm_info.addr_info.net_ip[port][2],
                   ipc_shm_info.addr_info.net_ip[port][3]);
    }
    DebugP_log("--------------------------------------------------------------------------------------------\r\n");
}
