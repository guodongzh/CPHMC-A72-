
#include "irig_b_interface.h"
#include "time_handle_main_prog.h"
#include "sync_time_logic/sync_logic_prog.h"
#include "irigb_decoder.h"
#include "i2c_ds1339.h"

#define SWITCH_ON      0xaa
#define SWITCH_OFF     0x55

#define _1second			(uint32_t)(_1us*1000*1000)

LOCATE_OUT_PPS_INF_STRUCT		Locate_out_pps_inf;
IRIGB_DECODER_LIB_STRUCT Irigb_Decoder_lib;			//接收到B码后存放的各码元信息数据库
IRIGB_FPGA_2_CPU_INTERFACE_STRUCT	Irigb_Fpga_2_cpu_interface_old;

// 全局变量：统计B码跳变源头累计次数和PPS跳变源头累计次数
uint32_t irigb_edge_jump_count = 0;
uint32_t pps_edge_jump_count = 0;

static int32_t irigb_decoder_pushback(void);
static void irigb_decoder_reset(void);
static uint32_t DateTime2SOC(const struct tm* ptmDateTime);


void deal_output_pps_prog(IRIGB_FPGA_2_CPU_INTERFACE_STRUCT *p_Irigb_Fpga_2_cpu_interface);
void deal_irigb_decoder_prog(IRIGB_FPGA_2_CPU_INTERFACE_STRUCT *p_Irigb_Fpga_2_cpu_interface);
void deal_irigb_decoder_idle_prog(void);
// ###############################################################################################################

//设置
//1. nEdgeRevert_flag B码电平是否取反，
//2. checkMode 校验模式，奇校验、偶校验、无校验
//3. nTZShift_min 设置时区，以分为单位 北京为8 * 60 =480
//4. nShiftNsPerSecThld IRIG-B秒准时沿漂动门槛值，单位纳秒，范围[30,1000],默认=500ns
void decoder_irigb_init(void)
{

	memset((int8_t *)&Irigb_Decoder_lib,0,sizeof(Irigb_Decoder_lib));
	memset((int8_t *)&Irigb_Fpga_2_cpu_interface_old,0,sizeof(Irigb_Fpga_2_cpu_interface_old));
	memset((int8_t *)&Locate_out_pps_inf,0,sizeof(Locate_out_pps_inf));
	Locate_out_pps_inf.nQuartzPropBrif=FPGA_CLK_1SEC;

	memset(&Irigb_sync_monitor_inf,0,sizeof(Irigb_sync_monitor_inf));

//B码参数初始化
	Irigb_Decoder_lib.checkMode = (eIrigbCheckMode)Irigb_user_config.checkMode;						//B码检验模式，无校验/奇校验/偶校验
	Irigb_Decoder_lib.nEdgeRevert_flag = 0;								//B码沿ARM侧永远不取反
}



//功能：根据接收到的B码信息，填入Irigb_Decoder_lib --B码解析数据库
void irigb_decoder_main(void)
{
    IRIGB_FPGA_2_CPU_INTERFACE_STRUCT Irigb_Fpga_2_cpu_interface_new;
    uint32_t refresh_flag;
    int32_t  ret_val;
    uint32_t delta_ClkTag;
    uint32_t temp_uint32_t;
	
    // 读中断沿时标
    if (Read_int_Fpga_count(&temp_uint32_t) == TRUE) {
        delta_ClkTag = temp_uint32_t - Clk_Time_Edge.Hard_Int_CLkTag;

//        temp_uint32 + 3*125 * r0_main_intr_time
        Clk_Time_Edge.Hard_Int_CLkTag = temp_uint32_t + 3 * 125 *init_param.core0_main_intr_time;

        if (Irigb_task_time.start_time_1s > 2) {  // 2秒后开始统计
            if (delta_ClkTag > Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_HARD_INT_PERIOD]) {
                Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_HARD_INT_PERIOD] = delta_ClkTag;
            }
            if (delta_ClkTag < Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MIN_HARD_INT_PERIOD]) {
                Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MIN_HARD_INT_PERIOD] = delta_ClkTag;
            }
        }
    }
	else{
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_READ_INT_FPGA_COUNT_FALSE_NUMBER]++;		//读中断沿出错计数
	}

	ret_val=Read_Fpga_2_Cpu_Irigb_rec_inf(&Irigb_Fpga_2_cpu_interface_new);		//读入FPGA的B码沿信息
	if(ret_val==TRUE){				//报文内容有刷新
//FPGA给CPU的B码可信状态
		Irigb_sync_monitor_inf.Fpga_2_cpu_Last_Irigb_Rec_Reliably=Irigb_Fpga_2_cpu_interface_new.Last_Irigb_Rec_Reliably;
//判断B码是否有刷新
		refresh_flag =0;
		if(Irigb_Fpga_2_cpu_interface_new.Irigb_CapValue!=Irigb_Fpga_2_cpu_interface_old.Irigb_CapValue){
			Irigb_Fpga_2_cpu_interface_old.Irigb_CapValue=Irigb_Fpga_2_cpu_interface_new.Irigb_CapValue;
			refresh_flag = TRUE;
		}
		if(Irigb_Fpga_2_cpu_interface_new.Irigb_edgePolarity!=Irigb_Fpga_2_cpu_interface_old.Irigb_edgePolarity){
			Irigb_Fpga_2_cpu_interface_old.Irigb_edgePolarity=Irigb_Fpga_2_cpu_interface_new.Irigb_edgePolarity;
			refresh_flag = TRUE;
		}
		if(refresh_flag==TRUE){													//B码信息有变化
			deal_irigb_decoder_prog(&Irigb_Fpga_2_cpu_interface_new);			//B码解包
			irigb_edge_jump_count++; // 统计B码跳变源头累计次数
		}
//判断输出的PPS上升沿是否有刷新
		if(Irigb_Fpga_2_cpu_interface_new.PPS_Rising_Edge_CapValue!=Irigb_Fpga_2_cpu_interface_old.PPS_Rising_Edge_CapValue){
			Irigb_Fpga_2_cpu_interface_old.PPS_Rising_Edge_CapValue=Irigb_Fpga_2_cpu_interface_new.PPS_Rising_Edge_CapValue;
			deal_output_pps_prog(&Irigb_Fpga_2_cpu_interface_new);
			pps_edge_jump_count++; // 统计PPS跳变源头累计次数
//更新当前时间
			Clk_Time_Edge.nHwClkPeriod = Locate_out_pps_inf.nQuartzPropBrif;							//调整后本地两个绝对秒沿之间相隔的硬时钟数[0表示无效]
		}
	}
	else{				//B码空闲时
		deal_irigb_decoder_idle_prog();
	}
}

