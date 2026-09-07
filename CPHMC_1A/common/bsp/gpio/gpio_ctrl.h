/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       gpio_ctrl.h
 *@author     LiuRui
 *@date       2025.11.17
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.11.17  1.0       LiuRui
 ******************************************************************************/

#ifndef _GPIO_CTRL_H
#define _GPIO_CTRL_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <ti/osal/osal.h>
#include <ti/drv/gpio/GPIO.h>
#include <ti/drv/gpio/soc/GPIO_soc.h>
#include <ti/csl/src/ip/gpio/V0/gpio.h>
#include "board/board.h"
#include "debug_config.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
/**
 * NOTE: port num == inst num
 */


/*front panel buttons*/
#define PIN_NUM_PBTN0         14 /* PBTN0 INPUT */
#define PIN_NUM_PBTN1         15 /* PBTN1 INPUT */
#define PIN_NUM_PBTN2         16 /* PBTN2 INPUT */
#define PIN_NUM_PBTN3         17 /* PBTN3 INPUT */

// 灯板的在位
#define PIN_NUM_PEXIST        20

// 前面板的类型码
#define PIN_NUM_PFUNC0        21
#define PIN_NUM_PFUNC1        22

// power A
#define PIN_NUM_PWRA_EX       49 /* 电源A在位信号*/
#define PIN_NUM_PWRA_FALA     50 /* 告警A信号*/
#define PIN_NUM_PWRA_FALB     51 /* 告警B信号*/
#define PIN_NUM_PWRA_TMP      52 /* 过温信号*/

// power B
#define PIN_NUM_PWRB_EX       53 /* 电源B在位信号*/
#define PIN_NUM_PWRB_FALA     54 /* 告警A信号*/
#define PIN_NUM_PWRB_FALB     55 /* 告警B信号*/
#define PIN_NUM_PWRB_TMP      56 /* 过温信号*/

/*phy enable*/
#define PIN_NUM_SLOTA_EN      63 /* SLOTA_EN OUTPUT */
#define PIN_NUM_SLOTB_EN      64 /* SLOTB_EN OUTPUT */
#define PIN_NUM_SLOTC_EN      65 /* SLOTC_EN OUTPUT */
#define PIN_NUM_SLOTD_EN      66 /* SLOTD_EN OUTPUT */

/*four daughter card online state*/
#define PIN_NUM_SLOTA_EX      67 /* SLOTA_EX INPUT */
#define PIN_NUM_SLOTB_EX      69 /* SLOTB_EX INPUT */
#define PIN_NUM_SLOTC_EX      70 /* SLOTC_EX INPUT */
#define PIN_NUM_SLOTD_EX      71 /* SLOTD_EX INPUT */

/*uart txen*/
#define PIN_NUM_UART8_TXEN    74 /* UART8_TXEN OUTPUT */
#define PIN_NUM_UART9_TXEN    75 /* UART9_TXEN OUTPUT */

/*ssd ctrl*/
#define PIN_NUM_PCIE_PEWAKE   87 /* PCIE_PEWAKE# INPUT */
#define PIN_NUM_PCIE_SUSCLK   88 /* PCIE_SUSCLK OUTPUT */
#define PIN_NUM_PCIE_PERST    89 /* PCIE_PERST# OUTPUT */
#define PIN_NUM_PCIE_DEVSLP   90 /* PCIE_DEVSLP OUTPUT */
#define PIN_NUM_PCIE_CLKREQ   91 /* PCIE_CLKREQ# INPUT */

/*fpga flash ctrl*/
#define PIN_NUM_FLASH_CTRL1   86 /* flash ctrl OUTPUT */
#define PIN_NUM_FLASH_CTRL0   93 /* flash ctrl OUTPUT */

// 继电器时钟控制信号
#define PIN_NUM_SOC_DO_CHG    94 /* OUTPUT */

// 继电器A控制信号
#define PIN_NUM_SOC_ALM_CTL0  85 /*第一个控制信号*/
#define PIN_NUM_SOC_ALM_CTL1  95 /*第二个控制信号*/

// 继电器B控制信号
#define PIN_NUM_SOC_FAIL_CTL0 96 /*第一个控制信号*/
#define PIN_NUM_SOC_FAIL_CTL1 92 /*第二个控制信号*/

/*sd card power enable*/
#define PIN_NUM_SD_PWEN       111 /* SD_PWEN OUTPUT */

#define PIN_NUM_SPI_FPGA0     39 /* slot 2 fpga OUTPUT */
#define PIN_NUM_SPI_FPGA1     40 /* slot 3 fpga OUTPUT */

#define PIN_NUM_WDT0     12 /*FPGA WDT0 OUTPUT */
#define PIN_NUM_WDT1     13 /*FPGA WDT1 OUTPUT */

#if defined(BUILD_MCU2_0)

/*io interrupt pin num*/
#define PIN_NUM_INT           23 /* INT INPUT */
#define PIN_NUM_INT_C66_0     27 /* INT INPUT */
#define PIN_NUM_INT_C66_1     28 /* INT INPUT */

#define GPIO_INST_NUM         0
#define GPIO_INT_IDX          0
#define GPIO_INT_IDX_C66_0    1
#define GPIO_INT_IDX_C66_1    2

#elif defined(BUILD_MCU2_1)

#define PIN_NUM_INT   24 /* Pin 24 INPUT */
#define GPIO_INST_NUM 0
#define GPIO_INT_IDX  0

#elif defined(BUILD_MCU3_0)

/*io interrupt pin num*/
#define PIN_NUM_INT   25 /* Pin 25 INPUT */
#define GPIO_INST_NUM 0
#define GPIO_INT_IDX  0

#elif defined(BUILD_MCU3_1)

#define PIN_NUM_INT   26 /* Pin 25 INPUT */
#define GPIO_INST_NUM 0
#define GPIO_INT_IDX  0

#elif defined(BUILD_C66X_1)

#elif defined(BUILD_C66X_1)

#elif defined(BUILD_C7X_1)

#define PIN_NUM_INT   29 /* Pin 29 INPUT */
#define GPIO_INST_NUM 0
#define GPIO_INT_IDX  0

#elif defined(BUILD_MCU1_1)

#define PIN_NUM_INT   30 /* Pin 30 INPUT */
#define GPIO_INST_NUM 0
#define GPIO_INT_IDX  0

#endif


/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

/**
 * @brief gpio controller init
 */
void gpio_ctrl_init(void);

/**
 * @brief gpio get buttons status
 */
void gpio_get_buttons_status(void); 

/**
 * @brief gpio enable rs485 uart8 txen
 */
void gpio_enable_rs485_uart8_txen(bool enable);

/**
 * @brief gpio enable rs485 uart9 txen
 */
void gpio_enable_rs485_uart9_txen(bool enable);

/** 
 * @brief gpio enable rs485 rxen
 */
void gpio_enable_rs485_rxen(void);

/**
 * @brief gpio do0 on
 */
void gpio_do0_on(void);

/**
 * @brief gpio do1 on
 */
void gpio_do1_on(void);

/**
 * @brief gpio do0 off
 */
void gpio_do0_off(void);

/**
 * @brief gpio do1 off
 */
void gpio_do1_off(void);

void gpio_wdt_toggle(void);

#ifdef __cplusplus
}
#endif

#endif  //_GPIO_CTRL_H
