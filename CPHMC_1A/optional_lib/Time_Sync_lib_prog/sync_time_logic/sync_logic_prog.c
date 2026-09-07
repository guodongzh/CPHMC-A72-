
#include "sync_logic_prog.h"
#include "pcie_fpga.h"
#include "app_main.h"
#include "time_handle_main_prog.h"
#include "irigb_decoder.h"
#include "cfg_prase_app.h"
#include "sntp_client/sntp_client.h"
#include <math.h>

IRIGB_SHAKE_INF_STRUCT	Irigb_shake_inf;
IRIGB_SETTING_STRUCT	Irigb_setting;
SYNC_LOGIC_INF_STRUCT	sync_logic_inf;
CLK_TIME_EDGE			Clk_Time_Edge;						//对时信息

IRIGB_CPU_2_FPGA_INTERFACE_STRUCT	cpu_2_FPGA_sync_inf_interface;
static uint32_t g_sync_active_source = TIME_SYNC_ACTIVE_IRIGB;
static bool g_has_ever_synced_irigb = false;

IRIGB_USER_CONFIG_STRUCT Irigb_user_config = {
	0,			// Irigb_Reverse_Polarity_flag: 0:不取反
	0,			// checkMode: 0:无校验(eIRIGB_Chk_None)
	15,		    // PPS_SyncThld_us: 10us
	TIME_SYNC_MODE_DUAL_REDUNDANT,	// SyncMode: 1:仅IRIGB对时 2:仅SNTP对时 3:优先IRIGB对时，其次SNTP对时
	8 * 60		// nTZShift_min: 北京时区
};

