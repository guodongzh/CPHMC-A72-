/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pcie_ft3.c
 *@author     LiuRui
 *@date       2025.03.31
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.03.31  1.0        LiuRui
 *2025.04.18  2.0        wenjunf
 ******************************************************************************/
#include "debug_config.h"
#include "ft3_data.h"

ft3_cfg_chk_t ft3_tx_cfg_all[FT3_SOLT_NUM][FT3_TX_CH_NUM] __attribute__((section(".ft3_tx_cfg")));
ft3_cfg_chk_t ft3_rx_cfg_all[FT3_SOLT_NUM][FT3_RX_CH_NUM] __attribute__((section(".ft3_rx_cfg")));
#ifndef BUILD_C7X_1
#ifndef BUILD_C66X
ft3_cfg_chk_t ft3_tx_cfg_all_prv[FT3_SOLT_NUM][FT3_TX_CH_NUM];
ft3_cfg_chk_t ft3_rx_cfg_all_prv[FT3_SOLT_NUM][FT3_RX_CH_NUM];
#else
ft3_cfg_chk_t ft3_tx_cfg_all_prv[FT3_SOLT_NUM][FT3_TX_CH_NUM] __attribute__((section(".ft3_tx_cfg_prv")));
ft3_cfg_chk_t ft3_rx_cfg_all_prv[FT3_SOLT_NUM][FT3_RX_CH_NUM] __attribute__((section(".ft3_rx_cfg_prv")));
#endif
#endif

ft3_tx_ctrl_t ft3_tx_ctrl[FT3_SOLT_NUM] = {0};
ft3_rx_ctrl_t ft3_rx_ctrl[FT3_SOLT_NUM] = {0};

pcie_ft3_fbk_t pcie_ft3_fbk = {0};
pcie_ft3_data_t pcie_ft3_data = {0};

#ifndef USE_SPI_CFG
/**
 * @brief set ft3 cfg
 * @param cfg_data  pointer to cfg data
 * @param slot_num  slot number(0~3）
 * @param sfp_num   sfp port number(0~15)
 * @param port_num  port number of single sfp port (0 or 1),
 *                  For the single receiver single transmitter module, 
 *                  port 1 is the receiving port and port 0 is the sending port
 * @param is_tx     true: get tx cfg; false:get rx cfg
 */
