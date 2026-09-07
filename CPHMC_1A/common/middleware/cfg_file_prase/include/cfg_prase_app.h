/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       cfg_prase_app.h
 *@author     wenjunf
 *@date       2026.01.22
 *@brief      配置文件解析应用接口
 *@par        History
 *Date        Version   Author     Description
 *2026.01.22  1.0       wenjunf    example
 ******************************************************************************/
#ifndef _CFG_PRASE_APP_H
#define _CFG_PRASE_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#ifdef SOC_AM64X
#include "can_const_define.h"
#endif

#include "cfg_prase_drv.h"
#include "file_system.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define STR_MAX_LEN (32)
#ifndef SWITCH_ON
#define SWITCH_ON   (0xAA)
#define SWITCH_OFF  (0x55)
#endif
#define INET_ADDRSTRLEN (16)
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
// 配置文件错误状态共享内存监视
typedef struct _cfg_prase_Monitor_Inf_Struct
{
    uint32_t cfg_err_number;
    uint32_t cfg_err_location;
} cfg_prase_Monitor_Inf_Struct;

#ifdef SOC_AM64X
typedef struct
{
    uint32_t DI_slot_val;
    uint32_t DI_shake_time; // DI板消抖时间(ms)
} DI_SLOT_SETTING_VALUE_STRUCT;

typedef struct
{
    uint32_t DO_slot_val;
    uint32_t DO_qd_flag; // DO板是否经背板启动(1为经背板启动，0为经本板启动)
} DO_SLOT_SETTING_VALUE_STRUCT;

typedef struct
{
    uint32_t                     DI_slot_num;
    uint32_t                     voltage_level; // DI板电压(110V/220V/48V/24V)
    DI_SLOT_SETTING_VALUE_STRUCT DI_slot_cfg[MAX_DICPU_NUM];
} DI_SLOT_SETTING_STRUCT;

typedef struct
{
    uint32_t                     DO_slot_num;
    DO_SLOT_SETTING_VALUE_STRUCT DO_slot_cfg[MAX_DOCPU_NUM];
} DO_SLOT_SETTING_STRUCT;

typedef struct
{
    uint8_t  Plat_Type[STR_MAX_LEN];  // 平台类型
    uint8_t  Dev_Type[STR_MAX_LEN];   // 装置类型
    uint8_t  Cfg_Ver[STR_MAX_LEN];    // 配置文件版本
    uint32_t cfg_file_checksum;
} IO_SLOT_CFG_STRUCT;
#endif

typedef struct
{
    uint32_t valid_flag; //配置文件B码配置是否有效
    uint32_t Time_Mode;//对时方式：SNTP/1588/B码对时
    int32_t Time_Zone;//时区：以分钟为单位，表示东八区还是西八区
    uint32_t B_Code_Check;//B码校验方式：奇/偶校验/无校验
    uint32_t B_Code_Shake_us;//B码抖动us
    uint32_t B_Code_Polarity;//B码极性取反标志：0为不取反，1为取反
    uint8_t Sntp_Serve_IP[INET_ADDRSTRLEN];//SNTP对时服务端的IP地址
} IRIGB_CFG_STRUCT;

/* 一键下载配置结构体 */
typedef struct
{
    uint32_t valid_flag; //配置文件一键下载配置是否有效
    uint32_t group_ip;  // Group_IP配置值，由后续接口负责计算组播IP地址
} ONEKEY_DOWNLOAD_CFG_STRUCT;

typedef struct
{
    uint32_t valid_flag;
    uint32_t dev_addr;
} DEVICE_ADDR_CFG_STRUCT;

/* 端口IP配置结构体 */
typedef struct
{
    uint8_t core_num;   // Core序号(1-4)
    uint8_t eth_num;    // 外网口序号(1-16)
    uint8_t ip_addr[INET_ADDRSTRLEN]; // IP地址 192.168.17.10
} PORT_IP_CFG_STRUCT;

typedef struct
{
    uint32_t valid_flag; //配置文件端口IP配置是否有效
    uint32_t set_ip_number;
    PORT_IP_CFG_STRUCT port_ip_cfg[64]; // 16*4
} PORT_IP_All_CFG_STRUCT;

typedef struct
{
    uint32_t valid_flag; //配置文件版本信息是否有效
    uint8_t  Plat_Type[STR_MAX_LEN];  // 平台类型
    uint8_t  Dev_Type[STR_MAX_LEN];   // 装置类型
    uint8_t  Cfg_Ver[STR_MAX_LEN];    // 配置文件版本
} CFG_VERINFO_STRUCT;

typedef struct
{
    CFG_VERINFO_STRUCT verinfo;
    IRIGB_CFG_STRUCT irigb_cfg;
    DEVICE_ADDR_CFG_STRUCT device_addr_cfg;
    PORT_IP_All_CFG_STRUCT all_port_ip_cfg; // 端口IP配置
    uint32_t cfg_file_checksum;
} IRIGB_IP_CFG_STRUCT;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern IRIGB_IP_CFG_STRUCT IRIGB_IP_Cfg;
extern cfg_prase_Monitor_Inf_Struct cfg_prase_Monitor_Inf;

int32_t  DIO_cfg_read_main(void);
int8_t   read_io_slot_cfg_checkcode(void);
int8_t   read_io_slot_cfg_verinfo_cfg(void);
int8_t   read_io_slot_di_do_cfg(void);
int8_t   cmp_slot_name_string(int8_t *p_slot_name_int8,uint8_t *p_slot_type);
uint32_t remove_line_breaks(const uint8_t *p_cfgfile, uint8_t *p_cleaned_content, uint32_t buffer_size);
uint32_t calc_cfg_checksum(const uint8_t *p_cfgfile);

/* IRIGB配置解析函数 */
int32_t IRIGB_IP_cfg_read_main(void);
int32_t boot_cfg_read_main(void);
int8_t  read_irigb_ip_cfg_checkcode(void);
int8_t  read_irigb_ip_cfg_verinfo_cfg(void);
int8_t  read_irigb_cfg(void);

/* IP地址转换函数 */
int8_t ip_string_to_array(const char *ip_str, uint8_t *ip_array);

/* 一键下载配置解析函数 */
int8_t read_device_addr_cfg(void);

/* 端口IP配置解析函数 */
int8_t read_port_ip_cfg(void);
int8_t read_boot_cfg(void);

extern uint8_t boot_cfg[8];

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _CFG_PRASE_APP_H */