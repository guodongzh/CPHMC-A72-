#ifndef __SYNCTIME_TIME_H__INCLUDE__
#define __SYNCTIME_TIME_H__INCLUDE__


#include "irig_b_interface.h"

#define CONST_SYNC_DELAY_TIME_S                9    // 同步确认时间
#define MAX_SYNC_KEEP_TIME_S                   600  // 守时最大时间

#define		TIME_SYNC_MODE_IRIGB_ONLY				1
#define		TIME_SYNC_MODE_SNTP_ONLY				2
#define		TIME_SYNC_MODE_DUAL_REDUNDANT			3

#define		TIME_SYNC_ACTIVE_IRIGB					0
#define		TIME_SYNC_ACTIVE_SNTP					1
// IRIGB码可信状态
#define IRIGB_RELIABLE_STATE_FOLLOW            0x01  // Bit0=1与源时钟保持同步(fellow)
#define IRIGB_RELIABLE_STATE_KEEP              0x02  // Bit1=1 靠晶振守时(keep)
#define IRIGB_RELIABLE_STATE_LOST              0x04  // Bit2=1 失步(lost)
#define IRIGB_RELIABLE_STATE_SRC_JUMP          0x08  // Bit3=1 时钟源跳变,

// 时钟源有效，当失步时，将时钟源置无效（FPGA的逻辑要求)，判断连续10个有效时钟（抖动小于10us)
#define IRIGB_RELIABLE_STATE_SRC_VALID         0x10  // Bit4=1

#define IRIGB_RELIABLE_STATE_FPGA_PPS_WIDTH_OK 0x20  // Bit5=1 FPGA检测秒脉冲宽度正常

// 同步逻辑信息
typedef struct
{
    uint32_t m_SyncState;  // 沿同步状态机
    uint32_t m_KeepMode;  // 守时模式 0--无源守时模式 1-有源秒无跳变守时模式（秒沿误差的偏差大于阈值）
                        // 2--有源秒有跳变守时模式
    uint32_t m_UnFollowTimeSec;  // 非跟踪状态时长
} SYNC_LOGIC_INF_STRUCT;

// B码抖动信息
typedef struct
{
    uint32_t i_SrcAlive;                       // 对时源信号
    uint32_t Irigb_rec_ok_flag;                // B码接收正确标志
    uint32_t not_rec_Irigb_time_1s;            // 没有收到B码时间，收到B码则清0
    uint32_t continue_rec_Irigb_number;        // 连续接收B码的个数
    uint32_t Irigb_nHwClkPeriod_old;           // B码上次的秒沿宽度
    uint32_t Irigb_width_no_shake_number;      // B码宽度抖动小于10us次数
    uint32_t Irigb_width_shake_number;         // B码宽度抖动大于10us次数
    uint32_t PPS_Edge_Err_Over_Number;         // B码秒沿与输出秒沿误差大于10us次数
    uint32_t PPS_Edge_Err_Over_2_Fpga_Number;  // 发送FPGA的B码信息，B码秒沿与输出秒沿误差大于10us次数
} IRIGB_SHAKE_INF_STRUCT;

// Irigb设定值
typedef struct
{
    int16_t nTZShift_min;      // 时区（分钟）
    uint32_t PPS_SyncThldClk;  // B码与输出PPS之间误差同步判定门槛，单位是FPGA_CLK计数
    uint32_t PPS_JumpClk;      // 判断输出PPS跳变时刻，单位是FPGA_CLK计数

    uint32_t shake_ThldClk;              // B码抖动判定门槛，单位是FPGA_CLK计数
    uint32_t r_IRQFreq;                  // 中断周期
    uint32_t send_2_FPGA_sync_inf_time;  // 没有B码时向FPGA发送同步信息间隔,9/10的中断处发送

    uint32_t Irigb_idle_time_hard_int_number;  // B码空闲中断次数62.5ms

} IRIGB_SETTING_STRUCT;

// IRIGB用户配置参数
typedef struct
{
	uint32_t	Irigb_Reverse_Polarity_flag;	// B码极性取反标志 (0:不取反, 1:取反)
	uint32_t	checkMode;						// B码检验模式 (0:无校验, 1:奇校验, 2:偶校验)
	uint32_t	PPS_SyncThld_us;				// B码PPS秒沿抖动门槛值(微秒)，例如 10us
    uint32_t	SyncMode;						// 对时模式 (1:B码, 2:SNTP, 3:双冗余)
	int16_t	nTZShift_min;					// 时区偏移(分钟)，例如 8 * 60
}IRIGB_USER_CONFIG_STRUCT;

extern IRIGB_USER_CONFIG_STRUCT Irigb_user_config;

extern IRIGB_SHAKE_INF_STRUCT	Irigb_shake_inf;
extern IRIGB_SETTING_STRUCT		Irigb_setting;
extern SYNC_LOGIC_INF_STRUCT	sync_logic_inf;
extern	CLK_TIME_EDGE			Clk_Time_Edge;						//对时信息
extern IRIGB_CPU_2_FPGA_INTERFACE_STRUCT	cpu_2_FPGA_sync_inf_interface;

uint8_t synctime_sync_statemachine(void);
void synctime_time_run_hard_int(void);
void synctime_time_init(void);
void utc_plus_plus(uint32_t *pSOC, uint32_t *pLeapSec);
void check_Irigb_pps_shake(void);
void send_cpu_2_fpga_inf_prog(void);
int8_t check_Irigb_pps_delta(void);
void sync_logic_update_active_source(void);
uint32_t sync_logic_get_active_source(void);
bool sync_logic_allow_irigb_update(void);
bool sync_logic_allow_sntp_update(void);

#endif
