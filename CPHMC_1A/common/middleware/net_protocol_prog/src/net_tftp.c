/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_tftp.c
*@author     xuesen
*@date       2026.05.06
*@brief      TFTP扩展升级协议处理实现。
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

#define TFTP_IDLE_TIMEOUT_MS         (20000U) // TFTP会话空闲超时时间
#define TFTP_STATE_REPORT_PERIOD_MS  (1000U)  // TFTP状态帧发送周期
#define TFTP_DIAG_REPORT_BLOCKS      (64U)    // 正常数据帧诊断信息输出周期
// CAN升级代理专用的重启窗口、看门狗和转发状态仅在目标核心编译
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
#define TFTP_RST_VALID_WINDOW_MS     (5000U)  // 升级完成后重启帧有效时间
#endif

static uint16_t   m_TftpEthPhyNo;

static uint32_t m_TftpBlock;
static uint8_t  m_TftpBlock_rlov; // 16位块号回绕标志
static uint16_t m_RmtTftpPort;
static ipaddr_t m_RmtTftpIP;

// 升级属性
static update_Prop update_prop;

static int32_t file_index = -1;

static uint32_t file_offset = 0;

static enum eTftpState m_TftpState; // 状态机
static uint16_t        m_TftpTID = TFTP_TID;
static FILE_INFO       info; // RRQ读取的文件信息

emTFTP_ERR_INFO   emTftpErr;
emTFTP_STATE_INFO emTftpState;

uint8_t g_IsUpdateFile = 0; // 是否正在升级程序

TFTP_SYSTEM_INF_STRUCT tftp_system_inf = {0};

uint16_t g_update_state                = 0;
uint32_t g_update_total_size           = 0;
uint32_t g_update_current_read_offset  = 0;
uint32_t g_update_current_write_offset = 0;

uint32_t file_checksum_total = 0;

// TFTP下载会话诊断统计，仅用于定位丢帧、重传和总校验异常
static uint32_t m_TftpDiagDataBlocks      = 0;
static uint32_t m_TftpDiagDuplicateBlocks = 0;
static uint32_t m_TftpDiagDataBytes       = 0;
static uint32_t m_TftpDiagLastBlockChecksum = 0;
static uint32_t m_TftpDiagLastBlockLength   = 0;
static uint8_t  m_TftpDataBuffer[TFTP_BLOCK_LEN] = {0}; // 本地写入使用的稳定数据副本
static bool     m_TftpLastSumSuccess = false;
static uint32_t m_TftpLastSuccessSum = 0;

uint8_t eth_send_buff[MAX_ETH_BUFF_BYTE_NUMBER] = {0}; // 发送缓冲区

bool is_group_net = false; // 当前是否处于组网环境
static bool     m_TftpIsGroupNet = false;
static ipaddr_t m_TftpSrcIP      = 0;
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
static volatile uint8_t m_TftpStopFeedWatchdogFlag = 0;
#endif
static uint32_t m_TftpIdleTimeMs  = 0;
static uint32_t m_TftpStateTimeMs = 0;
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
static uint32_t m_TftpRstWindowMs = 0;
#endif

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
uint8_t __attribute__((weak)) io_transfer_upgrade_data(uint8_t *pRawData, uint8_t dst_can_id, uint32_t send_len);
static bool tftp_transfer_remote_upgrade_data(uint8_t *pRawData, uint8_t dst_can_id, uint32_t send_len);
#endif
static void tftp_init(void);
static void tftp_refresh_activity(void);
static void tftp_recv_rrq(NET_RAW_PKG_INFO *pInfo);
static void tftp_send_fif(NET_RAW_PKG_INFO *pInfo, const char *name, uint32_t size, uint32_t crc);
static void tftp_recv_wrq(NET_RAW_PKG_INFO *pInfo);
static void tftp_recv_data(NET_RAW_PKG_INFO *pInfo);
static bool tftp_get_payload_len(NET_RAW_PKG_INFO *pInfo, uint32_t *payload_len);
static bool tftp_parse_rrq_wrq(NET_RAW_PKG_INFO *pInfo,
                               char *filename,
                               char *mode,
                               char *file_len,
                               const uint8_t *pMacData,
                               uint32_t payload_len);
static void tftp_recv_sum(NET_RAW_PKG_INFO *pInfo);
static void tftp_recv_diff(NET_RAW_PKG_INFO *pInfo);

static void tftp_send_ack(NET_RAW_PKG_INFO *pInfo, uint16_t block);
static void tftp_send_error(NET_RAW_PKG_INFO *pInfo, emTFTP_ERR_INFO err);
static bool tftp_send_ok(NET_RAW_PKG_INFO *pInfo);
void tftp_send_state(uint16_t state, uint32_t total_size, uint32_t current_read_offset, uint32_t current_write_offset);
static void tftp_send_lok(NET_RAW_PKG_INFO *pInfo);
static void tftp_send_dres(NET_RAW_PKG_INFO *pInfo, uint32_t file_len, uint32_t file_chksum);
static void tftp_recv_lck(NET_RAW_PKG_INFO *pInfo);
static void tftp_recv_stop(NET_RAW_PKG_INFO *pInfo);
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
static void tftp_recv_rst(NET_RAW_PKG_INFO *pInfo);
static void tftp_stop_feed_watchdog(void);
static void tftp_open_rst_window(void);
static bool tftp_rst_window_is_valid(void);
#endif
static void tftp_capture_session_context(NET_RAW_PKG_INFO *pInfo);
static ipaddr_t tftp_get_session_src_ip(bool *pIsNetworking);
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
static void tftp_recv_req(NET_RAW_PKG_INFO *pInfo);
#endif
static bool tftp_reject_new_session_if_busy(NET_RAW_PKG_INFO *pInfo, uint8_t dst_id);
static bool tftp_send_frame(bool is_networking,
                            uint32_t report_len,
                            const net_send_route_t *pRoute,
                            const uint8_t *pDstMac);


/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 初始化TFTP会话状态。
 */


void tftp_init(void)
{
    m_TftpState = eTftpState_Idle;
    m_TftpTID   = TFTP_TID;

    m_TftpBlock   = 0;
    m_RmtTftpIP   = 0;
    m_RmtTftpPort = 0;
    m_TftpIsGroupNet = false;
    m_TftpSrcIP      = self_ipaddr;

    emTftpErr        = TFTP_ERR_NONE;
    m_TftpBlock_rlov = 0;
    g_IsUpdateFile   = 0;
    m_TftpDiagDataBlocks      = 0;
    m_TftpDiagDuplicateBlocks = 0;
    m_TftpDiagDataBytes       = 0;
    m_TftpDiagLastBlockChecksum = 0;
    m_TftpDiagLastBlockLength   = 0;
    m_TftpLastSumSuccess        = false;
    m_TftpLastSuccessSum        = 0;
    m_TftpIdleTimeMs  = 0;
    m_TftpStateTimeMs = 0;
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    m_TftpRstWindowMs = 0;
#endif
    return;
}

/**
 * @brief 刷新TFTP会话活动时间。
 */
static void tftp_refresh_activity(void)
{
    m_TftpIdleTimeMs = 0;
}

/**
 * @brief 记录TFTP错误位置。
 * @param err_code 错误码
 */
static void tftp_log_error(uint8_t err_code)
{
    tftp_system_inf.tftp_err_location[tftp_system_inf.tftp_err_number++] = err_code;
    if (tftp_system_inf.tftp_err_number >= MAX_TFTP_ERR_NUMBER)
        tftp_system_inf.tftp_err_number = 0;
}

/**
 * @brief 从TFTP报文字节流读取16位字段。
 * @param pData 字段起始地址
 * @return 读取到的16位字段值
 */
static uint16_t tftp_read_u16(const uint8_t *pData)
{
    uint16_t value;

    memcpy(&value, pData, sizeof(value));
    return value;
}

/**
 * @brief 从TFTP报文字节流读取32位字段。
 * @param pData 字段起始地址
 * @return 读取到的32位字段值
 */
static uint32_t tftp_read_u32(const uint8_t *pData)
{
    uint32_t value;

    memcpy(&value, pData, sizeof(value));
    return value;
}

/**
 * @brief 判断TFTP报文是否属于组网地址或组网MAC。
 * @param pInfo 原始报文信息
 * @return true表示组网报文，false表示普通报文
 */
static bool tftp_packet_is_networking(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    ipaddr_t       dst_ip;

    // 未启用组网TFTP的装置统一按普通网口处理
    if (!net_profile_enable_networking_tftp())
    {
        return false;
    }

    if ((pInfo == NULL) || (pInfo->pRawPkg == NULL))
    {
        return false;
    }

    pRaw   = (const uint8_t *)pInfo->pRawPkg;
    dst_ip = tftp_read_u32(pRaw + IP_DST_OFFSET);
    if (ipaddr_cmp(dst_ip, net_ipaddr))
    {
        return true;
    }

    return (memcmp(pInfo->dstMac, net_SrcMacAddr.addr, ENET_MAC_ADDR_LEN) == 0);
}

#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
/**
 * @brief 按装置策略转发远程升级数据。
 * @param pRawData TFTP载荷起始地址
 * @param dst_can_id 目标CAN ID
 * @param send_len 转发长度
 * @return true表示转发成功，false表示禁用或转发失败
 */
static bool tftp_transfer_remote_upgrade_data(uint8_t *pRawData, uint8_t dst_can_id, uint32_t send_len)
{
    if (!net_profile_enable_remote_can_upgrade())
    {
        return false;
    }

    return (io_transfer_upgrade_data(pRawData, dst_can_id, send_len) == 0U);
}
#endif

/**
 * @brief 捕获TFTP当前会话的组网上下文。
 * @param pInfo 原始报文信息
 */
static void tftp_capture_session_context(NET_RAW_PKG_INFO *pInfo)
{
    m_TftpIsGroupNet = tftp_packet_is_networking(pInfo);
    m_TftpSrcIP      = (m_TftpIsGroupNet == true) ? net_ipaddr : self_ipaddr;
    is_group_net     = m_TftpIsGroupNet;
}

/**
 * @brief 获取TFTP当前会话使用的源IP地址。
 * @param pIsNetworking 输出是否为组网会话
 * @return 当前会话源IP地址
 */
static ipaddr_t tftp_get_session_src_ip(bool *pIsNetworking)
{
    if (pIsNetworking != NULL)
    {
        *pIsNetworking = m_TftpIsGroupNet;
    }

    return (m_TftpSrcIP != 0) ? m_TftpSrcIP : self_ipaddr;
}

/**
 * @brief 解析TFTP报文中的UDP载荷长度。
 * @param pInfo 原始报文信息
 * @param payload_len 输出的TFTP载荷长度
 * @return true表示解析成功，false表示报文无效
 */
static bool tftp_get_payload_len(NET_RAW_PKG_INFO *pInfo, uint32_t *payload_len)
{
    const uint8_t *pRaw;
    uint16_t      udp_len;

    if ((pInfo == NULL) || (pInfo->pRawPkg == NULL) || (payload_len == NULL) ||
        (pInfo->nLength < (NET_IP_HEAD_LEN + CFG_UDPH_LEN)))
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][DROP] stage=payload_input info_null=%u raw_null=%u frame_len=%u\r\n",
                               (pInfo == NULL) ? 1U : 0U,
                               ((pInfo == NULL) || (pInfo->pRawPkg == NULL)) ? 1U : 0U,
                               (pInfo != NULL) ? pInfo->nLength : 0U);
        return false;
    }

    pRaw = (const uint8_t *)pInfo->pRawPkg;
    udp_len = ntohs(tftp_read_u16(pRaw + UDP_LEN_OFFSET));
    if ((udp_len < CFG_UDPH_LEN) || ((uint32_t)NET_IP_HEAD_LEN + udp_len > pInfo->nLength))
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][DROP] stage=udp_len frame_len=%u udp_len=%u min=%u\r\n",
                               pInfo->nLength,
                               udp_len,
                               CFG_UDPH_LEN);
        return false;
    }

    *payload_len = (uint32_t)udp_len - CFG_UDPH_LEN;
    return true;
}

/**
 * @brief 在TFTP会话忙时拒绝新的会话请求。
 * @param pInfo 原始报文信息
 * @param dst_id 新会话目标CAN ID
 * @return true表示已拒绝新会话，false表示允许继续处理
 */
