/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       redun_switch.h
 *@author
 *@date       2026.08.13
 *@brief      Redundancy switch data transport interface
 ******************************************************************************/

#ifndef _REDUN_SWITCH_H_
#define _REDUN_SWITCH_H_

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include "pcie_fpga.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
/* This module is deployed only on MCU1_1. The train routing is fixed. */
#define RSMC_TX_TRAIN_INDEX         (2U)
#define RSMC_RX_TRAIN_INDEX         (1U)
#define RSMC_SRC_SLOT               (0U)
#define RSMC_PORT_NUM               (4U)
#define RSMC_PORT_FIRST             (0xF0U)
#define RSMC_IRQ_NUM                (8U)
#define RSMC_CAR_TYPE               CAR_TYPE_S
#define RSMC_PAYLOAD_MAX_LEN        (CAR_LEN_S - CAR_HEADER_LEN - CHECK_SUM_LEN)
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef struct _redun_switch_diag
{
    uint32_t tx_ok_cnt;
    uint32_t tx_no_buff_cnt;
    uint32_t rx_ok_cnt;
    uint32_t rx_repeat_cnt;
    uint32_t rx_port_err_cnt;
    uint32_t rx_len_err_cnt;
} redun_switch_diag_t;

typedef struct _redun_switch_rx_data
{
    uint8_t *payload;
    uint16_t payload_len;
    uint32_t time_tag;
    uint8_t valid;
} redun_switch_rx_data_t;

typedef struct _redun_switch_tx_data
{
    uint8_t *payload;
    uint16_t payload_len;
} redun_switch_tx_data_t;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void redun_switch_cfg(uint16_t payload_len);
void redun_switch_build_tx_car(void);
void redun_switch_calc_checksum(void);
void redun_switch_recv_handler(train_rx_t *train_rx, uint8_t src_slot, car_rx_t *car);
void reset_redun_switch_diag_processing_flags(void);

uint8_t *redun_switch_get_tx_payload(uint8_t port_nr, uint16_t *payload_len);
uint8_t *redun_switch_get_rx_payload(uint8_t port_nr, uint16_t *payload_len);

const volatile redun_switch_rx_data_t *redun_switch_get_rx_data(uint8_t port_nr);
const volatile redun_switch_diag_t *redun_switch_get_diag(void);
diag_info_t *get_redun_switch_diag_info(uint8_t port_nr);

uint8_t *trainrx_header(uint8_t port_nr);
uint8_t *traintx_header(uint8_t port_nr);

#ifdef __cplusplus
}
#endif

#endif /* _REDUN_SWITCH_H_ */