//处理装置输出的PPS上升沿报文
void deal_output_pps_prog(IRIGB_FPGA_2_CPU_INTERFACE_STRUCT *p_Irigb_Fpga_2_cpu_interface)
{
uint32_t nQuartzPropBrif;

	nQuartzPropBrif=p_Irigb_Fpga_2_cpu_interface->PPS_Rising_Edge_CapValue-Locate_out_pps_inf.PPS_nHwClockTag_Rise_old;
	if ((nQuartzPropBrif < (FPGA_CLK_1SEC - 150 * FPGA_CLK_1US))					//输出的PPS秒宽超范围
		|| (nQuartzPropBrif > (FPGA_CLK_1SEC + 150 * FPGA_CLK_1US)))
	{
		Locate_out_pps_inf.PPS_nHwClockTag_Rise_old=p_Irigb_Fpga_2_cpu_interface->PPS_Rising_Edge_CapValue;		//刷新旧值
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_FPGA_PPS_EDGE_DIFF_ERR_NUMBER]++;		//输出秒沿超标计数
		Locate_out_pps_inf.PPS_nHwClockTag_Rise_new=Locate_out_pps_inf.PPS_nHwClockTag_Rise_new + Locate_out_pps_inf.nQuartzPropBrif;		
		return;																		//错误的周期拒绝下发
	}
//输出的秒沿宽度正确
	Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_FPGA_PPS_EDGE_DIFF_OK_NUMBER]++;		//输出秒沿正确计数
	Locate_out_pps_inf.PPS_nHwClockTag_Rise_new=p_Irigb_Fpga_2_cpu_interface->PPS_Rising_Edge_CapValue;
	Locate_out_pps_inf.PPS_nHwClockTag_Rise_old=p_Irigb_Fpga_2_cpu_interface->PPS_Rising_Edge_CapValue;		//刷新旧值
	Locate_out_pps_inf.nQuartzPropBrif=nQuartzPropBrif;


}

//处理B码码元解析报文
void deal_irigb_decoder_prog(IRIGB_FPGA_2_CPU_INTERFACE_STRUCT *p_Irigb_Fpga_2_cpu_interface)
{
static int32_t m_CapedDir = 0;	//上次捕获的沿方向 0=下降沿; 1=上升沿
static uint32_t m_nLastHwClkTag = 0;	// 上次捕获的沿计数值
uint32_t nHwClkSpan;
uint32_t nHwClockTag_Rise;						//上升沿的计数值
eIRIGBCodeElement irigb_ce;

	Irigb_Decoder_lib.nNoSignalCntr = 0;							//无信号计时器清0
	Irigb_Decoder_lib.nErrorState &= ~(IRIG_B_NO_SIGNAL);			//清解码错误状态

	if (m_CapedDir == 0)								// 上次捕获的沿方向 0=下降沿; 1=上升沿
	{// 上次是下降沿，这次期望上升沿
		if (p_Irigb_Fpga_2_cpu_interface->Irigb_edgePolarity != 1){				//边沿极性 ref.ePulsePolarity，与上次捕获的沿相同，则丢弃
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=1;
			return;												// 不是上升沿就丢弃
		}
	}
	else
	{// 上次是上升沿，这次期望下降沿
		if (p_Irigb_Fpga_2_cpu_interface->Irigb_edgePolarity != 0){				//边沿极性 ref.ePulsePolarity，与上次捕获的沿相同，则丢弃
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=2;
			return;												// 不是下降沿就丢弃
		}
	}
	nHwClkSpan = p_Irigb_Fpga_2_cpu_interface->Irigb_CapValue - m_nLastHwClkTag;		//PulseInfo.hwClockTag--沿的硬件时标，nHwClkSpan--码元的宽度
	if (nHwClkSpan < 100 * FPGA_CLK_1US){												//脉宽小于100us，为扰动
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=3;
		return;														// 码元宽度 间隔小于100us的算抖动
	}
	m_nLastHwClkTag = p_Irigb_Fpga_2_cpu_interface->Irigb_CapValue;						//刷新上次的时标值
	if(m_CapedDir ==0){																	// = 1 - m_CapedDir; 正式收到了预期的脉冲，更换预期的沿标志
		m_CapedDir = 1;
	}
	else{
		m_CapedDir =0;
	}
	// 注：IRIG-B推荐使用上升沿作为准时沿，算法逻辑部分也是完全依照标准开发的
	// 如果源使用下降沿做准时沿则置标志 nEdgeRevert_flag=1
//	if (Irigb_Decoder_lib.nEdgeRevert_flag)	{					//是否需要将B码反向功能，B码取反功能根据B码的解析情况，自动识别
//		p_Irigb_Fpga_2_cpu_interface->Irigb_edgePolarity = 1 - p_Irigb_Fpga_2_cpu_interface->Irigb_edgePolarity;				//如为取反配置，则将B码的沿取反	
//	}
	irigb_ce = Analysis_Irigb_Codeelement(&nHwClockTag_Rise, p_Irigb_Fpga_2_cpu_interface, &(Irigb_Decoder_lib.irigb_CodeEle));		//返回B码码元“0”，“1”、“P"，nHwClockTag--上升沿的计数值
	if (irigb_ce <= eICE_P)											//捕捉码元正确
	{// 码元正确
		Irigb_Decoder_lib.irigb_ce = irigb_ce;						//码元信号 “0”码，“1”码，“P”码
		Irigb_Decoder_lib.nHwClockTag_Rise = nHwClockTag_Rise;				//上升沿的计数值
		Irigb_Decoder_lib.nSchedulClk = 1;							//新的B码码元状态 1--刚收到新的B码码元 ,脉宽正确		
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_NEDGE_OK_NUMBER]++;		//B码码元接收正确
	}
	else if (irigb_ce > eICE_BUFFERING)			//出错
	{
		irigb_decoder_reset();								//解B码复位，重新开始
		Irigb_Decoder_lib.irigb_ce = irigb_ce;
		Irigb_Decoder_lib.nSchedulClk = 0;					//重新收B码码元，
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_NEDGE_ERR_NUMBER]++;		//B码码元接收出错计数
	}
}