static bool tftp_reject_new_session_if_busy(NET_RAW_PKG_INFO *pInfo, uint8_t dst_id)
{
    const uint8_t *pRaw;
    uint16_t       opcode;
    ipaddr_t       src_ip;
    uint16_t       src_port;
    ipaddr_t       old_ip;
    ipaddr_t       old_src_ip;
    uint16_t       old_port;
    uint8_t        old_dst_canid;
    bool           old_is_group_net;

    if (m_TftpState == eTftpState_Idle)
    {
        return false;
    }

    if ((pInfo == NULL) || (pInfo->pRawPkg == NULL))
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][BUSY] invalid_new_session state=%u\r\n", m_TftpState);
        return true;
    }

    pRaw = (const uint8_t *)pInfo->pRawPkg;
    opcode   = ntohs(tftp_read_u16(pRaw + TFTP_OPCODE_OFFSET));
    src_ip   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    src_port = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));

    // 远程IO擦除Flash期间，上位机可能重发同一WRQ，避免误判为新会话。
    if ((m_TftpState == eTftpState_Wrq) && (update_prop == REMOTE_UPDATE) && (opcode == TFTP_WRQ) &&
        (src_ip == m_RmtTftpIP) && (src_port == m_RmtTftpPort) && (dst_id == dst_canid))
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][BUSY] remote_wrq_retry src=0x%08X:%u dst=0x%02X state=%u\r\n",
                               src_ip,
                               src_port,
                               dst_id,
                               m_TftpState);
        return true;
    }

    NET_PROTOCOL_DEBUG_LOG("[TFTP][BUSY] reject opcode=%u new=0x%08X:%u/0x%02X active=0x%08X:%u/0x%02X state=%u block=%u\r\n",
                           opcode,
                           src_ip,
                           src_port,
                           dst_id,
                           m_RmtTftpIP,
                           m_RmtTftpPort,
                           dst_canid,
                           m_TftpState,
                           m_TftpBlock);

    old_ip        = m_RmtTftpIP;
    old_src_ip    = m_TftpSrcIP;
    old_port      = m_RmtTftpPort;
    old_dst_canid = dst_canid;
    old_is_group_net = m_TftpIsGroupNet;

    m_RmtTftpIP   = src_ip;
    m_RmtTftpPort = src_port;
    dst_canid     = dst_id;
    tftp_capture_session_context(pInfo);
    emTftpErr     = TFTP_ERR_SESSION_BUSY;
    tftp_send_error(pInfo, emTftpErr);

    m_RmtTftpIP   = old_ip;
    m_TftpSrcIP   = old_src_ip;
    m_RmtTftpPort = old_port;
    dst_canid     = old_dst_canid;
    m_TftpIsGroupNet = old_is_group_net;
    is_group_net     = old_is_group_net;
    return true;
}

/**
 * @brief 清理TFTP会话和升级状态。
 */
static void tftp_clear_state(void)
{
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
    if ((m_TftpState != eTftpState_Idle) || (m_TftpDiagDataBlocks != 0U) || (m_TftpDiagDuplicateBlocks != 0U))
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][CLEAR] err=%d state=%d block=%u blocks=%u duplicate=%u bytes=%u total=%u sum=0x%08X\r\n",
                   emTftpErr,
                   m_TftpState,
                   m_TftpBlock,
                   m_TftpDiagDataBlocks,
                   m_TftpDiagDuplicateBlocks,
                   m_TftpDiagDataBytes,
                   g_update_total_size,
                   file_checksum_total);
    }
#endif

    m_RmtTftpIP         = 0;
    m_RmtTftpPort       = 0;
    m_TftpIsGroupNet    = false;
    m_TftpSrcIP         = self_ipaddr;
    g_IsUpdateFile      = 0;
    file_checksum_total = 0;
    emTftpState         = TFTP_STATE_IDLE;
    g_update_state      = emTftpState;
    file_offset         = 0;
    m_TftpState         = eTftpState_Idle;
    m_TftpBlock_rlov    = 0;
    m_TftpDiagDataBlocks      = 0;
    m_TftpDiagDuplicateBlocks = 0;
    m_TftpDiagDataBytes       = 0;
    m_TftpLastSumSuccess      = false;
    m_TftpLastSuccessSum      = 0;
    if (file_index >= 0)
    {
        int32_t close_ret = file_close(file_index, 0);
        if (close_ret < 0)
        {
            NET_PROTOCOL_DEBUG_LOG("[TFTP][CLEAR][CLOSE_ERR] handle=%d ret=%d\r\n", file_index, close_ret);
        }
        file_index = -1;
    }
    dst_canid  = 0;
    m_TftpIdleTimeMs  = 0;
    m_TftpStateTimeMs = 0;
}

/**
 * @brief 执行TFTP 1ms周期维护。
 */
void tftp_1ms_swi_task(void)
{
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    if (m_TftpRstWindowMs > 0U)
    {
        m_TftpRstWindowMs--;
    }
#endif

    m_TftpStateTimeMs++;
    if (m_TftpStateTimeMs >= TFTP_STATE_REPORT_PERIOD_MS)
    {
        m_TftpStateTimeMs = 0;
        if (g_IsUpdateFile == TFTP_UPGRADE_STATUS_WRITING)
        {
            tftp_send_state(g_update_state,
                            g_update_total_size,
                            g_update_current_read_offset,
                            g_update_current_write_offset);
        }
    }

    if (m_TftpState == eTftpState_Idle)
    {
        m_TftpIdleTimeMs = 0;
        return;
    }

    m_TftpIdleTimeMs++;
    if (m_TftpIdleTimeMs < TFTP_IDLE_TIMEOUT_MS)
    {
        return;
    }

    // 上位机异常退出且无STOP帧时，超时释放主控TFTP会话。
    tftp_clear_state();
}

/**
 * @brief 解析并校验TFTP WRQ报文。
 * @param pInfo 原始报文信息
 * @param info 输出的WRQ解析结果
 * @param pRawData TFTP载荷起始地址
 * @return true表示解析成功，false表示解析失败
 */
static bool parse_tftp_wrq_packet(NET_RAW_PKG_INFO *pInfo, tftp_wrq_info_t *info, const uint8_t *pRawData, uint32_t payload_len)
{
    // 提取文件名、模式和文件长度字段
    if ((info == NULL) || (pRawData == NULL) ||
        !tftp_parse_rrq_wrq(pInfo, info->filename, info->mode, info->file_len_str, pRawData, payload_len))
    {
        return false;
    }

    // 文件长度字符串转换为字节数
    info->file_len = atoi(info->file_len_str);

    // 校验范围覆盖操作码、字符串字段和各字段终止符
    uint32_t checksum_len = TFTP_OP_LEN + strlen(info->filename) + strlen(info->mode) + strlen(info->file_len_str) + 3;

    // 校验和按4字节对齐
    if (checksum_len % 4 != 0)
        checksum_len = (checksum_len / 4 + 1) * 4;

    // 读取发送方附带的校验和
    if ((checksum_len + TFTP_CKSUM_LEN) > payload_len)
    {
        emTftpErr = TFTP_ERR_WRQ_PARSE_FAIL;
        return false;
    }
    memcpy(&info->expected_checksum, pRawData + checksum_len, sizeof(info->expected_checksum));

    // 使用同一范围计算本地校验和
    if (info->expected_checksum != calculate_checksum(pRawData, checksum_len))
    {
        emTftpErr = TFTP_ERR_WRQ_CHECKSUM_FAIL;
        tftp_log_error(emTftpErr);
        return false;
    }

    return true;
}

/**
 * @brief 打开本地升级文件并设置本地升级状态。
 * @param pInfo 原始报文信息
 * @param filename 文件名
 * @return true表示准备成功，false表示准备失败
 */
static bool handle_local_update(NET_RAW_PKG_INFO *pInfo, const char *filename)
{
    EACH_FLASHFAT_STRUCT pResult = {0};
    if (GetFileAttrByName(filename, &pResult))
    {
        file_index = file_open(filename, FAT_MODE_W_OPEN);
        if (file_index < 0)
        {
            emTftpErr = TFTP_ERR_FILE_OPEN_FAIL; // 打开文件失败
            tftp_send_error(pInfo, emTftpErr);
            return false;
        }

        g_update_current_read_offset = 0;
        update_prop                  = LOCAL_UPDATE;
        return true;
    }
    else
    {
        emTftpErr = TFTP_ERR_FILE_NOT_IN_FAT; // FAT表中未找到对应的文件名
        tftp_send_error(pInfo, emTftpErr);
    }

    update_prop = ERROR;
    return false;
}

/**
 * @brief 校验TFTP会话来源IP和端口。
 * @param pRaw 原始IP报文起始地址
 * @return true表示来源匹配，false表示来源不匹配
 */
static bool tftp_validate_source(const uint8_t *pRaw)
{
    ipaddr_t ip   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    uint16_t port = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));

    if ((m_RmtTftpIP != ip) || (m_RmtTftpPort != port))
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][DROP] stage=source rx=0x%08X:%u active=0x%08X:%u state=%u block=%u\r\n",
                               ip,
                               port,
                               m_RmtTftpIP,
                               m_RmtTftpPort,
                               m_TftpState,
                               m_TftpBlock);
        emTftpErr = TFTP_ERR_IP_PORT_MISMATCH;
        tftp_log_error(emTftpErr);
        tftp_init();
        return false;
    }
    return true;
}

/**
 * @brief 解析RRQ或WRQ通用字符串字段。
 * @param pInfo 原始报文信息
 * @param filename 输出的文件名或关键字
 * @param mode 输出的传输模式
 * @param file_len 输出的文件长度字符串
 * @param pMacData TFTP载荷起始地址
 * @return true表示解析成功，false表示字段非法
 */
static bool tftp_parse_rrq_wrq(NET_RAW_PKG_INFO *pInfo,
                               char *filename,
                               char *mode,
                               char *file_len,
                               const uint8_t *pMacData,
                               uint32_t payload_len)
{
    uint16_t       i, j, k;
    uint32_t       pos;
    const uint8_t *pRecv;

    // 按字节解析TFTP字符串字段
    pRecv = (const uint8_t *)pMacData;
    if ((filename == NULL) || (mode == NULL) || (file_len == NULL) || (pRecv == NULL) ||
        (payload_len <= TFTP_OP_LEN))
    {
        emTftpErr = TFTP_ERR_WRQ_PARSE_FAIL;
        return false;
    }

    // 跳过TFTP扩展操作码区域
    pos = TFTP_OP_LEN;

    // 解析filename字段
    for (i = 0; i < (TFTP_FILENAME_LEN - 1); i++, pos++)
    {
        if (pos >= payload_len)
        {
            emTftpErr = TFTP_ERR_FILENAME_TOO_LONG;
            tftp_send_error(pInfo, emTftpErr);
            return false;
        }
        filename[i] = pRecv[pos];  // 按字节复制
        if ('\0' == filename[i]) // 文件名遇到终止符，结束
        {
            pos++;
            break;
        }
    }
    // 如果文件名未在限制长度内结束，则判定文件名超长，返回错误
    if ((TFTP_FILENAME_LEN - 1) == i)
    {
        emTftpErr = TFTP_ERR_FILENAME_TOO_LONG; // 文件名过长错误码
        tftp_send_error(pInfo, emTftpErr);
        return false;
    }

    /****************** 解析 mode 字段 ******************/
    for (j = 0; j < (TFTP_MODE_LEN - 1); j++, pos++)
    {
        if (pos >= payload_len)
        {
            emTftpErr = TFTP_ERR_MODE_TOO_LONG;
            tftp_send_error(pInfo, emTftpErr);
            return false;
        }
        mode[j] = pRecv[pos];  // 继续解析mode
        if ('\0' == mode[j]) // 模式字段结束
        {
            pos++;
            break;
        }
    }
    // 模式字段超过最大限制
    if ((TFTP_MODE_LEN - 1) == j)
    {
        emTftpErr = TFTP_ERR_MODE_TOO_LONG; // 模式过长错误码
        tftp_send_error(pInfo, emTftpErr);
        return false;
    }

    // 解析file_len字段
    for (k = 0; k < (TFTP_FILE_LEN - 1); k++, pos++)
    {
        if (pos >= payload_len)
        {
            emTftpErr = TFTP_ERR_FILELEN_TOO_LONG;
            tftp_send_error(pInfo, emTftpErr);
            return false;
        }
        file_len[k] = pRecv[pos]; // 文件长度字符串
        if ('\0' == file_len[k])
        {
            pos++;
            break;
        }
    }
    // 文件长度字段超过最大限制
    if ((TFTP_FILE_LEN - 1) == k)
    {
        emTftpErr = TFTP_ERR_FILELEN_TOO_LONG; // 文件长度字段过长错误码
        tftp_send_error(pInfo, emTftpErr);
        return false;
    }

    return true;
}

/**
 * @brief 解析扩展TFTP载荷长度。
 * @param pRaw 原始IP报文起始地址
 * @return 载荷长度
 */
static uint32_t tftp_get_data_length(const uint8_t *pRaw)
{
    uint16_t len_low  = *(const uint8_t *)(pRaw + TFTP_CANID_OFFSET + 2); // 第3字节：低8位
    uint16_t len_high = *(const uint8_t *)(pRaw + TFTP_CANID_OFFSET + 3); // 第4字节：高8位
    return (len_high << 8) | len_low;
}

/**
 * @brief 扩展16位TFTP块号到32位连续块号。
 * @param block 当前16位块号
 * @return 扩展后的连续块号
 */
static uint32_t tftp_expand_block(uint16_t block)
{
    if (block == 0x0000 && m_TftpBlock == 0xFFFF + m_TftpBlock_rlov * 0x10000)
    {
        m_TftpBlock_rlov++;
    }
    return block + m_TftpBlock_rlov * 0x10000;
}

/**
 * @brief 解析TFTP数据块号。
 * @param pRaw 原始IP报文起始地址
 * @return 块号
 */
static uint16_t tftp_get_block_number(const uint8_t *pRaw)
{
    return ntohs(tftp_read_u16(pRaw + TFTP_BLOCK_OFFSET));
}

/**
 * @brief 校验TFTP扩展帧校验和。
 * @param data TFTP载荷起始地址
 * @param len 有效载荷长度
 * @param expected 期望校验和
 * @return true表示校验通过，false表示校验失败
 */
