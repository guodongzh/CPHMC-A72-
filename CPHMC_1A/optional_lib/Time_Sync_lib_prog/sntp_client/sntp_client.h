#ifndef __SNTP_CLIENT_H__
#define __SNTP_CLIENT_H__

#include "net_all_include.h"

#ifdef __cplusplus
extern "C" {
#endif

// SNTP 端口
#define SNTP_PORT 123

// SNTP 模式
#define SNTP_MODE_CLIENT 3  // 客户端模式
#define SNTP_MODE_SERVER 4  // 服务器模式

// NTP 时间戳偏差 (1900年到1970年的秒数)
#define NTP_TIMESTAMP_DELTA 2208988800u

// SNTP 报文结构体
typedef struct
{
    uint8_t li_vn_mode;      // 闰秒指示符(LI), 版本号(VN), 模式(Mode)
    uint8_t stratum;         // 层级 (Stratum level)
    uint8_t poll;            // 轮询间隔 (Poll interval)
    uint8_t precision;       // 精度 (Precision)
    uint32_t root_delay;     // 根延迟 (Root delay)
    uint32_t root_dispersion;// 根离散度 (Root dispersion)
    uint32_t reference_id;   // 参考时钟标识符 (Reference ID)
    uint32_t ref_ts_sec;     // 参考时间戳 秒 (Reference Timestamp seconds)
    uint32_t ref_ts_frac;    // 参考时间戳 小数 (Reference Timestamp fraction)
    uint32_t orig_ts_sec;    // 原始时间戳 秒 (Origin Timestamp seconds)
    uint32_t orig_ts_frac;   // 原始时间戳 小数 (Origin Timestamp fraction)
    uint32_t recv_ts_sec;    // 接收时间戳 秒 (Receive Timestamp seconds)
    uint32_t recv_ts_frac;   // 接收时间戳 小数 (Receive Timestamp fraction)
    uint32_t trans_ts_sec;   // 发送时间戳 秒 (Transmit Timestamp seconds)
    uint32_t trans_ts_frac;  // 发送时间戳 小数 (Transmit Timestamp fraction)
} sntp_msg_t;

// 函数原型
void sntp_init(void);       // SNTP 初始化
void sntp_poll(void);       // SNTP 轮询任务
bool sntp_is_valid(void);   // 检查 SNTP 是否有效

#ifdef __cplusplus
}
#endif

#endif // __SNTP_CLIENT_H__
