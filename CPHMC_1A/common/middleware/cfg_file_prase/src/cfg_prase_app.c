/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       cfg_prase_app.c
 *@author     wenjunf
 *@date       2026.01.22
 *@brief      配置文件解析应用接口
 *@par        History
 *Date        Version   Author     Description
 *2026.01.22  1.0       wenjunf    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "cfg_prase_app.h"
#if !defined(CORE_R5F1) || !defined(SMARTDEV_FUNC)
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
uint8_t sdam0_FileRd_Buf[32768] = {0};
uint8_t cleaned_cfg_buffer[4096] = {0};
uint8_t cfg_file_buffer[4096] = {0};
uint8_t boot_cfg[8] = {0};

#ifdef SOC_AM64X
// 插件类型
const int8_t DIO_SLOT_NAME_DEF[4][4] = {
    "DI",
    "DO",
    "AI",
    "OP",
};

// DIO slot config
DI_SLOT_SETTING_STRUCT DI_slot_cfg = {0};
DO_SLOT_SETTING_STRUCT DO_slot_cfg = {0};
IO_SLOT_CFG_STRUCT IO_Slot_Cfg = {0};

#endif

// 配置文件错误状态共享内存监视
cfg_prase_Monitor_Inf_Struct cfg_prase_Monitor_Inf = {0};

// IRIGB and IP config
IRIGB_IP_CFG_STRUCT IRIGB_IP_Cfg = {0};
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
/**
 * IRIGB IP配置读取主函数
 * @return 0=成功, 负数=失败
 */
int32_t IRIGB_IP_cfg_read_main(void)
{
    uint8_t *p_cfgfile_content = cfg_file_buffer;
    FILE_INFO cfg_file_info = {0};

    memset(&cfg_prase_Monitor_Inf, 0, sizeof(cfg_prase_Monitor_Inf));
    memset(&IRIGB_IP_Cfg, 0, sizeof(IRIGB_IP_Cfg));

    int32_t file_handle = file_open("irigb_ip.cfg", FAT_MODE_R_OPEN);
    if (file_handle < 0)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 1;
        DebugP_log("[Error] file_open <irigb_ip.cfg> err!! file_handle=%d\r\n", file_handle);
        return -1;
    }

    uint8_t retval = file_info_read(file_handle, &cfg_file_info);
    if (!retval)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 2;
        DebugP_log("[Error] file_info_read <irigb_ip.cfg> err!! file_handle=%d\r\n", file_handle);
        return -2;
    }

    /* Bug3: 文件大小须小于缓冲区，预留1字节给'\0'，防止file_read越界写及后续解析越界读 */
    if (cfg_file_info.Size >= sizeof(cfg_file_buffer))
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 3;
        DebugP_log("[Error] <irigb_ip.cfg> too large! Size=%d, buf=%d\r\n",
                   cfg_file_info.Size,
                   (int)sizeof(cfg_file_buffer));
        file_close(file_handle, 0);
        return -3;
    }

    int32_t file_len = file_read(p_cfgfile_content, 0, cfg_file_info.Size, file_handle);
    if (file_len != cfg_file_info.Size)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 3;
        DebugP_log("[Error] file_read <irigb_ip.cfg> err!! file_len=%d, cfg_file_info.Size=%d\r\n",
                   file_len,
                   cfg_file_info.Size);
        return -3;
    }

    /* Bug3: 确保配置内容以'\0'结尾，保证后续字符串解析不越界 */
    p_cfgfile_content[file_len] = '\0';

    file_close(file_handle, 0);

    int32_t ret = ini_Initialize((const char *)p_cfgfile_content,
                                 (int32_t *)&sdam0_FileRd_Buf[0],
                                 sizeof(sdam0_FileRd_Buf));
    if (ret != 0)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 4;
        DebugP_log("[Error] ini_Initialize <irigb_ip.cfg> err! ret_code=%d\r\n", ret);
        return -4;
    }

    if (read_irigb_ip_cfg_checkcode() != TRUE)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 5;
        DebugP_log("[Error] read_irigb_ip_cfg_checkcode <irigb_ip.cfg> Failed to process config "
                   "file content\r\n");
        return -5;
    }

    uint32_t cleaned_length =
        remove_line_breaks(p_cfgfile_content, cleaned_cfg_buffer, sizeof(cleaned_cfg_buffer));
    if (cleaned_length == 0)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 6;
        DebugP_log(
            "[Error] remove_line_breaks <irigb_ip.cfg> Failed to process config file content\r\n");
        return -6;
    }

    uint32_t cfg_checksum = calc_cfg_checksum(cleaned_cfg_buffer);
    if (cfg_checksum != IRIGB_IP_Cfg.cfg_file_checksum)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 7;
        DebugP_log("[Error] <irigb_ip.cfg> checksum verify failed!! cfg_checksum = 0x%X, "
                   "IRIGB_IP_Cfg.cfg_file_checksum = 0x%X\r\n",
                   cfg_checksum,
                   IRIGB_IP_Cfg.cfg_file_checksum);
        return -7;
    }

    if (read_irigb_ip_cfg_verinfo_cfg() != TRUE)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 8;
        DebugP_log("[Error] read_irigb_ip_cfg_verinfo_cfg <irigb_ip.cfg> Failed to process config "
                   "file content\r\n");
        return -8;
    }
    else
    {
        IRIGB_IP_Cfg.verinfo.valid_flag = TRUE;
    }

    // 打印平台类型,装置类型,配置文件版本
    DebugP_log("[IRIGB_IP_CFG] <irigb_ip.cfg> Plat_Type : %s, Dev_Type : %s, Cfg_Ver : %s\r\n",
               IRIGB_IP_Cfg.verinfo.Plat_Type,
               IRIGB_IP_Cfg.verinfo.Dev_Type,
               IRIGB_IP_Cfg.verinfo.Cfg_Ver);

    // 读取IRIGB配置
    if (read_irigb_cfg() != TRUE)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 9;
        DebugP_log(
            "[Error] read_irigb_cfg <irigb_ip.cfg> Failed to process config file content\r\n");
        return -9;
    }
    else
    {
        IRIGB_IP_Cfg.irigb_cfg.valid_flag = TRUE;
    }

    // 读取装置地址配置
    if (read_device_addr_cfg() != TRUE)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 10;
        DebugP_log("[Error] read_device_addr_cfg <irigb_ip.cfg> Failed to process config file "
                   "content\r\n");
        return -10;
    }
    else
    {
        IRIGB_IP_Cfg.device_addr_cfg.valid_flag = TRUE;
    }

    // 读取端口IP配置
    if (read_port_ip_cfg() != TRUE)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 11;
        DebugP_log(
            "[Error] read_port_ip_cfg <irigb_ip.cfg> Failed to process config file content\r\n");
        return -11;
    }
    else
    {
        IRIGB_IP_Cfg.all_port_ip_cfg.valid_flag = TRUE;
    }

    return 0;
}

