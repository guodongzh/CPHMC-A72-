/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       report_interface.c
 *@author     wenjunf
 *@date       2025.07.23
 *@brief      报告管理
 *@par        History
 *Date        Version   Author     Description
 *2025.07.23  1.0       wenjunf    报告管理
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#ifndef WINNT
#include "report_interface.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
// 报告管理内存监视
Report_Manage_Monitor_Inf_Struct Report_manage_moniter_inf = {0};

// 各类型报告的 Flash 存储区域配置
static ReportFlashRegion report_regions[REPORT_TYPE_NUM];

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */
static uint32_t       calc_report_checksum(uint8_t *data);
static void           prepare_report(void *report, uint8_t type, void *p_report_region);
static int32_t        is_report_valid(void *report);
static inline int32_t get_region_idx(uint8_t type);
static int32_t        erase_if_needed(uint32_t offset, uint8_t region_idx);
/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// 将报文存盘
// report_type--报文类型，p_uint8--存盘内容指针,report_len--存盘报文长度
// 报文长度固定为128Bytes
// 返回 FALSE--出错 TRUE--正确
int32_t Save_Report_to_Flash(uint32_t report_type, uint8_t *p_report_content, uint32_t report_len)
{
    uint8_t  page_data[256];  // 页缓存，用于非页对齐写入
    uint32_t write_addr;
    uint32_t write_len;
    uint8_t report_temp[MAX_REPORT_LENGTH] = {0};

    // 检查输入参数有效性
    if (p_report_content == NULL || report_len != MAX_REPORT_LENGTH)
    {
        Report_manage_moniter_inf.Report_save_err_number++;
        Report_manage_moniter_inf.Report_save_err_location = 1;
        return FALSE;
    }

    // 检查报告类型是否有效
    if (report_type < REPORT_OTHER_TYPE_ID || report_type > REPORT_SOE_TYPE_ID)
    {
        Report_manage_moniter_inf.Report_save_err_number++;
        Report_manage_moniter_inf.Report_save_err_location = 2;
        return FALSE;
    }

    report_manage_flash_handle = flash_handle1;

    // 计算报告类型对应的区域索引
    uint32_t           region_idx = get_region_idx(report_type);
    ReportFlashRegion *region     = &report_regions[region_idx];

    // 更新最新索引计数值
    region->report_new_cnt++;

    memcpy(report_temp, p_report_content, MAX_REPORT_LENGTH);

    // 准备报告数据
    prepare_report(report_temp, report_type, region);

    // 计算当前存储地址
    // 根据上电找到的最新索引往下存写报告
    uint32_t offset     = region->start_addr + (region->write_idx * MAX_REPORT_LENGTH);

    // 判页对齐
    if ((offset & 0xFF) == 0)
    {
        write_addr = offset;
        write_len  = MAX_REPORT_LENGTH;

        if (!erase_if_needed(offset, region_idx))
        {
            Report_manage_moniter_inf.Report_save_err_number++;
            Report_manage_moniter_inf.Report_save_err_location = 3;
            return FALSE;
        }

        if (Flash_write_com(report_manage_flash_handle,
                             write_addr, report_temp, write_len, FLASH_1) != SystemP_SUCCESS)
        {
            Report_manage_moniter_inf.Report_save_err_number++;
            Report_manage_moniter_inf.Report_save_err_location = 4;
            return FALSE;
        }
        else
        {
            Report_manage_moniter_inf.Report_save_OK_number++;
        }
    }
    else
    {
        uint32_t page_addr = offset & 0xFFFFFF00;

        if (!erase_if_needed(page_addr, region_idx))
        {
            Report_manage_moniter_inf.Report_save_err_number++;
            Report_manage_moniter_inf.Report_save_err_location = 3;
            return FALSE;
        }

        if (Flash_read_com(report_manage_flash_handle,
                            page_addr, page_data, MAX_REPORT_LENGTH, FLASH_1) != SystemP_SUCCESS)
        {
            // 如果读取失败，将前128字节填充为0xFF
            memset(page_data, 0xFF, MAX_REPORT_LENGTH);
        }
        // 非页对齐拼成一页写
        memcpy(page_data + MAX_REPORT_LENGTH, report_temp, MAX_REPORT_LENGTH);

        write_addr = page_addr;
        write_len  = 256;

        if (Flash_write_com(report_manage_flash_handle,
                             write_addr, page_data, write_len, FLASH_1) != SystemP_SUCCESS)
        {
            Report_manage_moniter_inf.Report_save_err_number++;
            Report_manage_moniter_inf.Report_save_err_location = 4;
            return FALSE;
        }
        else
        {
            Report_manage_moniter_inf.Report_save_OK_number++;
        }
    }

    region->read_idx = region->write_idx; // 写成功后更新读索引
    Report_manage_moniter_inf.Report_save_index = region->write_idx;
    // 更新当前索引（循环）
    region->write_idx = (region->write_idx + 1) & (region->max_count - 1);

    return TRUE;
}