static bool tftp_check_checksum(const uint8_t *data, uint32_t len, uint32_t expected)
{
    uint32_t aligned_len = (len % 4 == 0) ? len : (len / 4 + 1) * 4;
    uint32_t calculated  = calculate_checksum(data, aligned_len + TFTP_HEADER_LEN);
    return calculated == expected;
}

/**
 * @brief 写入或转发TFTP DATA载荷。
 * @param pData DATA数据起始地址
 * @param len DATA数据长度
 */
static bool tftp_handle_data_write(const uint8_t *pData, uint32_t len)
{
    int32_t ret;

    if ((pData == NULL) || (len == 0))
    {
        return false;
    }

    if (update_prop == LOCAL_UPDATE)
    {
        ret = file_write((uint8_t *)pData, file_offset, len, file_index);
        if (ret != (int32_t)len)
        {
            return false;
        }
        file_offset += len;
        return true;
    }
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    else if (update_prop == REMOTE_UPDATE)
    {
        if (!tftp_transfer_remote_upgrade_data((uint8_t *)pData, dst_canid, len))
        {
            return false;
        }
        // 远程升级数据转发到CAN
        return true;
    }
#endif
    return false;
}

/**
 * @brief 完成当前TFTP传输并复位会话状态。
 */
static void tftp_finish_transfer(void)
{
    g_update_state   = emTftpState;
    file_offset      = 0;
    m_TftpState      = eTftpState_Idle;
    m_TftpBlock_rlov = 0;
    g_IsUpdateFile   = 0;
}

/**
 * @brief 解析并校验TFTP RRQ报文。
 * @param pInfo 原始报文信息
 * @param info 输出的RRQ解析结果
 * @param pRawData TFTP载荷起始地址
 * @return true表示解析成功，false表示解析失败
 */
static bool parse_tftp_rrq_packet(NET_RAW_PKG_INFO *pInfo, tftp_rrq_info_t *info, const uint8_t *pRawData, uint32_t payload_len)
{
    const uint8_t *p = pRawData;
    uint32_t       pos = TFTP_OP_LEN;
    bool           keyword_done = false;
    bool           mode_done = false;

    if ((pInfo == NULL) || (info == NULL) || (pRawData == NULL) || (payload_len <= TFTP_OP_LEN))
    {
        emTftpErr = TFTP_ERR_RRQ_PARSE_FAIL;
        return false;
    }

    // 解析关键字
    int i = 0;
    while (i < (TFTP_FILENAME_LEN - 1))
    {
        if (pos >= payload_len)
        {
            emTftpErr = TFTP_ERR_FILENAME_TOO_LONG;
            return false;
        }
        info->keyword[i] = p[pos++];
        if (info->keyword[i] == '\0')
        {
            keyword_done = true;
            break;
        }
        i++;
    }
    if (!keyword_done)
    {
        emTftpErr = TFTP_ERR_FILENAME_TOO_LONG;
        return false;
    }

    // 解析模式字段
    int j = 0;
    while (j < (TFTP_MODE_LEN - 1))
    {
        if (pos >= payload_len)
        {
            emTftpErr = TFTP_ERR_MODE_TOO_LONG;
            return false;
        }
        info->mode[j] = p[pos++];
        if (info->mode[j] == '\0')
        {
            mode_done = true;
            break;
        }
        j++;
    }
    if (!mode_done)
    {
        emTftpErr = TFTP_ERR_MODE_TOO_LONG;
        return false;
    }

    // 校验范围覆盖操作码、关键字和模式字段
    uint32_t checksum_len = TFTP_OP_LEN + (strlen(info->keyword) + 1) + (strlen(info->mode) + 1);

    // 校验和按4字节对齐
    if (checksum_len % 4 != 0)
        checksum_len = (checksum_len / 4 + 1) * 4;

    // 校验和比对
    if ((checksum_len + TFTP_CKSUM_LEN) > payload_len)
    {
        emTftpErr = TFTP_ERR_RRQ_PARSE_FAIL;
        return false;
    }
    memcpy(&info->expected_checksum, pRawData + checksum_len, sizeof(info->expected_checksum));

    if (info->expected_checksum != calculate_checksum(pRawData, checksum_len))
    {
        emTftpErr = TFTP_ERR_RRQ_CHECKSUM_FAIL;
        return false;
    }

    return true;
}

/**
 * @brief 根据关键字查找可上载文件信息。
 * @param keyword 文件关键字
 * @param real_name 输出的真实文件名
 * @param real_size 输出的文件大小
 * @param real_crc 输出的文件校验和
 * @return true表示找到文件，false表示未找到
 */
static bool find_file_info_by_keyword(const char *keyword, char *real_name, uint32_t *real_size, uint32_t *real_crc)
{
    file_index = file_open(keyword, FAT_MODE_R_OPEN);

    if (file_index < 0)
        return false;

    if (!file_info_read(file_index, &info))
    {
        int32_t close_ret = file_close(file_index, 0);
        if (close_ret < 0)
        {
            NET_PROTOCOL_DEBUG_LOG("[TFTP][RRQ][CLOSE_ERR] handle=%d ret=%d\r\n", file_index, close_ret);
        }
        file_index = -1;
        return false;
    }

    //    if (info.Check_Sum != info.File_Src_Sum)
    //        return false;

    strcpy(real_name, info.Name);
    *real_size = info.Size;
    *real_crc  = info.File_Sum;

    // file_close(file_index, 0);

    return true;
}

/**
 * @brief 处理TFTP读请求。
 * @param pInfo 原始报文信息
 */
void tftp_recv_rrq(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    uint8_t *      pRawData;
    const uint8_t *pTftpBase;
    uint32_t payload_len;

    if (!tftp_get_payload_len(pInfo, &payload_len))
    {
        return;
    }
    pRaw      = (const uint8_t *)pInfo->pRawPkg;
    pRawData  = (uint8_t *)(pRaw + TFTP_DATA_OFFSET);
    pTftpBase = pRaw + TFTP_CANID_OFFSET;

    // 提取目标CAN ID和目标IP
    uint8_t  dst_id = pTftpBase[0];
    ipaddr_t dst_ip = tftp_read_u32(pRaw + IP_DST_OFFSET);

    // 初始化会话环境
    tftp_init();
    m_RmtTftpIP   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    m_RmtTftpPort = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));
    tftp_capture_session_context(pInfo);
    dst_canid     = dst_id;
    self_canid    = net_profile_calc_master_canid();

    // 解析RRQ载荷
    tftp_rrq_info_t info = {0};
    if (!parse_tftp_rrq_packet(pInfo, &info, pRawData, payload_len))
    {
        emTftpErr = TFTP_ERR_RRQ_PARSE_FAIL;
        goto ERRDONE;
    }

    // 按目标ID判断本机处理或转发到IO板卡
    if (ipaddr_cmp(dst_ip, self_ipaddr) ||
        (net_profile_enable_networking() && ipaddr_cmp(dst_ip, net_ipaddr)))
    {
        // 主控CPU范围内只允许本机CAN ID处理
        if (dst_id >= 0x41 && dst_id <= 0x6F)
        {
            if (dst_id == self_canid)
            {
                update_prop                           = LOCAL_UPDATE;
                char     real_name[TFTP_FILENAME_LEN] = {0};
                uint32_t real_size                    = 0;
                uint32_t real_crc                     = 0;

                // 本地文件检索
                if (!find_file_info_by_keyword(info.keyword, real_name, &real_size, &real_crc))
                {
                    emTftpErr = TFTP_ERR_FILE_NOT_FOUND;
                    goto ERRDONE;
                }

                // 状态机切换到读取
                m_TftpState = eTftpState_Rrq;
                emTftpState = TFTP_STATE_READING;

                // 回复FIF帧给上位机
                tftp_send_fif(pInfo, real_name, real_size, real_crc);

                // 广播当前读取状态
                tftp_send_state(emTftpState, real_size, 0, 0);
            }
            else
            {
                emTftpErr = TFTP_ERR_DST_ID_NOT_SELF;
                goto ERRDONE;
            }
        }
        // IO板卡和面板范围通过CAN转发
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
        else if (dst_id >= 0x01 && dst_id <= DOWN_FILE_PANEL_CAN_ID)
        {
            update_prop = REMOTE_UPDATE;

            // RRQ请求透传给IO板卡，本核心作为网关转发响应
            if (!tftp_transfer_remote_upgrade_data(pRawData, dst_id, payload_len))
            {
                emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
                goto ERRDONE;
            }
        }
#endif
        else
        {
            emTftpErr = TFTP_ERR_INVALID_DST_ID;
            goto ERRDONE;
        }
    }
    else
    {
        emTftpErr = TFTP_ERR_IP_MISMATCH;
        goto ERRDONE;
    }

    return;

ERRDONE:
    tftp_send_error(pInfo, emTftpErr);
    tftp_clear_state();
}


/**
 * @brief 发送TFTP文件信息帧。
 * @param pInfo 原始报文信息
 * @param name 文件名
 * @param size 文件大小
 * @param crc 文件校验和
 */
void tftp_send_fif(NET_RAW_PKG_INFO *pInfo, const char *name, uint32_t size, uint32_t crc)
{
    uint16_t  nLength,      udpchecksum;
    uint32_t  report_len,   frame_cs;
    uint8_t * pSend    = eth_send_buff;
    uint8_t * pPayloadStart;
    net_send_route_t send_route = {0};
    ipaddr_t                  src_ip;
    bool                      is_networking = false;
    char                      len_str[32];

    memset(eth_send_buff, 0, sizeof(eth_send_buff));
    src_ip = tftp_get_session_src_ip(&is_networking);
    snprintf(len_str, sizeof(len_str), "%u", size);

    pSend         += NET_IP_HEAD_LEN;
    pPayloadStart = pSend + 8;
    pSend         = pPayloadStart;

    // 填充FIF载荷
    *pSend++ = 0x7D;      // CAN目的ID
    *pSend++ = dst_canid; // CAN源ID
    *pSend++ = 0;         // 固定长度字段
    *pSend++ = 0;         // 备用

    *(uint16_t *)pSend = htons(16); // FIF操作码
    pSend              += 2;

    memcpy(pSend, name, strlen(name)); // 文件名
    pSend    += strlen(name);
    *pSend++ = 0; // 字符串结束符

    memcpy(pSend, len_str, strlen(len_str)); // 文件长度ASCII
    pSend    += strlen(len_str);
    *pSend++ = 0; // 字符串结束符

    *(uint32_t *)pSend = htonl(crc); // 文件校验和
    pSend              += 4;

    *pSend++ = 0;
    // 4字节对齐补零
    uintptr_t pad = (4 - ((uintptr_t)pSend % 4)) % 4;
    if (pad)
    {
        memset(pSend, 0, pad);
        pSend += pad;
    }

    // 帧校验和从CAN ID字段开始计算
    frame_cs           = calculate_checksum(pPayloadStart, (uint32_t)(pSend - pPayloadStart));
    *(uint32_t *)pSend = htonl(frame_cs);
    pSend              += 4;

    // 封装IP和UDP头部
    uint16_t total_payload_len = (uint16_t)(pSend - pPayloadStart);
    nLength                    = NET_IP_HEAD_LEN + TFTP_HEADER_LEN + total_payload_len;
    ip_head((uint32_t *)eth_send_buff, &src_ip, &m_RmtTftpIP, nLength, CFG_PROTO_UDP);

    uint8_t * pUdp          = eth_send_buff + NET_IP_HEAD_LEN;
    *(uint16_t *)(pUdp + 0) = htons(m_TftpTID);
    *(uint16_t *)(pUdp + 2) = htons(m_RmtTftpPort);
    *(uint16_t *)(pUdp + 4) = htons(UDP_LEN(total_payload_len));
    *(uint16_t *)(pUdp + 6) = 0;

    udpchecksum = udp_checksum(eth_send_buff, (TFTP_HEADER_LEN + total_payload_len));

    *(uint16_t *)(pUdp + 6) = htons(udpchecksum);

    report_len                             = (uint32_t)(pSend - eth_send_buff);
    net_port_make_route_from_packet(pInfo, &send_route);

    if (!tftp_send_frame(is_networking, report_len, &send_route, pInfo->rmtMac))
    {
        return;
    }
}

/**
 * @brief 处理TFTP写请求并记录下载会话初始信息。
 * @param pInfo 原始报文信息
 */
