/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       spi_cfg.h
 *@author     LiuRui
 *@date       2026.01.26
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.01.26  1.0       LiuRui
 ******************************************************************************/

#ifndef _spi_cfg
#define _spi_cfg

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "debug_config.h"
#include "pcie_fpga.h"
#include <string.h>
#include <ti/csl/csl_mcspi.h>
#include <ti/csl/soc.h>
#include <ti/osal/osal.h>

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define __PACKED        __attribute__((packed))

/**
 * cfg type
 */
#define CFG_TYPE_FT3    0x1
#define CFG_TYPE_ETH    0x2
#define CFG_TYPE_GETH   0x3
#define CFG_TYPE_SFP    0x4
#define CFG_TYPE_AURORA 0x5
#define CFG_TYPE_CAN    0x11
#define CFG_TYPE_AD     0x21
#define CFG_TYPE_LVDS   0x31
#define CFG_TYPE_INT    0x41
#define CFG_TYPE_INIT   0xfe
#define CFG_TYPE_SYS    0xff

/**
 * init flag
 */
#define INIT_START_CFG 0x1111
#define INIT_START_INTR 0xaa55

#define MAIN_FPGA_SOFT_VER 0x300B
#define MAIN_FPGA_FUN_VER 0x15f0

#define CSB_FPGA_SOFT_VER 0x250A
#define CSB_FPGA_FUN_VER 0x0101

#define ONLY_1S_TIME    1000000     //1000000us = 1s
#define AD_SAMPRATE     100000      //100KHZ的采样率
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/

typedef struct _pcie_cfg_head_spi
{
    uint16_t head;
    uint8_t cfg_type;
    uint8_t cfg_port_id;
    uint8_t cfg_slot_id;
    uint8_t resv;
    uint16_t cfg_payload_len;
} __PACKED pcie_cfg_head_spi_t;

typedef struct
{
    uint8_t unlock_com_err_dis : 1;
    uint8_t resv2 : 7;
} geth_commdis_bits_t;

typedef union
{
    uint8_t             value;
    geth_commdis_bits_t bits;
} geth_commdis_t;

typedef struct _geth_cfg_spi
{
    uint8_t train_rx_id;  // 1~4
    geth_commdis_t commdis;
    uint16_t max_time_out; //1=80ns
    uint16_t comm_t_set;   //1=8ns
    uint8_t monitor_mode;
    uint8_t Monitor_Samp_Slow_Mode;
    uint8_t Monitor_Samp_Slow_Point;
    uint8_t resv[23];
} __PACKED geth_cfg_spi_t;

typedef struct _eth_cfg_spi
{
    uint8_t train_rx_id;  // 1~4
    uint8_t forcelink_mode;
    uint8_t resv[30];
} __PACKED eth_cfg_spi_t;

typedef struct _aurora_cfg_spi
{
    uint8_t train_rx_id;  // 1~4
    uint8_t monitor_mode;
    uint8_t monitor_samp_slow;
    uint8_t resv[29];
} __PACKED aurora_cfg_spi_t;

typedef struct _spi_sys_init
{
    uint16_t arm_init_ok;
    uint16_t resv0[5];
    uint16_t soft_ver;
    uint8_t  fun_ver;
    uint8_t  pcie_ver;
    uint32_t soft_check_code;
    uint16_t resv1[2];
} __PACKED spi_sys_init_t;

typedef struct _train_car_info_spi
{
    uint8_t irq_num;  // 1~8
} __PACKED train_car_info_spi_t;


typedef struct _spi_sys_config
{
    uint8_t tx_squ_mod_port[4];
    uint8_t rx_squ_mod_port[4];
    uint8_t rx_squ_filt_time;
    uint8_t slot_id;
    uint32_t ad_samp_ratio;
    train_ratio_irq_t ratio_irq;

    /* CPU->FPGA */
    train_car_info_spi_t tx_car[10];

    /* FPGA->CPU */
    train_car_info_spi_t rx_car[4];

    uint16_t send_time; //定时发送时刻
    uint8_t soft_err_en;
    uint8_t vcc_fp_en;
} __PACKED spi_sys_config_t;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void spi_config_init();
void slot0_1_send_cfg_spi();
void slot2_send_cfg_spi();
void slot3_send_cfg_spi();
#ifdef __cplusplus
}
#endif

#endif  //_spi_cfg