// 读报文内容
// report_type--报文类型，report_index_ID--报文索引序号（从0开始,以最新报告索引为基地址的偏移），p_report_content--存放报文内容指针
// 返回 FALSE--出错 TRUE--正确
int32_t Read_Report_Content(uint32_t report_type, uint32_t report_index_ID, uint8_t *p_report_content)
{
    int32_t ret = TRUE;

    // 检查输入参数有效性
    if (p_report_content == NULL)
    {
        Report_manage_moniter_inf.Report_content_read_err_number++;
        Report_manage_moniter_inf.Report_content_read_err_location = 1;
        return FALSE;
    }

    // 检查报告类型是否有效
    if (report_type < REPORT_OTHER_TYPE_ID || report_type > REPORT_SOE_TYPE_ID)
    {
        Report_manage_moniter_inf.Report_content_read_err_number++;
        Report_manage_moniter_inf.Report_content_read_err_location = 2;
        return FALSE;
    }

    // 计算报告类型对应的区域索引
    uint32_t region_idx = get_region_idx(report_type);

    ReportFlashRegion *region = &report_regions[region_idx];

    // 检查索引是否有效
    if (report_index_ID >= region->max_count)
    {
        Report_manage_moniter_inf.Report_content_read_err_number++;
        Report_manage_moniter_inf.Report_content_read_err_location = 3;
        ret = FALSE;
        goto read_false;
    }

    report_manage_flash_handle = flash_handle1;

    // 根据最新报告索引为基准，索引ID为偏移，往前读取报告
    // 计算实际要读取的报告索引
    uint32_t actual_index = (region->read_idx >= report_index_ID)
                                ? (region->read_idx - report_index_ID)
                                : (region->max_count + region->read_idx - report_index_ID);

    uint32_t offset = region->start_addr + (actual_index * MAX_REPORT_LENGTH);

read_false:
    if (ret == TRUE)
    {
        // 从 Flash 读取报告内容
        if (Flash_read_com(report_manage_flash_handle,
                            offset, p_report_content, MAX_REPORT_LENGTH, FLASH_1) != SystemP_SUCCESS)
        {
            Report_manage_moniter_inf.Report_content_read_err_number++;
            Report_manage_moniter_inf.Report_content_read_err_location = 4;
            return FALSE;
        }

        ret = is_report_valid(p_report_content);
    }

    return ret;
}

// 计算报告校验和
static uint32_t calc_report_checksum(uint8_t *data)
{
    uint32_t  sum = 0;
    uint32_t *p   = (uint32_t *)data;
    for (uint32_t i = 0; i < 28; i++)
    {
        sum += *p++;
    }
    return sum;
}

// 准备报告数据（设置有效标志和校验和）
static void prepare_report(void *report, uint8_t type, void *p_report_region)
{
    uint8_t *p                = (uint8_t *)report;
    p[2]                      = REPORT_VALID_FLAG;  // 强制设置有效标志
    ReportFlashRegion *region = (ReportFlashRegion *)p_report_region;

 
    ReportStruct *pReport = (ReportStruct *)report;
    pReport->report_index = region->report_new_cnt;
    pReport->checksum     = calc_report_checksum(p);
  
}