//同步逻辑初始化程序
void synctime_time_init(void)
{
uint32_t nRtcSoc;
uint32_t	temp_uint32_t;
const IRIGB_USER_CONFIG_STRUCT default_irigb_user_config = {
	0,			// Irigb_Reverse_Polarity_flag: 0:不取反
	0,			// checkMode: 0:无校验(eIRIGB_Chk_None)
	15,		    // PPS_SyncThld_us: 默认15us
	TIME_SYNC_MODE_DUAL_REDUNDANT,	// SyncMode: 1:仅IRIGB对时 2:仅SNTP对时 3:优先IRIGB对时，其次SNTP对时
	8 * 60		// nTZShift_min: 北京时区
};

	memset((int8_t *)&Irigb_shake_inf,0,sizeof(Irigb_shake_inf));
	memset((int8_t *)&sync_logic_inf,0,sizeof(sync_logic_inf));
	memset((int8_t *)&Clk_Time_Edge,0,sizeof(Clk_Time_Edge));						//对时信息

	// 先使用默认配置，再按配置文件有效值逐项覆盖
	Irigb_user_config = default_irigb_user_config;
	if (IRIGB_IP_Cfg.irigb_cfg.valid_flag == TRUE)
	{
		// 时区用户可自由配置，不做范围限制
		Irigb_user_config.nTZShift_min = IRIGB_IP_Cfg.irigb_cfg.Time_Zone;

		// 对时模式仅允许 1/2/3
		if ((IRIGB_IP_Cfg.irigb_cfg.Time_Mode >= TIME_SYNC_MODE_IRIGB_ONLY) &&
			(IRIGB_IP_Cfg.irigb_cfg.Time_Mode <= TIME_SYNC_MODE_DUAL_REDUNDANT))
		{
			Irigb_user_config.SyncMode = IRIGB_IP_Cfg.irigb_cfg.Time_Mode;
		}
		else
		{
			cfg_prase_Monitor_Inf.cfg_err_number++;
			cfg_prase_Monitor_Inf.cfg_err_location = 40;
		}

		// B码PPS秒沿抖动门槛值不能为0
		if (IRIGB_IP_Cfg.irigb_cfg.B_Code_Shake_us != 0)
		{
			Irigb_user_config.PPS_SyncThld_us = IRIGB_IP_Cfg.irigb_cfg.B_Code_Shake_us;
		}
		else
		{
			cfg_prase_Monitor_Inf.cfg_err_number++;
			cfg_prase_Monitor_Inf.cfg_err_location = 41;
		}

		// B码检验模式仅允许 0/1/2
		if (IRIGB_IP_Cfg.irigb_cfg.B_Code_Check <= 2)
		{
			Irigb_user_config.checkMode = IRIGB_IP_Cfg.irigb_cfg.B_Code_Check;
		}
		else
		{
			cfg_prase_Monitor_Inf.cfg_err_number++;
			cfg_prase_Monitor_Inf.cfg_err_location = 42;
		}

		// B码极性取反标志仅允许 0/1
		if (IRIGB_IP_Cfg.irigb_cfg.B_Code_Polarity <= 1)
		{
			Irigb_user_config.Irigb_Reverse_Polarity_flag = IRIGB_IP_Cfg.irigb_cfg.B_Code_Polarity;
		}
		else
		{
			cfg_prase_Monitor_Inf.cfg_err_number++;
			cfg_prase_Monitor_Inf.cfg_err_location = 43;
		}
	}
//设置秒沿误差门槛值
	temp_uint32_t= Irigb_user_config.PPS_SyncThld_us * 1000;				//10us 同步判断门槛单位微秒 大于该值时判为失步
	Irigb_setting.PPS_SyncThldClk = (temp_uint32_t * FPGA_CLK_1US) / 1000;		//转换为FPGA的时钟个数

//设置抖动门槛值
	temp_uint32_t= 10000;														//10us 抖动判断门槛单位纳秒 大于该值时判为失步
	Irigb_setting.shake_ThldClk = (temp_uint32_t * FPGA_CLK_1US) / 1000;		//转换为FPGA的时钟个数

//设置抖动门槛值
	temp_uint32_t= 100000;													//100us 输出PPS跳变单位纳秒 大于该值时判为失步
	Irigb_setting.PPS_JumpClk = (temp_uint32_t * FPGA_CLK_1US) / 1000;		//转换为FPGA的时钟个数

//设置中断
	Irigb_setting.r_IRQFreq = (1000000/init_param.core0_main_intr_time);					//Core0的硬中断时间，由Core1初始化，通过共享内存获得
	Irigb_setting.send_2_FPGA_sync_inf_time=(Irigb_setting.r_IRQFreq *9)/10;		//向FPGA发送同步时刻
	Irigb_setting.nTZShift_min = Irigb_user_config.nTZShift_min;						//不采用B码时区时，用户设置的时区   默认为北京时区		

	temp_uint32_t=62500;										//62.5ms
	Irigb_setting.Irigb_idle_time_hard_int_number=temp_uint32_t/(1000000/Irigb_setting.r_IRQFreq);	//62.5ms的硬中断次数

	Irigb_shake_inf.i_SrcAlive = FALSE;										//没有时钟

	RtcGetTimeSoc(&nRtcSoc);				//读时钟	
	if (nRtcSoc < 946684800)
	{
		nRtcSoc = 946684800;
	}

	Clk_Time_Edge.nUTC = nRtcSoc;						//对时源SOC时标
	Clk_Time_Edge.nTimeQuality = 0x0F;					//对时源品质

}

uint32_t sync_logic_get_active_source(void)
{
	return g_sync_active_source;
}

bool sync_logic_allow_irigb_update(void)
{
	if (Irigb_user_config.SyncMode == TIME_SYNC_MODE_SNTP_ONLY)
	{
		return false;
	}
	if (Irigb_user_config.SyncMode == TIME_SYNC_MODE_IRIGB_ONLY)
	{
		return true;
	}
	return (g_sync_active_source == TIME_SYNC_ACTIVE_IRIGB);
}

bool sync_logic_allow_sntp_update(void)
{
	if (Irigb_user_config.SyncMode == TIME_SYNC_MODE_SNTP_ONLY)
	{
		return true;
	}
	if (Irigb_user_config.SyncMode == TIME_SYNC_MODE_IRIGB_ONLY)
	{
		return false;
	}
	if (g_sync_active_source == TIME_SYNC_ACTIVE_SNTP)
	{
		return true;
	}
	if (g_has_ever_synced_irigb == false)
	{
		if (Irigb_shake_inf.not_rec_Irigb_time_1s >= 30)
		{
			return true;
		}
	}
	else
	{
		if (sync_logic_inf.m_SyncState == eSCHS_Lose)
		{
			return true;
		}
	}
	return false;
}

