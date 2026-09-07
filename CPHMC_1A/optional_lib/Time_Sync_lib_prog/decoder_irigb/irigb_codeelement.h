#ifndef __IRIG_B_CODE_ELEMENT_H__INCLUDE__
#define __IRIG_B_CODE_ELEMENT_H__INCLUDE__

/// 同步信息BIT定义`

#define SYNCLK_SYNC_FOLLOW_MASK (0x01)  ///< bit0: 与原时钟保持同步时(follow)置1
#define SYNCLK_SYNC_KEEP_MASK   (0x02)  ///< bit1: 靠晶振守时状态时(keeping)置1
#define SYNCLK_SYNC_RESERVED    (0x04)  ///< bit2: 保留位
#define SYNCLK_SYNC_FORCAST_MASK (0x04)  ///< bit2: 同步预告(同步预告出现后的下一个秒沿可能会正式置同步)
#define SYNCLK_SYNC_SRC_OK_MASK (0x08)  ///< bit3: 源时钟有效时置1

typedef enum
{
    ePP_Fall = 0,  ///< 下降沿
    ePP_Raise,     ///< 上升沿

    ePP_Count
} ePulsePolarity;  ///< 极性定义

// B码边沿信息
typedef struct
{
    uint32_t hwClockTag;   // 硬件时标
    uint8_t edgePolarity;  // 边沿极性 ref.ePulsePolarity
} IRIG_B_PULSE;

typedef struct
{
    uint32_t psCounter;  // 上次B码沿接收状态标志  0--B码第一个沿(即边沿初始态)必须为上升沿，
                       // 1--上次B码为上升沿，2--上次B码为下降沿标志
    uint32_t tPulseRaise;  // B码上升沿FPGA时标
    uint32_t tPulseFall;   // B码下降沿FPGA时标
} IRIGB_CODE_ELEMENT_STRUCT;

typedef enum
{
    eICE_ZERO = 0,      // 0码元
    eICE_ONE,           // 1码元
    eICE_P,             // P码元
    eICE_BUFFERING,     // 缓冲中
    eICE_ERROR_FIRST,   // 在起始状态，B码捕捉下降沿时出错
    eICE_ERROR_SECOND,  // B码已捕捉到起始状态的上升沿，捕捉下降沿时捕捉出错
    eICE_ERROR_THIRD,  // B码已捕捉到B码的下降沿 捕捉第3个沿为上升沿时捕捉出错

    eICE_ERROR_ONE,   // 1错误
    eICE_ERROR_P,     // P错误
    eICE_ERROR_ZERO,  // 0错误

    eICE_Count
} eIRIGBCodeElement;

eIRIGBCodeElement Analysis_Irigb_Codeelement(uint32_t *pHwClockTag,
                           IRIGB_FPGA_2_CPU_INTERFACE_STRUCT *p_Irigb_Fpga_2_cpu_interface,
                           IRIGB_CODE_ELEMENT_STRUCT *p_Irigb_code_element);

#endif
