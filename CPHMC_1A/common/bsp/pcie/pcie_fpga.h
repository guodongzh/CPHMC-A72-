/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pcie_fpga.h
 *@author     LiuRui
 *@date       2024.12.18
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2024.12.18  1.0       LiuRui
 ******************************************************************************/

#ifndef _PCIE_TRAIN_H
#define _PCIE_TRAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include "pcie_init.h"
#include <ti/drv/udma/udma.h>

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define __PACKED               __attribute__((packed))
#define USE_SPI_CFG

/**
 * frame type
 */
#define FRAME_TYPE_FT3         0x1
#define FRAME_TYPE_ETH         0x2
#define FRAME_TYPE_GETH        0x3
#define FRAME_TYPE_SFP         0x4
#define FRAME_TYPE_AURORA      0x5
#define FRAME_TYPE_CAN         0x11
#define FRAME_TYPE_AD          0x21
#define FRAME_TYPE_REDUNDANCY  0x22  // 双机冗余通信
#define FRAME_TYPE_LVDS        0x31
#define FRAME_TYPE_LAN         0x41  // internal network

/**
 * train info
 */
#define TRAIN_HEADER_LEN       128  // train header length
#define CAR_HEADER_LEN         32   // car header length
#define CHECK_SUM_LEN          4
#define CAR_LEN_S              128   // car max length
#define CAR_LEN_B              2048  // car max length
#define CAR_NUM                16    // the number of car
#define ALIGNED_SIZE           16

/**
 * car type flag
 */
#define CAR_TYPE_S             0x00
#define CAR_TYPE_B             0xff

/**
 *data dir
 */
#define CPU_TO_FPGA            0x1234
#define FPGA_TO_CPU            0x4321

/**
 * cfg addr
 */

/**
 * @{ PCIE0
 */
#define PCIE0_SYS_INIT_ADDR    PCIE0_XDMA_BYPASS_BASE
#define PCIE0_SYS_CFG_ADDR     (PCIE0_SYS_INIT_ADDR + 0X1000)

#define PCIE0_GETH1_CFG_ADDR   (PCIE0_SYS_INIT_ADDR + 0X100000)

#define PCIE0_FT3_TX1_CFG_ADDR (PCIE0_SYS_INIT_ADDR + 0X200000)
#define PCIE0_FT3_RX1_CFG_ADDR (PCIE0_SYS_INIT_ADDR + 0x280000)

#define PCIE0_ETH1_CFG_ADDR    (PCIE0_SYS_INIT_ADDR + 0X300000)

#define AURORA_CFG_ADDR        (PCIE0_SYS_INIT_ADDR + 0X500000)

/**
 * @}
 */

/**
 * @{ PCIE2
 */

#define PCIE2_SYS_INIT_ADDR    PCIE2_XDMA_BYPASS_BASE
#define PCIE2_SYS_CFG_ADDR     (PCIE2_SYS_INIT_ADDR + 0X1000)

#define PCIE2_GETH1_CFG_ADDR   (PCIE2_SYS_INIT_ADDR + 0X100000)

#define PCIE2_FT3_TX1_CFG_ADDR (PCIE2_SYS_INIT_ADDR + 0X200000)
#define PCIE2_FT3_RX1_CFG_ADDR (PCIE2_SYS_INIT_ADDR + 0x280000)

#define PCIE2_ETH1_CFG_ADDR    (PCIE2_SYS_INIT_ADDR + 0X300000)

/**
 * @}
 */

/**
 * @{ PCIE3
 */

#define PCIE3_SYS_INIT_ADDR    PCIE3_XDMA_BYPASS_BASE
#define PCIE3_SYS_CFG_ADDR     (PCIE3_SYS_INIT_ADDR + 0X1000)

#define PCIE3_GETH1_CFG_ADDR   (PCIE3_SYS_INIT_ADDR + 0X100000)

#define PCIE3_FT3_TX1_CFG_ADDR (PCIE3_SYS_INIT_ADDR + 0X200000)
#define PCIE3_FT3_RX1_CFG_ADDR (PCIE3_SYS_INIT_ADDR + 0x280000)

#define PCIE3_ETH1_CFG_ADDR    (PCIE3_SYS_INIT_ADDR + 0X300000)

/**
 * @}
 */

/**
 *
 */
#define FT3_CH_CFG_OFFSET      (0x1000)
#define FT3_RX_CH_NUM          (32U)
#define FT3_TX_CH_NUM          (32U)
#define FT3_SOLT_NUM           (4U)

#define GETH_CFG_OFFSET        (0x1000)
#define GETH_SOLT_NUM          (4U)
#define GETH_PORT_NUM          (8U)