void tftp_recv_wrq(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    uint8_t *      pRawData;
    uint32_t payload_len;

    if (!tftp_get_payload_len(pInfo, &payload_len))
    {
        return;
    }
    pRaw     = (const uint8_t *)pInfo->pRawPkg;
    pRawData = (uint8_t *)(pRaw + TFTP_DATA_OFFSET);

    // 提取目标CAN ID和目标IP
    uint8_t dst_id = pRaw[TFTP_CANID_OFFSET];
    ipaddr_t dst_ip    = tftp_read_u32(pRaw + IP_DST_OFFSET);

    // 初始化会话环境
    tftp_init();
    m_RmtTftpIP   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    m_RmtTftpPort = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));
    tftp_capture_session_context(pInfo);
    dst_canid     = dst_id;
    self_canid    = net_profile_calc_master_canid();

    // 解析WRQ载荷
    tftp_wrq_info_t info = {0};
    if (!parse_tftp_wrq_packet(pInfo, &info, pRawData, payload_len))
    {
        emTftpErr = TFTP_ERR_WRQ_PARSE_FAIL;
        goto ERRDONE;
    }

    // 按目标ID判断本机更新或转发到IO板卡
    if (ipaddr_cmp(dst_ip, self_ipaddr) ||
        (net_profile_enable_networking() && ipaddr_cmp(dst_ip, net_ipaddr)))
    {
        if (dst_id >= 0x41 && dst_id <= 0x6F)
        {
            if (dst_id == self_canid)
            {
                g_IsUpdateFile = 1;

                // 本地更新
                if (!handle_local_update(pInfo, info.filename))
                {
                    emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
                    goto ERRDONE;
                }
            }
            else
            {
                emTftpErr = TFTP_ERR_DST_ID_NOT_SELF;
                goto ERRDONE;
            }
        }
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
        else if (dst_id >= 0x01 && dst_id <= DOWN_FILE_PANEL_CAN_ID)
        {
            g_IsUpdateFile = 1;

            // 转发到IO板卡
            update_prop = REMOTE_UPDATE;
            if (!tftp_transfer_remote_upgrade_data(pRawData, dst_id, payload_len))
            {
                emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
                goto ERRDONE;
            }
        }
#endif
        else
        {
            emTftpErr = TFTP_ERR_INVALID_DST_ID;
            goto ERRDONE;
        }
    }
    else
    {
        emTftpErr = TFTP_ERR_IP_MISMATCH;
        goto ERRDONE;
    }

    // 状态机切换与回复
    m_TftpState = eTftpState_Wrq;
    m_TftpBlock = 0;

    if (update_prop == LOCAL_UPDATE)
    {
        tftp_send_ack(pInfo, 0);
    }

    // 更新全局状态并广播
    emTftpState                   = TFTP_STATE_WRITING;
    g_update_state                = emTftpState;
    g_update_total_size           = info.file_len;
    g_update_current_read_offset  = 0;
    g_update_current_write_offset = 0;

#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
    NET_PROTOCOL_DEBUG_LOG("[TFTP][WRQ] file=%s size=%u src_ip=0x%08X src_port=%u dst_id=0x%02X handle=%d\r\n",
               info.filename,
               info.file_len,
               m_RmtTftpIP,
               m_RmtTftpPort,
               dst_id,
               file_index);
#endif

    tftp_send_state(emTftpState, g_update_total_size, 0, 0);
    return;

ERRDONE:
    tftp_send_error(pInfo, emTftpErr);
    tftp_clear_state();
}

/**
 * @brief 发送TFTP数据块。
 * @param pInfo 原始报文信息
 * @param block_num 数据块号
 */
void tftp_send_data(NET_RAW_PKG_INFO *pInfo, uint16_t block_num)
{
    // 状态检查
    if (m_TftpState != eTftpState_Rrq)
        return;

    uint16_t  nLength,                                udpchecksum;
    uint32_t  report_len,                             tftp_cs;
    uint8_t * pSend                              = eth_send_buff;
    net_send_route_t send_route = {0};
    ipaddr_t                  src_ip;
    bool                      is_networking = false;

    // 根据文件大小计算当前块实际读取长度
    uint32_t offset    = (block_num - 1) * 1024;
    uint32_t read_size = 0;

    // 判断文件是否还有剩余数据
    if (info.Size > offset)
    {
        read_size = info.Size - offset;
        if (read_size > 1024)
        {
            read_size = 1024; // 单块最大读取长度
        }
    }
    else
    {
        // 文件大小正好为块大小整数倍时发送0字节结束包
        read_size = 0;
    }

    uint8_t f_buf[1024] = {0};
    int32_t r_len       = 0;

    // 只有需要读取数据时才调用底层读取函数
    if (read_size > 0)
    {
        r_len = file_read(f_buf, offset, read_size, file_index);
        if (r_len < 0)
        {
            tftp_send_error(pInfo, TFTP_ERR_FILE_READ_FAIL);
            return;
        }
    }
    // 初始化发送缓冲区
    memset(eth_send_buff, 0, sizeof(eth_send_buff));

    // 选择源IP和发送环境
    src_ip = tftp_get_session_src_ip(&is_networking);

    // 计算TFTP载荷长度
    int      pad              = (4 - (r_len % 4)) % 4;     // 4字节对齐补零长度
    uint32_t tftp_payload_len = 8 + (uint32_t)r_len + pad; // CAN头、TFTP头、数据和补零
    uint32_t total_tftp_len   = tftp_payload_len + 4;      // 附加帧校验和

    // 构造IP头部
    nLength = NET_IP_HEAD_LEN + CFG_UDPH_LEN + total_tftp_len;
    ip_head((uint32_t *)pSend, &src_ip, &m_RmtTftpIP, nLength, CFG_PROTO_UDP);
    pSend += NET_IP_HEAD_LEN;

    // 构造UDP头部
    *(uint16_t *)(pSend + 0) = htons(m_TftpTID);               // 源端口
    *(uint16_t *)(pSend + 2) = htons(m_RmtTftpPort);           // 目的端口
    *(uint16_t *)(pSend + 4) = htons(UDP_LEN(total_tftp_len)); // UDP 载荷长度
    *(uint16_t *)(pSend + 6) = 0;                              // 校验和占位
    pSend                    += 8;

    uint8_t * pTftpBase = pSend; // TFTP帧校验起始地址

    // 填充CAN控制字段
    pSend[0] = 0x7D;                           // 目的 ID
    pSend[1] = dst_canid;                      // 源 ID
    pSend[2] = (uint8_t)(r_len & 0xFF);        // 数据长度低 8 位 (可能为 0)
    pSend[3] = (uint8_t)((r_len >> 8) & 0xFF); // 数据长度高 8 位
    pSend    += 4;

    // 填充TFTP操作码和块号
    *(uint16_t *)(pSend + 0) = htons(TFTP_DATA);
    *(uint16_t *)(pSend + 2) = htons(block_num);
    pSend                    += 4;

    // 填充文件数据并补齐对齐字节
    if (r_len > 0)
    {
        memcpy(pSend, f_buf, r_len);
        pSend += r_len;
    }

    if (pad > 0)
    {
        memset(pSend, 0, pad);
        pSend += pad;
    }

    // 计算TFTP帧校验和
    tftp_cs            = calculate_checksum(pTftpBase, tftp_payload_len);
    *(uint32_t *)pSend = htonl(tftp_cs);
    pSend              += 4;

    // 更新UDP校验和
    udpchecksum                                        = udp_checksum(eth_send_buff, (CFG_UDPH_LEN + total_tftp_len));
    *(uint16_t *)(eth_send_buff + NET_IP_HEAD_LEN + 6) = htons(udpchecksum);

    // 发送处理
    report_len                             = (uint32_t)(pSend - eth_send_buff);
    net_port_make_route_from_packet(pInfo, &send_route);

    if (!tftp_send_frame(is_networking, report_len, &send_route, pInfo->rmtMac))
    {
        // 最后一块未成功入队时保留文件和RRQ状态，等待客户端重试ACK
        return;
    }

    // 小于单块长度表示最后一包
    if (r_len < 1024)
    {
        int32_t close_ret = file_close(file_index, 0);
        if (close_ret < 0)
        {
            NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][CLOSE_ERR] handle=%d ret=%d\r\n", file_index, close_ret);
            return;
        }
        file_index  = -1;
        m_TftpState = eTftpState_Idle;
    }
}

/**
 * @brief 处理TFTP数据帧，使用稳定副本写入本地文件并记录分包诊断信息。
 * @param pInfo 原始报文信息
 */
void tftp_recv_data(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    const uint8_t *pRawData;

    uint16_t block             = 0U;
    uint32_t block_32          = 0U;
    uint32_t len               = 0U;
    uint32_t checksum_len      = 0U;
    uint32_t expected_checksum = 0U;
    uint32_t payload_len       = 0U;

    if (!tftp_get_payload_len(pInfo, &payload_len) || (payload_len < (TFTP_HEADER_LEN + TFTP_CKSUM_LEN)))
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][DROP] stage=payload payload=%u min=%u\r\n",
                               payload_len,
                               TFTP_HEADER_LEN + TFTP_CKSUM_LEN);
        return;
    }
    pRaw     = (const uint8_t *)pInfo->pRawPkg;
    pRawData = pRaw + TFTP_DATA_OFFSET;

    // 校验来源和当前状态
    if (!tftp_validate_source(pRaw))
        goto ERRDONE;

    if (m_TftpState != eTftpState_Wrq)
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][DROP] stage=state state=%u expected=%u last_block=%u\r\n",
                               m_TftpState,
                               eTftpState_Wrq,
                               m_TftpBlock);
        emTftpErr = TFTP_ERR_NOT_WRQ_STATE;
        goto ERRDONE;
    }

    // 获取数据长度与块号
    len      = tftp_get_data_length(pRaw);
    block    = tftp_get_block_number(pRaw);
    block_32 = tftp_expand_block(block);

    // 校验和验证
    checksum_len      = TFTP_HEADER_LEN + len;
    if ((((checksum_len + 3) & ~3U) + TFTP_CKSUM_LEN) > payload_len)
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][DROP] stage=checksum_bounds block=%u len=%u checksum_len=%u payload=%u\r\n",
                               block_32,
                               len,
                               checksum_len,
                               payload_len);
        emTftpErr = TFTP_ERR_DATA_CHECKSUM_FAIL;
        goto ERRDONE;
    }
    expected_checksum = tftp_read_u32(pRawData + ((checksum_len + 3) & ~3));

    if (!tftp_check_checksum(pRawData, len, expected_checksum))
    {
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
        uint32_t aligned_len        = (len + 3U) & ~3U;
        uint32_t calculated_checksum = calculate_checksum(pRawData, aligned_len + TFTP_HEADER_LEN);
        NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][CHECKSUM_ERR] block=%u expanded=%u len=%u expected=0x%08X calculated=0x%08X payload=%u\r\n",
                   block,
                   block_32,
                   len,
                   expected_checksum,
                   calculated_checksum,
                   payload_len);
#endif
        emTftpErr = TFTP_ERR_DATA_CHECKSUM_FAIL;
        goto ERRDONE;
    }

    // 块号逻辑处理
    if (block_32 == m_TftpBlock + 1) // 正常顺序块
    {
        if (update_prop == LOCAL_UPDATE)
        {
            // 更新统计并写入本地文件
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
            uint32_t write_offset_before = file_offset;
#endif
            if (len > sizeof(m_TftpDataBuffer))
            {
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
                NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][LENGTH_ERR] block=%u len=%u max=%u\r\n",
                           block_32,
                           len,
                           (uint32_t)sizeof(m_TftpDataBuffer));
#endif
                emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
                goto ERRDONE;
            }

            const uint8_t *source_data = pRaw + TFTP_RDATA_OFFSET;
            uint32_t block_checksum    = calculate_checksum(source_data, len);

            // Flash操作耗时较长，先复制数据，避免接收DMA复用原报文缓冲区
            memcpy(m_TftpDataBuffer, source_data, len);
            uint32_t copied_checksum = calculate_checksum(m_TftpDataBuffer, len);
            if (copied_checksum != block_checksum)
            {
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
                NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][COPY_CHANGED] block=%u len=%u source=0x%08X copy=0x%08X\r\n",
                           block_32,
                           len,
                           block_checksum,
                           copied_checksum);
#endif
                // 不推进块号且不回复ACK，等待上位机重发当前块
                return;
            }

            m_TftpBlock = block_32;
            g_update_current_write_offset += len;
            // g_update_current_read_offset = g_update_current_write_offset;

            if (!tftp_handle_data_write(m_TftpDataBuffer, len))
            {
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
                NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][WRITE_ERR] block=%u len=%u offset=%u handle=%d\r\n",
                           block_32,
                           len,
                           write_offset_before,
                           file_index);
#endif
                emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
                goto ERRDONE;
            }
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
            uint32_t block_checksum_after = calculate_checksum(source_data, len);
            if (block_checksum_after != block_checksum)
            {
                NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][SOURCE_CHANGED] block=%u len=%u offset=%u before=0x%08X after=0x%08X copy=0x%08X\r\n",
                           block_32,
                           len,
                           write_offset_before,
                           block_checksum,
                           block_checksum_after,
                           copied_checksum);
            }
#endif
            file_checksum_total += copied_checksum;
            m_TftpDiagDataBlocks++;
            m_TftpDiagDataBytes += len;
            m_TftpDiagLastBlockChecksum = copied_checksum;
            m_TftpDiagLastBlockLength   = len;

#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
            if (((m_TftpDiagDataBlocks % TFTP_DIAG_REPORT_BLOCKS) == 0U) || (len < TFTP_BLOCK_LEN))
            {
                NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA] block=%u len=%u offset=%u->%u block_sum=0x%08X total_sum=0x%08X bytes=%u/%u duplicate=%u\r\n",
                           block_32,
                           len,
                           write_offset_before,
                           file_offset,
                           block_checksum,
                           file_checksum_total,
                           m_TftpDiagDataBytes,
                           g_update_total_size,
                           m_TftpDiagDuplicateBlocks);
            }
#endif

            if (len < TFTP_BLOCK_LEN)
            {
                tftp_finish_transfer();
            }

            tftp_send_ack(pInfo, block);
            return;
        }
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
        else  // 远程IO转发
        {
            m_TftpBlock = block_32;
            uint32_t r_len                = payload_len;
            g_update_current_write_offset += r_len;
            if (!tftp_handle_data_write(pRawData, r_len))
            {
                emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
                goto ERRDONE;
            }
            return;
        }