void ft3_set_cfg(ft3_cfg_t *cfg_data,
                 uint8_t slot_num,
                 uint8_t sfp_num,
                 uint8_t port_num,
                 uint8_t is_tx)
{
    uint32_t *data; /* for calc check sum */
    uint32_t check_sum;
    uint8_t errcode = CFG_VALID;
    uint32_t ch_num = 0;

    /* attention ch_num, csa csb,csd */
    ch_num = (sfp_num * 2) + port_num;
    if (slot_num > 3 || port_num > 1)
        return;
    if ((slot_num == 0 && sfp_num <= 3) ||
        (slot_num == 1 && sfp_num <= 15) ||
        (slot_num == 2 && sfp_num <= 15) ||
        (slot_num == 3 && sfp_num <= 15))
    {
        cfg_data->frame_head = CPU_TO_FPGA;
        cfg_data->train_rx_id = 1;
        if (cfg_data->blk_num == 1)  // 可能为非标准FT3帧,且数据长度双字节对齐
        {
            // TODO 对应用进行限制:last_blk_len单位为双字节
            if (cfg_data->last_blk_len >= 1 && cfg_data->last_blk_len <= 45u)
            {
                // ft3帧头(2Bytes)+有效数据(双字节对齐)+CRC(2Bytes)
                cfg_data->last_blk_len *= sizeof(uint16_t);
                cfg_data->frm_data_len = 2 + cfg_data->last_blk_len;  // last_blk_len包含CRC
            }
            else
            {
                errcode = ERR_LAST_BLK_LEN;
            }
        }
        else if (cfg_data->blk_num >= 2 && cfg_data->blk_num <= 5)
        {
            cfg_data->frm_data_len = 2 + 18 * cfg_data->blk_num;
            cfg_data->last_blk_len = 18;  // 若为标准ft3,该值由底层填写,应用可忽略
        }
        else
        {
            errcode = ERR_BLK_NUM;
        }
        data = (uint32_t *)cfg_data;
        check_sum = 0;
        for (int i = 0; i < sizeof(*cfg_data) / 4 - 1; ++i)
        {
            check_sum += data[i];
        }
        cfg_data->check_sum = check_sum;

        uint32_t index = 0x01 << ch_num;
        if (is_tx)
        {
            memcpy(&ft3_tx_cfg_all[slot_num][ch_num].cfg_data, cfg_data, sizeof(*cfg_data));
            ft3_tx_cfg_all[slot_num][ch_num].cfg_state = errcode;
            ft3_tx_ctrl[slot_num].ft3_ch_bit_map |= index;

            // record information for debug
            ft3_tx_ctrl[slot_num].port_num++;
            ft3_tx_ctrl[slot_num].last_blk_len[ch_num] = ft3_tx_cfg_all[slot_num][ch_num].cfg_data.last_blk_len;
            uint16_t tx_length = CAR_HEADER_LEN + CHECK_SUM_LEN +
                                 ft3_tx_cfg_all[slot_num][ch_num].cfg_data.frm_data_len;
            uint16_t tx_diff = tx_length % ALIGNED_SIZE;
            if (tx_diff != 0)
            {
                tx_diff = ALIGNED_SIZE - tx_diff;
                tx_length += tx_diff;
            }
            ft3_tx_ctrl[slot_num].diff_car_len[ch_num] = tx_diff;
            ft3_tx_ctrl[slot_num].car_len[ch_num] = tx_length;
            ft3_tx_ctrl[slot_num].payload_len[ch_num] = ft3_tx_cfg_all[slot_num][ch_num].cfg_data.frm_data_len;
        }
        else
        {
            memcpy(&ft3_rx_cfg_all[slot_num][ch_num].cfg_data, cfg_data, sizeof(*cfg_data));
            ft3_rx_cfg_all[slot_num][ch_num].cfg_state = errcode;
            ft3_rx_ctrl[slot_num].ft3_ch_bit_map |= index;

            // record information for debug
            ft3_rx_ctrl[slot_num].port_num++;
            ft3_rx_ctrl[slot_num].port_id[ch_num] = 1;
            ft3_rx_ctrl[slot_num].last_blk_len[ch_num] = ft3_rx_cfg_all[slot_num][ch_num].cfg_data.last_blk_len;
            uint16_t rx_length = CAR_HEADER_LEN + 4 + ft3_rx_cfg_all[slot_num][ch_num].cfg_data.frm_data_len;
            uint16_t rx_diff = rx_length % ALIGNED_SIZE;
            if (rx_diff != 0)
            {
                rx_diff = ALIGNED_SIZE - rx_diff;
                rx_length += rx_diff;
            }
            ft3_rx_ctrl[slot_num].car_len[ch_num] = rx_length;
            ft3_rx_ctrl[slot_num].payload_len[ch_num] = ft3_rx_cfg_all[slot_num][ch_num].cfg_data.frm_data_len;
        }
    }
}

/**
 * @brief get ft3 cfg
 * @param [out] cfg_data
 * @param slot_num  slot number(0~3）
 * @param sfp_num   sfp port number(0~15)
 * @param port_num  port number of single sfp port (0 or 1),
 *                  For the single receiver single transmitter module, 
 *                  port 1 is the receiving port and port 0 is the sending port
 * @param is_tx true: get tx cfg; false:get rx cfg
 * @return error code
 */
