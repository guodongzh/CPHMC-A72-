/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_tftp_err.h
*@author     xuesen
*@date       2026.05.06
*@brief      TFTP扩展升级错误码定义。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

#ifndef __NET_TFTP_ERR_H__INCLUDE__
#define __NET_TFTP_ERR_H__INCLUDE__

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/


#define MAX_ERR_INFO_LEN 64

// TFTP错误码
#if 1


/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

typedef enum
{
    TFTP_ERR_NONE = 0, // 无错误

    // 主控板本地逻辑、文件系统和上位机协议错误
    TFTP_ERR_WRQ_PARSE_FAIL,     // WRQ包解析失败
    TFTP_ERR_FILENAME_TOO_LONG,  // 文件名过长
    TFTP_ERR_MODE_TOO_LONG,      // 模式字段过长
    TFTP_ERR_FILELEN_TOO_LONG,   // 文件长度字段过长
    TFTP_ERR_IP_MISMATCH,        // 目标IP不匹配
    TFTP_ERR_LOCAL_UPDATE_FAIL,  // 本地更新处理失败
    TFTP_ERR_FILE_NOT_IN_FAT,    // FAT表中未找到对应的文件名
    TFTP_ERR_FILE_OPEN_FAIL,     // 打开文件失败
    TFTP_ERR_NOT_WRQ_STATE,      // 当前状态不是WRQ
    TFTP_ERR_SESSION_BUSY,       // 当前TFTP会话忙，拒绝新请求
    TFTP_ERR_IP_PORT_MISMATCH,   // 当前IP和端口号不匹配
    TFTP_ERR_WRQ_CHECKSUM_FAIL,  // WRQ帧校验和错误
    TFTP_ERR_DATA_CHECKSUM_FAIL, // DATA帧校验和错误
    TFTP_ERR_BLOCK_NUM_NOT_SEQ,  // 当前块号不连续
    TFTP_ERR_FILE_CHECKSUM_FAIL, // 总文件校验和错误
    TFTP_ERR_INVALID_DST_ID,     // 无效的目标CAN ID
    TFTP_ERR_DST_ID_NOT_SELF,    // 目标CAN ID不匹配（既不是本机也不是IO板卡）
    TFTP_ERR_DIFF_TIMEOUT,       // 增量下载请求超时未收到响应
    TFTP_ERR_DIFF_CHECKSUM_FAIL, // DIFF帧校验和错误
    TFTP_ERR_LCK_CHECKSUM_FAIL,  // LCK帧校验和错误
    TFTP_ERR_RRQ_CHECKSUM_FAIL,  // RRQ帧校验和错误
    TFTP_ERR_FILE_NOT_FOUND,     // 文件未找到
    TFTP_ERR_RRQ_NO_RESPONSE,    // RRQ请求超时未收到FIF帧
    TFTP_ERR_FILE_READ_FAIL,     // 文件读取失败
    TFTP_ERR_RRQ_PARSE_FAIL,     // RRQ包解析失败
    TFTP_ERR_ACK_CHECKSUM_FAIL,  // ACK帧校验和错误
    TFTP_ERR_STOP_CHECKSUM_FAIL, // STOP帧校验和错误
    TFTP_ERR_RST_CHECKSUM_FAIL,  // RST帧校验和错误
    TFTP_ERR_LCK_NO_LOK,         // 主控板未回复LCK的LOK帧

    // IO板卡通信和转发错误
    TFTP_ERR_WRQ_NO_ACK,           // IO板卡未回复WRQ的ACK帧
    TFTP_ERR_DATA_NO_ACK,          // IO板卡未回复DATA的ACK帧
    TFTP_ERR_REQ_NO_ROK,           // IO板卡未回复REQ的ROK帧
    TFTP_ERR_CAN_SRC_ID_INVALID,   // 错误的CAN源ID
    TFTP_ERR_DATA_LEN_OUT_RANGE,   // 数据长度超出范围
    TFTP_ERR_OPCODE_INVALID,       // 错误的操作码
    TFTP_ERR_CHECKSUM_MISMATCH,    // 校验和不匹配
    TFTP_ERR_TOTAL_SUM_FAIL,       // 文件总校验和不匹配
    TFTP_ERR_REQ_PARSE_FAIL,       // REQ包解析失败
    TFTP_ERR_WRQ_PARSE_FAIL2,      // WRQ包解析失败（细分）
    TFTP_ERR_DATA_PARSE_FAIL,      // DATA包解析失败
    TFTP_ERR_SUM_PARSE_FAIL,       // SUM包解析失败
    TFTP_ERR_LCK_PARSE_FAIL,       // 链路检测 LCK 包解析失败
    TFTP_ERR_IO_RRQ_PARSE_FAIL,    // IO板卡 RRQ 包解析失败
    TFTP_ERR_DIFF_PARSE_FAIL,      // 增量下载 DIFF 包解析失败
    TFTP_ERR_ACK_PARSE_FAIL,       // 一键上载 ACK 包解析失败
    TFTP_ERR_FIRMWARE_NOT_FOUND,   // 固件信息未找到（RRQ）
    TFTP_ERR_FILENAME_MISMATCH,    // 请求的文件名不匹配（RRQ）
    TFTP_ERR_UPLOAD_NOT_ACTIVE,    // 非上载状态下收到 ACK
    TFTP_ERR_UNEXPECTED_BLOCK_NUM, // 收到非预期的 ACK 块号
    TFTP_ERR_FILENAME_WRQMISMATCH, // 请求下载的文件名与板卡类型不匹配（WRQ）

    emTFTP_ERR_SIZE // 错误码计数
} emTFTP_ERR_INFO;

#endif

typedef struct
{
    const char s8_ErrInfo[MAX_ERR_INFO_LEN];


    uint8_t u8_ErrLen;
} ST_ERR_INFO;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/


extern emTFTP_ERR_INFO emTftpErr;

#ifdef __cplusplus
}
#endif

#endif  //__NET_TFTP_H__INCLUDE__