//处理B码空闲时
//B码从90~97为idle即70ms，所以应用判定idle状态持续时间大于62.5ms，才进入idle状态Irigb_Decoder_lib.nNormState = 0
void deal_irigb_decoder_idle_prog(void)
{
int32_t  nRetValue;

	if (Irigb_Decoder_lib.nNoSignalCntr > Irigb_setting.Irigb_idle_time_hard_int_number){   						//约40~63ms的idel状态，250*250us=62.5ms，接收B码进入idle状态
		Irigb_Decoder_lib.nErrorState |= IRIG_B_NO_SIGNAL;				//40~63ms没有收到B的Capture，置无信号标志
		Irigb_Decoder_lib.nNormState = 0;								//清获得PPS沿标志，重新获得P码元标志，接收B码进入idle状态
		Irigb_Decoder_lib.nEdgeErrCntr = 0;								//清B码沿出错次数
	}

	Irigb_Decoder_lib.nNoSignalCntr++;								//无信号计时 ，收到B码后立即清0
	
	if(Irigb_Decoder_lib.nNoSignalCntr>Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_REC_IRIGB_NO_SINGLE_TIME_250US]){
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_MAX_REC_IRIGB_NO_SINGLE_TIME_250US]=Irigb_Decoder_lib.nNoSignalCntr;
	}
	if(Irigb_Decoder_lib.nSchedulClk==1){				// 码元状态 ，0-B码码元出错，1-新的B码码元正确、2-该码元解析完毕
		nRetValue = irigb_decoder_pushback();			//返回 码元计数器，实际解码的个数，码元序列解报文 
		Irigb_Decoder_lib.nSchedulClk = 2;				//2--该码元解析完毕
		if (nRetValue == 77)							//该码元解码完毕，一共77个码元
		{
			Irigb_Decoder_lib.nEdgeErrCntr = 0;				//清上升或下降沿错误计数器
		}
	}
}