/**
 * 启动配置配置读取主函数
 * @return 0=成功, 负数=失败
 */
int32_t boot_cfg_read_main(void)
{
    uint8_t *p_cfgfile_content = cfg_file_buffer;
    FILE_INFO cfg_file_info = {0};

    memset(&cfg_prase_Monitor_Inf, 0, sizeof(cfg_prase_Monitor_Inf));
    memset(&IRIGB_IP_Cfg, 0, sizeof(IRIGB_IP_Cfg));

    int32_t file_handle = file_open("boot.cfg", FAT_MODE_R_OPEN);
    if (file_handle < 0)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 1;
        DebugP_log("[Error] file_open <boot.cfg> err!! file_handle=%d\r\n", file_handle);
        return -1;
    }

    uint8_t retval = file_info_read(file_handle, &cfg_file_info);
    if (!retval)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 2;
        DebugP_log("[Error] file_info_read <boot.cfg> err!! file_handle=%d\r\n", file_handle);
        return -2;
    }

    /* Bug3: 文件大小须小于缓冲区，预留1字节给'\0'，防止file_read越界写及后续解析越界读 */
    if (cfg_file_info.Size >= sizeof(cfg_file_buffer))
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 3;
        DebugP_log("[Error] <boot.cfg> too large! Size=%d, buf=%d\r\n",
                   cfg_file_info.Size,
                   (int)sizeof(cfg_file_buffer));
        file_close(file_handle, 0);
        return -3;
    }

    int32_t file_len = file_read(p_cfgfile_content, 0, cfg_file_info.Size, file_handle);
    if (file_len != cfg_file_info.Size)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 3;
        DebugP_log("[Error] file_read <boot.cfg> err!! file_len=%d, cfg_file_info.Size=%d\r\n",
                   file_len,
                   cfg_file_info.Size);
        return -3;
    }

    /* Bug3: 确保配置内容以'\0'结尾，保证后续字符串解析不越界 */
    p_cfgfile_content[file_len] = '\0';

    file_close(file_handle, 0);

    int32_t ret = ini_Initialize((const char *)p_cfgfile_content,
                                 (int32_t *)&sdam0_FileRd_Buf[0],
                                 sizeof(sdam0_FileRd_Buf));
    if (ret != 0)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 4;
        DebugP_log("[Error] ini_Initialize <boot.cfg> err! ret_code=%d\r\n", ret);
        return -4;
    }

    if (read_irigb_ip_cfg_checkcode() != TRUE)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 5;
        DebugP_log("[Error] read_irigb_ip_cfg_checkcode <boot.cfg> Failed to process config "
                   "file content\r\n");
        return -5;
    }

    uint32_t cleaned_length = remove_line_breaks(p_cfgfile_content, cleaned_cfg_buffer, sizeof(cleaned_cfg_buffer));
    if (cleaned_length == 0)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 6;
        DebugP_log(
            "[Error] remove_line_breaks <boot.cfg> Failed to process config file content\r\n");
        return -6;
    }

    uint32_t cfg_checksum = calc_cfg_checksum(cleaned_cfg_buffer);
    if (cfg_checksum != IRIGB_IP_Cfg.cfg_file_checksum)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 7;
        DebugP_log("[Error] <boot.cfg> checksum verify failed!! cfg_checksum = 0x%X, "
                   "IRIGB_IP_Cfg.cfg_file_checksum = 0x%X\r\n",
                   cfg_checksum,
                   IRIGB_IP_Cfg.cfg_file_checksum);
        return -7;
    }

    if (read_irigb_ip_cfg_verinfo_cfg() != TRUE)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 8;
        DebugP_log("[Error] read_irigb_ip_cfg_verinfo_cfg <boot.cfg> Failed to process config "
                   "file content\r\n");
        return -8;
    }
    else
    {
        IRIGB_IP_Cfg.verinfo.valid_flag = TRUE;
    }

    // 打印平台类型,装置类型,配置文件版本
    DebugP_log("[BOOT_CFG] <boot.cfg> Plat_Type : %s, Dev_Type : %s, Cfg_Ver : %s\r\n",
               IRIGB_IP_Cfg.verinfo.Plat_Type,
               IRIGB_IP_Cfg.verinfo.Dev_Type,
               IRIGB_IP_Cfg.verinfo.Cfg_Ver);
    read_boot_cfg();

    return 0;
}