// 验证报告有效性
static int32_t is_report_valid(void *report)
{
    uint8_t *p = (uint8_t *)report;
    if (p[2] != REPORT_VALID_FLAG)
    {
        Report_manage_moniter_inf.Report_content_read_err_number++;
        Report_manage_moniter_inf.Report_content_read_err_location = 5;
        return FALSE;
    }

    uint32_t stored_checksum = *(uint32_t *)(p + 124);
    if (calc_report_checksum(p) == stored_checksum)
    {
        Report_manage_moniter_inf.Report_content_read_OK_number++;
        return TRUE;
    }
    else
    {
        Report_manage_moniter_inf.Report_content_read_err_number++;
        Report_manage_moniter_inf.Report_content_read_err_location = 6;
        return FALSE;
    }
}

// 获取报告区域索引
static inline int32_t get_region_idx(uint8_t type)
{
    return ((type == REPORT_SOE_TYPE_ID) ? 5u : (type - REPORT_OTHER_TYPE_ID));
}

// 按需擦除扇区
static int32_t erase_if_needed(uint32_t offset, uint8_t region_idx)
{
    int32_t ret = TRUE;

    ReportFlashRegion *region = &report_regions[region_idx];
    uint32_t last_sectorID_temp[REPORT_TYPE_NUM];

    uint32_t sector;
    uint32_t page;
    if (Flash_offsetToSectorPage_com(report_manage_flash_handle,
                                      offset, &sector, &page) != SystemP_SUCCESS)
    {
        ret = FALSE;
        goto erase_if_needed_false;
    }

    if (sector != (region->last_erase_sectorID))
    {
        if (Flash_eraseSector_com(report_manage_flash_handle, sector, FLASH_1) != SystemP_SUCCESS)
        {
            ret = FALSE;
            goto erase_if_needed_false;
        }

        region->last_erase_sectorID = sector;
        if (Flash_read_com(report_manage_flash_handle,
                            RECORD_ERASED_SECTOR_ADD, (uint8_t *)last_sectorID_temp, sizeof(last_sectorID_temp), FLASH_1) != SystemP_SUCCESS)
        {
            ret = FALSE;
            goto erase_if_needed_false;
        }
        last_sectorID_temp[region_idx] = region->last_erase_sectorID;
        if (Flash_offsetToSectorPage_com(report_manage_flash_handle,
                                          RECORD_ERASED_SECTOR_ADD, &sector, &page) != SystemP_SUCCESS)
        {
            ret = FALSE;
            goto erase_if_needed_false;
        }
        if (Flash_eraseSector_com(report_manage_flash_handle, sector, FLASH_1) != SystemP_SUCCESS)
        {
            ret = FALSE;
            goto erase_if_needed_false;
        }
        if (Flash_write_com(report_manage_flash_handle,
                             RECORD_ERASED_SECTOR_ADD, (uint8_t *)last_sectorID_temp, sizeof(last_sectorID_temp), FLASH_1) != SystemP_SUCCESS)
        {
            ret = FALSE;
            goto erase_if_needed_false;
        }
    }

erase_if_needed_false:
    return ret;
}

/*
 * 上电调用，寻找最新报告索引，用于指定报告存写和读取报告的区域
 * @param report_type 报告类型
 */