uint8_t ft3_get_cfg(ft3_cfg_t *cfg_data,
                    uint8_t slot_num,
                    uint8_t sfp_num,
                    uint8_t port_num,
                    uint8_t is_tx)
{
    uint8_t errcode = 0;
    uint32_t ch_num = (sfp_num * 2) + port_num;

    if (sfp_num >= 0 && sfp_num <= 15 && port_num <= 1)
    {
        if (is_tx)
        {
            memcpy(cfg_data, &ft3_tx_cfg_all[slot_num][ch_num].cfg_data_bak, sizeof(*cfg_data));
            errcode = ft3_tx_cfg_all[slot_num][ch_num].cfg_state;
        }
        else
        {
            memcpy(cfg_data, &ft3_rx_cfg_all[slot_num][ch_num].cfg_data_bak, sizeof(*cfg_data));
            errcode = ft3_rx_cfg_all[slot_num][ch_num].cfg_state;
        }
    }
    else
    {
        errcode = ERR_CH_NUM;
    }
    return errcode;
}
#else
/**
 * @brief set ft3 cfg
 * @param cfg_data  pointer to cfg data
 * @param slot_id  slot number(0~3）
 * @param sfp_id   sfp port number(0~15)
 * @param port_id  port number of single sfp port (0 or 1),
 *                  For the single receiver single transmitter module,
 *                  port 1 is the receiving port and port 0 is the sending port
 * @param is_tx     true: get tx cfg; false:get rx cfg
 */
void ft3_set_cfg(ft3_cfg_t *cfg_data,
                 uint8_t slot_id,
                 uint8_t sfp_id,
                 uint8_t port_id,
                 uint8_t is_tx)
{
    uint32_t *data; /* for calc check sum */
    uint32_t check_sum;
    uint8_t errcode = CFG_VALID;
    uint32_t ch_num = 0;
#ifdef BUILD_C7X_1
    ft3_cfg_chk_t (*g_ft3_tx_cfg_all)[FT3_TX_CH_NUM] = ft3_tx_cfg_all;
    ft3_cfg_chk_t (*g_ft3_rx_cfg_all)[FT3_RX_CH_NUM] = ft3_rx_cfg_all;
#else
    ft3_cfg_chk_t (*g_ft3_tx_cfg_all)[FT3_TX_CH_NUM] = ft3_tx_cfg_all_prv;
    ft3_cfg_chk_t (*g_ft3_rx_cfg_all)[FT3_RX_CH_NUM] = ft3_rx_cfg_all_prv;
#endif

    /* attention ch_num, csa csb,csd */
    ch_num = (sfp_id * 2) + port_id;
    if (slot_id > 3 || port_id > 1)
        return;
    if ((slot_id == 0 && sfp_id <= 3) ||
        (slot_id == 1 && sfp_id <= 15) ||
        (slot_id == 2 && sfp_id <= 15) ||
        (slot_id == 3 && sfp_id <= 15))
    {
        cfg_data->train_rx_id = 1;
        if (cfg_data->blk_num == 1)  // 可能为非标准FT3帧,且数据长度双字节对齐
        {
            // TODO 对应用进行限制:last_blk_len单位为双字节
            if (cfg_data->last_blk_len >= 1 && cfg_data->last_blk_len <= 45u)
            {
                // ft3帧头(2Bytes)+有效数据(双字节对齐)+CRC(2Bytes)
                cfg_data->last_blk_len *= sizeof(uint16_t);
                cfg_data->frm_data_len = 2 + cfg_data->last_blk_len;  // last_blk_len包含CRC
            }
            else
            {
                errcode = ERR_LAST_BLK_LEN;
            }
        }
        else if (cfg_data->blk_num >= 2 && cfg_data->blk_num <= 5)
        {
            cfg_data->frm_data_len = 2 + (18 * cfg_data->blk_num);
            cfg_data->last_blk_len = 18;  // 若为标准ft3,该值由底层填写,应用可忽略
        }
        else
        {
            errcode = ERR_BLK_NUM;
        }
        cfg_data->monitor_mode = 1;
        data = (uint32_t *)cfg_data;
        check_sum = 0;
        for (int i = 0; i < (sizeof(*cfg_data) / 4) - 1; ++i)
        {
            check_sum += data[i];
        }

        uint32_t index = 0x01 << ch_num;
        if (is_tx)
        {
            memcpy(&g_ft3_tx_cfg_all[slot_id][ch_num].cfg_data, cfg_data, sizeof(*cfg_data));
            g_ft3_tx_cfg_all[slot_id][ch_num].cfg_state = errcode;
            ft3_tx_ctrl[slot_id].ft3_ch_bit_map |= index;

            // record information for debug
            ft3_tx_ctrl[slot_id].port_num++;
            ft3_tx_ctrl[slot_id].last_blk_len[ch_num] = g_ft3_tx_cfg_all[slot_id][ch_num].cfg_data.last_blk_len;
            uint16_t tx_length = CAR_HEADER_LEN + CHECK_SUM_LEN +
                                 g_ft3_tx_cfg_all[slot_id][ch_num].cfg_data.frm_data_len;
            uint16_t tx_diff = tx_length % ALIGNED_SIZE;
            if (tx_diff != 0)
            {
                tx_diff = ALIGNED_SIZE - tx_diff;
                tx_length += tx_diff;
            }
            ft3_tx_ctrl[slot_id].diff_car_len[ch_num] = tx_diff;
            ft3_tx_ctrl[slot_id].car_len[ch_num] = tx_length;
            ft3_tx_ctrl[slot_id].payload_len[ch_num] = g_ft3_tx_cfg_all[slot_id][ch_num].cfg_data.frm_data_len;
        }
        else
        {
            memcpy(&g_ft3_rx_cfg_all[slot_id][ch_num].cfg_data, cfg_data, sizeof(*cfg_data));
            g_ft3_rx_cfg_all[slot_id][ch_num].cfg_state = errcode;
            ft3_rx_ctrl[slot_id].ft3_ch_bit_map |= index;

            // record information for debug
            ft3_rx_ctrl[slot_id].port_num++;
            ft3_rx_ctrl[slot_id].port_id[ch_num] = 1;
            ft3_rx_ctrl[slot_id].last_blk_len[ch_num] = g_ft3_rx_cfg_all[slot_id][ch_num].cfg_data.last_blk_len;
            uint16_t rx_length = CAR_HEADER_LEN + 4 + g_ft3_rx_cfg_all[slot_id][ch_num].cfg_data.frm_data_len;
            uint16_t rx_diff = rx_length % ALIGNED_SIZE;
            if (rx_diff != 0)
            {
                rx_diff = ALIGNED_SIZE - rx_diff;
                rx_length += rx_diff;
            }
            ft3_rx_ctrl[slot_id].car_len[ch_num] = rx_length;
            ft3_rx_ctrl[slot_id].payload_len[ch_num] = g_ft3_rx_cfg_all[slot_id][ch_num].cfg_data.frm_data_len;
        }
    }
}