#endif
    }
    if (block_32 == m_TftpBlock) // 重复块
    {
        if (update_prop == LOCAL_UPDATE)
        {
            m_TftpDiagDuplicateBlocks++;
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
            uint32_t duplicate_checksum = calculate_checksum(pRaw + TFTP_RDATA_OFFSET, len);
            NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][DUPLICATE] block=%u len=%u duplicate=%u offset=%u frame_sum=0x%08X last_len=%u last_sum=0x%08X match=%u\r\n",
                       block_32,
                       len,
                       m_TftpDiagDuplicateBlocks,
                       file_offset,
                       duplicate_checksum,
                       m_TftpDiagLastBlockLength,
                       m_TftpDiagLastBlockChecksum,
                       ((len == m_TftpDiagLastBlockLength) &&
                        (duplicate_checksum == m_TftpDiagLastBlockChecksum)) ? 1U : 0U);
#endif
            tftp_send_ack(pInfo, block);
        }
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
        else if (update_prop == REMOTE_UPDATE)
        {
            // 重复包仍透传给IO板卡
            uint32_t r_len = payload_len;
            if (!tftp_handle_data_write(pRawData, r_len))
            {
                emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
                goto ERRDONE;
            }
        }
#endif
        return;
    }
    // 块号不连续
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
    NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][BLOCK_ERR] block=%u expanded=%u expected=%u last=%u len=%u bytes=%u\r\n",
               block,
               block_32,
               m_TftpBlock + 1U,
               m_TftpBlock,
               len,
               m_TftpDiagDataBytes);
#endif
    emTftpErr = TFTP_ERR_BLOCK_NUM_NOT_SEQ;
    goto ERRDONE;

ERRDONE:
    NET_PROTOCOL_DEBUG_LOG("[TFTP][DATA][ERROR] err=%u state=%u block=%u last=%u payload=%u update=%u\r\n",
                           emTftpErr,
                           m_TftpState,
                           block_32,
                           m_TftpBlock,
                           payload_len,
                           update_prop);
    tftp_send_error(pInfo, emTftpErr);
    tftp_clear_state();
}

/**
 * @brief 处理TFTP确认帧。
 * @param pInfo 原始报文信息
 */
void tftp_recv_ack(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    const uint8_t *pTftpBase;
    uint32_t       payload_len = 0U;

    if (!tftp_get_payload_len(pInfo, &payload_len) || (payload_len < (TFTP_HEADER_LEN + TFTP_CKSUM_LEN)))
    {
        return;
    }
    pRaw      = (const uint8_t *)pInfo->pRawPkg;
    pTftpBase = pRaw + TFTP_CANID_OFFSET;

    // 提取目标CAN ID和网络信息
    uint8_t dst_id = pTftpBase[0];
    m_RmtTftpIP    = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    m_RmtTftpPort  = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));

    self_canid = net_profile_calc_master_canid();

    // ACK帧固定载荷为8字节
    uint32_t checksum_len = 8;
    uint32_t expected_cs  = tftp_read_u32(pTftpBase + checksum_len);

    if (expected_cs != calculate_checksum(pTftpBase, checksum_len))
    {
        emTftpErr = TFTP_ERR_ACK_CHECKSUM_FAIL;
        goto ERRDONE;
    }

    // 主控CPU范围内由本地RRQ状态处理
    if (dst_id >= 0x41 && dst_id <= 0x6F)
    {
        if (dst_id == self_canid)
        {
            // 仅在本地读取状态处理ACK
            if (m_TftpState == eTftpState_Rrq)
            {
                update_prop = LOCAL_UPDATE;
                // 提取大端块编号
                uint16_t block_num = (uint16_t)((pTftpBase[6] << 8) | pTftpBase[7]);

                // 收到块N的ACK后发送第N+1块
                tftp_send_data(pInfo, block_num + 1);
            }
        }
        else
        {
            emTftpErr = TFTP_ERR_DST_ID_NOT_SELF;
            goto ERRDONE;
        }
    }
    // IO板卡和面板范围通过CAN转发
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    else if (dst_id >= 0x01 && dst_id <= DOWN_FILE_PANEL_CAN_ID)
    {
        uint8_t * pRawData = (uint8_t *)(pRaw + TFTP_DATA_OFFSET);
        update_prop        = REMOTE_UPDATE;

        // ACK透传给IO板卡，后续DATA由主控封装回UDP
        if (!tftp_transfer_remote_upgrade_data(pRawData, dst_id, payload_len))
        {
            emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
            goto ERRDONE;
        }
    }
#endif
    else
    {
        emTftpErr = TFTP_ERR_INVALID_DST_ID;
        goto ERRDONE;
    }

    return;

ERRDONE:
    tftp_send_error(pInfo, emTftpErr);
    tftp_clear_state();
}

/**
 * @brief 发送TFTP确认帧并记录发送队列结果。
 * @param pInfo 原始报文信息
 * @param block 确认的块号
 */
static void tftp_send_ack(NET_RAW_PKG_INFO *pInfo, uint16_t block)
{
    uint16_t  nLength,                                udpchecksum;
    uint32_t  report_len,                             tftp_cs;
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
    int32_t   send_ret = 0;
#endif
    uint8_t * pSend                              = eth_send_buff;
    net_send_route_t send_route = {0};
    ipaddr_t                  src_ip;
    bool                      is_networking = false;

    // 初始化缓冲区
    memset(eth_send_buff, 0, sizeof(eth_send_buff));

    // 选择源IP和发送环境
    src_ip = tftp_get_session_src_ip(&is_networking);

    // 构造IP头部
    nLength = NET_IP_HEAD_LEN + CFG_UDPH_LEN + TFTP_ACK_LEN + TFTP_CKSUM_LEN;
    ip_head((uint32_t *)pSend, &src_ip, &m_RmtTftpIP, nLength, CFG_PROTO_UDP);
    pSend += NET_IP_HEAD_LEN;

    // 构造UDP头部
    *(uint16_t *)(pSend + 0) = htons(m_TftpTID);                              // 源端口
    *(uint16_t *)(pSend + 2) = htons(m_RmtTftpPort);                          // 目的端口
    *(uint16_t *)(pSend + 4) = htons(UDP_LEN(TFTP_ACK_LEN + TFTP_CKSUM_LEN)); // 长度
    *(uint16_t *)(pSend + 6) = 0;                                             // 校验和占位
    pSend                    += 8;

    // 填充CAN控制字段
    pSend[0]                 = 0x7D;      // 固定目的ID
    pSend[1]                 = dst_canid; // 源ID
    *(uint16_t *)(pSend + 2) = 0;
    pSend                    += 4;

    // 填充TFTP ACK内容
    *(uint16_t *)(pSend + 0) = htons(TFTP_ACK);
    *(uint16_t *)(pSend + 2) = htons(block);
    pSend                    += 4;

    // 计算TFTP帧校验和
    tftp_cs            = calculate_checksum(eth_send_buff + NET_IP_HEAD_LEN + CFG_UDPH_LEN, TFTP_ACK_LEN);
    *(uint32_t *)pSend = htonl(tftp_cs);
    pSend              += 4;

    // 更新UDP校验和
    udpchecksum                                        = udp_checksum(eth_send_buff, (CFG_UDPH_LEN + TFTP_ACK_LEN + TFTP_CKSUM_LEN));
    *(uint16_t *)(eth_send_buff + NET_IP_HEAD_LEN + 6) = htons(udpchecksum);

    // 发送处理
    report_len                             = (uint32_t)(pSend - eth_send_buff);
    net_port_make_route_from_packet(pInfo, &send_route);

    // 按发送环境分流
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
    send_ret = tftp_send_frame(is_networking, report_len, &send_route, pInfo->rmtMac) ? 1 : 0;
    if ((send_ret <= 0) || ((block % TFTP_DIAG_REPORT_BLOCKS) == 0U))
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][ACK] block=%u ret=%d path=%s slot=%u port=%u type=0x%02X remote_port=%u\r\n",
                   block,
                   send_ret,
                   is_networking ? "networking" : "raw",
                   pInfo->Src_Slot_ID,
                   pInfo->Src_Port_ID,
                   pInfo->PortType,
                   m_RmtTftpPort);
    }
#else
    (void)tftp_send_frame(is_networking, report_len, &send_route, pInfo->rmtMac);
#endif
}

/**
 * @brief 将已组装的TFTP报文提交到对应以太网发送队列。
 * @param is_networking true表示组网发送，false表示普通网口发送
 * @param report_len 报文长度
 * @param pRoute 统一发送路由
 * @param pDstMac 目的MAC地址
 * @return true表示成功入队，false表示发送队列拒绝报文
 */
static bool tftp_send_frame(bool is_networking,
                            uint32_t report_len,
                            const net_send_route_t *pRoute,
                            const uint8_t *pDstMac)
{
    int32_t send_ret;

    if (is_networking)
    {
        send_ret = net_port_send_networking(eth_send_buff,
                                            report_len,
                                            pDstMac,
                                            CFG_ETHTYPE_IP,
                                            pRoute);
    }
    else
    {
        send_ret = net_port_send_raw(eth_send_buff,
                                     report_len,
                                     pDstMac,
                                     CFG_ETHTYPE_IP,
                                     pRoute);
    }

    if (send_ret <= 0)
    {
        net_protocol_debug.err_location = 0x07;
        net_protocol_debug.err_number++;
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
        NET_PROTOCOL_DEBUG_LOG("[TFTP][SEND_ERR] networking=%u len=%u ret=%d\r\n",
                   is_networking ? 1U : 0U,
                   report_len,
                   send_ret);
        if (pRoute != NULL)
        {
            NET_PROTOCOL_DEBUG_LOG("[TFTP][SEND_ERR] route type=0x%02X slot=%u port=%u mask=0x%04X\r\n",
                                   pRoute->port_type,
                                   pRoute->slot_id,
                                   pRoute->port_id,
                                   (pRoute->slot_id < ARRAYSIZEOF(pRoute->eth_mask))
                                       ? pRoute->eth_mask[pRoute->slot_id]
                                       : 0U);
        }
#endif
        return false;
    }

    return true;
}

/**
 * @brief 处理TFTP总校验帧，支持帧校验和成功响应丢失后的重复确认。
 * @param pInfo 原始报文信息
 */
void tftp_recv_sum(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    const uint8_t *pRawData;
    uint32_t       payload_len;
    uint32_t       checksum_offset;
    uint32_t       expected_checksum;

    checksum_offset = (TFTP_SUM_OFFSET + sizeof(uint32_t) + 3U) & ~3U;
    if (!tftp_get_payload_len(pInfo, &payload_len) ||
        (payload_len < (checksum_offset + TFTP_CKSUM_LEN)))
    {
        return;
    }
    pRaw     = (const uint8_t *)pInfo->pRawPkg;
    pRawData = pRaw + TFTP_DATA_OFFSET;

    // SUM帧校验失败时保留当前文件状态，等待上位机重发
    expected_checksum = tftp_read_u32(pRawData + checksum_offset);
    if (!tftp_check_checksum(pRawData, sizeof(uint32_t), expected_checksum))
    {
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
        uint32_t calculated_checksum = calculate_checksum(pRawData, checksum_offset);
        NET_PROTOCOL_DEBUG_LOG("[TFTP][SUM][CHECKSUM_ERR] expected=0x%08X calculated=0x%08X payload=%u\r\n",
                   expected_checksum,
                   calculated_checksum,
                   payload_len);
#endif
        return;
    }

    m_RmtTftpIP   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    m_RmtTftpPort = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));

    // 提取文件总校验和
    uint32_t tftp_sum = tftp_read_u32(pRawData + TFTP_SUM_OFFSET);

    if (update_prop == LOCAL_UPDATE)
    {
        // 首次SUM已处理成功时，重复SUM只需再次回复OK
        if (m_TftpLastSumSuccess && (tftp_sum == m_TftpLastSuccessSum))
        {
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
            NET_PROTOCOL_DEBUG_LOG("[TFTP][SUM][RETRY_OK] remote=0x%08X bytes=%u last_block=%u\r\n",
                       tftp_sum,
                       m_TftpDiagDataBytes,
                       m_TftpBlock);
#endif
            (void)tftp_send_ok(pInfo);
            return;
        }

#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
        NET_PROTOCOL_DEBUG_LOG("[TFTP][SUM] remote=0x%08X local=0x%08X size=%u bytes=%u last_block=%u blocks=%u duplicate=%u\r\n",
                   tftp_sum,
                   file_checksum_total,
                   g_update_total_size,
                   m_TftpDiagDataBytes,
                   m_TftpBlock,
                   m_TftpDiagDataBlocks,
                   m_TftpDiagDuplicateBlocks);
#endif

        // 校验和匹配后关闭文件并回复OK
        if (tftp_sum == file_checksum_total)
        {
            int32_t close_ret = file_close(file_index, tftp_sum);
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
            NET_PROTOCOL_DEBUG_LOG("[TFTP][SUM][CLOSE] handle=%d ret=%d remote=0x%08X bytes=%u\r\n",
                       file_index,
                       close_ret,
                       tftp_sum,
                       m_TftpDiagDataBytes);
#endif
            if (close_ret <= 0)
            {
                NET_PROTOCOL_DEBUG_LOG("[TFTP][SUM][CLOSE_ERR] handle=%d ret=%d remote=0x%08X local=0x%08X bytes=%u\r\n",
                           file_index,
                           close_ret,
                           tftp_sum,
                           file_checksum_total,
                           m_TftpDiagDataBytes);
                emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
                goto ERRDONE;
            }

            emTftpState = TFTP_STATE_WRITE_DONE;
            tftp_send_state(emTftpState, g_update_total_size, g_update_current_read_offset, g_update_current_write_offset);
            m_TftpLastSuccessSum = tftp_sum;
            m_TftpLastSumSuccess = true;
            if (!tftp_send_ok(pInfo))
            {
                // 文件已可靠关闭，保留成功校验信息供重复SUM再次触发OK回复
                file_checksum_total = 0;
                file_index          = -1;
                return;
            }
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
            tftp_open_rst_window();
#endif

            // 清理本地校验状态
            file_checksum_total = 0;
            file_index          = -1;
            return;
        }
        // 校验和不匹配
#if (NET_PROTOCOL_DEBUG_LOG_ENABLE == 1U)
        NET_PROTOCOL_DEBUG_LOG("[TFTP][SUM][MISMATCH] remote=0x%08X local=0x%08X diff=0x%08X size=%u bytes=%u\r\n",
                   tftp_sum,
                   file_checksum_total,
                   tftp_sum - file_checksum_total,
                   g_update_total_size,
                   m_TftpDiagDataBytes);
#endif
        {
            int32_t close_ret = file_close(file_index, 0);
            if (close_ret < 0)
            {
                NET_PROTOCOL_DEBUG_LOG("[TFTP][SUM][ABORT_CLOSE_ERR] handle=%d ret=%d\r\n", file_index, close_ret);
            }
            file_index = -1;
        }
        emTftpErr = TFTP_ERR_FILE_CHECKSUM_FAIL;
        goto ERRDONE;
    }
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    // 远程升级时透传SUM载荷
    if (!tftp_handle_data_write(pRawData, payload_len))
    {
        emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
        goto ERRDONE;
    }
    return;