void sync_logic_update_active_source(void)
{
	if (Irigb_user_config.SyncMode == TIME_SYNC_MODE_IRIGB_ONLY)
	{
		g_sync_active_source = TIME_SYNC_ACTIVE_IRIGB;
	}
	else if (Irigb_user_config.SyncMode == TIME_SYNC_MODE_SNTP_ONLY)
	{
		g_sync_active_source = TIME_SYNC_ACTIVE_SNTP;
	}
	else if (g_sync_active_source == TIME_SYNC_ACTIVE_IRIGB)
	{
		if (sntp_is_valid() == true)
		{
			if (g_has_ever_synced_irigb == false)
			{
				if (Irigb_shake_inf.not_rec_Irigb_time_1s >= 30)
				{
					g_sync_active_source = TIME_SYNC_ACTIVE_SNTP;
				}
			}
			else
			{
				if (sync_logic_inf.m_SyncState == eSCHS_Lose)
				{
					g_sync_active_source = TIME_SYNC_ACTIVE_SNTP;
				}
			}
		}
	}
	else
	{
		if ((Irigb_shake_inf.i_SrcAlive == TRUE) && (Irigb_shake_inf.Irigb_width_no_shake_number >= 9))
		{
			g_sync_active_source = TIME_SYNC_ACTIVE_IRIGB;
		}
	}

	if (g_sync_active_source == TIME_SYNC_ACTIVE_SNTP)
	{
		if (sntp_is_valid() == true)
		{
			Clk_Time_Edge.nTimeState = SYNCLK_SYNC_SRC_OK_MASK | SYNCLK_SYNC_FOLLOW_MASK;
			Clk_Time_Edge.nTimeQuality = 0x00;
		}
		else
		{
			Clk_Time_Edge.nTimeState = 0;
			Clk_Time_Edge.nTimeQuality = 0x0F;
		}
	}
}
//检查Irigb与内部PPS沿之间误差
// TRUE--误差大于10us ，FALSE--误差小于10us
int8_t check_Irigb_pps_delta(void)
{
uint32_t   delta_temp_uint32_t;
int8_t	 ret_val;

	ret_val =FALSE;

	Irigb_shake_inf.Irigb_nHwClkPeriod_old=Irigb_Decoder_lib.issueEdge.nHwClkPeriod;
//检查2个秒沿之间的误差
	if(Irigb_shake_inf.i_SrcAlive == TRUE){				//对时源正常
	    if(Irigb_Decoder_lib.issueEdge.nHwClkTag>Locate_out_pps_inf.PPS_nHwClockTag_Rise_new){
	        delta_temp_uint32_t=Irigb_Decoder_lib.issueEdge.nHwClkTag-Locate_out_pps_inf.PPS_nHwClockTag_Rise_new;
	    }
	    else{
	        delta_temp_uint32_t=Locate_out_pps_inf.PPS_nHwClockTag_Rise_new-Irigb_Decoder_lib.issueEdge.nHwClkTag;
	    }

		if(delta_temp_uint32_t>=Locate_out_pps_inf.nQuartzPropBrif){				//此处为了预防连续大于两个周期
		    delta_temp_uint32_t= delta_temp_uint32_t -Locate_out_pps_inf.nQuartzPropBrif;
		}
		if(delta_temp_uint32_t>=Locate_out_pps_inf.nQuartzPropBrif){
		    delta_temp_uint32_t=delta_temp_uint32_t-Locate_out_pps_inf.nQuartzPropBrif;
		}
//如误差大于500ms后，将1000ms-差值
		if((delta_temp_uint32_t>(Locate_out_pps_inf.nQuartzPropBrif>>1))&&(delta_temp_uint32_t<Locate_out_pps_inf.nQuartzPropBrif)){
		    delta_temp_uint32_t=Locate_out_pps_inf.nQuartzPropBrif-delta_temp_uint32_t;
		}
		if(delta_temp_uint32_t>Irigb_setting.PPS_SyncThldClk){			//秒沿之间的误差大于10us
			ret_val=TRUE;
		}

		if(delta_temp_uint32_t>Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_PPS_OUT_IRIGB_NEDGE_DIFF_125M]){
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_PPS_OUT_IRIGB_NEDGE_DIFF_125M]=delta_temp_uint32_t;
		}

		if(delta_temp_uint32_t>Irigb_setting.PPS_JumpClk){
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_LOCATE_PPS_JUMP_UTC] =Irigb_sync_monitor_inf.count_utc_1s;
		}
	}

	return ret_val;
}

