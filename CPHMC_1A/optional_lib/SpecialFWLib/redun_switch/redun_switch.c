/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       redun_switch.c
 *@date       2026.08.13
 *@brief      Redundancy switch data transport implementation
 ******************************************************************************/

#include "redun_switch.h"

#include <string.h>

volatile uint16_t cycdata_cnt = 0;
volatile uint32_t cycdata = 0;
typedef struct _redun_switch_ctx
{
    volatile redun_switch_rx_data_t rx_data;
    redun_switch_tx_data_t tx_data;
    uint8_t rx_payload[RSMC_PAYLOAD_MAX_LEN];
    uint32_t car_head_sum;
    uint32_t tx_cycdata;
    uint16_t port_frame_cnt;
    uint16_t rx_frame_cnt;
    uint16_t payload_len;
    diag_info_t diag_info;
    uint8_t diag_process_flag;
} redun_switch_ctx_t;

redun_switch_ctx_t g_redun_switch[RSMC_PORT_NUM] = {0};
static volatile redun_switch_diag_t g_redun_switch_diag = {0};

static train_tx_t *redun_switch_get_train_tx(void)
{
    return &slot0_1_train_tx[RSMC_TX_TRAIN_INDEX];
}

static train_rx_t *redun_switch_get_train_rx(void)
{
    return &slot0_1_train_rx[RSMC_RX_TRAIN_INDEX];
}

static uint16_t redun_switch_align_car_len(uint16_t payload_len)
{
    uint16_t car_len = CAR_HEADER_LEN + payload_len + CHECK_SUM_LEN;
    uint16_t remainder = car_len % ALIGNED_SIZE;

    if (remainder != 0U)
    {
        car_len += ALIGNED_SIZE - remainder;
    }

    return car_len;
}

static uint32_t redun_switch_calc_sum(const void *data, uint16_t len)
{
    const uint32_t *sum_data = (const uint32_t *)data;
    uint32_t check_sum = 0;
    uint16_t word_num = len / sizeof(uint32_t);
    uint16_t i;

    for (i = 0; i < word_num; ++i)
    {
        check_sum += sum_data[i];
    }

    return check_sum;
}

void redun_switch_cfg(uint16_t payload_len)
{
    uint8_t port_nr;

    if (payload_len > RSMC_PAYLOAD_MAX_LEN)
    {
        payload_len = RSMC_PAYLOAD_MAX_LEN;
    }

    for (port_nr = 0U; port_nr < RSMC_PORT_NUM; ++port_nr)
    {
        g_redun_switch[port_nr].payload_len = payload_len;
        g_redun_switch[port_nr].tx_data.payload = NULL;
        g_redun_switch[port_nr].tx_data.payload_len = payload_len;
    }
}

void redun_switch_build_tx_car(void)
{
    train_tx_t *train_tx = redun_switch_get_train_tx();
    car_tx_t *car = NULL;
    int car_index;
    uint16_t car_len;
    uint32_t *sum_data;
    uint16_t i;
    uint8_t port_nr;

    if (g_redun_switch[0].payload_len == 0U)
    {
        redun_switch_cfg(RSMC_PAYLOAD_MAX_LEN);
    }

    car_len = redun_switch_align_car_len(g_redun_switch[0].payload_len);

    for (port_nr = 0U; port_nr < RSMC_PORT_NUM; ++port_nr)
    {
        car = NULL;
        car_index = train_tx_get_buff(train_tx, RSMC_CAR_TYPE, &car);
        if ((car_index < 0) || (car == NULL))
        {
            g_redun_switch[port_nr].tx_data.payload = NULL;
            g_redun_switch_diag.tx_no_buff_cnt++;
            continue;
        }

        car->fream_head = CPU_TO_FPGA;
        car->port_frame_cnt = ++g_redun_switch[port_nr].port_frame_cnt;
        car->msg_type = FRAME_TYPE_REDUNDANCY;
        car->send_mode1 = 0U;
        car->send_mode2 = 0U;
        car->payload_len = g_redun_switch[port_nr].payload_len;
        car->irq_num = RSMC_IRQ_NUM;
        car->solt0_port_bit_map = (1U << port_nr);
        car->solt1_port_bit_map = 0U;
        car->solt2_port_bit_map = 0U;
        car->solt3_port_bit_map = 0U;

        g_redun_switch[port_nr].tx_data.payload = &car->payload;
        g_redun_switch[port_nr].tx_data.payload_len =
            g_redun_switch[port_nr].payload_len;
        memset(g_redun_switch[port_nr].tx_data.payload,
               0,
               g_redun_switch[port_nr].tx_data.payload_len);

        train_tx->car_len_s[car_index] = car_len;

        sum_data = (uint32_t *)car;
        g_redun_switch[port_nr].car_head_sum = 0U;
        for (i = 0U; i < (CAR_HEADER_LEN / sizeof(uint32_t)); ++i)
        {
            g_redun_switch[port_nr].car_head_sum += sum_data[i];
        }
    }
}