#ifdef SOC_AM64X

int32_t DIO_cfg_read_main(void)
{
    uint8_t *p_cfgfile_content = cfg_file_buffer;
    FILE_INFO cfg_file_info = {0};

    int32_t file_handle = file_open("dio_slot.cfg", FAT_MODE_R_OPEN);
    if (file_handle < 0)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 12;
        DebugP_log("[Error] file_open <dio_slot.cfg> err!! file_handle=%d\r\n", file_handle);
        return -1;
    }

    uint8_t retval = file_info_read(file_handle, &cfg_file_info);
    if (!retval)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 13;
        DebugP_log("[Error] file_info_read err!! file_handle=%d\r\n", file_handle);
        return -2;
    }

    /* Bug3: 文件大小须小于缓冲区，预留1字节给'\0'，防止file_read越界写及后续解析越界读 */
    if (cfg_file_info.Size >= sizeof(cfg_file_buffer))
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 14;
        DebugP_log("[Error] <dio_slot.cfg> too large! Size=%d, buf=%d\r\n",
                   cfg_file_info.Size,
                   (int)sizeof(cfg_file_buffer));
        file_close(file_handle, 0);
        return -3;
    }

    int32_t file_len = file_read(p_cfgfile_content, 0, cfg_file_info.Size, file_handle);
    if (file_len != cfg_file_info.Size)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 14;
        DebugP_log("[Error] file_read err!! file_len=%d, cfg_file_info.Size=%d\r\n",
                   file_len,
                   cfg_file_info.Size);
        return -3;
    }

    /* Bug3: 确保配置内容以'\0'结尾，保证后续字符串解析不越界 */
    p_cfgfile_content[file_len] = '\0';

    file_close(file_handle, 0);

    int32_t ret = ini_Initialize((const char *)p_cfgfile_content,
                                 (int32_t *)&sdam0_FileRd_Buf[0],
                                 sizeof(sdam0_FileRd_Buf));
    if (ret != 0)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 15;
        DebugP_log("[Error] ini_Initialize err! ret_code=%d\r\n", ret);
        return -4;
    }
    if (read_io_slot_cfg_checkcode() != TRUE)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 16;
        DebugP_log("[Error] read_io_slot_cfg_checkcode Failed to process config file content\r\n");
        return -5;
    }

    uint32_t cleaned_length =
        remove_line_breaks(p_cfgfile_content, cleaned_cfg_buffer, sizeof(cleaned_cfg_buffer));
    if (cleaned_length == 0)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 17;
        DebugP_log("[Error] remove_line_breaks Failed to process config file content\r\n");
        return -6;
    }

    uint32_t cfg_checksum = calc_cfg_checksum(cleaned_cfg_buffer);
    if (cfg_checksum != IO_Slot_Cfg.cfg_file_checksum)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 18;
        DebugP_log("[Error] Cfg file checksum verify failed!! cfg_checksum = 0x%X, "
                   "IO_Slot_Cfg.cfg_file_checksum = 0x%X\r\n",
                   cfg_checksum,
                   IO_Slot_Cfg.cfg_file_checksum);
        return -7;
    }

    if (read_io_slot_cfg_verinfo_cfg() != TRUE)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 19;
        DebugP_log(
            "[Error] read_io_slot_cfg_verinfo_cfg Failed to process config file content\r\n");
        return -8;
    }

    // 打印平台类型,装置类型,配置文件版本
    DebugP_log("[R1] Plat_Type : %s, Dev_Type : %s, Cfg_Ver : %s\r\n",
               IO_Slot_Cfg.Plat_Type,
               IO_Slot_Cfg.Dev_Type,
               IO_Slot_Cfg.Cfg_Ver);

    // 读取DI_DO_Slot_Cfg配置
    if (read_io_slot_di_do_cfg() != TRUE)
    {
        cfg_prase_Monitor_Inf.cfg_err_number++;
        cfg_prase_Monitor_Inf.cfg_err_location = 20;
        DebugP_log("[Error] read_io_slot_di_do_cfg Failed to process config file content\r\n");
        return -9;
    }
    return 0;
}

