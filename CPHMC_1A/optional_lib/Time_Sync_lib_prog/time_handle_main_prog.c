#include <string.h>

#include "sync_time_logic/sync_logic_prog.h"
#include "time_handle_main_prog.h"
#include "irigb_decoder.h"
#include "cfg_prase_app.h"
#include "sntp_client/sntp_client.h"

#define SWITCH_ON      0xaa
#define SWITCH_OFF     0x55

IRIGB_SYNC_MONITOR_INF_STRUCT Irigb_sync_monitor_inf;

IRIGB_TASK_TIME_STRUCT Irigb_task_time;
TIME_TEST_INF_STRUCT Share_Ram_time_test_inf;

// 外部变量声明：B码和PPS跳变源头累计次数
extern uint32_t irigb_edge_jump_count;
extern uint32_t pps_edge_jump_count;

void irigb_sync_task_10s(void);
void irigb_sync_task_10min(void);
void input_shareram_irigb_test_inf_1s(void);

// ###########################################################################################
//										程序区
// ###########################################################################################
void init_irigb_sync_prog(void)
{
    memset((int8_t *)&Irigb_task_time, 0, sizeof(Irigb_task_time));
    decoder_irigb_init();
    synctime_time_init();
}

extern uint32_t ISR_TimeValues[4];

// 同步对时主程序
void synctime_main_prog(void)
{
    irigb_decoder_main();          // IRIGB报文解析主程序
    synctime_time_run_hard_int();  // 秒等分维护

    if (Irigb_shake_inf.Irigb_rec_ok_flag == SWITCH_ON)
    {  // 接收到B码标志
        Irigb_shake_inf.Irigb_rec_ok_flag = 0;
        check_Irigb_pps_shake();  // 判B码抖动
        if (check_Irigb_pps_delta() == TRUE)
        {  // 2个PPS之间的误差大于10us
            Irigb_shake_inf.PPS_Edge_Err_Over_Number++;
            Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_FPGA_PPS_EDGE_DIFF_ERR_NUMBER]++;  // 秒沿超标计数
        }
        else
        {
            Irigb_shake_inf.PPS_Edge_Err_Over_Number = 0;
        }
        synctime_sync_statemachine();  // 合并单元的同步时钟逻辑，1.同步--守时--失步
                                       // 之间的切换逻辑，2.各守时模式m_KeepMode（0、1、2）的切换
        Irigb_task_time.send_cpu_2_fpga_inf_flag = SWITCH_ON;
    }
    else
    {
        if (Irigb_shake_inf.i_SrcAlive == FALSE)
        {  // 对时源没有
            if (Clk_Time_Edge.nFrc == Irigb_setting.send_2_FPGA_sync_inf_time)
            {
                synctime_sync_statemachine();  // 合并单元的同步时钟逻辑，1.同步--守时--失步
                                               // 之间的切换逻辑，2.各守时模式m_KeepMode（0、1、2）的切换
                if (sync_logic_inf.m_SyncState == eSCHS_Lose)
                {  // 在失步状态
                    Irigb_shake_inf.PPS_Edge_Err_Over_Number++;
                }
                Irigb_task_time.send_cpu_2_fpga_inf_flag = SWITCH_ON;
            }
        }
    }

    if (Irigb_task_time.send_cpu_2_fpga_inf_flag == SWITCH_ON)
    {
        Irigb_task_time.send_cpu_2_fpga_inf_flag = 0;
        send_cpu_2_fpga_inf_prog();  // 给FPGA的信息
    }
    Clk_Time_Edge.sTimeZone_min = Irigb_setting.nTZShift_min;  // 时区
    Clk_Time_Edge.uSec_100_cnt = (Clk_Time_Edge.nFrc * init_param.core0_main_intr_time) / 100;
    Write_ShareRam_Clk_time_Edge(&Clk_Time_Edge);              // 向共享内存写对时信息

    // ISR_TimeValues[0]：硬中断间隔
    // ISR_TimeValues[1]：R0没用
    // ISR_TimeValues[2]：硬中断总执行时间
    // ISR_TimeValues[3]：硬中断最大执行时间
    if (ISR_TimeValues[2] > Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_CORE0_HARD_INT_COST_TIME_25M])
    {
        Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_CORE0_HARD_INT_COST_TIME_25M] = ISR_TimeValues[2];
    }
}