/**
 * @brief get ft3 cfg
 * @param [out] cfg_data
 * @param slot_id  slot number(0~3）
 * @param sfp_id   sfp port number(0~15)
 * @param port_id  port number of single sfp port (0 or 1),
 *                  For the single receiver single transmitter module,
 *                  port 1 is the receiving port and port 0 is the sending port
 * @param is_tx true: get tx cfg; false:get rx cfg
 * @return error code
 */
uint8_t ft3_get_cfg(ft3_cfg_t *cfg_data,
                    uint8_t slot_id,
                    uint8_t sfp_id,
                    uint8_t port_id,
                    uint8_t is_tx)
{
    uint8_t errcode = 0;
    uint32_t ch_num = (sfp_id * 2) + port_id;

    if (((slot_id == 0 && sfp_id <= 3) ||
        (slot_id == 1 && sfp_id <= 15) ||
        (slot_id == 2 && sfp_id <= 15) ||
        (slot_id == 3 && sfp_id <= 15)) && (port_id <= 1))
    {
        if (is_tx)
        {
            CacheP_Inv(&ft3_tx_cfg_all[slot_id][ch_num].cfg_data_bak, sizeof(*cfg_data));
            memcpy(cfg_data, &ft3_tx_cfg_all[slot_id][ch_num].cfg_data_bak, sizeof(*cfg_data));
            errcode = ft3_tx_cfg_all[slot_id][ch_num].cfg_state;
        }
        else
        {
            CacheP_Inv(&ft3_rx_cfg_all[slot_id][ch_num].cfg_data_bak, sizeof(*cfg_data));
            memcpy(cfg_data, &ft3_rx_cfg_all[slot_id][ch_num].cfg_data_bak, sizeof(*cfg_data));
            errcode = ft3_rx_cfg_all[slot_id][ch_num].cfg_state;
        }
    }
    else
    {
        errcode = ERR_CH_NUM;
    }
    return errcode;
}