#endif

ERRDONE:
    tftp_send_error(pInfo, emTftpErr);
    tftp_clear_state();
}

/**
 * @brief 发送TFTP成功帧。
 * @param pInfo 原始报文信息
 * @return true表示成功入队，false表示发送失败
 */
static bool tftp_send_ok(NET_RAW_PKG_INFO *pInfo)
{
    uint16_t  nLength,                                udpchecksum;
    uint32_t  report_len,                             tftp_cs;
    uint8_t * pSend                              = eth_send_buff;
    net_send_route_t send_route = {0};
    ipaddr_t                  src_ip;
    bool                      is_networking = false;

    // 初始化缓冲区
    memset(eth_send_buff, 0, sizeof(eth_send_buff));

    // 选择源IP和发送环境
    src_ip = tftp_get_session_src_ip(&is_networking);

    // 构造IP头部
    nLength = NET_IP_HEAD_LEN + CFG_UDPH_LEN + TFTP_OK_LEN + TFTP_CKSUM_LEN;
    ip_head((uint32_t *)pSend, &src_ip, &m_RmtTftpIP, nLength, CFG_PROTO_UDP);
    pSend += NET_IP_HEAD_LEN;

    // 构造UDP头部
    *(uint16_t *)(pSend + 0) = htons(m_TftpTID);
    *(uint16_t *)(pSend + 2) = htons(m_RmtTftpPort);
    *(uint16_t *)(pSend + 4) = htons(UDP_LEN(TFTP_OK_LEN + TFTP_CKSUM_LEN));
    *(uint16_t *)(pSend + 6) = 0;
    pSend                    += 8;

    // 填充CAN控制字段
    pSend[0]                 = 0x7D;
    pSend[1]                 = dst_canid;
    *(uint16_t *)(pSend + 2) = 0;
    pSend                    += 4;

    // 填充TFTP OK内容
    *(uint16_t *)(pSend + 0) = htons(TFTP_OK); // 操作码
    *(uint16_t *)(pSend + 2) = 0;              // 填充
    pSend                    += 4;

    // 计算TFTP帧校验和
    tftp_cs            = calculate_checksum(eth_send_buff + NET_IP_HEAD_LEN + CFG_UDPH_LEN, TFTP_OK_LEN);
    *(uint32_t *)pSend = htonl(tftp_cs);
    pSend              += 4;

    // 更新UDP校验和
    udpchecksum                                        = udp_checksum(eth_send_buff, (CFG_UDPH_LEN + TFTP_OK_LEN + TFTP_CKSUM_LEN));
    *(uint16_t *)(eth_send_buff + NET_IP_HEAD_LEN + 6) = htons(udpchecksum);

    // 发送处理
    report_len                             = (uint32_t)(pSend - eth_send_buff);
    net_port_make_route_from_packet(pInfo, &send_route);

    // 按发送环境分流
    return tftp_send_frame(is_networking, report_len, &send_route, pInfo->rmtMac);
}

/**
 * @brief 发送TFTP错误帧。
 * @param pInfo 原始报文信息
 * @param err 错误码
 */
static void tftp_send_error(NET_RAW_PKG_INFO *pInfo, emTFTP_ERR_INFO err)
{
    uint16_t  nLength,                                udpchecksum;
    uint32_t  report_len,                             tftp_cs;
    uint8_t * pSend                              = eth_send_buff;
    net_send_route_t send_route = {0};
    ipaddr_t                  src_ip;
    bool                      is_networking = false;

    // 初始化缓冲区
    memset(eth_send_buff, 0, sizeof(eth_send_buff));

    // 选择源IP和发送环境
    src_ip = tftp_get_session_src_ip(&is_networking);

    // 组装IP头部
    nLength = NET_IP_HEAD_LEN + CFG_UDPH_LEN + TFTP_ERR_LEN + TFTP_CKSUM_LEN;
    ip_head((uint32_t *)pSend, &src_ip, &m_RmtTftpIP, nLength, CFG_PROTO_UDP);
    pSend += NET_IP_HEAD_LEN;

    // 组装UDP头部
    *(uint16_t *)(pSend + 0) = htons(m_TftpTID);                              // 源端口
    *(uint16_t *)(pSend + 2) = htons(m_RmtTftpPort);                          // 目的端口
    *(uint16_t *)(pSend + 4) = htons(UDP_LEN(TFTP_ERR_LEN + TFTP_CKSUM_LEN)); // 长度
    *(uint16_t *)(pSend + 6) = 0;                                             // 校验和占位
    pSend                    += 8;

    // 填充CAN控制字段
    pSend[0]                 = 0x7D;      // 目的ID
    pSend[1]                 = dst_canid; // 源ID
    *(uint16_t *)(pSend + 2) = 0;
    pSend                    += 4;

    // 填充TFTP ERROR内容
    *(uint16_t *)(pSend + 0) = htons(TFTP_ERROR);
    *(uint16_t *)(pSend + 2) = htons((uint16_t)err);
    pSend                    += 4;

    // 计算TFTP帧校验和
    tftp_cs            = calculate_checksum(eth_send_buff + NET_IP_HEAD_LEN + CFG_UDPH_LEN, TFTP_ERR_LEN);
    *(uint32_t *)pSend = htonl(tftp_cs);
    pSend              += 4;

    // 更新UDP校验和
    udpchecksum                                        = udp_checksum(eth_send_buff, (CFG_UDPH_LEN + TFTP_ERR_LEN + TFTP_CKSUM_LEN));
    *(uint16_t *)(eth_send_buff + NET_IP_HEAD_LEN + 6) = htons(udpchecksum);

    // 发送处理
    report_len                             = (uint32_t)(pSend - eth_send_buff);
    net_port_make_route_from_packet(pInfo, &send_route);

    // 错误帧发送失败也由调用方继续清理本地会话
    (void)tftp_send_frame(is_networking, report_len, &send_route, pInfo->rmtMac);
}

/**
 * @brief 发送TFTP升级状态帧。
 * @param state 当前升级状态
 * @param total_size 文件总大小
 * @param current_read_offset 当前读取偏移
 * @param current_write_offset 当前写入偏移
 */
void tftp_send_state(uint16_t state, uint32_t total_size, uint32_t current_read_offset, uint32_t current_write_offset)
{
    ethaddr_t ethaddr;
    uint16_t  nLength,                                udpchecksum;
    uint32_t  report_len,                             tftp_cs;
    uint32_t  PortType                           = 0, Src_Slot_ID = 0, Src_Port_ID = 0;
    uint8_t * pSend                              = eth_send_buff;
    net_send_route_t send_route = {0};
    ipaddr_t                  src_ip;
    bool                      is_networking;

    src_ip = tftp_get_session_src_ip(&is_networking);

    // 缓冲区清零
    memset(eth_send_buff, 0, sizeof(eth_send_buff));

    // 解析目标MAC
    if (!arp_check(&ethaddr, &m_RmtTftpIP, &PortType, &Src_Slot_ID, &Src_Port_ID))
    {
        arp_request(&m_RmtTftpIP, m_TftpEthPhyNo);
        iptoeth(&ethaddr, &m_RmtTftpIP);
    }

    // 组装IP头部
    nLength = NET_IP_HEAD_LEN + CFG_UDPH_LEN + TFTP_STATE_LEN + TFTP_CKSUM_LEN;
    ip_head((uint32_t *)pSend, &src_ip, &m_RmtTftpIP, nLength, CFG_PROTO_UDP);
    pSend += NET_IP_HEAD_LEN;

    // 组装UDP头部
    *(uint16_t *)(pSend + 0) = htons(m_TftpTID);                                // 源端口
    *(uint16_t *)(pSend + 2) = htons(m_RmtTftpPort);                            // 目的端口
    *(uint16_t *)(pSend + 4) = htons(UDP_LEN(TFTP_STATE_LEN + TFTP_CKSUM_LEN)); // UDP 长度
    *(uint16_t *)(pSend + 6) = 0;                                               // 校验和占位
    pSend                    += 8;

    // 填充CAN控制字段
    *pSend                   = 0x7D;      // 目的ID
    *(pSend + 1)             = dst_canid; // 源ID
    *(uint16_t *)(pSend + 2) = 0;         // 长度与预留字段
    pSend                    += 4;

    // 组装TFTP STATE载荷
    *(uint16_t *)(pSend + 0)  = htons(TFTP_STATE);
    *(uint16_t *)(pSend + 2)  = htons(state);
    *(uint32_t *)(pSend + 4)  = htonl(total_size);
    *(uint32_t *)(pSend + 8)  = htonl(current_read_offset);
    *(uint32_t *)(pSend + 12) = htonl(current_write_offset);
    pSend                     += 16;

    // 计算TFTP帧校验和
    tftp_cs            = calculate_checksum(eth_send_buff + NET_IP_HEAD_LEN + CFG_UDPH_LEN, TFTP_STATE_LEN);
    *(uint32_t *)pSend = htonl(tftp_cs);
    pSend              += 4;

    // 更新UDP校验和
    udpchecksum                                        = udp_checksum(eth_send_buff, (CFG_UDPH_LEN + TFTP_STATE_LEN + TFTP_CKSUM_LEN));
    *(uint16_t *)(eth_send_buff + NET_IP_HEAD_LEN + 6) = htons(udpchecksum);

    // 计算最终报文长度
    report_len = (uint32_t)(pSend - eth_send_buff);

    // 发送数据
    send_route.port_type = PortType;
    send_route.slot_id   = Src_Slot_ID;
    send_route.port_id   = Src_Port_ID;
    if (Src_Slot_ID < MAX_ENET_SLOT_NUMBER)
    {
        send_route.eth_mask[Src_Slot_ID] = (uint16_t)(1U << (Src_Port_ID & 0xF));
    }

    (void)tftp_send_frame(is_networking, report_len, &send_route, ethaddr.addr);
}

/**
 * @brief 处理TFTP链路检测帧。
 * @param pInfo 原始报文信息
 */
void tftp_recv_lck(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    const uint8_t *pTftpBase;
    uint32_t       payload_len;

    if (!tftp_get_payload_len(pInfo, &payload_len) || (payload_len < (TFTP_HEADER_LEN + TFTP_CKSUM_LEN)))
    {
        return;
    }
    pRaw      = (const uint8_t *)pInfo->pRawPkg;
    pTftpBase = pRaw + TFTP_CANID_OFFSET;

    // 初始化会话并提取网络信息
    tftp_init();
    m_RmtTftpIP   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    m_RmtTftpPort = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));
    tftp_capture_session_context(pInfo);

    // 提取目标CAN ID
    uint8_t dst_id = pTftpBase[0];
    dst_canid      = dst_id;
    self_canid     = net_profile_calc_master_canid();

    // LCK帧固定载荷为8字节
    uint32_t checksum_len = 8;
    uint32_t expected_cs  = tftp_read_u32(pTftpBase + checksum_len);

    if (expected_cs != calculate_checksum(pTftpBase, checksum_len))
    {
        emTftpErr = TFTP_ERR_LCK_CHECKSUM_FAIL;
        goto ERRDONE;
    }

    // 主控CPU范围内由本机响应
    if (dst_id >= 0x41 && dst_id <= 0x6F)
    {
        if (dst_id == self_canid)
        {
            // 本机链路检测成功，回复LOK
            update_prop = LOCAL_UPDATE;
            tftp_send_lok(pInfo);
        }
        else
        {
            emTftpErr = TFTP_ERR_DST_ID_NOT_SELF;
            goto ERRDONE;
        }
    }
    // IO板卡和面板范围通过CAN转发
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    else if (dst_id >= 0x01 && dst_id <= DOWN_FILE_PANEL_CAN_ID)
    {
        uint8_t * pRawData = (uint8_t *)(pRaw + TFTP_DATA_OFFSET);

        update_prop = REMOTE_UPDATE;
        if (!tftp_transfer_remote_upgrade_data(pRawData, dst_id, payload_len))
        {
            emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
            goto ERRDONE;
        }
    }