int8_t read_io_slot_cfg_checkcode(void)
{
    char s1[32];
    char s2[32];
    //    char  s3[32];
    char *str;
    char *p;
    int8_t space = ' ';
    int32_t i, len;

    sprintf(s1, "CheckCode");
    // 读Sbl启动核心数量
    sprintf(s2, "CHECKSUM");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            IO_Slot_Cfg.cfg_file_checksum = ini_str2hex(p);
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }
    return TRUE;
}

int8_t read_io_slot_cfg_verinfo_cfg(void)
{
    char s1[32];
    char s2[32];
    //   char  s3[32];
    char *str;
    char *p;
    int8_t space = ' ';
    int32_t i, len;

    sprintf(s1, "Version_Info");
    // 读平台类型
    sprintf(s2, "Plat_Type");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            if (STR_MAX_LEN < len)
            {
                printf("len=%d>10", len);
            }
            for (i = 0; i < len; i++)
            {
                IO_Slot_Cfg.Plat_Type[i] = p[i];
            }
            IO_Slot_Cfg.Plat_Type[i] = 0;
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    // 读装置类型
    sprintf(s2, "Dev_Type");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            if (STR_MAX_LEN < len)
            {
                printf("len=%d>10", len);
            }
            for (i = 0; i < len; i++)
            {
                IO_Slot_Cfg.Dev_Type[i] = p[i];
            }
            IO_Slot_Cfg.Dev_Type[i] = 0;
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    // 读Cfg_Ver
    sprintf(s2, "Cfg_Ver");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            if (STR_MAX_LEN < len)
            {
                printf("len=%d>10", len);
            }
            for (i = 0; i < len; i++)
            {
                IO_Slot_Cfg.Cfg_Ver[i] = p[i];
            }
            IO_Slot_Cfg.Cfg_Ver[i] = 0;
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    return TRUE;
}