#endif
/**
 * @brief send std ft3 frame (one block frame)
 * @param slot_id  slot number(0~3）
 * @param sfp_id   sfp port number(0~15)
 * @param port_num  port number of single sfp port (0 or 1),
 *                  For the single receiver single transmitter module, 
 *                  port 1 is the receiving port and port 0 is the sending port
 * @param blk_num   the number of block
 * @param data1~data8  the data of ft3 frame
 */
void ft3_send_data(uint8_t slot_id,
                   uint8_t sfp_id,
                   uint8_t port_num,
                   uint8_t blk_num,
                   uint16_t data1,
                   uint16_t data2,
                   uint16_t data3,
                   uint16_t data4,
                   uint16_t data5,
                   uint16_t data6,
                   uint16_t data7,
                   uint16_t data8)
{
    if (slot_id < FT3_SOLT_NUM &&
        (sfp_id >= 0 && sfp_id <= 15) &&
        port_num <= 1 &&
        blk_num < FT3_MAX_BLK_NUM)
    {
        uint32_t ch_num = (sfp_id * 2) + port_num;
        if (((1 << ch_num) & ft3_tx_ctrl[slot_id].ft3_ch_bit_map))
        {
            // 底层填入ft3帧头,应用只需要填ft3数据
            // 指针偏移到ft3有效数据(ft3帧头占2字节)
            ft3_blk_data_t *p_blk_data = (ft3_blk_data_t *)(pcie_ft3_data.payload[slot_id][ch_num] + 2u);
            p_blk_data->blk_data[blk_num].data[0] = data1;
            p_blk_data->blk_data[blk_num].data[1] = data2;
            p_blk_data->blk_data[blk_num].data[2] = data3;
            p_blk_data->blk_data[blk_num].data[3] = data4;
            p_blk_data->blk_data[blk_num].data[4] = data5;
            p_blk_data->blk_data[blk_num].data[5] = data6;
            p_blk_data->blk_data[blk_num].data[6] = data7;
            p_blk_data->blk_data[blk_num].data[7] = data8;
            p_blk_data->blk_data[blk_num].crc = 0;  // 填0,crc由fpga计算
        }
    }
}

/**
 * @brief sending non-standard FT3 frames
 * @param slot_id  slot number(0~3）
 * @param sfp_id   sfp port number(0~15)
 * @param port_num  port number of single sfp port (0 or 1),
 *                  For the single receiver single transmitter module, 
 *                  port 1 is the receiving port and port 0 is the sending port
 * @param ext_data_len
 * @return pointer to the sending area
 */
uint16_t *extend_ft3_send_data(uint8_t slot_id,
                               uint8_t sfp_id,
                               uint8_t port_num,
                               uint8_t *ext_data_len)
{
    if (slot_id < FT3_SOLT_NUM &&
        (sfp_id >= 0 && sfp_id <= 15) &&
        port_num <= 1)
    {
        uint32_t ch_num = (sfp_id * 2) + port_num;
        if (((1 << ch_num) & ft3_tx_ctrl[slot_id].ft3_ch_bit_map))
        {
            if ((ft3_tx_ctrl[slot_id].last_blk_len[ch_num] != STD_FT3_BLK_LEN) &&
                (pcie_ft3_data.payload[slot_id][ch_num] != NULL))
            {
                // 指针偏移到ft3有效数据(ft3帧头占2字节)
                *ext_data_len = (ft3_tx_ctrl[slot_id].last_blk_len[ch_num] / 2) - 1;
                return (uint16_t *)(pcie_ft3_data.payload[slot_id][ch_num] + 2);
            }
        }
    }
    *ext_data_len = 0;
    return NULL;
}