#define ETH_CFG_OFFSET         (0x1000)
#define ETH_SOLT_NUM           (4U)
#define ETH_PORT_NUM           (8U)

#define AURORA_CFG_OFFSET      (0x1000)
#define AURORA_SLOT_NUM        (4U)
#define AURORA_PORT_NUM        (8U)

#define USE_C66x_RAM           0

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

typedef struct _sys_init
{
    uint16_t fream_head;
    uint16_t arm_init_ok;
    uint16_t resv0[6];
    uint16_t soft_ver;
    uint16_t fun_ver;
    uint32_t soft_code;
    uint16_t resv1[2];
    uint32_t check_sum;
} __PACKED sys_init_t;

typedef struct _eth_cfg
{
    uint16_t fream_head;
    uint8_t train_rx_id;  // 1~4
    uint8_t resv[25];
    uint32_t check_sum;
} __PACKED eth_cfg_t;

typedef struct _aurora_cfg
{
    uint16_t fream_head;
    uint8_t train_rx_id;  // 1~4
    uint8_t resv[25];
    uint32_t check_sum;
} __PACKED aurora_cfg_t;

#ifndef USE_SPI_CFG
typedef struct _ft3_cfg
{
    uint16_t frame_head;
    uint8_t train_rx_id;  // 1~4
    uint8_t resv1;
    uint8_t blk_num;
    uint8_t last_blk_len;
    uint16_t frm_data_len;
    uint16_t max_time_out;
    uint16_t comm_t_set;
    uint8_t bps_div;
    uint8_t polset : 2;
    uint8_t unlock_com_err_dis : 1;
    uint8_t resv2 : 5;
    uint8_t samp_slow;
    uint8_t resv3[13];
    uint32_t check_sum;
} __PACKED ft3_cfg_t;
#else

typedef struct _ft3_cfg
{
    uint8_t train_rx_id;  // 1~4
    uint8_t FT3Type;      // 1: single channel; 2: three channels
    uint8_t blk_num;
    uint8_t last_blk_len;
    uint16_t frm_data_len;
    uint16_t max_time_out;
    uint16_t comm_t_set;
    uint8_t bps_div;
    uint8_t polset : 2;
    uint8_t unlock_com_err_dis : 1;
    uint8_t resv2 : 5;
    uint8_t monitor_mode;
    uint8_t monitor_samp_slow_mode;
    uint8_t monitor_samp_slow_point;
    uint8_t resv3[17];
} __PACKED ft3_cfg_t;


#endif
/**
 * intr period(hz)
 */
typedef struct _train_ratio_irq
{
    uint32_t ratio_irq1;
    uint32_t ratio_irq2;
    uint32_t ratio_irq3;
    uint32_t ratio_irq4;
    uint32_t ratio_irq5;
    uint32_t ratio_irq6;
    uint32_t ratio_irq7;
    uint32_t ratio_irq8;
} __PACKED train_ratio_irq_t;

/**
 * all addr is pcie addr
 */
typedef struct _train_car_info
{
    uint8_t irq_num;  // 1~8
    uint8_t resv0;
    uint16_t time;
    uint32_t header_addr;
    uint32_t car_addr_s0;
    uint32_t car_addr_s1;
    uint32_t car_addr_b0;
    uint32_t car_addr_b1;
    uint32_t resv1;
    uint32_t resv2;
} __PACKED train_car_info_t;

/**
 * sys cfg
 */
typedef struct _sys_config
{
    uint16_t fream_head;
    uint8_t tx_squ_mod_port[4];
    uint8_t rx_squ_mod_port[4];
    uint8_t rx_squ_filt_time;
    uint8_t slot_id;
    uint32_t ad_samp_ratio;
    train_ratio_irq_t ratio_irq;

    /* CPU->FPGA */
    train_car_info_t tx_car[10];

    /* FPGA->CPU */
    train_car_info_t rx_car[4];

    uint32_t resv1[3];
    uint32_t sum_check;
} __PACKED sys_config_t;

typedef struct _train_header_rx
{
    uint16_t fream_head;
    uint16_t heart_beat_cnt;
    uint8_t car_id;
    uint8_t car_num;
    uint8_t resv0[2];
    uint32_t pcie_addr;
    uint32_t irq_time_stamp;
    uint8_t fbk_slot_id;
    uint8_t fbk_port_id;
    uint8_t fbk_data[14];
    uint8_t resv1[12];
    uint32_t solt0_port_state;
    uint32_t solt1_port_state;
    uint32_t solt2_port_state;
    uint32_t solt3_port_state;
    uint8_t resv2[4];
    uint8_t sys_status_data[32];
    uint8_t resv3[28];
    uint32_t check_sum;
} __PACKED train_header_rx_t;