//功能 :根据收到的码元值，对B码进行解析，将解析结果填入到Irigb_Decoder_lib
//返回：已收到正确B码码元个数，当前的码元状态[0~100]表示正常,<0表示异常。
int32_t irigb_decoder_pushback(void)
{
uint16_t bcdValue;
uint8_t  nBitValue;
int32_t  nNextPos;				//码元计数器，已收到的码元个数
uint32_t nHwClkSpan;
int8_t   bCheckOK;
int8_t   bLeapLegal;
int32_t  tm_shifthour;
eIRIGBCodeElement codeElement;
uint32_t hwClockTag_Rise;
uint32_t	temp_uint32_t;
static uint32_t last_issue_utc = 0;
static uint8_t utc_stable_cnt = 0;

	codeElement = Irigb_Decoder_lib.irigb_ce;					//码元类型
	hwClockTag_Rise = Irigb_Decoder_lib.nHwClockTag_Rise;			//B码上升沿计数值

	if (codeElement > eICE_P){			//码元标志出错或等待下一个B码码元沿，
		return -1;
	}

	nBitValue = (uint8_t)codeElement;									//新收到的B码码元

	if (Irigb_Decoder_lib.ceCounter >= 100){		//码元计数器，已收到的码元个数
		Irigb_Decoder_lib.ceCounter = 0;
	}
	nNextPos = Irigb_Decoder_lib.ceCounter;
	if (Irigb_Decoder_lib.ceCounter != 0){
		Irigb_Decoder_lib.ceCounter++;
	}

	// 判码元连续
	nHwClkSpan = (uint32_t)(hwClockTag_Rise - Irigb_Decoder_lib.lastHwClockTag_Rise);			//码元宽度
	if ((nHwClkSpan < 9 * FPGA_CLK_1MS)									//码元宽度<9ms 或大于11ms出错，正常为10ms，整个码元宽度是否在9~11ms之间
		|| (nHwClkSpan > 11 * FPGA_CLK_1MS))
	{
		irigb_decoder_reset();										//解析B码缓存区复位
		Irigb_Decoder_lib.lastHwClockTag_Rise = hwClockTag_Rise;								//更新上次B码上升沿FPGA计数值
		Irigb_Decoder_lib.lastCodeElement = codeElement;							//更新上次的B码码元
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=19;
		return -1;
	}

	Irigb_Decoder_lib.lastHwClockTag_Rise = hwClockTag_Rise;					//更新上次B码上升沿FPGA计数值

	if (codeElement == eICE_P)							//如为P码元
	{// "P"
		if (Irigb_Decoder_lib.lastCodeElement == eICE_P)			//如上次也为P码元，则为同步头，B码解析重新开始
		{// 双P确认同步头
			Irigb_Decoder_lib.ceCounter = 1;					// 码元0已入栈，已收到码元计数器从头开始，B码已收到的码元序号=1
			Irigb_Decoder_lib.ppsClockTag = hwClockTag_Rise;		// 秒沿时标，秒沿的计数值，上升沿
			Irigb_Decoder_lib.lastCodeElement = codeElement;	//更新上次码元标志
			Irigb_Decoder_lib.checksum = 0;					//校验和清0
			Irigb_Decoder_lib.LeapSecondFlag = 0;				//润秒标志清0
			Irigb_Decoder_lib.timeQuality = 0;					//时间品质清0
			Irigb_Decoder_lib.timeShift = 0;

			if (Irigb_Decoder_lib.nNormState == 0)				//是否有B码的idle状态，进入过idle状态后则nNormState = 0，时钟稳定了
			{
				Irigb_Decoder_lib.nNormState = 1;				//获得PPS秒沿标志
				Irigb_Decoder_lib.issueTime.nHwClkTag = hwClockTag_Rise;		//秒沿时刻的计数值
				Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_PPS_OK_NUMBER]++;
			}
			else{														//没有进入idle状态
				Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
				Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=20;
			}
			return 1;
		}
		else											//上次不是P码元
		{
			Irigb_Decoder_lib.lastCodeElement = codeElement;		//将P码元置上次码元
			if ((nNextPos % 10) != 9)					//收到码元个数不是9的倍数,不是分隔符，出错，每10个B码码元有1个“P”码分隔符
			{// reset
				Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
				Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=21;
				irigb_decoder_reset();				//复位B码
				return -1;			// P出现在非P位置上
			}
		}
	}
	else												// "0"或"1"
	{
		Irigb_Decoder_lib.lastCodeElement = codeElement;			//更新上次码元标志
		if (nNextPos == 0)											//还没有收到P码元
		{
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=22;
			return -3;									// 尚未找到双P
		}
		if ((nNextPos % 10) == 9)						//一个数据单元接收完毕,应出现P码元的位置，收到的为”0“或”1“，非P码元
		{
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=23;
			irigb_decoder_reset();
			return -2;				// 非P出现在P位置上
		}

		if (nNextPos < 75){
			Irigb_Decoder_lib.checksum ^= nBitValue;				//计算B码校验码
		}
		// 秒
		if (nNextPos <= 4)								//秒个位
		{
			Irigb_Decoder_lib.bcdBuff[nNextPos - 1] = nBitValue;
			if (nNextPos == 4)							//秒的个位接收完毕
			{
				Irigb_Decoder_lib.bcdOneth = (Irigb_Decoder_lib.bcdBuff[3] << 3) + (Irigb_Decoder_lib.bcdBuff[2] << 2) + (Irigb_Decoder_lib.bcdBuff[1] << 1) + Irigb_Decoder_lib.bcdBuff[0];
			}
		}
		else if (nNextPos <= 8)							//秒十位
		{
			if (nNextPos >= 6)
			{
				Irigb_Decoder_lib.bcdBuff[nNextPos - 6] = nBitValue;
				if (nNextPos == 8)					//秒的十位接收完毕
				{
					Irigb_Decoder_lib.bcdTenth = (Irigb_Decoder_lib.bcdBuff[2] << 2) + (Irigb_Decoder_lib.bcdBuff[1] << 1) + Irigb_Decoder_lib.bcdBuff[0];
					Irigb_Decoder_lib.tmDateTime.tm_sec = Irigb_Decoder_lib.bcdTenth * 10 + Irigb_Decoder_lib.bcdOneth;
				}
			}
		}
		else if (nNextPos <= 59)			//获得分、时、天数、年
		{
			Irigb_Decoder_lib.bcdBuff[nNextPos % 10] = nBitValue;
			if ((nNextPos % 10) == 8)
			{//
				Irigb_Decoder_lib.bcdOneth = ((Irigb_Decoder_lib.bcdBuff[3] << 3) + (Irigb_Decoder_lib.bcdBuff[2] << 2) + (Irigb_Decoder_lib.bcdBuff[1] << 1) + Irigb_Decoder_lib.bcdBuff[0]) & 0x0F;
				Irigb_Decoder_lib.bcdTenth = ((Irigb_Decoder_lib.bcdBuff[8] << 3) + (Irigb_Decoder_lib.bcdBuff[7] << 2) + (Irigb_Decoder_lib.bcdBuff[6] << 1) + Irigb_Decoder_lib.bcdBuff[5]) & 0x0F;
				bcdValue = Irigb_Decoder_lib.bcdTenth * 10 + Irigb_Decoder_lib.bcdOneth;
				switch (nNextPos / 10)
				{
				case 1:	// 分
					Irigb_Decoder_lib.tmDateTime.tm_min  = (Irigb_Decoder_lib.bcdTenth & 0x07) * 10 + Irigb_Decoder_lib.bcdOneth;
					break;
				case 2:	// 时
					Irigb_Decoder_lib.tmDateTime.tm_hour = (Irigb_Decoder_lib.bcdTenth & 0x03) * 10 + Irigb_Decoder_lib.bcdOneth;
					break;
				case 3:	// 日
					Irigb_Decoder_lib.tmDateTime.tm_yday = bcdValue - 1; /* tm_yday : days since January 1 - [0,365] */
					break;
				case 4:	// 日百位
					Irigb_Decoder_lib.tmDateTime.tm_yday += (bcdValue & 0x03) * 100;
					break;
				case 5:	// 年
					Irigb_Decoder_lib.tmDateTime.tm_year = 2000 + bcdValue;
					break;
				default:
					break;
				}
			}
		}
		else
		{// 码元 60~100
			if (nNextPos <= 63)							//闰秒标志
			{
				Irigb_Decoder_lib.LeapSecondFlag |= (nBitValue << (nNextPos - 60));
			}
			else if (nNextPos == 64)					//时区符号
			{
				Irigb_Decoder_lib.timeShiftSign = nBitValue;
			}
			else if (nNextPos <= 68)					//时区
			{
				Irigb_Decoder_lib.timeShift |= (nBitValue << (nNextPos - 65));
			}
			else if (nNextPos == 70)					//
			{
				temp_uint32_t = DateTime2SOC(&(Irigb_Decoder_lib.tmDateTime)) - (Irigb_setting.nTZShift_min *60);		//将年天数时分秒转为大秒数，tm_shiftsec--时区
				if(temp_uint32_t!=(Irigb_Decoder_lib.tmSOC_UTC +1)){
					Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_UTC_NOT_CONTINUE_NUMBER]++;		//B码UTC不连续
				}
				Irigb_Decoder_lib.tmSOC_UTC = temp_uint32_t;
			}
			else if (nNextPos <= 74)				//时间品质
			{
				Irigb_Decoder_lib.timeQuality |= (nBitValue << (nNextPos - 71));
			}
			else if (nNextPos == 75)						//校验码
			{
				Irigb_Decoder_lib.validSign = nBitValue;
				Irigb_Decoder_lib.checksum ^= Irigb_Decoder_lib.validSign;
				Irigb_Decoder_lib.checksum &= 0x01;

				bCheckOK = TRUE;
				if (Irigb_Decoder_lib.checkMode != eIRIGB_Chk_None)				//不是无校验
				{
					if (Irigb_Decoder_lib.checkMode == eIRIGB_Chk_Odd)				// 奇校验
					{
						if (Irigb_Decoder_lib.checksum != 0x01){
							bCheckOK = FALSE;
							Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
							Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=24;
						}
					}
					else if (Irigb_Decoder_lib.checkMode == eIRIGB_Chk_Even)		//偶校验
					{
						if (Irigb_Decoder_lib.checksum != 0x00){
							bCheckOK = FALSE;
							Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
							Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=25;
						}
					}
				}

				if (bCheckOK==TRUE)								//校验正确,B码接收正确
				{
					Irigb_Decoder_lib.nErrorState &= ~(IRIG_B_ERR_CHECKSUM | IRIG_B_ERR_CODEELE);
					Irigb_Decoder_lib.srcCheckOK = 0x5A;
				}
				else														//校验出错
				{
					Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
					Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=26;
					Irigb_Decoder_lib.nErrorState |= IRIG_B_ERR_CHECKSUM;
					Irigb_Decoder_lib.srcCheckOK = 0xA5;
				}
//				Irigb_Decoder_lib.nEdgeErrCntr = 0;

				if (Irigb_Decoder_lib.timeQuality <= 4)								// 4:=<1us   ，时间品质<4
				{
					Irigb_Decoder_lib.nErrorState &= ~IRIG_B_ERR_TQ;					//时间品质位正确
				}
				else
				{
					Irigb_Decoder_lib.nErrorState |= IRIG_B_ERR_TQ;					//时间品质位出错
				}
				if (Irigb_Decoder_lib.srcCheckOK == 0x5A)								//校验正确
				{// 校验成功，发布时间
					// 闰秒标志有效性判定
					bLeapLegal = TRUE;
					if (((Irigb_Decoder_lib.LeapSecondFlag & SYNCLK_LS_FRCST_MASK) != 0)			//润秒预告标志到，或当前秒数=60
						|| (Irigb_Decoder_lib.tmDateTime.tm_sec == 60))
					{// 须判定闰秒有效性

						if (Irigb_Decoder_lib.tmDateTime.tm_min != 59)	// 不是59分不可置闰秒标志
						{
							bLeapLegal = FALSE;	// 非法闰秒
						}
						else
						{
							if (Irigb_Decoder_lib.timeShiftSign == 1)// 西半球时间
								tm_shifthour = 0 - Irigb_Decoder_lib.timeShift;
							else
								tm_shifthour = Irigb_Decoder_lib.timeShift;

							if (tm_shifthour > 0) // 东半球
							{// 按月初判
								if ((Irigb_Decoder_lib.tmDateTime.tm_hour != (tm_shifthour - 1))
									|| (Irigb_Decoder_lib.tmDateTime.tm_mday != 1)
									|| ((Irigb_Decoder_lib.tmDateTime.tm_mon != 0) && (Irigb_Decoder_lib.tmDateTime.tm_mon != 6))) // 1月1日或7月1日
								{
									bLeapLegal = FALSE;	// 非法闰秒
								}
							}
							else //零时区和西半球
							{// 按月末判
								if ((Irigb_Decoder_lib.tmDateTime.tm_hour + Irigb_Decoder_lib.timeShift) != 23)
								{
									bLeapLegal = FALSE;	// 非法闰秒
								}
								if (!(((Irigb_Decoder_lib.tmDateTime.tm_mon == 5) && (Irigb_Decoder_lib.tmDateTime.tm_mday == 30))  //6月30日
									  || ((Irigb_Decoder_lib.tmDateTime.tm_mon == 11) && (Irigb_Decoder_lib.tmDateTime.tm_mday == 31)))) //12月31日
								{
									bLeapLegal = FALSE;	// 非法闰秒
								}
							}
						}
					}

					if (bLeapLegal==FALSE)							//非法闰秒
					{
						Irigb_Decoder_lib.nErrorState |= IRIG_B_ERR_LEAPSEC;
						Irigb_Decoder_lib.LeapSecondFlag &= ~(SYNCLK_LS_FRCST_MASK);
						Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
						Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=27;

					}
					else											//闰秒正确
					{
						Irigb_Decoder_lib.nErrorState &= ~(IRIG_B_ERR_LEAPSEC);
					}

					Irigb_Decoder_lib.issueTime.nHwClkTag = Irigb_Decoder_lib.ppsClockTag;			//秒沿的计数值
					Irigb_Decoder_lib.issueTime.nUTC = Irigb_Decoder_lib.tmSOC_UTC;					//UTC时间
					Irigb_Decoder_lib.issueTime.nTimeQuality = Irigb_Decoder_lib.timeQuality;			//时间品质
					Irigb_Decoder_lib.issueTime.nLeapSecond = Irigb_Decoder_lib.LeapSecondFlag;		//润秒
					if (Irigb_Decoder_lib.tmDateTime.tm_sec == 60){
						Irigb_Decoder_lib.issueTime.nLeapSecond |= SYNCLK_LS_POS_MASK;			//置闰秒时刻标志，正闰秒时刻
					}
					Irigb_Decoder_lib.issueTime.nTimeState = SYNCLK_SYNC_SRC_OK_MASK | SYNCLK_SYNC_FOLLOW_MASK;				//时钟源正确，处于更随状态

				}
			}
			else if (nNextPos == 76)							//接收到完整的B码
			{
				if (Irigb_Decoder_lib.srcCheckOK == 0x5A)					//校验码正确标志
				{
					Irigb_Decoder_lib.nNormState = 0;								//清获得PPS沿标志，重新获得P码元标志
					
					memcpy(&(Irigb_Decoder_lib.issueEdge), &(Irigb_Decoder_lib.issueTime), sizeof(Irigb_Decoder_lib.issueEdge));						
					Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_ALL_IRIGB_OK_NUMBER]++;
					nHwClkSpan = Irigb_Decoder_lib.issueTime.nHwClkTag - Irigb_Decoder_lib.lastPPSHwTag;												//秒沿宽度
					if ((nHwClkSpan < (FPGA_CLK_1SEC - (150 * FPGA_CLK_1US)))|| (nHwClkSpan > (FPGA_CLK_1SEC + (150 * FPGA_CLK_1US)))){					//秒的宽度不满足要求
						Irigb_Decoder_lib.lastPPSHwTag = Irigb_Decoder_lib.issueTime.nHwClkTag;					//保留上一个PPS的时标
						Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
						Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=28;
						return -1;
					}
					if(Irigb_shake_inf.i_SrcAlive!=TRUE){				// B码从没有到有的Utc时间
						Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_SRC_ALIFE_UTC] = Irigb_sync_monitor_inf.count_utc_1s;
					}
					Irigb_shake_inf.i_SrcAlive = TRUE;						//时钟源信号正常

					Irigb_shake_inf.not_rec_Irigb_time_1s=0;			//清没有收到B码计时
					Irigb_shake_inf.Irigb_rec_ok_flag=SWITCH_ON;		//置接收到B码标志
					Irigb_shake_inf.continue_rec_Irigb_number++;		//连续接收B码的个数	
					Irigb_Decoder_lib.issueEdge.nHwClkPeriod = nHwClkSpan;						//秒沿之间的计数值
					Irigb_Decoder_lib.lastPPSHwTag = Irigb_Decoder_lib.issueTime.nHwClkTag; //保留上一个PPS的时标

					if (Irigb_Decoder_lib.nNormState == 1)					//进入idle状态，且已获得秒沿标志,连续收到2个P码码元
					{// 出第一个秒沿特殊处理						
						Irigb_Decoder_lib.nNormState = 2;					//秒沿处理完毕						
						Irigb_Decoder_lib.issueEdge.nHwClkTag = Irigb_Decoder_lib.issueTime.nHwClkTag;		//秒沿的FPGA计数值
					}
					else										//即接收B码没有进入idle状态，
					{
						Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_NO_IDLE_STATE_NUMBER]++;					//接收B码无idle状态计数										
					}	
						
					if(Irigb_Decoder_lib.issueTime.nUTC == last_issue_utc + 1)
					{
						utc_stable_cnt++;
					}
					else
					{
						utc_stable_cnt = 0;
					}

					last_issue_utc = Irigb_Decoder_lib.issueTime.nUTC;

					if(utc_stable_cnt >= 3)   //连续3次稳定
					{
						if (sync_logic_allow_irigb_update() == true)
						{
							Clk_Time_Edge.nUTC = Irigb_Decoder_lib.issueTime.nUTC;
						}
						// 可记录同步发生
						utc_stable_cnt = 3;   //防止一直自增
					}								
				}
			}
		}
	}
	return (Irigb_Decoder_lib.ceCounter);					//已收到B码码元个数
}