//检查Irigb的秒沿宽度是否抖动
void check_Irigb_pps_shake(void)
{
uint32_t   delta_temp_uint32_t;

//检查Irigb码的脉宽是否抖动
	if(Irigb_shake_inf.continue_rec_Irigb_number>2){					//刚收到Irigb码2个以上
	    if(Irigb_Decoder_lib.issueEdge.nHwClkPeriod>Irigb_shake_inf.Irigb_nHwClkPeriod_old){
	        delta_temp_uint32_t=Irigb_Decoder_lib.issueEdge.nHwClkPeriod-Irigb_shake_inf.Irigb_nHwClkPeriod_old;
	    }
	    else{
	        delta_temp_uint32_t= Irigb_shake_inf.Irigb_nHwClkPeriod_old -Irigb_Decoder_lib.issueEdge.nHwClkPeriod;
	    }
		if(delta_temp_uint32_t>Irigb_setting.shake_ThldClk){		//大于抖动定值10us
			Irigb_shake_inf.Irigb_width_shake_number++;
			Irigb_shake_inf.Irigb_width_no_shake_number=0;
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_IRIGB_WIDTH_SHAKE_NUMBER]++;		//秒秒宽抖动超标计数
		}
		else{
			Irigb_shake_inf.Irigb_width_no_shake_number++;
			Irigb_shake_inf.Irigb_width_shake_number=0;
		}
	    if(delta_temp_uint32_t>Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_IRIGB_WIDTH_SHAKE_TIME_125M]){
	        Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_IRIGB_WIDTH_SHAKE_TIME_125M]=delta_temp_uint32_t;
	    }
	}

}

//向FPGA发送时钟信息
void send_cpu_2_fpga_inf_prog(void)
{

//填写给FPGA的信息
	cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably=0;
	if(sync_logic_inf.m_SyncState==eSCHS_Follow){
		cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably |=IRIGB_RELIABLE_STATE_FOLLOW;
	}
	if(sync_logic_inf.m_SyncState==eSCHS_Keeping){
		cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably |=IRIGB_RELIABLE_STATE_KEEP;
	}
	if(sync_logic_inf.m_SyncState==eSCHS_Lose){
		cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably |=IRIGB_RELIABLE_STATE_LOST;
	}
 


//置时钟源有效
	if(Irigb_shake_inf.Irigb_width_no_shake_number>=9)			//连续9秒B码脉宽稳定,且没有失步
	{	

		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_CORE0_SEND_FPGA_SRC_VALID_NUMBER]++;			//向FPGA发送时钟源可信次数
	
		cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably |=IRIGB_RELIABLE_STATE_SRC_VALID;
//置时钟源跳变	
		if(check_Irigb_pps_delta()==TRUE){
			Irigb_shake_inf.PPS_Edge_Err_Over_2_Fpga_Number ++;
		}
		else{
			Irigb_shake_inf.PPS_Edge_Err_Over_2_Fpga_Number = 0;
		}
		if(Irigb_shake_inf.PPS_Edge_Err_Over_2_Fpga_Number>=2){
			cpu_2_FPGA_sync_inf_interface.Last_Irigb_Rec_Reliably |=IRIGB_RELIABLE_STATE_SRC_JUMP;
		}	
    }
	else{
		Irigb_shake_inf.PPS_Edge_Err_Over_2_Fpga_Number = 0;
	}

	cpu_2_FPGA_sync_inf_interface.cmd_id=0x01;
	cpu_2_FPGA_sync_inf_interface.updata_cnt++;									//更新计数
	cpu_2_FPGA_sync_inf_interface.Irigb_Reverse_Polarity_flag = Irigb_user_config.Irigb_Reverse_Polarity_flag;
	Write_Fpga_from_Cpu_Irigb_rec_inf(&cpu_2_FPGA_sync_inf_interface);					//向FPGA发送同步信息

}