typedef struct _train_header_tx
{
    uint16_t fream_head;
    uint16_t heart_beat_cnt;
    uint8_t car_id;  // FPGA current read car ID, 1 or 2
    uint8_t car_num_s;
    uint8_t car_num_b;
    uint8_t resv1;
    uint8_t resv2[8];
    uint16_t car_len_s[16];
    uint16_t car_len_b[16];
    uint8_t resv3[16];
    uint8_t cmd[28];
    uint32_t check_sum;
} __PACKED train_header_tx_t;

typedef struct _car_tx
{
    uint16_t fream_head;
    volatile uint16_t port_frame_cnt;
    uint8_t msg_type;
    uint8_t send_mode1 : 4;
    uint8_t send_mode2 : 4;
    uint16_t payload_len;
    uint32_t solt0_port_bit_map;
    uint32_t solt1_port_bit_map;
    uint32_t solt2_port_bit_map;
    uint32_t solt3_port_bit_map;
    uint16_t irq_num : 4; //1~7
    uint16_t resv1 : 12;
    uint8_t resv2[6];
    uint8_t payload;
} __PACKED car_tx_t;

typedef struct _car_rx
{
    uint16_t fream_head;
    volatile uint16_t port_frame_cnt;
    uint8_t src_slot;
    uint8_t src_port;
    uint8_t port_type;
    uint8_t resv0;
    uint16_t payload_len;
    uint32_t time_cnt;
    uint8_t resv1[2];
    uint16_t eth_type;
    uint16_t appid;
    uint16_t net_storm_state;
    uint8_t dst_mac_addr[6];
    uint8_t resv2[4];
    uint8_t payload;
} __PACKED car_rx_t;

typedef struct _train_rx
{
    train_header_rx_t *header;
    car_rx_t *car0[CAR_NUM]; /*app do not touch this*/
    car_rx_t *car1[CAR_NUM]; /*app do not touch this*/

    /**
     * {@brief debug info
     */
    volatile uint32_t rx_eth_fpga;
    volatile uint32_t rx_geth_fpga;
    volatile uint32_t rx_ft3_fpga;
    volatile uint32_t rx_aurora_fpga;
    volatile uint32_t car_num_ok_cnt;
    volatile uint32_t car_num_errs;
    volatile uint32_t car_len_errs[CAR_NUM];
    volatile uint32_t car_sum_errs[CAR_NUM];
    volatile uint32_t header_sum_err;
    volatile uint16_t pre_ft3_frame_cnt[FT3_SOLT_NUM][FT3_RX_CH_NUM];
    volatile uint16_t pre_eth_frame_cnt[ETH_PORT_NUM];
    volatile uint16_t pre_aurora_frame_cnt[AURORA_SLOT_NUM][AURORA_PORT_NUM];
    /**
     * @}
     */
    const uint16_t car_size; /*app do not touch this*/
    const uint8_t car_num;   /*app do not touch this*/
} train_rx_t;

typedef struct _train_tx
{
    train_header_tx_t *header;
    uint16_t car_len_s[CAR_NUM]; /*app must update this*/
    uint16_t car_len_b[CAR_NUM]; /*app must update this*/
    car_tx_t *car_s[2][CAR_NUM]; /*app do not touch this*/
    car_tx_t *car_b[2][CAR_NUM]; /*app do not touch this*/
    const uint16_t car_size_s;   /*app do not touch this*/
    const uint16_t car_size_b;   /*app do not touch this*/
    const uint8_t car_num_s;     /*app do not touch this*/
    const uint8_t car_num_b;     /*app do not touch this*/
    uint8_t write_index_s;       /*app do not touch this*/
    uint8_t write_index_b;       /*app do not touch this*/
} train_tx_t;

typedef struct _car_ctrl
{
    uint8_t frame_cnt;
    uint8_t *car[CAR_NUM];
} car_ctrl_t;

typedef struct _diag_info
{
    uint32_t time_tag;
    uint32_t last_time_tag;
    uint8_t last_data_valid;
    uint8_t data_valid;
    uint32_t data_oks;
    uint32_t head_sum_errs;
    uint32_t car_sum_errs;
    uint32_t app_sum_errs;
    uint32_t lost_frm_cnt;
} diag_info_t;