void ft3_build_tx_car(train_tx_t *tx_cfg, uint8_t slot_num, pcie_ft3_data_t *pPcieCmdPtr)
{
    car_tx_t *s_car = NULL;
    if (slot_num < FT3_SOLT_NUM)
    {
        for (int ch_num = 0; ch_num < FT3_TX_CH_NUM; ++ch_num)
        {
            if (((1 << ch_num) & ft3_tx_ctrl[slot_num].ft3_ch_bit_map))
            {
                uint16_t ft3_car_len = ft3_tx_ctrl[slot_num].car_len[ch_num];
                uint16_t ft3_payload_len = ft3_tx_ctrl[slot_num].payload_len[ch_num];
                int car_index = train_tx_get_buff(tx_cfg, CAR_TYPE_S, &s_car);
                if (car_index < 0 || s_car == NULL)
                {
                    pPcieCmdPtr->payload[slot_num][ch_num] = NULL;
                    continue;
                }
                
                pPcieCmdPtr->port_frame_cnt[slot_num][ch_num]++;

                s_car->fream_head = CPU_TO_FPGA;
                s_car->port_frame_cnt = pPcieCmdPtr->port_frame_cnt[slot_num][ch_num];
                s_car->msg_type = FRAME_TYPE_FT3;

                if (slot_num == 0 || slot_num == 1)
                    s_car->irq_num = 7;
                else if (slot_num == 2)
                    s_car->irq_num = 5;
                else
                    s_car->irq_num = 4;

                s_car->send_mode1 = 1;
                s_car->send_mode2 = 1;
                s_car->payload_len = ft3_payload_len;
                if (slot_num == 0)
                    s_car->solt0_port_bit_map = (1 << ch_num) & ft3_tx_ctrl[slot_num].ft3_ch_bit_map;
                else if (slot_num == 1)
                    s_car->solt1_port_bit_map = (1 << ch_num) & ft3_tx_ctrl[slot_num].ft3_ch_bit_map;
                else if (slot_num == 2)
                    s_car->solt2_port_bit_map = (1 << ch_num) & ft3_tx_ctrl[slot_num].ft3_ch_bit_map;
                else if (slot_num == 3)
                    s_car->solt3_port_bit_map = (1 << ch_num) & ft3_tx_ctrl[slot_num].ft3_ch_bit_map;

                *((uint16_t *)&(s_car->payload)) = FT3_FRAME_HEAD;

                pPcieCmdPtr->payload[slot_num][ch_num] = &s_car->payload;
                tx_cfg->car_len_s[car_index] = ft3_car_len;

                uint32_t check_sum = 0;
                uint16_t length = CAR_HEADER_LEN / 4;
                uint32_t *sum_data = (uint32_t *)s_car;
                for (int k = 0; k < length; ++k)
                {
                    check_sum += sum_data[k];
                }
                pPcieCmdPtr->car_head_sum[slot_num][ch_num] = check_sum;
            }
        }
    }
}