int8_t read_io_slot_di_do_cfg(void)
{
    char s1[32];
    char s2[32];
    //   char   s3[32];
    char *str;
    char *p;
    int8_t space = ',';
    int32_t i, len;
    int is_slot_enable = 0;
    uint8_t type = 0;
    int8_t string_code[5] = {0};
    uint32_t di_loop_slot = 0;
    uint32_t do_loop_slot = 0;

    sprintf(s1, "DIO_Slot_Cfg");
    sprintf(s2, "DI_Slot_Num");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            DI_slot_cfg.DI_slot_num = ini_str2hex(p);
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    sprintf(s2, "DO_Slot_Num");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            DO_slot_cfg.DO_slot_num = ini_str2hex(p);
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    sprintf(s2, "DI_VOLT");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            DI_slot_cfg.voltage_level = ini_str2hex(p);
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    // 使用循环读取Slot_0到Slot_10的配置
    for (int slot_num = 0; slot_num < 11; slot_num++)
    {
        sprintf(s2, "Slot_%d", slot_num);
        str = (char *)ini_GetVarStr(&s1[0], &s2[0], &i);
        if (str != 0)
        {
            p = (char *)ini_splitStr(str, space, 0, &len);
            if (p)
            {
                is_slot_enable = ini_str2hex(p);  // 是否使能槽位
            }
            else
            {
                printf("Slot_%d: Not Found enable field\n", slot_num);
                continue;  // 继续处理下一个槽位
            }

            if (is_slot_enable == 1)
            {
                p = (char *)ini_splitStr(str, space, 1, &len);
                if (p)
                {
                    /* Bug: string_code仅5字节，len超长会strncpy越界写，需做截断并补'\0' */
                    if (len < 0)
                    {
                        len = 0;
                    }
                    if (len > (int32_t)(sizeof(string_code) - 1))
                    {
                        len = (int32_t)(sizeof(string_code) - 1);
                    }
                    strncpy((char *)&string_code[0], p, len);
                    string_code[len] = '\0';
                }
                else
                {
                    printf("Slot_%d: Not Found type field\n", slot_num);
                    continue;  // 继续处理下一个槽位
                }

                p = (char *)ini_splitStr(str, space, 2, &len);
                if (p)
                {
                    int8_t ret_val = cmp_slot_name_string(&string_code[0], &type);
                    if (ret_val == FALSE)
                    {
                        printf("Slot_%d: Not Found type field\n", slot_num);
                        continue;  // 继续处理下一个槽位
                    }
                    if (type == DI_SLOT_TYPE)  // DI板
                    {
                        /* Bug: 防止DI_slot_cfg数组越界写 */
                        if (di_loop_slot < MAX_DICPU_NUM)
                        {
                            DI_slot_cfg.DI_slot_cfg[di_loop_slot].DI_slot_val = slot_num;
                            DI_slot_cfg.DI_slot_cfg[di_loop_slot].DI_shake_time = ini_str2hex(p);
                            di_loop_slot++;
                        }
                        else
                        {
                            printf("DI slot count exceeds MAX_DICPU_NUM(%d)\n", MAX_DICPU_NUM);
                        }
                    }
                    else if (type == DO_SLOT_TYPE)  // DO板
                    {
                        /* Bug: 防止DO_slot_cfg数组越界写 */
                        if (do_loop_slot < MAX_DOCPU_NUM)
                        {
                            DO_slot_cfg.DO_slot_cfg[do_loop_slot].DO_slot_val = slot_num;
                            DO_slot_cfg.DO_slot_cfg[do_loop_slot].DO_qd_flag =
                                (ini_str2hex(p) == 1) ? SWITCH_ON : SWITCH_OFF;
                            do_loop_slot++;
                        }
                        else
                        {
                            printf("DO slot count exceeds MAX_DOCPU_NUM(%d)\n", MAX_DOCPU_NUM);
                        }
                    }
                }
            }
        }
        else
        {
            printf("Slot_%d: Not Found\n", slot_num);
            return FALSE;
        }
    }

    return TRUE;
}

// 根据字符串获得库号
// 输入p_lib_name_int8--字符串
// 输出库号序，p_each_name_lib_cfg--返回库指针
// 返回 TRUE--找到库名称，FALSE--没有找到
int8_t cmp_slot_name_string(int8_t *p_slot_name_int8, uint8_t *p_slot_type)
{
    int8_t ret_val;
    uint16_t loop_road;

    ret_val = FALSE;

    // 先比较交流量
    for (loop_road = 0; loop_road < 4; loop_road++)
    {
        if (strcmp((char *)p_slot_name_int8, (const char *)&DIO_SLOT_NAME_DEF[loop_road][0]) == 0)
        {  // 字符串相等

            if (loop_road == 0)
            {
                *p_slot_type = DI_SLOT_TYPE;
            }
            else if (loop_road == 1)
            {
                *p_slot_type = DO_SLOT_TYPE;
            }
            else if (loop_road == 2)
            {
                *p_slot_type = AI_SLOT_TYPE;
            }
            else
            {
                *p_slot_type = OP_SLOT_TYPE;
            }
            ret_val = TRUE;
            break;
        }
    }

    return ret_val;
}
#endif

/**
 * 计算32位分组校验和，排除[CheckCode]部分及其后面的数据
 * @param p_cfgfile 配置文件内容指针
 * @return 32位无符号校验和
 */
uint32_t calc_cfg_checksum(const uint8_t *p_cfgfile)
{
    if (p_cfgfile == NULL)
    {
        return 0;
    }

    // 查找[CheckCode]标记的位置
    const char *cfg_section = "[CheckCode]";
    const uint8_t *checkcode_pos = (const uint8_t *)strstr((const char *)p_cfgfile, cfg_section);

    // 确定计算校验和的数据长度
    uint32_t length = 0;
    if (checkcode_pos != NULL)
    {
        // 检索到[CheckCode]字段，则只计算到该位置之前的数据加和
        length = (uint32_t)(checkcode_pos - p_cfgfile);
    }
    else
    {
        // 没检索到[CheckCode]，则计算整个文件
        length = strlen((const char *)p_cfgfile);
    }

    uint32_t checksum = 0;
    const uint8_t *current = p_cfgfile;
    uint32_t remaining = length;

    // 按4字节分组处理
    while (remaining >= 4)
    {
        // 构造一个32位值，按小端序处理
        uint32_t group = ((uint32_t)current[0]) | ((uint32_t)current[1] << 8) |
                         ((uint32_t)current[2] << 16) | ((uint32_t)current[3] << 24);

        checksum += group;
        current += 4;
        remaining -= 4;
    }

    // 处理不足4字节的部分
    if (remaining > 0)
    {
        uint32_t group = 0;
        // 根据剩余字节数，将数据放在低位，高位补零
        for (uint32_t i = 0; i < remaining; i++)
        {
            group |= ((uint32_t)current[i]) << (i * 8);
        }
        checksum += group;
    }

    return checksum;
}