typedef struct _init_param
{
    uint32_t core0_main_intr_time;
    uint32_t core1_main_intr_time;
    uint32_t core2_main_intr_time;
    uint32_t core3_main_intr_time;
    uint32_t core4_main_intr_time;
    uint32_t core5_main_intr_time;
    uint32_t core6_main_intr_time;
    uint32_t core7_main_intr_time;
    uint32_t core0_swi_time_interval;
    uint32_t core1_swi_time_interval;
    uint32_t core2_swi_time_interval;
    uint32_t core3_swi_time_interval;
    uint32_t core4_swi_time_interval;
    uint32_t core5_swi_time_interval;
    uint32_t core6_swi_time_interval;
    uint32_t core7_swi_time_interval;
} app_sys_param;

typedef struct
{
    train_rx_t *train_rx;
    uint8_t slot_num;
} core_train_rx_t;

typedef struct
{
    train_tx_t *train_tx;
    uint8_t slot_num;
} core_train_tx_ft3_t;

typedef struct
{
    train_tx_t *train_tx;
    uint8_t slot_num;
} core_train_tx_aurora_t;

typedef void (*train_recv_handler)(train_rx_t *, uint8_t , car_rx_t *);

extern app_sys_param init_param;

extern train_tx_t slot0_1_train_tx[10];
extern train_tx_t slot2_train_tx[10];
extern train_tx_t slot3_train_tx[10];

extern train_rx_t slot0_1_train_rx[4];
extern train_rx_t slot2_train_rx[4];
extern train_rx_t slot3_train_rx[4];

extern train_tx_t slot0_1_train_c66tx[1];
extern train_rx_t slot0_1_train_c66rx[4];

extern core_train_rx_t core_train_rx[8];
extern core_train_rx_t core_train_c66rx[8];
extern train_tx_t *core_train_tx[7];
extern train_tx_t *core_train_c66tx[7];
extern core_train_tx_ft3_t core_train_tx_ft3[5];
extern core_train_tx_aurora_t core_train_tx_aurora[2];
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

int train_irq_tim_cfg(sys_config_t *sys_config,
                      uint16_t t1_us,
                      uint16_t t2_us,
                      uint16_t t3_us,
                      uint16_t t4_us,
                      uint16_t t5_us,
                      uint16_t t6_us,
                      uint16_t t7_us,
                      uint16_t t8_us);

int train_tx_irq_num_cfg(sys_config_t *sys_config,
                         uint8_t exp_irq,
                         uint8_t fast1_irq,
                         uint8_t fast2_irq,
                         uint8_t fast3_irq,
                         uint8_t fast4_irq,
                         uint8_t fast5_irq,
                         uint8_t fast6_irq,
                         uint8_t fast7_irq,
                         uint8_t fast8_irq,
                         uint8_t slow_irq);

int train_xdma_time_cfg(sys_config_t *sys_config,
                        uint16_t time1,
                        uint16_t time2,
                        uint16_t time3,
                        uint16_t time4,
                        uint16_t time5,
                        uint16_t time6,
                        uint16_t time7,
                        uint16_t time8,
                        uint16_t time9,
                        uint16_t time10);

int train_rx_irq_num_cfg(sys_config_t *sys_config,
                         uint8_t exp_irq,
                         uint8_t fast_irq_s,
                         uint8_t fast_irq_b,
                         uint8_t slow_irq);

int train_reg_recv_handler(uint8_t frame_type, train_recv_handler handler);
int train_recv(train_rx_t *train_rx, uint8_t src_slot);

int train_tx_get_buff(train_tx_t *train_tx, uint8_t car_type, car_tx_t **buff);

int train_tx_update_header(train_tx_t *train_tx);

int pcie_send_cfg_with_dma(const void *cfg, void *cfg_bak, uint64_t dest_addr, uint32_t len);
int pcie_send_cfg_by_cpu(const void *cfg, void *cfg_bak, uint64_t dest_addr, uint32_t len);

void slot0_1_send_all_cfg(sys_config_t *sys_config, sys_config_t *sys_config_bak);
void slot3_send_all_cfg(sys_config_t *sys_config, sys_config_t *sys_config_bak);
void slot2_send_all_cfg(sys_config_t *sys_config, sys_config_t *sys_config_bak);

uint64_t train_tx_ram_addr_init(train_tx_t *train_tx, uint32_t train_num, uint64_t _ram_addr);
uint32_t train_rx_ram_addr_init(train_rx_t *train_rx, uint32_t train_num, uint64_t _ram_addr);
void slot_fpga_init_comm();
void train_all_ram_addr_init();
int slot0_1_fpga_init(sys_config_t *sys_config);
int slot2_fpga_init(sys_config_t *sys_config);
int slot3_fpga_init(sys_config_t *sys_config);

#ifdef __cplusplus
}
#endif

#endif  //_PCIE_TRAIN_H
