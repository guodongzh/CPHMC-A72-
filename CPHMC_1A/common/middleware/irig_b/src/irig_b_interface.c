/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       irig_b_interface.c
 *@author     wenjunf
 *@date       2025.09.18
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.09.18  1.0       wenjunf    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "irig_b_interface.h"
#include "pcie_fpga.h"
#include "ipc_common.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
uint32_t g_rx_slow_train_header_vld = 0;  // 慢速索引车数据是否有效标志
//FIXME 需要增加共享内存定义
SHARE_IRIGB_RAM_STRUCT g_shm_irigb_info __attribute__((section(".irigb_shm_section"))) = {0};
SHARE_IRIGB_RAM_STRUCT g_shm_irigb_info_c6x;
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// 功能：从FPGA读入B码接收信息接口程序
// 输入：p_Irigb_Fpga_2_cpu_interface--驱动需要填写的结构体指针
// 返回：TRUE--数据已刷新，FALSE--数据没有刷新
bool Read_Fpga_2_Cpu_Irigb_rec_inf(IRIGB_FPGA_2_CPU_INTERFACE_STRUCT *p_Irigb_Fpga_2_cpu_interface)
{
    train_rx_t *slow_rx_train = irig_b_train_rx;
    static uint16_t updata_old_cnt = 0;
    bool status = FALSE;

    pcie_sys_status_data_t *p_sys_status_data = (pcie_sys_status_data_t *)slow_rx_train->header->sys_status_data;
    if (!g_rx_slow_train_header_vld)
    {
        memset(p_Irigb_Fpga_2_cpu_interface, 0, sizeof(IRIGB_FPGA_2_CPU_INTERFACE_STRUCT));
        return FALSE;
    }
    bool is_refresh_flag = check_frame_u16_cnt_add(p_sys_status_data->updata_cnt, updata_old_cnt);
    if (is_refresh_flag)
    {
        status = TRUE;  // 数据已刷新
    }
    else
    {
        status = FALSE;  // 数据没有刷新
    }
    updata_old_cnt = p_sys_status_data->updata_cnt;
    p_Irigb_Fpga_2_cpu_interface->Irigb_edgePolarity = p_sys_status_data->Irigb_edgePolarity;
    p_Irigb_Fpga_2_cpu_interface->Last_Irigb_Rec_Reliably = p_sys_status_data->Last_Irigb_Rec_Reliably;
    p_Irigb_Fpga_2_cpu_interface->Irigb_CapValue = p_sys_status_data->Irigb_CapValue;
    p_Irigb_Fpga_2_cpu_interface->PPS_Rising_Edge_CapValue = p_sys_status_data->PPS_Rising_Edge_CapValue;
    return status;
}

// 功能：从CPU写入FPGA的B码发送信息接口程序
// 输入：p_Irigb_cpu_2_Fpga_interface--驱动需要填写的结构体指针
void Write_Fpga_from_Cpu_Irigb_rec_inf(IRIGB_CPU_2_FPGA_INTERFACE_STRUCT *p_Irigb_cpu_2_Fpga_interface)
{
    train_tx_t *slow_tx_train = irig_b_train_tx;
    pcie_sys_cmd_data_t *p_sys_cmd_data = (pcie_sys_cmd_data_t *)slow_tx_train->header->cmd;
    p_sys_cmd_data->cmd_id = p_Irigb_cpu_2_Fpga_interface->cmd_id;
    p_sys_cmd_data->updata_cnt = p_Irigb_cpu_2_Fpga_interface->updata_cnt;
    p_sys_cmd_data->Last_Irigb_Rec_Reliably = p_Irigb_cpu_2_Fpga_interface->Last_Irigb_Rec_Reliably;
    p_sys_cmd_data->Irigb_Reverse_Polarity_flag = p_Irigb_cpu_2_Fpga_interface->Irigb_Reverse_Polarity_flag;
}

// 读中断时刻FPGA的计数值
// 输出：p_int_Fpga_count--本次中断的FPGA的计数值
// 返回：TRUE--计数值已刷新，FALSE--计数值没有更新
bool Read_int_Fpga_count(uint32_t *p_int_Fpga_count)
{
    train_rx_t *slow_rx_train = irig_b_train_rx;
    static uint32_t updata_old_cnt = 0;
    bool status = FALSE;

    if (!g_rx_slow_train_header_vld)
    {
        return FALSE;
    }
    bool is_refresh_flag = check_frame_u32_cnt_add(slow_rx_train->header->irq_time_stamp, updata_old_cnt);
    if (is_refresh_flag)
    {
        status = TRUE;  // 计数值已刷新
    }
    else
    {
        status = FALSE;  // 计数值没有更新
    }
    updata_old_cnt = slow_rx_train->header->irq_time_stamp;
    *p_int_Fpga_count = slow_rx_train->header->irq_time_stamp;
    return status;
}

// 向共享内存写对时沿信息
void Write_ShareRam_Clk_time_Edge(CLK_TIME_EDGE *p_Clk_time_Edge)
{
    memcpy(&g_shm_irigb_info, p_Clk_time_Edge, sizeof(CLK_TIME_EDGE));
    cache_wb_com(&g_shm_irigb_info.Clk_time_Edge, sizeof(CLK_TIME_EDGE), CacheP_TYPE_ALL);
}