void irigb_sync_task_1s(void)
{
    Irigb_task_time.start_time_1s++;
    Irigb_task_time.task_10s++;
    Irigb_task_time.task_10min++;
    Irigb_task_time.nEdgeRevThld_time_1s++;

    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_FOLLOW_TIME_SEC]++;
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_KEEP_TIME_SEC]++;
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_LOST_TIME_SEC]++;
    Irigb_sync_monitor_inf.count_utc_1s++;

    sync_logic_inf.m_UnFollowTimeSec++;

    if (Irigb_task_time.start_time_1s >= 10000)
    {
        Irigb_task_time.start_time_1s = 10000;
    }

    input_shareram_irigb_test_inf_1s();

    Irigb_shake_inf.not_rec_Irigb_time_1s++;
    if (Irigb_shake_inf.not_rec_Irigb_time_1s > 3)
    {  // 连续3秒没有收到B码
        Irigb_shake_inf.continue_rec_Irigb_number = 0;
        Irigb_shake_inf.i_SrcAlive = FALSE;            // 对时源没有
        Irigb_shake_inf.PPS_Edge_Err_Over_Number = 0;  // 清秒沿之间抖动大于阈值的次数
        Irigb_shake_inf.Irigb_width_shake_number = 0;
        Irigb_shake_inf.Irigb_width_no_shake_number = 0;
    }

    if (Irigb_task_time.task_10s >= 10)
    {
        Irigb_task_time.task_10s = 0;
        irigb_sync_task_10s();
    }

    if (Irigb_task_time.task_10min >= 120)
    {  // 2分钟
        Irigb_task_time.task_10min = 0;
        irigb_sync_task_10min();
    }

    sntp_poll();
    sync_logic_update_active_source();
}

void irigb_sync_task_10s(void)
{
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_REC_IRIGB_NO_SINGLE_TIME_250US] = 0;  // 10秒清除无信号计时
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_CORE0_HARD_INT_COST_TIME_25M]       = 0;
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MIN_HARD_INT_PERIOD] = 0xFFFFFFFF;
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_HARD_INT_PERIOD] = 0;
}

void irigb_sync_task_10min(void)
{
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_IRIGB_WIDTH_SHAKE_TIME_125M]   = 0;
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_PPS_OUT_IRIGB_NEDGE_DIFF_125M] = 0;

}