#endif
    else
    {
        emTftpErr = TFTP_ERR_INVALID_DST_ID;
        goto ERRDONE;
    }

    return;

ERRDONE:
    tftp_send_error(pInfo, emTftpErr);
    tftp_clear_state();
}

/**
 * @brief 发送TFTP链路检测成功帧。
 * @param pInfo 原始报文信息
 */
static void tftp_send_lok(NET_RAW_PKG_INFO *pInfo)
{
    uint16_t  nLength,                                udpchecksum;
    uint32_t  report_len,                             tftp_cs;
    uint8_t * pSend                              = eth_send_buff;
    net_send_route_t send_route = {0};
    ipaddr_t                  src_ip;
    bool                      is_networking = false;

    // 初始化发送缓冲区
    memset(eth_send_buff, 0, sizeof(eth_send_buff));

    // 选择源IP和发送环境
    src_ip = tftp_get_session_src_ip(&is_networking);

    // 组装IP头部
    nLength = NET_IP_HEAD_LEN + CFG_UDPH_LEN + TFTP_LOK_LEN + TFTP_CKSUM_LEN;
    ip_head((uint32_t *)pSend, &src_ip, &m_RmtTftpIP, nLength, CFG_PROTO_UDP);
    pSend += NET_IP_HEAD_LEN;

    // 组装UDP头部
    *(uint16_t *)(pSend + 0) = htons(m_TftpTID);                              // 源端口
    *(uint16_t *)(pSend + 2) = htons(m_RmtTftpPort);                          // 目的端口
    *(uint16_t *)(pSend + 4) = htons(UDP_LEN(TFTP_LOK_LEN + TFTP_CKSUM_LEN)); // 长度
    *(uint16_t *)(pSend + 6) = 0;                                             // 校验和占位
    pSend                    += 8;

    // 填充CAN控制字段
    pSend[0]                 = 0x7D;      // 目的ID
    pSend[1]                 = dst_canid; // 源ID
    *(uint16_t *)(pSend + 2) = 0;
    pSend                    += 4;

    // 组装操作码与对齐字段
    *(uint16_t *)(pSend + 0) = htons(TFTP_LOK);
    *(uint16_t *)(pSend + 2) = 0; // 填充
    pSend                    += 4;

    // 计算TFTP帧校验和
    tftp_cs            = calculate_checksum(eth_send_buff + NET_IP_HEAD_LEN + CFG_UDPH_LEN, TFTP_LOK_LEN);
    *(uint32_t *)pSend = htonl(tftp_cs);
    pSend              += 4;

    // 更新UDP校验和
    udpchecksum                                        = udp_checksum(eth_send_buff, (CFG_UDPH_LEN + TFTP_LOK_LEN + TFTP_CKSUM_LEN));
    *(uint16_t *)(eth_send_buff + NET_IP_HEAD_LEN + 6) = htons(udpchecksum);

    // 发送处理
    report_len                             = (uint32_t)(pSend - eth_send_buff);
    net_port_make_route_from_packet(pInfo, &send_route);

    // 按发送环境分流
    (void)tftp_send_frame(is_networking, report_len, &send_route, pInfo->rmtMac);
}

/**
 * @brief 处理TFTP增量下载查询帧。
 * @param pInfo 原始报文信息
 */
void tftp_recv_diff(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    const uint8_t *pTftpBase;
    uint32_t       payload_len;

    if (!tftp_get_payload_len(pInfo, &payload_len))
    {
        return;
    }
    pRaw      = (const uint8_t *)pInfo->pRawPkg;
    pTftpBase = pRaw + TFTP_CANID_OFFSET;

    tftp_init();

    // 提取网络与目标CAN ID信息
    m_RmtTftpIP   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    m_RmtTftpPort = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));
    tftp_capture_session_context(pInfo);

    uint8_t dst_id = pTftpBase[0];
    dst_canid      = dst_id;
    self_canid     = net_profile_calc_master_canid();

    // 预解析文件名以确定校验和位置
    char     req_filename[FILE_NAME_LEN] = {0};
    uint32_t checksum_len                = 6; // CAN头(4) + Opcode(2)
    int      i;

    for (i = 0; i < FILE_NAME_LEN; i++)
    {
        if ((6U + (uint32_t)i) >= payload_len)
        {
            emTftpErr = TFTP_ERR_FILENAME_TOO_LONG;
            goto ERRDONE;
        }
        req_filename[i] = (char)pTftpBase[6 + i];
        if (req_filename[i] == '\0')
        {
            checksum_len += (i + 1);
            break;
        }
    }

    if (i == FILE_NAME_LEN)
    {
        emTftpErr = TFTP_ERR_FILENAME_TOO_LONG;
        goto ERRDONE;
    }

    // 校验和按4字节对齐
    int pad = checksum_len % 4;
    if (pad != 0)
    {
        checksum_len += (4 - pad);
    }

    // 提取并比对校验和
    if ((checksum_len + TFTP_CKSUM_LEN) > payload_len)
    {
        emTftpErr = TFTP_ERR_DIFF_CHECKSUM_FAIL;
        goto ERRDONE;
    }
    uint32_t expected_cs = tftp_read_u32(pTftpBase + checksum_len);
    if (expected_cs != calculate_checksum(pTftpBase, checksum_len))
    {
        emTftpErr = TFTP_ERR_DIFF_CHECKSUM_FAIL;
        goto ERRDONE;
    }

    // 主控CPU范围内由本机查询
    if (dst_id >= 0x41 && dst_id <= 0x6F)
    {
        if (dst_id == self_canid)
        {
            update_prop              = LOCAL_UPDATE;
            uint32_t  local_file_len = 0, local_file_cs = 0;
            FILE_INFO cfg_file_info  = {0};

            if (req_filename[0] != '\0')
            {
                int32_t handle = file_open(req_filename, FAT_MODE_R_OPEN);
                if (handle >= 0)
                {
                    if (file_info_read(handle, &cfg_file_info) == 1 && cfg_file_info.Use_Flag == 0xA5)
                    {
                        if (cfg_file_info.File_Sum == cfg_file_info.File_Src_Sum)
                        {
                            local_file_len = cfg_file_info.Size;
                            local_file_cs  = cfg_file_info.File_Sum;
                        }
                    }
                    {
                        int32_t close_ret = file_close(handle, 0);
                        if (close_ret < 0)
                        {
                            NET_PROTOCOL_DEBUG_LOG("[TFTP][DIFF][CLOSE_ERR] handle=%d ret=%d\r\n", handle, close_ret);
                        }
                    }
                }
            }
            // 回复本地查询结果
            tftp_send_dres(pInfo, local_file_len, local_file_cs);
        }
        else
        {
            emTftpErr = TFTP_ERR_DST_ID_NOT_SELF;
            goto ERRDONE;
        }
    }
    // IO板卡和面板范围通过CAN转发
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    else if (dst_id >= 0x01 && dst_id <= DOWN_FILE_PANEL_CAN_ID)
    {
        uint8_t * pRawData = (uint8_t *)(pRaw + TFTP_DATA_OFFSET);
        update_prop        = REMOTE_UPDATE;
        // 转发原始TFTP载荷
        if (!tftp_transfer_remote_upgrade_data(pRawData, dst_id, payload_len))
        {
            emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
            goto ERRDONE;
        }
    }
#endif
    else
    {
        emTftpErr = TFTP_ERR_INVALID_DST_ID;
        goto ERRDONE;
    }

    return;

ERRDONE:
    tftp_send_error(pInfo, emTftpErr);
    tftp_clear_state();
}

/**
 * @brief 发送TFTP增量下载查询响应帧。
 * @param pInfo 原始报文信息
 * @param file_len 文件长度
 * @param file_chksum 文件校验和
 */
static void tftp_send_dres(NET_RAW_PKG_INFO *pInfo, uint32_t file_len, uint32_t file_chksum)
{
    uint16_t  nLength,                                udpchecksum;
    uint32_t  report_len,                             tftp_cs;
    uint8_t * pSend                              = eth_send_buff;
    net_send_route_t send_route = {0};
    ipaddr_t                  src_ip;
    bool                      is_networking = false;

    // 初始化发送缓冲区
    memset(eth_send_buff, 0, sizeof(eth_send_buff));

    // 选择源IP和发送环境
    src_ip = tftp_get_session_src_ip(&is_networking);

    // 组装IP头部
    nLength = (uint16_t)(NET_IP_HEAD_LEN + CFG_UDPH_LEN + TFTP_DRES_LEN + TFTP_CKSUM_LEN);
    ip_head((uint32_t *)pSend, &src_ip, &m_RmtTftpIP, nLength, CFG_PROTO_UDP);
    pSend += NET_IP_HEAD_LEN;

    // 组装UDP头部
    *(uint16_t *)(pSend + 0) = htons(m_TftpTID);
    *(uint16_t *)(pSend + 2) = htons(m_RmtTftpPort);
    *(uint16_t *)(pSend + 4) = htons((uint16_t)UDP_LEN(TFTP_DRES_LEN + TFTP_CKSUM_LEN));
    *(uint16_t *)(pSend + 6) = 0; // 校验和占位
    pSend                    += 8;

    // 填充CAN控制字段
    pSend[0]                 = 0x7D;      // 目的ID
    pSend[1]                 = dst_canid; // 源ID
    *(uint16_t *)(pSend + 2) = 0;
    pSend                    += 4;

    // 组装TFTP DRES载荷
    *(uint16_t *)(pSend + 0)  = htons(TFTP_DRES);
    *(uint32_t *)(pSend + 2)  = htonl(file_len);    // 文件长度
    *(uint32_t *)(pSend + 6)  = htonl(file_chksum); // 校验和
    *(uint16_t *)(pSend + 10) = 0;                  // 2字节填充对齐
    pSend                     += 12;

    // 计算TFTP帧校验和
    tftp_cs            = calculate_checksum(eth_send_buff + NET_IP_HEAD_LEN + CFG_UDPH_LEN, TFTP_DRES_LEN);
    *(uint32_t *)pSend = htonl(tftp_cs);
    pSend              += 4;

    // 更新UDP校验和
    udpchecksum                                        = udp_checksum(eth_send_buff, (uint16_t)(CFG_UDPH_LEN + TFTP_DRES_LEN + TFTP_CKSUM_LEN));
    *(uint16_t *)(eth_send_buff + NET_IP_HEAD_LEN + 6) = htons(udpchecksum);

    // 发送处理
    report_len                             = (uint32_t)(pSend - eth_send_buff);
    net_port_make_route_from_packet(pInfo, &send_route);

    // 按发送环境分流
    (void)tftp_send_frame(is_networking, report_len, &send_route, pInfo->rmtMac);
}

/**
 * @brief 处理TFTP通信终止帧。
 * @param pInfo 原始报文信息
 */
static void tftp_recv_stop(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    const uint8_t *pTftpBase;
    uint32_t       payload_len;

    if (!tftp_get_payload_len(pInfo, &payload_len) || (payload_len < (TFTP_HEADER_LEN + TFTP_CKSUM_LEN)))
    {
        return;
    }
    pRaw      = (const uint8_t *)pInfo->pRawPkg;
    pTftpBase = pRaw + TFTP_CANID_OFFSET;

    // 记录来源信息，便于错误回复
    m_RmtTftpIP   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    m_RmtTftpPort = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));
    tftp_capture_session_context(pInfo);

    uint8_t dst_id = pTftpBase[0];
    dst_canid      = dst_id;
    self_canid     = net_profile_calc_master_canid();

    // STOP帧固定载荷为8字节
    uint32_t checksum_len = 8;
    uint32_t expected_cs  = tftp_read_u32(pTftpBase + checksum_len);

    if (expected_cs != calculate_checksum(pTftpBase, checksum_len))
    {
        emTftpErr = TFTP_ERR_STOP_CHECKSUM_FAIL;
        goto ERRDONE;
    }

    // 统一清理会话状态
    tftp_clear_state();

    return;

ERRDONE:
    tftp_send_error(pInfo, emTftpErr);
    tftp_clear_state();
    g_IsUpdateFile = 0; // 出错时强制复位升级标志
}

/**
 * @brief 停止喂狗接口。
 */
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
static void tftp_stop_feed_watchdog(void)
{
    m_TftpStopFeedWatchdogFlag = 1;
    m_TftpRstWindowMs          = 0;
}
#endif

/**
 * @brief 获取TFTP停止喂狗标志。
 * @return 1表示停止喂狗，0表示未停止喂狗
 */
uint8_t tftp_get_stop_feed_watchdog_flag(void)
{
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    return m_TftpStopFeedWatchdogFlag;
#else
    return 0;
#endif
}

