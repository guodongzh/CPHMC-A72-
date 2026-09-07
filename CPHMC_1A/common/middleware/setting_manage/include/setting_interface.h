/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       setting_interface.h
 *@author     wenjunf
 *@date       2025.09.03
 *@brief      定值管理接口
 *@par        History
 *Date        Version   Author     Description
 *2025.09.03  1.0       wenjunf    初版
 ******************************************************************************/
#ifndef _SETTING_INTERFACE_H
#define _SETTING_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "platform.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
// 报告管理共享内存监视
typedef struct _Setting_Manage_Monitor_Inf_Struct
{
    // 定值存储监视
    uint32_t Setting_save_OK_number;     // 定值存储成功次数
    uint32_t Setting_save_err_number;    // 定值存储出错次数
    uint32_t Setting_save_err_location;  // 定值存储出错定位

    // 定值读取上送监视
    uint32_t Setting_read_OK_number;     // 定值读取成功次数
    uint32_t Setting_read_err_number;    // 定值读取出错次数
    uint32_t Setting_read_err_location;  // 定值读取出错定位
} Setting_Manage_Monitor_Inf_Struct;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern Flash_Handle setting_manage_flash_handle;

int32_t read_param_flash(uint32_t flash_sector_ID, uint8_t *p_content, uint32_t content_length);
int32_t write_param_flash(uint32_t flash_sector_ID, uint8_t *p_content, uint32_t content_length);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _SETTING_INTERFACE_H */