void ft3_recv_handler(train_rx_t *train_rx, uint8_t src_slot, car_rx_t *car)
{
    uint8_t src_port = car->src_port;
    uint16_t ft3_frm_len = car->payload_len;

    if (src_slot < 4 && src_port < 32)
    {
        if (((1 << src_port) & ft3_rx_ctrl[src_slot].ft3_ch_bit_map) &&
            (ft3_frm_len == ft3_rx_ctrl[src_slot].payload_len[src_port]))
        {
            uint16_t ft3_head = *((uint16_t *)&car->payload);
            if (ft3_head == FT3_FRAME_HEAD)
            {
                if (car->port_frame_cnt != (uint16_t)(train_rx->pre_ft3_frame_cnt[src_slot][src_port] + 1))
                {
                    pcie_ft3_fbk.payload[src_slot][src_port] = &car->payload;
                    pcie_ft3_fbk.diag_info[src_slot][src_port].lost_frm_cnt++;
                    pcie_ft3_fbk.diag_info[src_slot][src_port].time_tag = car->time_cnt;
                    pcie_ft3_fbk.diag_info[src_slot][src_port].data_valid = 0;
                }
                else
                {
                    pcie_ft3_fbk.payload[src_slot][src_port] = &car->payload;
                    pcie_ft3_fbk.diag_info[src_slot][src_port].time_tag = car->time_cnt;
                    pcie_ft3_fbk.diag_info[src_slot][src_port].data_valid = 1;
                    pcie_ft3_fbk.diag_info[src_slot][src_port].data_oks++;
                }
                train_rx->pre_ft3_frame_cnt[src_slot][src_port] = car->port_frame_cnt;
            }
            else
            {
                pcie_ft3_fbk.payload[src_slot][src_port] = &car->payload;
                pcie_ft3_fbk.diag_info[src_slot][src_port].time_tag = car->time_cnt;
                pcie_ft3_fbk.diag_info[src_slot][src_port].data_valid = 0;
                pcie_ft3_fbk.diag_info[src_slot][src_port].app_sum_errs++;
            }
        }
        else
        {
            pcie_ft3_fbk.payload[src_slot][src_port] = NULL;
            pcie_ft3_fbk.diag_info[src_slot][src_port].time_tag = car->time_cnt;
            pcie_ft3_fbk.diag_info[src_slot][src_port].data_valid = 0;
            pcie_ft3_fbk.diag_info[src_slot][src_port].app_sum_errs++;
        }
    }
}

void ft3_calc_checksum(pcie_ft3_data_t *pPcieCmdPtr, uint8_t slot_num)
{
    if (slot_num < FT3_SOLT_NUM)
    {
        for (int ch_num = 0; ch_num < FT3_TX_CH_NUM; ++ch_num)
        {
            if (((1 << ch_num) & ft3_tx_ctrl[slot_num].ft3_ch_bit_map))
            {
                if (pPcieCmdPtr->payload[slot_num][ch_num] == NULL)
                {
                    continue;
                }

                uint32_t check_sum = 0;
                uint32_t *sum_data = (uint32_t *)pPcieCmdPtr->payload[slot_num][ch_num];
                uint16_t length = ((ft3_tx_ctrl[slot_num].car_len[ch_num] - CAR_HEADER_LEN) / 4) - 1;

                /*check and clear*/
                uint8_t diff_len = ft3_tx_ctrl[slot_num].diff_car_len[ch_num];
                if (diff_len > 0)
                {
                    uint8_t *p_clr = pPcieCmdPtr->payload[slot_num][ch_num] + ft3_tx_ctrl[slot_num].payload_len[ch_num];
                    memset(p_clr, 0u, diff_len);
                }
                for (int k = 0; k < length; ++k)
                {
                    check_sum += sum_data[k];
                }

                sum_data[length] = pPcieCmdPtr->car_head_sum[slot_num][ch_num] + check_sum;
            }
        }
    }
}

/**
 * @brief recv standard FT3 frames
 * @param slot_id  slot number(0~3）
 * @param sfp_id   sfp port number(0~15)
 * @param port_id  port number of single sfp port (0 or 1),
 *                  For the single receiver single transmitter module, 
 *                  port 1 is the receiving port and port 0 is the sending port
 * @param blk_num   the number of block
 * @return pointer to the receive area
 */