/**
 * 移除配置文件中的所有回车换行符，返回整理后的数据长度
 * @param p_cfgfile 原始配置文件内容指针
 * @param p_cleaned_content 输出的整理后内容缓冲区
 * @param buffer_size 缓冲区大小
 * @return 整理后数据的长度
 */
uint32_t
remove_line_breaks(const uint8_t *p_cfgfile, uint8_t *p_cleaned_content, uint32_t buffer_size)
{
    if (p_cfgfile == NULL || p_cleaned_content == NULL || buffer_size == 0)
    {
        return 0;
    }

    uint32_t original_length = strlen((const char *)p_cfgfile);
    if (original_length >= buffer_size)
    {
        original_length = buffer_size - 1;
    }

    uint32_t cleaned_index = 0;
    const uint8_t *src = p_cfgfile;
    const uint8_t *end = p_cfgfile + original_length;

    // 跳过不需要的字符
    while (src < end && cleaned_index < (buffer_size - 1))
    {
        // 跳过回车符和换行符
        if (*src == '\r' || *src == '\n')
        {
            src++;
            continue;
        }

        // 复制有效字符
        p_cleaned_content[cleaned_index++] = *src++;
    }

    // 添加字符串结束符
    p_cleaned_content[cleaned_index] = '\0';

    return cleaned_index;
}
/**
 * 读取IRIGB IP配置文件的校验码
 * @return TRUE=成功, FALSE=失败
 */
int8_t read_irigb_ip_cfg_checkcode(void)
{
    char s1[32];
    char s2[32];
    char *str;
    char *p;
    int8_t space = ' ';
    int32_t i, len;

    sprintf(s1, "CheckCode");
    sprintf(s2, "CHECKSUM");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            IRIGB_IP_Cfg.cfg_file_checksum = ini_str2hex(p);
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }
    return TRUE;
}

/**
 * 读取IRIGB IP配置文件的版本信息
 * @return TRUE=成功, FALSE=失败
 */
int8_t read_irigb_ip_cfg_verinfo_cfg(void)
{
    char s1[32];
    char s2[32];
    char *str;
    char *p;
    int8_t space = ' ';
    int32_t i, len;

    sprintf(s1, "IO_Slot_Cfg");

    // 读平台类型
    sprintf(s2, "Plat_Type");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            if (STR_MAX_LEN < len)
            {
                printf("len=%d>%d", len, STR_MAX_LEN);
                len = STR_MAX_LEN - 1;
            }
            for (i = 0; i < len; i++)
            {
                IRIGB_IP_Cfg.verinfo.Plat_Type[i] = p[i];
            }
            IRIGB_IP_Cfg.verinfo.Plat_Type[i] = 0;
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    // 读装置类型
    sprintf(s2, "Dev_Type");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            if (STR_MAX_LEN < len)
            {
                printf("len=%d>%d", len, STR_MAX_LEN);
                len = STR_MAX_LEN - 1;
            }
            for (i = 0; i < len; i++)
            {
                IRIGB_IP_Cfg.verinfo.Dev_Type[i] = p[i];
            }
            IRIGB_IP_Cfg.verinfo.Dev_Type[i] = 0;
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    // 读Cfg_Ver
    sprintf(s2, "Cfg_Ver");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            if (STR_MAX_LEN < len)
            {
                printf("len=%d>%d", len, STR_MAX_LEN);
                len = STR_MAX_LEN - 1;
            }
            for (i = 0; i < len; i++)
            {
                IRIGB_IP_Cfg.verinfo.Cfg_Ver[i] = p[i];
            }
            IRIGB_IP_Cfg.verinfo.Cfg_Ver[i] = 0;
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    return TRUE;
}

/**
 * 读取IRIGB对时配置信息
 * @return TRUE=成功, FALSE=失败
 */