//解析B码的结构体复位，连续300次复位，则将B码取反标志 ，取反
void irigb_decoder_reset(void)
{
	Irigb_Decoder_lib.ceCounter = 0;	// reset
	Irigb_Decoder_lib.nClkPerSec[0] = 0;
	Irigb_Decoder_lib.nClkPerSec[1] = 0;
	Irigb_Decoder_lib.nErrorState |= IRIG_B_ERR_CODEELE;				//置B码出错标志

    Irigb_Decoder_lib.nNormState = 0;								//清获得PPS沿标志，重新获得P码元标志

	// if (Irigb_Decoder_lib.nEdgeErrCntr == 0)							//
	// {
	// 	Irigb_task_time.nEdgeRevThld_time_1s=0;							//电平反向出错计时清0
	// 	Irigb_Decoder_lib.nEdgeErrCntr++;
	// }
	// else
	// {
	// 	Irigb_Decoder_lib.nEdgeErrCntr++;
	// 	if ((Irigb_Decoder_lib.nEdgeErrCntr > 300)&&(Irigb_task_time.nEdgeRevThld_time_1s>10))		//中断为250us，连续300个复位且运行大于10秒，沿错误计数器，当连续300个出错，B码沿逻辑自动取反，再重试B码是否正常接收
	// 	{// 反向
	// 		Irigb_Decoder_lib.nEdgeRevert_flag = 1 - Irigb_Decoder_lib.nEdgeRevert_flag;				//将B码沿自动取反，再测试一下是否正确
	// 		Irigb_Decoder_lib.nEdgeErrCntr = 0;
	// 		Irigb_task_time.nEdgeRevThld_time_1s=0;							//电平反向出错计时清0
	// 	}
	// }
}