//1个硬中断执行一次
void synctime_time_run_hard_int(void)
{
static uint32_t	pre_nFrc=0;								//预秒等分，当秒沿为1时，设置nFrc
int32_t	temp_int32;
static uint32_t	nHwClkTag_refresh_number;

	pre_nFrc++;
	if(pre_nFrc>=Irigb_setting.r_IRQFreq){		//预秒等分
		pre_nFrc=0;
	}

	Clk_Time_Edge.nFrc++;								//本地中断次数+1
	if (Clk_Time_Edge.nFrc >= Irigb_setting.r_IRQFreq){		//秒沿时刻 r_IRQFreq=5000 1秒的中断个数, 秒沿时刻				
		Clk_Time_Edge.nFrc = 0;										//中断计数清0
	}

	nHwClkTag_refresh_number++;
	// 查询本地FPGA的PPS输出秒沿 是否有新秒沿到来
	if(Locate_out_pps_inf.PPS_nHwClockTag_Rise_ack!=Locate_out_pps_inf.PPS_nHwClockTag_Rise_new){		//功能：读FPGA的本地的输出秒沿是否有更新，且为下降沿时
		Locate_out_pps_inf.PPS_nHwClockTag_Rise_ack=Locate_out_pps_inf.PPS_nHwClockTag_Rise_new;
		Clk_Time_Edge.nHwClkTag = Locate_out_pps_inf.PPS_nHwClockTag_Rise_new;					// FPGA本地PPS输出秒沿绝对时间硬时钟读数，
		nHwClkTag_refresh_number = 0;

	if(pre_nFrc==3){					//已确认秒沿
		Clk_Time_Edge.nFrc=3;						//收到本地秒沿后，将中断序号=1，由于要给其它Core，需提前1个采样脉冲计数值
	}
	else{
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_PPS_OUT_FRC_CNT_NOT_ZERO_NUMBER]++;
	}

		pre_nFrc=3;
	}

	if(Clk_Time_Edge.nFrc == 0)					//整秒时刻
	{
		utc_plus_plus(&Clk_Time_Edge.nUTC, &Clk_Time_Edge.nLeapSecond);		//UTC时间+1,考虑到润秒
		if(nHwClkTag_refresh_number>1000)			//秒沿计数没有刷新，则需加1秒脉宽
		{
			Clk_Time_Edge.nHwClkTag = Clk_Time_Edge.nHwClkTag + Clk_Time_Edge.nHwClkPeriod;
		}
		temp_int32=Clk_Time_Edge.nHwClkTag-Clk_Time_Edge.Hard_Int_CLkTag;
		temp_int32 = abs(temp_int32);
		if(temp_int32>=Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_PPS_OUT_CNT0_INT_DIFF_125M])
		{
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_PPS_OUT_CNT0_INT_DIFF_125M]=temp_int32;
		}
	}
}



//合并单元同步逻辑状态机
//功能
//1. 同步--守时--失步 之间的切换逻辑
//2. 各守时模式m_KeepMode（0、1、2）的切换
uint8_t  synctime_sync_statemachine(void)
{
//主状态机
	switch (sync_logic_inf.m_SyncState)
	{
	case eSCHS_Lose:															//当前为失步状态
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_FOLLOW_TIME_SEC]=0;								//清跟随时间
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_KEEP_TIME_SEC]=0;								//清守时时间
	
		if ((Irigb_shake_inf.Irigb_width_no_shake_number>=9)						//连续9秒B码脉宽稳定
			&& (Irigb_shake_inf.i_SrcAlive  == TRUE))							//有源			
		{																		//失步到跟踪状态
			sync_logic_inf.m_SyncState = eSCHS_Follow;							//置同步状态
			g_has_ever_synced_irigb = true;										//记录曾经同步过
			Clk_Time_Edge.nTimeState = SYNCLK_SYNC_SRC_OK_MASK | SYNCLK_SYNC_FOLLOW_MASK;			//对时状态 有源|同步
			Clk_Time_Edge.nTimeQuality = Irigb_Decoder_lib.issueTime.nTimeQuality;								//时间质量品质
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_LOST_2_FOLLOW_UTC]=Irigb_sync_monitor_inf.count_utc_1s;
		}
		else										
		{																				//失步到失步
			if (Irigb_shake_inf.i_SrcAlive == TRUE){									//对时源正常
				Clk_Time_Edge.nTimeState = SYNCLK_SYNC_SRC_OK_MASK;
			}
			else{							
				Clk_Time_Edge.nTimeState = 0;							//对时源异常
			}
		}
		break;

	case eSCHS_Follow:															//当前为跟随状态
			
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_KEEP_TIME_SEC]=0;
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_LOST_TIME_SEC]=0;
		sync_logic_inf.m_UnFollowTimeSec=0;

		if (Irigb_shake_inf.i_SrcAlive  == FALSE)									//无对时源
		{																																				
			sync_logic_inf.m_SyncState = eSCHS_Keeping;								//同步到守时，处于守时状态
			sync_logic_inf.m_KeepMode = 0;											//守时模式 0--无源守时模式 1-有源无跳变守时模式（秒沿误差的偏差大于阈值） 2--有源有跳变守时模式
			Clk_Time_Edge.nTimeState = SYNCLK_SYNC_KEEP_MASK;				//守时状态
		}
                //TODO 增加PPS沿大于10us需要同步调整