#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
/**
 * @brief 打开升级完成后的重启帧有效窗口。
 */
static void tftp_open_rst_window(void)
{
    m_TftpRstWindowMs = TFTP_RST_VALID_WINDOW_MS;
}

/**
 * @brief 判断重启帧是否处于有效窗口内。
 * @return true表示重启帧有效，false表示重启帧无效
 */
static bool tftp_rst_window_is_valid(void)
{
    return (m_TftpRstWindowMs > 0U);
}

/**
 * @brief 处理TFTP板卡重启帧。
 * @param pInfo 原始报文信息
 */
static void tftp_recv_rst(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    const uint8_t *pTftpBase;
    uint32_t       payload_len;

    if (!tftp_get_payload_len(pInfo, &payload_len) || (payload_len < (TFTP_RST_LEN + TFTP_CKSUM_LEN)))
    {
        return;
    }
    pRaw      = (const uint8_t *)pInfo->pRawPkg;
    pTftpBase = pRaw + TFTP_CANID_OFFSET;

    m_RmtTftpIP   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    m_RmtTftpPort = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));
    tftp_capture_session_context(pInfo);

    uint8_t dst_id = pTftpBase[0];
    dst_canid      = dst_id;
    self_canid     = net_profile_calc_master_canid();

    uint32_t checksum_len = TFTP_RST_LEN;
    uint32_t expected_cs  = tftp_read_u32(pTftpBase + checksum_len);

    if (expected_cs != calculate_checksum(pTftpBase, checksum_len))
    {
        emTftpErr = TFTP_ERR_RST_CHECKSUM_FAIL;
        goto ERRDONE;
    }

    // 主控CPU范围内由本机处理
    if (dst_id >= 0x41 && dst_id <= 0x6F)
    {
        if (dst_id == self_canid)
        {
            // 仅允许升级完成后5秒内的重启帧触发复位。
            if (tftp_rst_window_is_valid())
            {
                tftp_stop_feed_watchdog();
            }
        }
        else
        {
            emTftpErr = TFTP_ERR_DST_ID_NOT_SELF;
            goto ERRDONE;
        }
    }
    // IO板卡和面板范围通过CAN转发
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    else if (dst_id >= 0x01 && dst_id <= DOWN_FILE_PANEL_CAN_ID)
    {
        uint8_t *pRawData = (uint8_t *)(pRaw + TFTP_DATA_OFFSET);

        if (!tftp_transfer_remote_upgrade_data(pRawData, dst_id, payload_len))
        {
            emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
            goto ERRDONE;
        }
    }
#endif
    else
    {
        emTftpErr = TFTP_ERR_INVALID_DST_ID;
        goto ERRDONE;
    }

    return;

ERRDONE:
    tftp_send_error(pInfo, emTftpErr);
    tftp_clear_state();
    g_IsUpdateFile = 0;
}
#endif

#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
/**
 * @brief 处理IO板卡升级请求转发帧。
 * @param pInfo 原始报文信息
 */
void tftp_recv_req(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    uint32_t       payload_len;

    if (!tftp_get_payload_len(pInfo, &payload_len))
    {
        return;
    }
    pRaw = (const uint8_t *)pInfo->pRawPkg;

    // 提取目标CAN ID
    dst_canid = pRaw[TFTP_CANID_OFFSET];

    // 初始化远程升级环境
    tftp_init();
    update_prop = REMOTE_UPDATE;

    // 保存远程传输端网络信息
    m_RmtTftpIP   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
    m_RmtTftpPort = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));
    tftp_capture_session_context(pInfo);

    // 传输数据到IO模块
    if (!tftp_transfer_remote_upgrade_data((uint8_t *)(pRaw + TFTP_DATA_OFFSET), dst_canid, payload_len))
    {
        emTftpErr = TFTP_ERR_LOCAL_UPDATE_FAIL;
    }
}

/**
 * @brief 将CAN侧响应封装为TFTP报文发送。
 * @param data CAN侧响应载荷
 * @param len 载荷长度
 */
void tftp_can_send_packet(const uint8_t *data, uint16_t len)
{
    ethaddr_t ethaddr;
    uint16_t  opcode = 0;
    uint16_t  nLength, udpchecksum;
    uint32_t  report_len;
    uint32_t  PortType                           = 0, Src_Slot_ID = 0, Src_Port_ID = 0;
    uint8_t * pSend                              = eth_send_buff;
    net_send_route_t send_route = {0};
    uint32_t                  src_ip;
    bool                      is_networking;
    bool                      is_remote_update_done = false;

    if (data == NULL || len == 0)
        return;

    tftp_refresh_activity();

    if (len >= TFTP_OP_LEN)
    {
        opcode                = ntohs(tftp_read_u16(data + 4));
        is_remote_update_done = ((update_prop == REMOTE_UPDATE) &&
                                 ((opcode == TFTP_OK) || (opcode == TFTP_ERROR) || (opcode == TFTP_ROK)));
    }

    // 缓冲区清零
    memset(eth_send_buff, 0, sizeof(eth_send_buff));

    // 解析目标MAC
    if (!arp_check(&ethaddr, &m_RmtTftpIP, &PortType, &Src_Slot_ID, &Src_Port_ID))
    {
        arp_request(&m_RmtTftpIP, m_TftpEthPhyNo);
        iptoeth(&ethaddr, &m_RmtTftpIP);
    }

    // 确定源IP地址
    src_ip = tftp_get_session_src_ip(&is_networking);

    // 组装IP头部
    nLength = (uint16_t)(NET_IP_HEAD_LEN + CFG_UDPH_LEN + len);
    ip_head((uint32_t *)pSend, &src_ip, &m_RmtTftpIP, nLength, CFG_PROTO_UDP);
    pSend += NET_IP_HEAD_LEN;

    // 组装UDP头部
    *(uint16_t *)(pSend + 0) = htons(m_TftpTID);                      // 本地源端口
    *(uint16_t *)(pSend + 2) = htons(m_RmtTftpPort);                  // 目的端口
    *(uint16_t *)(pSend + 4) = htons((uint16_t)(CFG_UDPH_LEN + len)); // UDP长度字段
    *(uint16_t *)(pSend + 6) = 0;                                     // 校验和占位
    pSend                    += 8;

    // 拷贝TFTP载荷
    memcpy(pSend, data, len);
    pSend += len;

    // 计算UDP校验和
    udpchecksum                                        = udp_checksum(eth_send_buff, (uint16_t)(CFG_UDPH_LEN + len));
    *(uint16_t *)(eth_send_buff + NET_IP_HEAD_LEN + 6) = htons(udpchecksum);

    // 计算总报文长度
    report_len = (uint32_t)(pSend - eth_send_buff);

    // 填充发送端口配置
    send_route.port_type = PortType;
    send_route.slot_id   = Src_Slot_ID;
    send_route.port_id   = Src_Port_ID;
    if (Src_Slot_ID < MAX_ENET_SLOT_NUMBER)
    {
        send_route.eth_mask[Src_Slot_ID] = (uint16_t)(1U << (Src_Port_ID & 0xF));
    }

    // 终止性响应未成功入队时保留会话，等待IO板卡或上位机重试
    if (!tftp_send_frame(is_networking, report_len, &send_route, ethaddr.addr))
    {
        return;
    }

    if (is_remote_update_done)
    {
        if (opcode == TFTP_OK)
        {
            tftp_open_rst_window();
        }

        // IO板卡终止性响应已回复上位机，释放主控TFTP会话。
        tftp_clear_state();
    }
}
#endif

/**
 * @brief 处理TFTP报文并对可回复的协议异常发送错误帧。
 * @param pInfo 原始报文信息
 * @param nEthNo 逻辑网口号
 */
void tftp_server(NET_RAW_PKG_INFO *pInfo, uint16_t nEthNo)
{
    const uint8_t *pRaw;
    uint16_t       opcode;
    uint32_t       payload_len = 0U;
    uint8_t        dst_id = 0;
    emTFTP_ERR_INFO server_err = TFTP_ERR_NONE;

    // IP和UDP头不完整时无法获取可靠的回复地址，只能丢弃报文
    if ((pInfo == NULL) || (pInfo->pRawPkg == NULL) ||
        (pInfo->nLength < (NET_IP_HEAD_LEN + CFG_UDPH_LEN)))
    {
        NET_PROTOCOL_DEBUG_LOG("[TFTP][DROP] stage=server_input info_null=%u raw_null=%u frame_len=%u\r\n",
                               (pInfo == NULL) ? 1U : 0U,
                               ((pInfo == NULL) || (pInfo->pRawPkg == NULL)) ? 1U : 0U,
                               (pInfo != NULL) ? pInfo->nLength : 0U);
        return;
    }
    pRaw = (const uint8_t *)pInfo->pRawPkg;

    self_canid = net_profile_calc_master_canid();
    dst_id    = self_canid;

    if (!tftp_get_payload_len(pInfo, &payload_len) ||
        (payload_len < (TFTP_HEADER_LEN + TFTP_CKSUM_LEN)))
    {
        server_err = TFTP_ERR_DATA_LEN_OUT_RANGE;
        goto SEND_ERROR;
    }

    tftp_refresh_activity();

    dst_id = pRaw[TFTP_CANID_OFFSET];

    // 非本核目标报文静默丢弃，避免组网场景多核心抢先回复错误帧
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    // 核0允许处理本机ID、IO板卡ID和面板ID
    if ((dst_id >= 0x41) && (dst_id <= 0x6F) && (dst_id != self_canid))
    {
        return;
    }
    if (((dst_id < 0x01) || (dst_id > DOWN_FILE_PANEL_CAN_ID)) && (dst_id != self_canid))
    {
        return;
    }
#else
    // 其他核心只处理本机ID
    if (dst_id != self_canid)
    {
        return;
    }
#endif

    // 提取操作码并处理字节序
    memcpy(&opcode, pRaw + TFTP_OPCODE_OFFSET, sizeof(opcode));
    opcode = ntohs(opcode);

    if (((opcode == TFTP_RRQ) || (opcode == TFTP_WRQ) || (opcode == TFTP_LCK) || (opcode == TFTP_DIFF)
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
         || (opcode == TFTP_REQ)
#endif
             ) &&
        tftp_reject_new_session_if_busy(pInfo, dst_id))
    {
        return;
    }

    m_TftpEthPhyNo = nEthNo;

    // 判断组网发送环境
    is_group_net = tftp_packet_is_networking(pInfo);

    // 按操作码分发
    switch (opcode)
    {
    case TFTP_RRQ: // 处理读请求
        tftp_recv_rrq(pInfo);
        break;

    case TFTP_WRQ: // 处理写请求
        tftp_recv_wrq(pInfo);
        break;

    case TFTP_DATA: // 处理数据帧
        tftp_recv_data(pInfo);
        break;

    case TFTP_ACK: // 处理确认帧
        tftp_recv_ack(pInfo);
        break;

    case TFTP_SUM: // 处理校验和帧
        tftp_recv_sum(pInfo);
        break;

    case TFTP_LCK: // 处理链路检查帧
        tftp_recv_lck(pInfo);
        break;

    case TFTP_DIFF: // 处理差异检查帧
        tftp_recv_diff(pInfo);
        break;
    case TFTP_STOP: // 处理通信终止帧
        tftp_recv_stop(pInfo);
        break;

#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
    case TFTP_RST: // 处理板卡重启帧
        tftp_recv_rst(pInfo);
        break;

    case TFTP_REQ: // 处理升级请求帧
        tftp_recv_req(pInfo);
        break;
#endif

    default: // 未知操作码返回协议错误
        server_err = TFTP_ERR_OPCODE_INVALID;
        goto SEND_ERROR;
    }

    return;

SEND_ERROR:
    NET_PROTOCOL_DEBUG_LOG("[TFTP][SERVER_ERROR] err=%u dst=0x%02X self=0x%02X payload=%u state=%u\r\n",
                           server_err,
                           dst_id,
                           self_canid,
                           payload_len,
                           m_TftpState);
    {
        ipaddr_t old_rmt_ip       = m_RmtTftpIP;
        ipaddr_t old_src_ip       = m_TftpSrcIP;
        uint16_t old_rmt_port     = m_RmtTftpPort;
        uint16_t old_eth_phy_no   = m_TftpEthPhyNo;
        uint8_t  old_dst_canid    = dst_canid;
        bool     old_is_group_net = m_TftpIsGroupNet;
        bool     old_group_net    = is_group_net;

        // 使用当前异常报文建立临时回复上下文，避免破坏正在进行的TFTP会话
        m_RmtTftpIP   = tftp_read_u32(pRaw + IP_SRC_OFFSET);
        m_RmtTftpPort = ntohs(tftp_read_u16(pRaw + UDP_SRCPORT_OFFSET));
        m_TftpEthPhyNo = nEthNo;
        dst_canid      = dst_id;
        tftp_capture_session_context(pInfo);

        emTftpErr = server_err;
        tftp_send_error(pInfo, emTftpErr);

        m_RmtTftpIP      = old_rmt_ip;
        m_RmtTftpPort    = old_rmt_port;
        m_TftpSrcIP      = old_src_ip;
        m_TftpEthPhyNo   = old_eth_phy_no;
        dst_canid        = old_dst_canid;
        m_TftpIsGroupNet = old_is_group_net;
        is_group_net     = old_group_net;
    }
}