// 向共享内存写对时调试信息
void Write_ShareRam_Time_test_inf(TIME_TEST_INF_STRUCT *p_time_test_inf)
{
    memcpy(&g_shm_irigb_info.time_test_inf, p_time_test_inf, sizeof(TIME_TEST_INF_STRUCT));
    cache_wb_com(&g_shm_irigb_info.time_test_inf, sizeof(TIME_TEST_INF_STRUCT), CacheP_TYPE_ALL);
}

// 读共享内存对时沿信息
void Read_ShareRam_Clk_time_Edge(CLK_TIME_EDGE *p_Clk_time_Edge)
{
    cache_inv_com(&g_shm_irigb_info.Clk_time_Edge, sizeof(CLK_TIME_EDGE), CacheP_TYPE_ALL);
    memcpy(p_Clk_time_Edge, &g_shm_irigb_info.Clk_time_Edge, sizeof(CLK_TIME_EDGE));
}

// 向共享内存写对时调试信息
void Read_ShareRam_Time_test_inf(TIME_TEST_INF_STRUCT *p_time_test_inf)
{
    cache_inv_com(&g_shm_irigb_info.time_test_inf, sizeof(TIME_TEST_INF_STRUCT), CacheP_TYPE_ALL);
    memcpy(p_time_test_inf, &g_shm_irigb_info.time_test_inf, sizeof(TIME_TEST_INF_STRUCT));
}

//将UTC时间转换为年月日十分秒
void UtcTime_To_StruTime_app(APP_UTC_TIME * pUtcTime_src, APP_StruTime * pStruTime_dest, int16_t nTZShift_min)
{

    uint32_t Secs;
    uint32_t year_elapsed=0;
    int i=0;
    uint32_t day_elapsed,tmp1,tmp2;
    uint32_t sec_elapsed;

    if(pUtcTime_src->utc_secs>(uint32_t)(BASE_SECOND2-(nTZShift_min *60))){
        Secs = pUtcTime_src->utc_secs + (nTZShift_min *60) - BASE_SECOND2;
    }
    else{
        Secs=0;
    }

    if ((pUtcTime_src->qflags & 0x80)==0x80){				//闰秒
        Secs-=1;
    }

    day_elapsed=Secs/86400L; // day elapsed from 2000/1/1/00:00:00
    sec_elapsed=Secs-day_elapsed*86400L; // seconds elapsed from 00:00:00 of today
    pStruTime_dest->Hour=sec_elapsed/3600; // hours elapsed from 0 o'clock of today
    pStruTime_dest->Min=(sec_elapsed-pStruTime_dest->Hour*3600)/60; // Minites elapsed from this hour
    pStruTime_dest->Sec=sec_elapsed-pStruTime_dest->Hour*3600-pStruTime_dest->Min*60;

    tmp1=tmp2=0;
    if(day_elapsed >= 366) // added by XiaoZQ ,2009-9-28
    {
        for(i=0;i<150;i++) // count year_elapsed from 2000
        {
            if(day_elapsed<tmp1){
                break;
            }
            if(i == ((i/4)*4)){				//闰年 366
                if ( (day_elapsed-tmp1) < 366 ){
                    break;
                }
            }
            else{							//其它为365

                if ( (day_elapsed-tmp1) < 365 ){
                    break;
                }
            }
            if( i == ((i/4)*4) ){
                tmp1++; // leap year 2009-10-8 17:00:07 hx chg
            }
            tmp1 += 365; // on more year elapsed
        }
    }
    year_elapsed = i;
    pStruTime_dest->Year = i +2000;
    day_elapsed-=tmp1; // day elapsed from the beginning of this year

    tmp1=0;
    for(i=1;i<=12;i++)						// count month_elapsed from the beginning of this year
    {
        if ( (i==1) || (i==3) || (i==5) || (i==7) || (i==8) || (i==10) || (i==12)){
            tmp2=31;
        }
        else if ( i==2 )
        {
            if(year_elapsed == ((year_elapsed/4)*4)){// leap year
                tmp2=29;
            }
            else{
                tmp2=28;
            }
        }
        else{
            tmp2=30;
        }
        if( day_elapsed<(tmp2+tmp1) ){
            break;
        }
        tmp1+=tmp2;	// a month passed
    }
    pStruTime_dest->Month = i;
    day_elapsed -= tmp1;
    pStruTime_dest->Date = day_elapsed + 1;

    pStruTime_dest->uSec_100 = pUtcTime_src->uSec_100;
    pStruTime_dest->uSec_100= pStruTime_dest->uSec_100 % 10000;		//100us为单位

    if(pStruTime_dest->Year>=2100){
        pStruTime_dest->Year=pStruTime_dest->Year % 2100;
    }
    if(pStruTime_dest->Month>12){
        pStruTime_dest->Month = pStruTime_dest->Month % 12;
    }
    if(pStruTime_dest->Date >31){
        pStruTime_dest->Date=pStruTime_dest->Date % 31;
    }
    if(pStruTime_dest->Hour>=24){
        pStruTime_dest->Hour=pStruTime_dest->Hour %24;
    }
    if(pStruTime_dest->Min>=60){
        pStruTime_dest->Min=pStruTime_dest->Min %60;
    }
    if ((pUtcTime_src->qflags & 0x80)==0x80){				//闰秒
        pStruTime_dest->Sec ++;
    }
    if(pStruTime_dest->Sec>=60){
        if ((pUtcTime_src->qflags & 0x80)!=0x80){
            pStruTime_dest->Sec=pStruTime_dest->Sec % 59;
        }
    }
}