//		else if (Irigb_shake_inf.PPS_Edge_Err_Over_Number >=1)		//有源且（本地PPS沿 -时钟源秒沿）误差的偏差大于阈值  阈值:>10us,
//		{
//			sync_logic_inf.m_SyncState = eSCHS_Keeping;						//同步状态进入守时状态
//			sync_logic_inf.m_KeepMode = 1;									//守时模式 0--无源守时模式 1-有源无跳变守时模式（秒沿误差的偏差大于阈值） 2--有源有跳变守时模式
//			Irigb_shake_inf.PPS_Edge_Err_Over_Number = 0;								//进入守时状态后，重新计数
//			Clk_Time_Edge.nTimeState = SYNCLK_SYNC_SRC_OK_MASK | SYNCLK_SYNC_KEEP_MASK;
//		}
		else									
		{// 有源且偏差小于阈值（10us)-保持同步				
			Clk_Time_Edge.nTimeQuality = Irigb_Decoder_lib.issueTime.nTimeQuality;		//时间品质取时钟源的时间品质
		}
		break;

	case eSCHS_Keeping:														//守时状态
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_FOLLOW_TIME_SEC]=0;
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_LOST_TIME_SEC]=0;

		if ((Irigb_shake_inf.i_SrcAlive  == TRUE)							//有源
			&& (Irigb_shake_inf.PPS_Edge_Err_Over_Number==0))				//偏差小于阈值（10us)
		{
			sync_logic_inf.m_SyncState = eSCHS_Follow;						//守时到同步，置更随状态
			Clk_Time_Edge.nTimeState = SYNCLK_SYNC_SRC_OK_MASK | SYNCLK_SYNC_FOLLOW_MASK;
			Clk_Time_Edge.nTimeQuality = Irigb_Decoder_lib.issueTime.nTimeQuality;			//本地时间品质=当前时钟源的时间品质
		}
		else																//无时钟源
		{
			if(sync_logic_inf.m_UnFollowTimeSec > MAX_SYNC_KEEP_TIME_S)			//失去时钟源>10分钟
			{
				Clk_Time_Edge.nTimeQuality = 0x0F;		//时间品质 置0FH
			}
			switch (sync_logic_inf.m_KeepMode)										//守时模式
			{
			case 0:																// 无源
				Clk_Time_Edge.nTimeState = SYNCLK_SYNC_KEEP_MASK;		//置守时模式
				if (Irigb_shake_inf.i_SrcAlive  == FALSE)							//无时钟源
				{																		//守时到失步
					if (sync_logic_inf.m_UnFollowTimeSec > MAX_SYNC_KEEP_TIME_S){		//=600 大于10min							
						sync_logic_inf.m_SyncState = eSCHS_Lose;						//置失步状态
						Clk_Time_Edge.nTimeState = 0;
						Irigb_shake_inf.Irigb_width_no_shake_number=0;
						Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_KEEP_2_LOST_UTC]=Irigb_sync_monitor_inf.count_utc_1s;
					}
				}
				else										//有时钟源，把一下时钟源后，清除对时管理时钟源跳变标志
				{
					sync_logic_inf.m_KeepMode = 1;				//置有源，秒无跳变模式，PPS秒沿误差大于阈值
					
				}
				break;

			case 1:														// 守时状态，且有源
				Clk_Time_Edge.nTimeState = SYNCLK_SYNC_SRC_OK_MASK | SYNCLK_SYNC_KEEP_MASK;

				if (Irigb_shake_inf.i_SrcAlive  != TRUE)			//无对时源
				{													// 失去对时源
					sync_logic_inf.m_KeepMode = 0;					//无对时源
					Clk_Time_Edge.nTimeState &= ~(SYNCLK_SYNC_SRC_OK_MASK);
				}
				else												//有对时源
				{													// 有对时源5秒失步
					if (Irigb_shake_inf.PPS_Edge_Err_Over_Number >= 4)				//PPS秒沿误差>阈值 最多连续5次进入失步，连续4次，置失步
					{
						sync_logic_inf.m_SyncState = eSCHS_Lose;						//置失步状态
						Clk_Time_Edge.nTimeState = SYNCLK_SYNC_SRC_OK_MASK;	//置有源
						Irigb_shake_inf.Irigb_width_no_shake_number=0;
						Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_KEEP_2_LOST_UTC]=Irigb_sync_monitor_inf.count_utc_1s;
					}
				}
				break;

			default:
				sync_logic_inf.m_KeepMode = 0;							//置无源守时模式
				Clk_Time_Edge.nTimeState &= ~(SYNCLK_SYNC_SRC_OK_MASK);
				break;
			}
		}
		break;

	default:
		sync_logic_inf.m_SyncState = eSCHS_Lose;						//置失步状态
		Clk_Time_Edge.nTimeState = 0;
		Clk_Time_Edge.nTimeQuality = 0x0F;
		break;
	}

	return sync_logic_inf.m_SyncState;
}