/// 将DataTime转化为SOC（UTC时间）将年、天数、时分秒转换为大秒数SOC时间戳
/// @warning 本函数基于 tm_yday 而非基于tm_mon & tm_mday; tm_year是基于2000年而非1970年定义的!
uint32_t DateTime2SOC(const struct tm* ptmDateTime)
{
	uint32_t nUtcTime;
	uint32_t TempUL;
	uint32_t nYear;

	//年份信息均只保留后2位
	nYear = ptmDateTime->tm_year;
	if (nYear >= 2000)
		nYear -= 2000;

	TempUL = nYear * 365 + (nYear + 3) / 4;	// 2000年开始
	TempUL += ptmDateTime->tm_yday;			// tm_yday : days since January 1 - [0,365]，从1月1日开始的天数，直接从B码解析获得

	nUtcTime = TempUL * 86400;
	nUtcTime += (ptmDateTime->tm_hour * 3600 + ptmDateTime->tm_min * 60 + ptmDateTime->tm_sec);
	nUtcTime += BASE_SECOND2;	//1970-2000.1.1的秒数（UTC时间）
	return nUtcTime;
}



//将年月日时分秒转成UTC时间
void StruTime_To_UtcTime(StruTime * pStruTime_src, UTC_TIME * pUtcTime_dest)
{
uint16_t j,day_diff=0;
uint16_t year_diff;
uint32_t soe_time_s,time_ms,time_s;
uint32_t data_tmp;


	year_diff=pStruTime_src->Year % 100; // year diff from 2000

	for(j=0;j<year_diff;j++) // counte day_diff from 2000 to the beginning of this year
	{
		day_diff+=365; // non-leap year
		if( j==((j/4)*4) ){
			day_diff++; // leap year
		}
	}

	for(j=1;j<pStruTime_src->Month;j++){
		// counte day_diff from this year's beginning to the end of last month
		if( (j==1) || (j==3) || (j==5) || (j==7) || (j==8) || (j==10) || (j==12) ){
			day_diff += 31; // 31 days in a month
		}
		else if (j==2)
		{
			if( year_diff == ((year_diff/4)*4) ){ // this year is a leap year
				day_diff += 29; // 29 days in Feb.
			}
			else{
				day_diff += 28;
			}
		}
		else{
			day_diff += 30; // 30 days in a month
		}
	}
	day_diff += pStruTime_src->Date - 1;
	// days diff from 2000/1/1 till yesterday
	time_ms = pStruTime_src->uSec_100/10;		// ms数
	time_s = pStruTime_src->Sec;

	// Millisecond elapsed from the beginning of a second,从1970年开始到现在的秒数
	soe_time_s =  T_1970_2000 + day_diff*86400L
		+ pStruTime_src->Hour * 3600
		+ pStruTime_src->Min * 60
		+ time_s - (Irigb_setting.nTZShift_min * 60);
	
	pUtcTime_dest->utc_secs = soe_time_s;


	data_tmp = time_ms * 0x10000; //0x1000 000		
	data_tmp = ((data_tmp/1000)*0x100); // 1000

	pUtcTime_dest->fraction=data_tmp;	
	pUtcTime_dest->uSec_100=pStruTime_src->uSec_100;
}