uint16_t *ft3_recv_data(uint8_t slot_id, uint8_t sfp_id, uint8_t port_id, uint8_t blk_num)
{
    uint16_t *data = NULL;
    if (slot_id < FT3_SOLT_NUM &&
        (sfp_id >= 0 && sfp_id <= 15) && port_id <= 1 &&
        blk_num < FT3_MAX_BLK_NUM)
    {
        uint32_t ch_num = (sfp_id * 2) + port_id;
        if (((1 << ch_num) & ft3_rx_ctrl[slot_id].ft3_ch_bit_map))
        {
            if (pcie_ft3_fbk.payload[slot_id][ch_num] != NULL)
            {
                std_ft3_frm_t *ft3_frm_ptr = (std_ft3_frm_t *)pcie_ft3_fbk.payload[slot_id][ch_num];
                data = &ft3_frm_ptr->blk_data[blk_num].data[0];
            }
        }
    }
    return data;
}

/**
 * @brief Receive non-standard FT3 frames
 * @param slot_id  slot number(0~3）
 * @param sfp_id   sfp port number(0~15)
 * @param port_id  port number of single sfp port (0 or 1),
 *                  For the single receiver single transmitter module, 
 *                  port 1 is the receiving port and port 0 is the sending port
 * @param valid_num
 * @return
 */
uint16_t *extend_ft3_recv_data(uint8_t slot_id, uint8_t sfp_id, uint8_t port_id, uint8_t *valid_num)
{
    if (slot_id < FT3_SOLT_NUM &&
        (sfp_id >= 0 && sfp_id <= 15) && port_id <= 1)
    {
        uint32_t ch_num = (sfp_id * 2) + port_id;
        if ((1 << ch_num) & ft3_rx_ctrl[slot_id].ft3_ch_bit_map)
        {
            if ((ft3_rx_ctrl[slot_id].last_blk_len[ch_num] != STD_FT3_BLK_LEN) &&
                (pcie_ft3_fbk.payload[slot_id][ch_num] != NULL))
            {
                *valid_num = (ft3_rx_ctrl[slot_id].last_blk_len[ch_num] / 2) - 1;  // 双字节为单位,不传CRC
                ext_ft3_frm_t *ext_ft3_frm_ptr = (ext_ft3_frm_t *)pcie_ft3_fbk.payload[slot_id][ch_num];
                return &ext_ft3_frm_ptr->ext_data[0];
            }
        }
    }
    *valid_num = 0;
    return NULL;
}

void reset_ft3_diag_processing_flags(void)
{
    memset(pcie_ft3_fbk.diag_process_flag, 0, sizeof(pcie_ft3_fbk.diag_process_flag));
}

/**
 * @brief get FT3 diagnostic information
 * @param slot_id  slot number(0~3）
 * @param sfp_id   sfp port number(0~15)
 * @param port_id  port number of single sfp port (0 or 1),
 *                  For the single receiver single transmitter module, 
 *                  port 1 is the receiving port and port 0 is the sending port
 * @return a pointer to the information if successful, or null if failed
 */
diag_info_t *get_ft3_diag_info(uint8_t slot_id, uint8_t sfp_id, uint8_t port_id)
{
    if (slot_id < FT3_SOLT_NUM &&
        (sfp_id >= 0 && sfp_id <= 15) && port_id <= 1)
    {
        uint32_t ch_num = (sfp_id * 2) + port_id;
        if ((1 << ch_num) & ft3_rx_ctrl[slot_id].ft3_ch_bit_map)
        {
            diag_info_t *diag_info = &(pcie_ft3_fbk.diag_info[slot_id][ch_num]);
            if (!pcie_ft3_fbk.diag_process_flag[slot_id][ch_num])
            {
                if (diag_info->data_valid && diag_info->last_data_valid &&
                    (diag_info->last_time_tag == diag_info->time_tag))
                {
                    diag_info->data_valid = 0;
                    pcie_ft3_fbk.payload[slot_id][ch_num] = NULL;
                }

                diag_info->last_data_valid = diag_info->data_valid;
                diag_info->last_time_tag = diag_info->time_tag;
                pcie_ft3_fbk.diag_process_flag[slot_id][ch_num] = 1;
            }
            return diag_info;
        }
    }
    return NULL;
}