/// 计算下一秒的SOC值和闰秒标志
/// @param	pSoc		当前SOC值
/// @param	pLeapSec	当前闰秒标志
void utc_plus_plus(uint32_t* pSOC, uint32_t* pLeapSec)
{
uint32_t  nLastSOC;
uint8_t  nLastLeapSecond;
uint8_t  nLpSecForecast;
uint8_t  nLpSecDirection;
uint8_t  nLpSecPositive;

uint32_t  nNewSOC;
uint8_t  nNewLeapSec;

	nLastSOC = *pSOC;
	nLastLeapSecond = *pLeapSec;
	nLpSecForecast  = nLastLeapSecond & 0x01;
	nLpSecDirection = ((nLastLeapSecond) >> 1) & 0x01;
	nLpSecPositive  = ((nLastLeapSecond) >> 2) & 0x01;

	nNewLeapSec = nLpSecDirection << 1;
	if (nLpSecPositive)
	{
		// 上一秒是60"本秒是00"
		nNewSOC = nLastSOC;	//(正)闰秒预告结束
	}
	else if (nLpSecForecast)
	{
		// 上一秒有闰秒预告
		if (nLpSecDirection)
		{
			// 负闰秒 58"->00"
			if ((nLastSOC % 60) == 58)
			{
				nNewSOC = nLastSOC + 2;	//(负)闰秒预告结束
			}
			else
			{
				nNewSOC = nLastSOC + 1;
				(nNewLeapSec) |= SYNCLK_LS_FRCST_MASK;
			}
		}
		else
		{
			// 正闰秒 59"->60"->00"
			if ((nLastSOC % 60) == 59)
			{
				(nNewLeapSec) |= SYNCLK_LS_FRCST_MASK | SYNCLK_LS_POS_MASK;
				nNewSOC = nLastSOC + 1;
			}
			else
			{
				nNewSOC = nLastSOC + 1;
				(nNewLeapSec) |= SYNCLK_LS_FRCST_MASK;
			}
		}
	}
	else
	{
		nNewSOC = nLastSOC + 1;
	}

	*pSOC = nNewSOC;
	*pLeapSec = nNewLeapSec;
}

