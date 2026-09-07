/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       setting_interface.c
 *@author     wenjunf
 *@date       2025.09.03
 *@brief      定值管理接口
 *@par        History
 *Date        Version   Author     Description
 *2025.09.03  1.0       wenjunf    初版
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "setting_interface.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
Setting_Manage_Monitor_Inf_Struct Setting_manage_moniter_inf = {0};
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// 读Flash的定值参数
// flash_sector_ID--扇区相对序号,从0开始
// p_content-- 存放内容的指针
// content_length--需要读出长度（Byte数）
// 返回：实际读出长度，<=0 为读出错
int32_t read_param_flash(uint32_t flash_sector_ID, uint8_t *p_content, uint32_t content_length)
{
    if (flash_sector_ID >= SECTORS_PER_CORE)
    {
        Setting_manage_moniter_inf.Setting_read_err_number++;
        Setting_manage_moniter_inf.Setting_read_err_location = 1;
        return -1;
    }

    if (p_content == NULL || content_length == 0)
    {
        Setting_manage_moniter_inf.Setting_read_err_number++;
        Setting_manage_moniter_inf.Setting_read_err_location = 2;
        return -2;
    }

    uint32_t target_offset = CURRENT_CORE_BASE_OFFSET + flash_sector_ID * FLASH_SECTOR_SIZE;
    uint32_t read_length   = (content_length > FLASH_SECTOR_SIZE) ? FLASH_SECTOR_SIZE : content_length;
    setting_manage_flash_handle = flash_handle1;

    int32_t result = Flash_read_com(setting_manage_flash_handle,
                                     target_offset, p_content, read_length, FLASH_1);
    
    if (result != SystemP_SUCCESS)
    {
        Setting_manage_moniter_inf.Setting_read_err_number++;
        Setting_manage_moniter_inf.Setting_read_err_location = 3;
        return result;
    }

    return read_length;
}

// 写Flash的定值参数
// flash_sector_ID--扇区相对序号,从0开始
// p_content-- 写内容的指针
// content_length--写入Flash的长度
// 返回：<=0 为写出错，1-写正确
int32_t write_param_flash(uint32_t flash_sector_ID, uint8_t *p_content, uint32_t content_length)
{
    // 参数检查
    if (flash_sector_ID >= SECTORS_PER_CORE)
    {
        Setting_manage_moniter_inf.Setting_save_err_number++;
        Setting_manage_moniter_inf.Setting_save_err_location = 1;
        return -1;
    }

    if (p_content == NULL || content_length == 0 || content_length > FLASH_SECTOR_SIZE)
    {
        Setting_manage_moniter_inf.Setting_save_err_number++;
        Setting_manage_moniter_inf.Setting_save_err_location = 2;
        return -2;
    }

    uint32_t target_offset = CURRENT_CORE_BASE_OFFSET + flash_sector_ID * FLASH_SECTOR_SIZE;

    uint32_t sector_num, page_num;
    setting_manage_flash_handle = flash_handle1;

    if (Flash_offsetToSectorPage_com(setting_manage_flash_handle,
                                      target_offset, &sector_num, &page_num) != SystemP_SUCCESS)
    {
        Setting_manage_moniter_inf.Setting_save_err_number++;
        Setting_manage_moniter_inf.Setting_save_err_location = 3;
        return -3;
    }
    if (Flash_eraseSector_com(setting_manage_flash_handle, sector_num, FLASH_1) != SystemP_SUCCESS)
    {
        Setting_manage_moniter_inf.Setting_save_err_number++;
        Setting_manage_moniter_inf.Setting_save_err_location = 4;
        return -4;
    }
    if (Flash_write_com(setting_manage_flash_handle,
                         target_offset, p_content, content_length, FLASH_1) != SystemP_SUCCESS)
    {
        Setting_manage_moniter_inf.Setting_save_err_number++;
        Setting_manage_moniter_inf.Setting_save_err_location = 5;
        return -5;
    }

    return 1;  // 写入成功
}