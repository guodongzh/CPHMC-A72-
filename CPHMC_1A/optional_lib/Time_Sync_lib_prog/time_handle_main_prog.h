#ifndef __TIME_HANDLE_MAIN_PROG_H_
#define __TIME_HANDLE_MAIN_PROG_H_ 1

#include "irig_b_interface.h"

// 调试按Bit定义
#define TEST_INF_BIT_CURRENT_SYNC_SOURCE_IRIGB  0   // 当前对时源：IRIGB
#define TEST_INF_BIT_CURRENT_SYNC_SOURCE_SNTP   1   // 当前对时源：SNTP
#define TEST_INF_BIT_CURRENT_SYNC_SOURCE_1588   2   // 当前对时源：1588
#define TEST_INF_BIT_SYNC_LOGIC_STATE_FOLLOW    3   // 同步逻辑状态跟随
#define TEST_INF_BIT_SYNC_LOGIC_STATE_KEEP      4   // 同步逻辑状态守时
#define TEST_INF_BIT_SYNC_LOGIC_STATE_LOSE      5   // 同步逻辑状态失步
#define TEST_INF_BIT_CPU_2_FPGA_FOLLOW_STATE    6   // CPU给FPGA同步状态
#define TEST_INF_BIT_CPU_2_FPGA_KEEP_STATE      7   // CPU给FPGA守时状态
#define TEST_INF_BIT_CPU_2_FPGA_LOST_STATE      8   // CPU给FPGA失步状态
#define TEST_INF_BIT_CPU_2_FPGA_SRC_JUMP_STATE  9   // CPU给FPGA时钟源跳变
#define TEST_INF_BIT_CPU_2_FPGA_SRC_VALID_STATE 10  // CPU给FPGA时钟源有效
#define TEST_INF_BIT_FPGA_2_CPU_FOLLOW_STATE    11  // FPGA给CPU同步状态
#define TEST_INF_BIT_FPGA_2_CPU_KEEP_STATE      12  // FPGA给CPU守时状态
#define TEST_INF_BIT_FPGA_2_CPU_LOST_STATE      13  // FPGA给CPU失步状态
#define TEST_INF_BIT_FPGA_2_CPU_JUMP_STATE      14  // FPGA给CPU时钟源跳变
#define TEST_INF_BIT_FPGA_2_CPU_VALID_STATE     15  // FPGA给CPU时钟源有效
#define TEST_INF_BIT_FPGA_2_CPU_PPS_WIDTH_OK    16  // FPGA给CPU秒脉宽正常
#define TEST_INF_BIT_IRIG_B_NO_SIGNAL           17  // 无信号, 约50ms无脉冲置位;捕获到脉冲立即清除
#define TEST_INF_BIT_IRIG_B_ERR_CODEELE         18  // 校验错-码元错, 码元序列错置位;解完前75个连续码元(如果有校验还要校验正确)清除
#define TEST_INF_BIT_IRIG_B_ERR_CHECKSUM        19  // 校验错-校验和错, 有校验且校验和错误时置位;下一秒校验和正确时清除
#define TEST_INF_BIT_IRIG_B_ERR_LEAPSEC         20  // 校验错-闰秒标志非法, 检测到非法闰秒标志时置位;下一秒无非法闰秒则清除
#define TEST_INF_BIT_IRIG_B_ERR_TQ              21  // 时间质量无效, 当时间质量为 大于4us 时置位;小于4us时清除
#define TEST_INF_BIT_SYNCLK_LS_FRCST_MASK              22  // 闰秒预告 闰秒之前的所有59秒均置1
#define TEST_INF_BIT_SYNCLK_LS_DIR_MASK                23  // 闰秒方向，0=正闰秒 1=负闰秒
#define TEST_INF_BIT_SYNCLK_LS_POS_MASK                24  // 正闰秒发生标志 仅在第60"闰秒时刻置1
#define TEST_INF_BIT_IRIGB_EDGEREVERT_FLAG             25  // Irigb取反标志1-取反
#define TEST_INF_BIT_IRIGB_CHECK_ODD_FLAG              26  // Irigb奇校验标志
#define TEST_INF_BIT_IRIGB_CHECK_EVEN_FLAG             27  // Irigb偶校验标志
#define TEST_INF_BIT_IRIGB_CFG_ERR                     28  // B码配置异常（解析异常或范围校验异常）