int8_t read_irigb_cfg(void)
{
    char s1[32];
    char s2[32];
    char *str;
    char *p;
    int8_t space = ' ';
    int32_t i, len;

    sprintf((char *)&s1[0], "Time_Param_Cfg");

    // 读对时方式
    sprintf((char *)&s2[0], "Time_Mode");
    str = (char *)ini_GetVarStr(&s1[0], &s2[0], &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            IRIGB_IP_Cfg.irigb_cfg.Time_Mode = ini_str2hex(p);
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    // 读时区
    sprintf(s2, "Time_Zone");
    str = (char *)ini_GetVarStr(&s1[0], &s2[0], &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            IRIGB_IP_Cfg.irigb_cfg.Time_Zone = ini_str2hex(p);
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    // 读B码校验方式
    sprintf(s2, "B_Code_Check");
    str = (char *)ini_GetVarStr(&s1[0], &s2[0], &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            IRIGB_IP_Cfg.irigb_cfg.B_Code_Check = ini_str2hex(p);
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    // 读B码抖动us
    sprintf(s2, "B_Code_Shake_us");
    str = (char *)ini_GetVarStr(&s1[0], &s2[0], &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            IRIGB_IP_Cfg.irigb_cfg.B_Code_Shake_us = ini_str2hex(p);
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    // 读取B码极性取反标志
    sprintf(&s2[0], "B_Code_Polarity");
    str = (char *)ini_GetVarStr(&s1[0], &s2[0], &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            IRIGB_IP_Cfg.irigb_cfg.B_Code_Polarity = ini_str2hex(p);
        }
        else
        {
            printf("Not Found");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    // 读取SNTP对时服务端的IP地址
    sprintf(s2, "Sntp_Serve_IP");
    str = (char *)ini_GetVarStr(&s1[0], &s2[0], &i);
    if (str != 0)
    {
        strncpy((char *)IRIGB_IP_Cfg.irigb_cfg.Sntp_Serve_IP, str, INET_ADDRSTRLEN - 1);
        IRIGB_IP_Cfg.irigb_cfg.Sntp_Serve_IP[INET_ADDRSTRLEN - 1] = 0;
    }
    else
    {
        printf("Not Found");
        return FALSE;
    }

    return TRUE;
}

/**
 * IP地址字符串转换为4个元素的数组
 * @param ip_str IP地址字符串，如"192.168.17.10"
 * @param ip_array 输出的IP地址数组，4个元素
 * @return TRUE=成功, FALSE=失败
 */
int8_t ip_string_to_array(const char *ip_str, uint8_t *ip_array)
{
    if (ip_str == NULL || ip_array == NULL)
    {
        return FALSE;
    }

    int a, b, c, d;
    int count = sscanf(ip_str, "%d.%d.%d.%d", &a, &b, &c, &d);

    if (count != 4)
    {
        return FALSE;
    }

    // 检查每个数字是否在有效范围内
    if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255)
    {
        return FALSE;
    }

    ip_array[0] = (uint8_t)a;
    ip_array[1] = (uint8_t)b;
    ip_array[2] = (uint8_t)c;
    ip_array[3] = (uint8_t)d;

    return TRUE;
}

/**
 * 读取设备地址配置
 * @return TRUE=成功, FALSE=失败
 */
int8_t read_device_addr_cfg(void)
{
    char *str;
    char *p;
    int32_t i, len;

    str = (char *)ini_GetVarStr("Device_Addr_Cfg", "Dev_Addr", &i);
    if (str == 0)
    {
        printf("Not Found Device_Addr_Cfg section\n");
        return FALSE;
    }

    p = (char *)ini_splitStr(str, ' ', 0, &len);
    if (p == NULL)
    {
        printf("Not Found Dev_Addr field\n");
        return FALSE;
    }

    IRIGB_IP_Cfg.device_addr_cfg.dev_addr = ini_str2hex(p);
    if (IRIGB_IP_Cfg.device_addr_cfg.dev_addr == 0 || IRIGB_IP_Cfg.device_addr_cfg.dev_addr > 255)
    {
        printf("Invalid Dev_Addr %d\n", IRIGB_IP_Cfg.device_addr_cfg.dev_addr);
        return FALSE;
    }

    IRIGB_IP_Cfg.device_addr_cfg.valid_flag = TRUE;
    DebugP_log("[DeviceAddr] Dev_Addr: %d\r\n", IRIGB_IP_Cfg.device_addr_cfg.dev_addr);
    return TRUE;
}

/**
 * 读取启动配置
 * @return TRUE=成功, FALSE=失败
 */
int8_t read_boot_cfg(void)
{
    char *str;
    char *p;
    int32_t i, len;
    uint8_t boot;
    char s1[32] = {0};

    for (int j = 0; j < sizeof(boot_cfg) / sizeof(boot_cfg[0]); ++j)
    {
        sprintf(s1, "Core%d", j);
        str = (char *)ini_GetVarStr("BOOT_CFG", s1, &i);
        if (str == 0)
        {
            printf("Not Found BOOT_CFG section\n");
            continue;
        }

        p = (char *)ini_splitStr(str, ' ', 0, &len);
        if (p == NULL)
        {
            printf("Not Found BOOT_CFG %s \n", s1);
            continue;
        }

        boot = ini_str2hex(p);
        if (boot == 0 || boot == 1 || boot == 2)
        {
            boot_cfg[j] = boot;
        }
    }

    for (int j = 0; j < 8; ++j)
    {
        printf("Core%d boot cfg=%d\n", j, boot_cfg[j]);
    }

    return TRUE;
}

/**
 * 解析端口IP配置
 * 根据Set_IP_number字段确定需要修改的外网口IP地址数量
 * @return TRUE=成功, FALSE=失败
 */
int8_t read_port_ip_cfg(void)
{
    char s1[32];
    char s2[32];
    char *str;
    char *p;
    int8_t space = ',';
    int32_t i, len;

    sprintf(s1, "Port_IP_Cfg");

    // 读取需要设置的IP个数
    sprintf(s2, "Set_IP_number");
    str = (char *)ini_GetVarStr(s1, s2, &i);
    if (str != 0)
    {
        p = (char *)ini_splitStr(str, space, 0, &len);
        if (p)
        {
            IRIGB_IP_Cfg.all_port_ip_cfg.set_ip_number = ini_str2hex(p);

            /* Bug7: 用数组实际容量做上界检查，避免后续port_ip_cfg[ip_index-1]越界；
             * set_ip_number为uint32_t，负值会变成极大数从而被此处拦截 */
            if (IRIGB_IP_Cfg.all_port_ip_cfg.set_ip_number >
                (sizeof(IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg) /
                 sizeof(IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg[0])))
            {
                printf("Set_IP_number %u exceeds maximum %u\n",
                       IRIGB_IP_Cfg.all_port_ip_cfg.set_ip_number,
                       (unsigned)(sizeof(IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg) /
                                  sizeof(IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg[0])));
                return FALSE;
            }
        }
        else
        {
            printf("Not Found Set_IP_number field\n");
            return FALSE;
        }
    }
    else
    {
        printf("Not Found Port_IP_Cfg section\n");
        return FALSE;
    }

    // 读取每个需要设置的IP配置
    for (int ip_index = 1; ip_index <= IRIGB_IP_Cfg.all_port_ip_cfg.set_ip_number; ip_index++)
    {
        sprintf(&s2[0], "Set_Eth_IP_%d", ip_index);
        str = (char *)ini_GetVarStr(&s1[0], &s2[0], &i);
        if (str != 0)
        {
            // 解析Core号
            p = (char *)ini_splitStr(str, space, 0, &len);
            if (p)
            {
                uint8_t core_num = ini_str2hex(p);
                if (core_num < 1 || core_num > 4)
                {
                    printf("Invalid Core number %d in Set_Eth_IP_%d (must be 1-4)\n",
                           core_num,
                           ip_index);
                    return FALSE;
                }
                IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg[ip_index - 1].core_num = core_num;
            }
            else
            {
                printf("Not Found Core number in Set_Eth_IP_%d\n", ip_index);
                return FALSE;
            }

            // 解析Eth序号
            p = (char *)ini_splitStr(str, space, 1, &len);
            if (p)
            {
                uint8_t eth_num = ini_str2hex(p);
                if (eth_num < 1 || eth_num > 16)
                {
                    printf("Invalid Eth number %d in Set_Eth_IP_%d (must be 1-16)\n",
                           eth_num,
                           ip_index);
                    return FALSE;
                }
                IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg[ip_index - 1].eth_num = eth_num;
            }
            else
            {
                printf("Not Found Eth number in Set_Eth_IP_%d\n", ip_index);
                return FALSE;
            }

            // 解析IP地址
            p = (char *)ini_splitStr(str, space, 2, &len);
            if (p)
            {
                strncpy((char *)IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg[ip_index - 1].ip_addr,
                        p,
                        INET_ADDRSTRLEN - 1);
                IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg[ip_index - 1]
                    .ip_addr[INET_ADDRSTRLEN - 1] = 0;
            }
            else
            {
                printf("Not Found IP address in Set_Eth_IP_%d\n", ip_index);
                return FALSE;
            }

            // 打印解析结果用于调试
            DebugP_log("[PortIP] IP %d: Core=%d, Eth=%d, IP=%s\r\n",
                       ip_index,
                       IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg[ip_index - 1].core_num,
                       IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg[ip_index - 1].eth_num,
                       IRIGB_IP_Cfg.all_port_ip_cfg.port_ip_cfg[ip_index - 1].ip_addr);
        }
        else
        {
            printf("Not Found Set_Eth_IP_%d\n", ip_index);
            return FALSE;
        }
    }

    return TRUE;
}

#endif