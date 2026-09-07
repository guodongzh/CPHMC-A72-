/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       aurora_data.c
 *@author     Xuquanbing
 *@date       2025.12.04
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.12.04  1.0       Xuquanbing
 *2025.12.29  1.1       LiuRui     refactor
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "aurora_data.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
aurora_send_cfg_t aurora_send_cfg = {0};
pcie_aurora_fbk_t pcie_aurora_fbk = {0};
pcie_aurora_data_t pcie_aurora_data = {0};
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
void aurora_cfg(uint8_t slot_id, uint8_t sfp_id, uint8_t data_num)
{
    if ((slot_id < AURORA_SLOT_NUM) && sfp_id < AURORA_PORT_NUM)
    {
        aurora_send_cfg.port_bit_map[sfp_id] = 0x01;
        if (data_num > 128)
            data_num = 128;
        aurora_send_cfg.data_num[sfp_id] = data_num;
    }
}

void aurora_send_data(uint8_t slot_id,
                      uint8_t sfp_id,
                      uint8_t blk_num,
                      uint32_t data1,
                      uint32_t data2,
                      uint32_t data3,
                      uint32_t data4,
                      uint32_t data5,
                      uint32_t data6,
                      uint32_t data7,
                      uint32_t data8)
{
    if ((slot_id < AURORA_SLOT_NUM) && (sfp_id >= 0 && sfp_id < 8) &&
        (blk_num < AURORA_MAX_BLK_NUM))
    {
        uint8_t ch_num = sfp_id * 2;

        aurora_frm_t *p_blk_data = (aurora_frm_t *)(pcie_aurora_data.payload[ch_num]);
        p_blk_data->blk_data[blk_num].data[0] = data1;
        p_blk_data->blk_data[blk_num].data[1] = data2;
        p_blk_data->blk_data[blk_num].data[2] = data3;
        p_blk_data->blk_data[blk_num].data[3] = data4;
        p_blk_data->blk_data[blk_num].data[4] = data5;
        p_blk_data->blk_data[blk_num].data[5] = data6;
        p_blk_data->blk_data[blk_num].data[6] = data7;
        p_blk_data->blk_data[blk_num].data[7] = data8;
    }
}

void aurora_build_tx_car(train_tx_t *tx_cfg, uint8_t slot_id, pcie_aurora_data_t *pPcieCmdPtr)
{
    car_tx_t *car = NULL;
    if (slot_id < AURORA_SLOT_NUM)
    {
        for (int sfp_id = 0; sfp_id < AURORA_PORT_NUM; ++sfp_id)
        {
            // only slot1
            if (aurora_send_cfg.port_bit_map[sfp_id] && slot_id == 1)
            {
                uint16_t payload_len = aurora_send_cfg.data_num[sfp_id] * 4; //one data is 4 byte
                uint16_t aurora_car_len = CAR_HEADER_LEN + CHECK_SUM_LEN + payload_len;
                uint16_t tx_diff = aurora_car_len % ALIGNED_SIZE;
                if (tx_diff != 0)
                {
                    tx_diff = ALIGNED_SIZE - tx_diff;
                    aurora_car_len += tx_diff;
                }
                int car_index = train_tx_get_buff(tx_cfg, CAR_TYPE_B, &car);
                if (car_index < 0 || car == NULL)
                {
                    pPcieCmdPtr->payload[sfp_id] = NULL;
                    continue;
                }

                pPcieCmdPtr->port_frame_cnt[sfp_id]++;

                car->fream_head = CPU_TO_FPGA;
                car->port_frame_cnt = pPcieCmdPtr->port_frame_cnt[sfp_id];
                car->msg_type = FRAME_TYPE_AURORA;
                car->solt1_port_bit_map = (1 << (sfp_id * 2));
                car->irq_num = 7;
                car->send_mode1 = 1;
                car->send_mode2 = 1;
                car->payload_len = payload_len;
                pPcieCmdPtr->payload[sfp_id] = &car->payload;
                tx_cfg->car_len_b[car_index] = aurora_car_len;

                uint32_t check_sum = 0;
                uint16_t length = CAR_HEADER_LEN / 4;
                uint32_t *sum_data = (uint32_t *)car;
                for (int k = 0; k < length; ++k)
                {
                    check_sum += sum_data[k];
                }
                pPcieCmdPtr->car_head_sum[sfp_id] = check_sum;
            }
        }
    }
}