static void redun_switch_calc_car_checksum(uint8_t port_nr)
{
    uint16_t car_len;
    uint16_t diff_len;
    uint16_t word_len;
    uint8_t *pad;
    uint32_t *sum_data;
    uint32_t check_sum;

    if ((port_nr >= RSMC_PORT_NUM) ||
        (g_redun_switch[port_nr].tx_data.payload == NULL) ||
        (g_redun_switch[port_nr].payload_len == 0U))
    {
        return;
    }

    car_len = redun_switch_align_car_len(g_redun_switch[port_nr].payload_len);
    diff_len = car_len -
               (CAR_HEADER_LEN + g_redun_switch[port_nr].payload_len + CHECK_SUM_LEN);
    if (diff_len > 0U)
    {
        pad = g_redun_switch[port_nr].tx_data.payload +
              g_redun_switch[port_nr].payload_len;
        memset(pad, 0, diff_len);
    }

    ((uint32_t *)g_redun_switch[port_nr].tx_data.payload)[1] =
        ++g_redun_switch[port_nr].tx_cycdata;

    word_len = ((car_len - CAR_HEADER_LEN) / sizeof(uint32_t)) - 1U;
    sum_data = (uint32_t *)g_redun_switch[port_nr].tx_data.payload;
    check_sum = redun_switch_calc_sum(g_redun_switch[port_nr].tx_data.payload,
                                      word_len * sizeof(uint32_t));
    sum_data[word_len] = g_redun_switch[port_nr].car_head_sum + check_sum;
    g_redun_switch_diag.tx_ok_cnt++;
}

void redun_switch_calc_checksum(void)
{
    uint8_t port_nr;

    for (port_nr = 0U; port_nr < RSMC_PORT_NUM; ++port_nr)
    {
        redun_switch_calc_car_checksum(port_nr);
    }
}

void redun_switch_recv_handler(train_rx_t *train_rx, uint8_t src_slot, car_rx_t *car)
{
    uint8_t repeat = 0U;
    uint8_t port_nr;
    uint8_t src_port;

    if ((train_rx != redun_switch_get_train_rx()) ||
        (src_slot != RSMC_SRC_SLOT) ||
        (car == NULL) ||
        (car->port_type != FRAME_TYPE_REDUNDANCY) ||
        (car->src_slot != RSMC_SRC_SLOT) ||
        (car->src_port < RSMC_PORT_FIRST) ||
        (car->src_port >= (RSMC_PORT_FIRST + RSMC_PORT_NUM)))
    {
        g_redun_switch_diag.rx_port_err_cnt++;
        return;
    }
    if(car->src_port != 0xF0)
    {
        cycdata_cnt++;
    }
    src_port = car->src_port;
    port_nr = (uint8_t)(src_port - RSMC_PORT_FIRST);

    if ((g_redun_switch[port_nr].payload_len != 0U) &&
        (car->payload_len != g_redun_switch[port_nr].payload_len))
    {
        g_redun_switch_diag.rx_len_err_cnt++;
        return;
    }
    if ((car->payload_len == 0U) ||
        (car->payload_len > RSMC_PAYLOAD_MAX_LEN))
    {
        g_redun_switch_diag.rx_len_err_cnt++;
        return;
    }

    if ((g_redun_switch[port_nr].rx_data.valid != 0U) &&
        (g_redun_switch[port_nr].rx_data.payload_len == car->payload_len) &&
        (g_redun_switch[port_nr].rx_data.time_tag == car->time_cnt))
    {
        repeat = 1U;
    }

    memcpy(g_redun_switch[port_nr].rx_payload,
           &car->payload,
           car->payload_len);
    g_redun_switch[port_nr].diag_info.time_tag = car->time_cnt;
    if (car->port_frame_cnt !=
        (uint16_t)(g_redun_switch[port_nr].rx_frame_cnt + 1U))
    {
        g_redun_switch[port_nr].diag_info.data_valid = 0U;
        g_redun_switch[port_nr].diag_info.lost_frm_cnt++;
    }
    else
    {
        g_redun_switch[port_nr].diag_info.data_valid = 1U;
        g_redun_switch[port_nr].diag_info.data_oks++;
    }
    g_redun_switch[port_nr].rx_frame_cnt = car->port_frame_cnt;
    g_redun_switch[port_nr].rx_data.payload = g_redun_switch[port_nr].rx_payload;
    g_redun_switch[port_nr].rx_data.payload_len = car->payload_len;
    g_redun_switch[port_nr].rx_data.time_tag = car->time_cnt;
    g_redun_switch[port_nr].rx_data.valid = 1U;
    g_redun_switch_diag.rx_ok_cnt++;
    if (repeat != 0U)
    {
        g_redun_switch_diag.rx_repeat_cnt++;
    }
}