//调试按32Bit定义
#define	TEST_INF_UINT32_CORE0_HARD_INT_COST_TIME_25M			0		//硬中断花费时间us
#define	TEST_INF_UINT32_REC_IRIGB_PPS_OK_NUMBER					1		//接收B码PPS秒沿次数
#define	TEST_INF_UINT32_REC_IRIGB_UTC_NOT_CONTINUE_NUMBER		2		//接收B码UTC不连续次数
#define	TEST_INF_UINT32_REC_IRIGB_NO_IDLE_STATE_NUMBER			3		//接收B码没有idle状态计数
#define	TEST_INF_UINT32_MAX_REC_IRIGB_NO_SINGLE_TIME_250US		4		//10秒内最大接收B码无信号计时,计时单位为250us	
#define	TEST_INF_UINT32_REC_IRIGB_NEDGE_ERR_NUMBER				5		//接收B码码元出错计数
#define	TEST_INF_UINT32_REC_IRIGB_NEDGE_OK_NUMBER				6		//接收B码码元正确次数
#define	TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION					7		//接收B码出错定位
#define	TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER					8		//接收B码出错计数
#define	TEST_INF_UINT32_REC_ALL_IRIGB_OK_NUMBER					9		//接收整个B码正确次数
#define	TEST_INF_UINT32_MAX_IRIGB_WIDTH_SHAKE_TIME_125M			10		//最大脉宽抖动125M计数值
#define	TEST_INF_UINT32_IRIGB_WIDTH_SHAKE_NUMBER				11		//Irigb脉宽抖动超标计数
#define	TEST_INF_UINT32_MAX_PPS_OUT_IRIGB_NEDGE_DIFF_125M		12		//PPS输出的沿与B码秒沿最大误差
#define	TEST_INF_UINT32_MAX_PPS_OUT_CNT0_INT_DIFF_125M			13		//PPS输出的沿与CNT=0中断沿最大误差
#define	TEST_INF_UINT32_FPGA_PPS_EDGE_DIFF_ERR_NUMBER			14		//pps秒沿偏差超标计数
#define	TEST_INF_UINT32_FPGA_PPS_EDGE_DIFF_OK_NUMBER			15		//pps秒沿正确计数
#define	TEST_INF_UINT32_FOLLOW_TIME_SEC							16		//跟踪状态的时长
#define	TEST_INF_UINT32_KEEP_TIME_SEC							17		//守时状态时长
#define	TEST_INF_UINT32_LOST_TIME_SEC							18		//失步状态的时长
#define	TEST_INF_UINT32_LOCATE_PPS_JUMP_UTC						19		//PPS输出跳变Utc时间
#define	TEST_INF_UINT32_SRC_ALIFE_UTC							20		//B码从没有到有的Utc时间
#define	TEST_INF_UINT32_LOST_2_FOLLOW_UTC						21		//从失步到跟随的UTC时间
#define	TEST_INF_UINT32_KEEP_2_LOST_UTC							22		//从守时到失步的UTC时间
#define	TEST_INF_UINT32_MAX_HARD_INT_PERIOD						23		//最大硬中断间隔
#define	TEST_INF_UINT32_MIN_HARD_INT_PERIOD						24		//最小硬中断间隔
#define	TEST_INF_UINT32_PPS_OUT_FRC_CNT_NOT_ZERO_NUMBER			25		//PPS沿输出时不是硬中断序号不等于0的计数值
#define	TEST_INF_UINT32_READ_INT_FPGA_COUNT_FALSE_NUMBER		26		//读中断沿出错计数值
#define TEST_INF_UINT32_IRIGB_SRC_JUMP_NUMBER             		27  	// B码跳变源头累计和PPS秒沿跳变次数
#define	TEST_INF_UINT32_CORE0_REC_PCIE_METH_OK_NUMBER			28		//接收PCIE百兆以太网OK帧数
#define	TEST_INF_UINT32_CORE0_REC_PCIE_METH_ERR_INF				29		//接收PCIE百兆以太网出错信息,次数*100+定位
#define	TEST_INF_UINT32_CORE0_REC_PCIE_METH_STORM_INF1			30		//Core0风暴信息1,报文类型+APPID
#define	TEST_INF_UINT32_CORE0_REC_PCIE_METH_STORM_INF2			31		//Core0风暴信息2,端口*100H+状态
#define	TEST_INF_UINT32_CORE0_REC_PCIE_METH_STORM_NUMBER		32		//Core0风暴帧数
#define	TEST_INF_UINT32_CORE0_SEND_FPGA_SRC_VALID_NUMBER		33	//向FPGA发送B码可信次数
#define	TEST_INF_UINT32_CURRENT_SYNC_MODE               		34		//当前对时模式（1：仅IRIGB，2：仅SNTP，3：优先IRIGB其次SNTP）
#define	TEST_INF_UINT32_IRIGB_CFG_ERR_INF						35		//B码配置异常信息：次数*100+定位

typedef enum
{
    eSCHS_Lose = 0,  ///< fail to sync with source clock. time NOT reliable.
    eSCHS_Follow,    ///< clock locked. sync normal. following source clock.
    eSCHS_Keeping,   ///< clock unlocked. self keeping.

    eSCHS_Count
} eSynClkHostState;

typedef struct
{
    uint32_t send_cpu_2_fpga_inf_flag;
    uint32_t start_time_1s;
    uint32_t task_10s;
    uint32_t task_10min;
    uint32_t nEdgeRevThld_time_1s;  // B码取反计时
} IRIGB_TASK_TIME_STRUCT;

// ###############################################################################################
//			B码对时监视信息
// ###############################################################################################
typedef struct
{
    uint32_t count_utc_1s;                        // 1秒连续计时
    uint32_t Fpga_2_cpu_Last_Irigb_Rec_Reliably;  // Fpga给CPU的B码可信状态
    uint32_t Hard_Int_CLkTag_old;
} IRIGB_SYNC_MONITOR_INF_STRUCT;

extern IRIGB_SYNC_MONITOR_INF_STRUCT Irigb_sync_monitor_inf;
extern IRIGB_TASK_TIME_STRUCT Irigb_task_time;
extern TIME_TEST_INF_STRUCT Share_Ram_time_test_inf;

void synctime_main_prog(void);

void sys_init_normal(void);
void init_irigb_sync_prog(void);
void irigb_sync_task_1s(void);

#endif