void find_report_new_index(uint32_t report_type)
{
    ReportStruct report   = {0};
    int32_t      type_idx = get_region_idx(report_type);

    ReportFlashRegion *pRegion           = &report_regions[type_idx];
    uint16_t           report_old_cnt    = 0;
    uint8_t            report_is_new_cnt = 0;

    for (uint32_t i = 0; i < pRegion->max_count; i++)
    {
        uint32_t offset = pRegion->start_addr + (i * MAX_REPORT_LENGTH);

        if (Flash_read_com(report_manage_flash_handle,
                            offset, (uint8_t *)&report, MAX_REPORT_LENGTH, FLASH_1) != SystemP_SUCCESS)
        {
            Report_manage_moniter_inf.Report_init_err_location = 1;
            Report_manage_moniter_inf.Report_init_err_number++;
            continue;
        }
        // 有效报告判断条件
        if (report.reportValidFlag == REPORT_VALID_FLAG)  // && report.checksum == calc_report_checksum(&report)
        {
            report_is_new_cnt = check_frame_u16_cnt_add(report.report_index, report_old_cnt);
            if (report_is_new_cnt == TRUE)
            {
                report_old_cnt = report.report_index;
            }
            else
            {
                // 发现旧索引值则退出
                // 需判断索引值是否溢出为0
                if (report.report_index == 0)
                {
                    pRegion->report_new_cnt = 0;
                    pRegion->write_idx      = 0;
                    pRegion->read_idx       = i;
                }
                else
                {
                    pRegion->report_new_cnt = report_old_cnt;
                    pRegion->write_idx      = i;
                    pRegion->read_idx       = i - 1;
                }
                pRegion->report_count = pRegion->max_count;
                break;
            }
        }
        else if (report.reportValidFlag == FLASH_ERASED_VAL && i == 0)
        {
            // 检查是否第一次存写报告
            offset = pRegion->start_addr + ((pRegion->max_count - 1) * MAX_REPORT_LENGTH);
            if (Flash_read_com(report_manage_flash_handle,
                                offset, (uint8_t *)&report, MAX_REPORT_LENGTH, FLASH_1) != SystemP_SUCCESS)
            {
                Report_manage_moniter_inf.Report_init_err_location = 2;
                Report_manage_moniter_inf.Report_init_err_number++;
                continue;
            }
            if (report.reportValidFlag == FLASH_ERASED_VAL)
            {
                // 报告区域全部被擦除,则认为首次存写报告，最新读写索引值为0
                pRegion->report_new_cnt = 0;
                pRegion->write_idx      = 0;
                pRegion->read_idx       = 0;
                pRegion->report_count   = 0;
            }
            else if (report.reportValidFlag == REPORT_VALID_FLAG)
            {
                // 报告区域部分被擦除，则认为最新读写索引值为最新有效报告索引值
                pRegion->report_new_cnt = report.report_index;
                pRegion->write_idx      = 0;
                pRegion->read_idx       = pRegion->max_count - 1;
                pRegion->report_count   = pRegion->max_count - REPORT_SECTOR_NUM;
            }
            break;
        }
        else
        {
            pRegion->report_new_cnt = report_old_cnt;
            pRegion->write_idx      = i;
            pRegion->read_idx       = i - 1;
            pRegion->report_count   = pRegion->max_count - (REPORT_SECTOR_NUM - (i & 0x1F));
            break;
        }
    }
}

// 报告管理初始化:用于找到所有类型的最新报告索引和上一次擦除的扇区号
int32_t report_init(void)
{
    int32_t  status                               = SystemP_SUCCESS;
    uint8_t  report_type_id                       = 0;
    uint32_t last_erase_sectorID[REPORT_TYPE_NUM] = {0};
    uint32_t start_addr[REPORT_TYPE_NUM]          = {OTHER_REPORT_OFFSET, ALM_REPORT_OFFSET, OPERATE_REPORT_OFFSET,
                                                     RESULT_REPORT_OFFSET, ACTIVE_REPORT_OFFSET, SOE_REPORT_OFFSET};
    uint32_t max_count[REPORT_TYPE_NUM]           = {MAX_OTHER_REPORT_NUM, MAX_ALM_REPORT_NUM, MAX_OPERATE_REPORT_NUM,
                                                     MAX_RESULT_REPORT_NUM, MAX_ACTIVE_REPORT_NUM, MAX_SOE_REPORT_NUM};

    // 初始化报告管理相关结构体
    memset(report_regions, 0x00, sizeof(report_regions));

    report_manage_flash_handle = flash_handle1;

    // 将每个类型的报文最后擦除的扇区号取出
    if (Flash_read_com(report_manage_flash_handle,
                       RECORD_ERASED_SECTOR_ADD, (uint8_t *)last_erase_sectorID, sizeof(last_erase_sectorID), FLASH_1) != SystemP_SUCCESS)
    {
        status = SystemP_FAILURE;
    }

    for (uint32_t i = 0; i < REPORT_TYPE_NUM; i++)
    {
        // 初始化不同类型报告的起始地址和最大数量
        report_regions[i].start_addr = start_addr[i];
        report_regions[i].max_count  = max_count[i];

        // flash中存放最后擦除扇区号对应的类型顺序和report_regions数组中类型顺序一致
        report_regions[i].last_erase_sectorID = last_erase_sectorID[i];

        // 寻找最新报告索引
        report_type_id = ((i == 5) ? REPORT_SOE_TYPE_ID : REPORT_OTHER_TYPE_ID + i);
        find_report_new_index(report_type_id);
    }

    return status;
}
#endif