uint8_t *redun_switch_get_tx_payload(uint8_t port_nr, uint16_t *payload_len)
{
    if (port_nr >= RSMC_PORT_NUM)
    {
        return NULL;
    }
    if (payload_len != NULL)
    {
        *payload_len = g_redun_switch[port_nr].tx_data.payload_len;
    }
    return g_redun_switch[port_nr].tx_data.payload;
}

uint8_t *redun_switch_get_rx_payload(uint8_t port_nr, uint16_t *payload_len)
{
    if (port_nr >= RSMC_PORT_NUM)
    {
        return NULL;
    }
    if (payload_len != NULL)
    {
        *payload_len = g_redun_switch[port_nr].rx_data.payload_len;
    }

    if (g_redun_switch[port_nr].rx_data.valid == 0U)
    {
        return NULL;
    }

    return g_redun_switch[port_nr].rx_data.payload;
}

const volatile redun_switch_rx_data_t *redun_switch_get_rx_data(uint8_t port_nr)
{
    if (port_nr >= RSMC_PORT_NUM)
    {
        return NULL;
    }
    return &g_redun_switch[port_nr].rx_data;
}

const volatile redun_switch_diag_t *redun_switch_get_diag(void)
{
    return &g_redun_switch_diag;
}

void reset_redun_switch_diag_processing_flags(void)
{
    uint8_t port_nr;

    for (port_nr = 0U; port_nr < RSMC_PORT_NUM; ++port_nr)
    {
        g_redun_switch[port_nr].diag_process_flag = 0U;
    }
}

diag_info_t *get_redun_switch_diag_info(uint8_t port_nr)
{
    diag_info_t *diag_info;

    if (port_nr >= RSMC_PORT_NUM)
    {
        return NULL;
    }

    diag_info = &g_redun_switch[port_nr].diag_info;
    if (g_redun_switch[port_nr].diag_process_flag == 0U)
    {
        if (diag_info->data_valid &&
            diag_info->last_data_valid &&
            (diag_info->last_time_tag == diag_info->time_tag))
        {
            diag_info->data_valid = 0U;
            g_redun_switch[port_nr].rx_data.valid = 0U;
            g_redun_switch[port_nr].rx_data.payload = NULL;
        }

        diag_info->last_data_valid = diag_info->data_valid;
        diag_info->last_time_tag = diag_info->time_tag;
        g_redun_switch[port_nr].diag_process_flag = 1U;
    }

    return diag_info;
}

uint8_t *trainrx_header(uint8_t port_nr)
{
    return redun_switch_get_rx_payload(port_nr, NULL);
}

uint8_t *traintx_header(uint8_t port_nr)
{
    return redun_switch_get_tx_payload(port_nr, NULL);
}