// ###############################################################################################
// B码调试信息
void input_shareram_irigb_test_inf_1s(void)
{
uint32_t	uint32_temp;
    // 填写bit信息
    Share_Ram_time_test_inf.time_test_bit_state = 0;
    
    // 当前对时源判断（根据g_sync_active_source）
    uint32_t active_src = sync_logic_get_active_source();
    if (active_src == TIME_SYNC_ACTIVE_IRIGB)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_CURRENT_SYNC_SOURCE_IRIGB);
    }
    else if (active_src == TIME_SYNC_ACTIVE_SNTP)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_CURRENT_SYNC_SOURCE_SNTP);
    }
    // TODO: 如果后续有 1588 支持，可在这里添加判断并设置 TEST_INF_BIT_CURRENT_SYNC_SOURCE_1588

    // 同步逻辑状态跟随
    if (sync_logic_inf.m_SyncState == eSCHS_Follow)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_SYNC_LOGIC_STATE_FOLLOW);
    }
    // 同步逻辑状态守时
    if (sync_logic_inf.m_SyncState == eSCHS_Keeping)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_SYNC_LOGIC_STATE_KEEP);
    }
    // 同步逻辑状态失步
    if (sync_logic_inf.m_SyncState == eSCHS_Lose)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_SYNC_LOGIC_STATE_LOSE);
    }
    // CPU给FPGA同步状态
    if ((cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_FOLLOW) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_CPU_2_FPGA_FOLLOW_STATE);
    }
    // CPU给FPGA守时状态
    if ((cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_KEEP) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_CPU_2_FPGA_KEEP_STATE);
    }
    // CPU给FPGA失步状态
    if ((cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_LOST) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_CPU_2_FPGA_LOST_STATE);
    }
    // CPU给FPGA时钟源跳变
    if ((cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_SRC_JUMP) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_CPU_2_FPGA_SRC_JUMP_STATE);
    }
    // CPU给FPGA时钟源有效
    if ((cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_SRC_VALID) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_CPU_2_FPGA_SRC_VALID_STATE);
    }
    // FPGA给CPU同步状态
    if ((Irigb_sync_monitor_inf.Fpga_2_cpu_Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_FOLLOW) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_FPGA_2_CPU_FOLLOW_STATE);
    }

    // FPGA给CPU守时状态
    if ((Irigb_sync_monitor_inf.Fpga_2_cpu_Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_KEEP) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_FPGA_2_CPU_KEEP_STATE);
    }
    // FPGA给CPU失步状态
    if ((Irigb_sync_monitor_inf.Fpga_2_cpu_Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_LOST) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_FPGA_2_CPU_LOST_STATE);
    }
    // FPGA给CPU时钟源跳变
    if ((Irigb_sync_monitor_inf.Fpga_2_cpu_Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_SRC_JUMP) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_FPGA_2_CPU_JUMP_STATE);
    }
    // FPGA给CPU时钟源有效
    if ((Irigb_sync_monitor_inf.Fpga_2_cpu_Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_SRC_VALID) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_FPGA_2_CPU_VALID_STATE);
    }
    // FPGA给CPU秒脉宽正常
    if ((Irigb_sync_monitor_inf.Fpga_2_cpu_Last_Irigb_Rec_Reliably & IRIGB_RELIABLE_STATE_FPGA_PPS_WIDTH_OK) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_FPGA_2_CPU_PPS_WIDTH_OK);
    }
    // B码对时状态
    //  无信号, 约50ms无脉冲置位;捕获到脉冲立即清除
    if ((Irigb_Decoder_lib.nErrorState & IRIG_B_NO_SIGNAL) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_IRIG_B_NO_SIGNAL);
    }
    // 校验错-码元错, 码元序列错置位;解完前75个连续码元(如果有校验还要校验正确)清除
    if ((Irigb_Decoder_lib.nErrorState & IRIG_B_ERR_CODEELE) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_IRIG_B_ERR_CODEELE);
    }
    // 校验错-校验和错, 有校验且校验和错误时置位;下一秒校验和正确时清除
    if ((Irigb_Decoder_lib.nErrorState & IRIG_B_ERR_CHECKSUM) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_IRIG_B_ERR_CHECKSUM);
    }
    // 校验错-闰秒标志非法, 检测到非法闰秒标志时置位;下一秒无非法闰秒则清除
    if ((Irigb_Decoder_lib.nErrorState & IRIG_B_ERR_LEAPSEC) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_IRIG_B_ERR_LEAPSEC);
    }
    // 时间质量无效, 当时间质量为 大于4us 时置位;小于4us时清除
    if ((Irigb_Decoder_lib.nErrorState & IRIG_B_ERR_TQ) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_IRIG_B_ERR_TQ);
    }
    // 闰秒预告 闰秒之前的所有59秒均置1
    if ((Irigb_Decoder_lib.LeapSecondFlag & SYNCLK_LS_FRCST_MASK) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_SYNCLK_LS_FRCST_MASK);
    }
    // 闰秒方向，0=正闰秒 1=负闰秒
    if ((Irigb_Decoder_lib.LeapSecondFlag & SYNCLK_LS_DIR_MASK) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_SYNCLK_LS_DIR_MASK);
    }
    // 正闰秒发生标志 仅在第60"闰秒时刻置1
    if ((Irigb_Decoder_lib.LeapSecondFlag & SYNCLK_LS_POS_MASK) != 0)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_SYNCLK_LS_POS_MASK);
    }
    // Irigb取反标志
    if (Irigb_Decoder_lib.nEdgeRevert_flag == 1)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_IRIGB_EDGEREVERT_FLAG);
    }
    // Irigb奇校验标志
    if (Irigb_Decoder_lib.checkMode == eIRIGB_Chk_Odd)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_IRIGB_CHECK_ODD_FLAG);
    }
    // Irigb偶校验标志
    if (Irigb_Decoder_lib.checkMode == eIRIGB_Chk_Even)
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_IRIGB_CHECK_EVEN_FLAG);
    }
    // B码配置异常（解析异常或范围校验异常）
    if ((cfg_prase_Monitor_Inf.cfg_err_number > 0) &&
        (((cfg_prase_Monitor_Inf.cfg_err_location >= 1) && (cfg_prase_Monitor_Inf.cfg_err_location <= 20)) ||
         (cfg_prase_Monitor_Inf.cfg_err_location >= 40)))
    {
        Share_Ram_time_test_inf.time_test_bit_state |= (1 << TEST_INF_BIT_IRIGB_CFG_ERR);
    }

    // 设置跳变源头累计次数：PPS * 1000000 + B码 * 1
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_IRIGB_SRC_JUMP_NUMBER] = pps_edge_jump_count * 1000000 + (irigb_edge_jump_count % 1000000);
    // 当前对时模式
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_CURRENT_SYNC_MODE] = Irigb_user_config.SyncMode;
    // B码配置异常信息：次数*100+定位
    Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_IRIGB_CFG_ERR_INF] =
        cfg_prase_Monitor_Inf.cfg_err_number * 100 + cfg_prase_Monitor_Inf.cfg_err_location;

    Write_ShareRam_Time_test_inf(&Share_Ram_time_test_inf);
}