//将UTC时间转换为年月日十分秒
void UtcTime_To_StruTime(UTC_TIME * pUtcTime_src,StruTime * pStruTime_dest)
{
double dFraction;
uint32_t Secs;
uint32_t year_elapsed=0;
int i=0;
uint32_t day_elapsed,tmp1,tmp2;
uint32_t sec_elapsed;

	if(pUtcTime_src->utc_secs>(uint32_t)(T_1970_2000-(Irigb_setting.nTZShift_min *60))){
		Secs = pUtcTime_src->utc_secs + (Irigb_setting.nTZShift_min *60) - T_1970_2000; 
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

	dFraction = (double) pUtcTime_src->fraction / (double) 0x01000000;
	pStruTime_dest->uSec_100 = (uint32_t)(dFraction*10000);
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


#ifdef WINNT

int8_t RtcGetTimeSoc(uint32_t *p_nRtcSoc)
{
	return TRUE;
}


int8_t RtcSetTimeSoc(uint32_t *p_nRtcSoc)
{
	return TRUE;
}

int32_t read_analog_data_init(void)
{
	return TRUE;
}
#else

//读历时时钟
#ifndef SOC_J721E

int8_t RtcGetTimeSoc(uint32_t *p_nRtcSoc)
{
analog_data_convert_t	analog_data_convert;
StruTime	StruTime_src;
UTC_TIME	UtcTime_dest;

	get_analog_data(&analog_data_convert);
	StruTime_src.Year=analog_data_convert.rtc_time_data.ucYear+2000;
	StruTime_src.Month=analog_data_convert.rtc_time_data.ucMon;
	StruTime_src.Date=analog_data_convert.rtc_time_data.ucDate;
	StruTime_src.Hour=analog_data_convert.rtc_time_data.ucHour;
	StruTime_src.Min=analog_data_convert.rtc_time_data.ucMin;
	StruTime_src.Sec=analog_data_convert.rtc_time_data.ucSec;
	StruTime_To_UtcTime(&StruTime_src, &UtcTime_dest);

	*p_nRtcSoc = UtcTime_dest.utc_secs;

	Irigb_sync_monitor_inf.count_utc_1s=UtcTime_dest.utc_secs;

	return	TRUE;
}


//设置历时时钟的时间
int8_t RtcSetTimeSoc(uint32_t *p_nRtcSoc)
{
DsTime_t	rtc_time;
StruTime	StruTime_dest;
UTC_TIME	UtcTime_src;

	UtcTime_src.utc_secs=*p_nRtcSoc;
	UtcTime_To_StruTime(&UtcTime_src,&StruTime_dest);

	rtc_time.ucYear =StruTime_dest.Year % 100;
	rtc_time.ucMon=StruTime_dest.Month;
	rtc_time.ucDate=StruTime_dest.Date;
	rtc_time.ucHour=StruTime_dest.Hour;
	rtc_time.ucMin=StruTime_dest.Min;
	rtc_time.ucSec=StruTime_dest.Sec;
	set_rtc_data(&rtc_time);
	return TRUE;
}

#endif

int8_t RtcGetTimeSoc(uint32_t *p_nRtcSoc)
{
    StruTime	StruTime_src;
    UTC_TIME	UtcTime_dest;

    ds1339_time_t time;

    rtc_ds1339_get_time(&time);
    
    StruTime_src.Year=time.year+2000;
    StruTime_src.Month=time.month;
    StruTime_src.Date=time.day;
    StruTime_src.Hour=time.hour;
    StruTime_src.Min=time.minute;
    StruTime_src.Sec=time.second;

    StruTime_To_UtcTime(&StruTime_src, &UtcTime_dest);

    *p_nRtcSoc = UtcTime_dest.utc_secs;

    Irigb_sync_monitor_inf.count_utc_1s=UtcTime_dest.utc_secs;


    return	TRUE;
}


//设置历时时钟的时间
int8_t RtcSetTimeSoc(const uint32_t *p_nRtcSoc)
{
    APP_UTC_TIME 	UtcTime_src = {0};
    APP_StruTime 	StruTime_dest = {0};

    cache_inv_com(&g_shm_irigb_info.Clk_time_Edge, sizeof(CLK_TIME_EDGE), CacheP_TYPE_ALL);
    UtcTime_src.utc_secs=g_shm_irigb_info.Clk_time_Edge.nUTC;
    UtcTime_src.fraction=g_shm_irigb_info.Clk_time_Edge.nFrc;
    UtcTime_src.uSec_100 = g_shm_irigb_info.Clk_time_Edge.uSec_100_cnt;
    UtcTime_To_StruTime_app(&UtcTime_src,&StruTime_dest, g_shm_irigb_info.Clk_time_Edge.sTimeZone_min);

    ds1339_time_t time;
    time.year = StruTime_dest.Year % 100;
    time.month = StruTime_dest.Month;
    time.day = StruTime_dest.Date;
    time.hour = StruTime_dest.Hour;
    time.minute = StruTime_dest.Min;
    time.second = StruTime_dest.Sec;
	
    rtc_ds1339_set_time(&time);

    return TRUE;
}


#endif