void aurora_calc_checksum(pcie_aurora_data_t *pPcieCmdPtr, uint8_t slot_id)
{
    if (slot_id < AURORA_SLOT_NUM)
    {
        for (int sfp_id = 0; sfp_id < AURORA_PORT_NUM; ++sfp_id)
        {
            if (aurora_send_cfg.port_bit_map[sfp_id])
            {
                if (pPcieCmdPtr->payload[sfp_id] == NULL)
                {
                    continue;
                }

                uint32_t check_sum = 0;
                uint32_t *sum_data = (uint32_t *)pPcieCmdPtr->payload[sfp_id];
                /*check and clear*/
                uint16_t payload_len = aurora_send_cfg.data_num[sfp_id] * 4; //one data is 4 byte
                uint16_t aurora_car_len = CAR_HEADER_LEN + CHECK_SUM_LEN + payload_len;
                uint16_t tx_diff = aurora_car_len % ALIGNED_SIZE;
                if (tx_diff != 0)
                {
                    tx_diff = ALIGNED_SIZE - tx_diff;
                    aurora_car_len += tx_diff;
                    uint8_t *p_clr = pPcieCmdPtr->payload[sfp_id] + payload_len;
                    memset(p_clr, 0u, tx_diff);
                }

                uint16_t length = ((aurora_car_len - CAR_HEADER_LEN) / 4) - 1;
                for (int k = 0; k < length; ++k)
                {
                    check_sum += sum_data[k];
                }
                sum_data[length] = pPcieCmdPtr->car_head_sum[sfp_id] + check_sum;
            }
        }
    }
}

void aurora_recv_handler(train_rx_t *train_rx, uint8_t src_slot, car_rx_t *car)
{
    uint8_t src_port = (car->src_port - 1) / 2;
    uint16_t aurora_frm_len = car->payload_len;
    pcie_aurora_fbk.len[src_port] = aurora_frm_len;

    if (src_slot < 4 && src_port < GETH_PORT_NUM)
    {
        pcie_aurora_fbk.diag_info[src_port].time_tag = car->time_cnt;
        if (aurora_frm_len <= 512)
        {
            if (car->port_frame_cnt !=
                (uint16_t)(train_rx->pre_aurora_frame_cnt[src_slot][src_port] + 1))
            {
                pcie_aurora_fbk.payload[src_port] = NULL;
                pcie_aurora_fbk.diag_info[src_port].lost_frm_cnt++;
                pcie_aurora_fbk.diag_info[src_port].data_valid = 0;
            }
            else
            {
                pcie_aurora_fbk.payload[src_port] = &car->payload;
                pcie_aurora_fbk.diag_info[src_port].data_valid = 1;
                pcie_aurora_fbk.diag_info[src_port].data_oks++;
            }
        }
        else
        {
            pcie_aurora_fbk.payload[src_port] = NULL;
            pcie_aurora_fbk.diag_info[src_port].data_valid = 0;
            pcie_aurora_fbk.diag_info[src_port].app_sum_errs++;
        }
        train_rx->pre_aurora_frame_cnt[src_slot][src_port] = car->port_frame_cnt;
    }
}

aurora_data_t *aurora_recv_data(uint8_t slot_id, uint8_t sfp_id, uint8_t blk_num)
{
    aurora_data_t *data = NULL;
    if (slot_id < AURORA_SLOT_NUM && (sfp_id >= 0 && sfp_id < AURORA_PORT_NUM) &&
        blk_num < AURORA_MAX_BLK_NUM)
    {
        if (pcie_aurora_fbk.payload[sfp_id] != NULL)
        {
            aurora_frm_t *aurora_frm = (aurora_frm_t *)pcie_aurora_fbk.payload[sfp_id];
            data = &aurora_frm->blk_data[blk_num];
        }
    }
    return data;
}

void reset_aurora_diag_processing_flags(void)
{
    memset(pcie_aurora_fbk.diag_process_flag, 0, sizeof(pcie_aurora_fbk.diag_process_flag));
}
diag_info_t *get_aurora_diag_info(uint8_t slot_id, uint8_t sfp_id)
{
    if (slot_id < AURORA_SLOT_NUM && (sfp_id >= 0 && sfp_id < AURORA_PORT_NUM))
    {
        diag_info_t *diag_info = &(pcie_aurora_fbk.diag_info[sfp_id]);
        if (!pcie_aurora_fbk.diag_process_flag[sfp_id])
        {
            if (diag_info->data_valid && diag_info->last_data_valid &&
                (diag_info->last_time_tag == diag_info->time_tag))
            {
                diag_info->data_valid = 0;
                pcie_aurora_fbk.payload[sfp_id] = NULL;
            }

            diag_info->last_data_valid = diag_info->data_valid;
            diag_info->last_time_tag = diag_info->time_tag;
            pcie_aurora_fbk.diag_process_flag[sfp_id] = 1;
        }

        return diag_info;
    }
    return NULL;
}
