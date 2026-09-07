/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       eth_data.c
 *@author     LiuRui
 *@date       2025.09.23
 *@brief      Gigabit Ethernet communication
 *@par        History
 *Date        Version   Author     Description
 *2025.09.23  1.0       LiuRui     first version
 *2025.12.19  1.1       LiuRui     refactor
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "g_eth_data.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
pcie_geth_fbk_t g_pcie_geth_fbk = {0};

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
void geth_send_data(uint8_t sfp_num, const uint16_t *app_data, eth_app_head_t *head)
{
//    eth_app_head_t app_head = {0};
//    uint16_t app_data[32]; // max len is 32
//
//    app_head.head = 0x05640564;
//    app_head.timetag = GETDWORD($TIME_TAG);
//    app_head.dat_num = GETWORD($APP_DAT_NUM);
//    app_head.frm_idx = GETWORD($FRM_INDX);
//
//    app_data[0]= GETWORD($X1);
//    app_data[1]= GETWORD($X2);
//    app_data[2]= GETWORD($X3);
//    app_data[3]= GETWORD($X4);
//    app_data[4]= GETWORD($X5);
//    app_data[5]= GETWORD($X6);
//    app_data[6]= GETWORD($X7);
//    app_data[7]= GETWORD($X8);
//    app_data[8]= GETWORD($X9);
//    app_data[9]= GETWORD($X10);
//    app_data[10]= GETWORD($X11);
//    app_data[11]= GETWORD($X12);
//    app_data[12]= GETWORD($X13);
//    app_data[13]= GETWORD($X14);
//    app_data[14]= GETWORD($X15);
//    app_data[15]= GETWORD($X16);
//    app_data[16]= GETWORD($X17);
//    app_data[17]= GETWORD($X18);
//    app_data[18]= GETWORD($X19);
//    app_data[19]= GETWORD($X20);
//    app_data[20]= GETWORD($X21);
//    app_data[21]= GETWORD($X22);
//    app_data[22]= GETWORD($X23);
//    app_data[23]= GETWORD($X24);
//    app_data[24]= GETWORD($X25);
//    app_data[25]= GETWORD($X26);
//    app_data[26]= GETWORD($X27);
//    app_data[27]= GETWORD($X28);
//    app_data[28]= GETWORD($X29);
//    app_data[29]= GETWORD($X30);
//    app_data[30]= GETWORD($X31);
//    app_data[31]= GETWORD($X32);
//
//    uint8_t sfp_num = GETBYTE($SFP_NR) - 1;
//    eth_app_head_t *head = &app_head;
//
//    if (sfp_num < 8)
//    {
//        /*dat_num must be a multiple of 8 */
//        if (head->dat_num % 8 == 0 &&
//            head->dat_num <= GETH_MAX_LEN &&
//            head->dat_num != 0)
//        {
//            uint8_t dest_port = sfp_num * 2;
//            train_tx_t *tx_cfg = &slot0_1_train_tx[1];
//            car_tx_t *car = NULL;
//            int car_index = train_tx_get_buff(tx_cfg, CAR_TYPE_B, &car);
//            if (car != NULL && car_index >= 0)
//            {
//                g_pcie_geth_fbk.send_frame_cnt[sfp_num]++;
//                car->payload_len = sizeof(eth_data_t) + (head->dat_num * sizeof(uint16_t)) + sizeof(eth_app_tail_t);
//                car->fream_head = CPU_TO_FPGA;
//                car->port_frame_cnt =  g_pcie_geth_fbk.send_frame_cnt[sfp_num];
//                car->irq_num = 7;
//                car->send_mode1 = 0x00;
//                car->send_mode2 = 0x01;
//                car->solt1_port_bit_map = 0x01 << dest_port;
//                car->msg_type = FRAME_TYPE_GETH;
//
//                eth_data_t *eth_data = (eth_data_t *)&car->payload;
//                eth_addr_t dst_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//                eth_addr_t src_mac = {0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
//
//                src_mac.mac_addr[0] += dest_port;
//                memcpy(eth_data->proto_head.dst_mac.mac_addr, dst_mac.mac_addr, sizeof(eth_addr_t));
//                memcpy(eth_data->proto_head.src_mac.mac_addr, src_mac.mac_addr, sizeof(eth_addr_t));
//                eth_data->proto_head.eth_type = 0xE008;
//                eth_data->proto_head.appid = 0x4000;
//                eth_data->proto_head.eth_overhead = 0x10;
//                eth_data->proto_head.comm_ver = 0x1;
//                memcpy(&eth_data->app_head, head, sizeof(eth_app_head_t));
//                memcpy(eth_data->app_data, app_data, head->dat_num * sizeof(uint16_t));
//
//                /*calc app check sum*/
//                uint32_t eth_check_sum = 0;
//                uint32_t *eth_sum_data = (&(eth_data->app_head.timetag));
//                for (int i = 0; i < (eth_data->app_head.dat_num / 2) + 2; ++i)
//                {
//                    eth_check_sum += eth_sum_data[i];
//                }
//                eth_check_sum = 0xffffffff - eth_check_sum;
//
//                uint8_t *app_data_ptr =  (uint8_t *)eth_data->app_data;
//                eth_app_tail_t *app_tail = (eth_app_tail_t *)&app_data_ptr[(eth_data->app_head.dat_num * 2)];
//                app_tail->check_sum = eth_check_sum;
//                app_tail->tail = GETH_FRAME_TAIL;
//
//                /*car length must be aligned 16 bytes, */
//                uint32_t *sum_data = (uint32_t *)car;
//                uint32_t check_sum = 0;
//                uint16_t length = CAR_HEADER_LEN + CHECK_SUM_LEN + car->payload_len;
//                uint16_t diff = length & (ALIGNED_SIZE - 1);
//                if (diff != 0)
//                {
//                    diff = ALIGNED_SIZE - diff;
//                }
//                length += diff;
//                tx_cfg->car_len_b[car_index] = length;
//
//                /*calc pcie data check sum*/
//                length = (length / 4) - 1;
//                for (int j = 0; j < length; ++j)
//                {
//                    check_sum += sum_data[j];
//                }
//                sum_data[length] = check_sum;
//            }
//        }
//    }
}

void geth_recv_handler(train_rx_t *train_rx, uint8_t src_slot, car_rx_t *car)
{
    uint8_t src_port = (car->src_port - 1) / 2;
    if (src_port < GETH_PORT_NUM)
    {
        g_pcie_geth_fbk.car[src_port] = car;
    }

//    //Odd channel numbers are for receiving
//    uint8_t src_port = ((GETBYTE($SFP_NR) - 1));
//    if (src_port < GETH_PORT_NUM)
//    {
//        geth_diag_info_t *diag_info = &(g_pcie_geth_fbk.diag_info[src_port]);
//
//        if (g_pcie_geth_fbk.car[src_port] != NULL)
//        {
//            car_rx_t *car = g_pcie_geth_fbk.car[src_port];
//
//            diag_info->comm_diag.time_tag = car->time_cnt;
//
//            eth_data_t *eth_data = (eth_data_t *)&car->payload;
//            if (eth_data->app_head.head == GETH_FRAME_HEAD)
//            {
//                if (eth_data->app_head.dat_num % 8 != 0 ||
//                    eth_data->app_head.dat_num > GETH_MAX_LEN ||
//                    eth_data->app_head.dat_num == 0)
//                {
//                    diag_info->dat_num_errs++;
//                    diag_info->comm_diag.lost_frm_cnt++;
//                    diag_info->comm_diag.data_valid = 0;
//                }
//                else
//                {
//                    uint8_t *app_data_ptr =  (uint8_t *)eth_data->app_data;
//                    eth_app_tail_t *app_tail = (eth_app_tail_t *)&app_data_ptr[(eth_data->app_head.dat_num * 2)];
//                    if (app_tail->tail != GETH_FRAME_TAIL)
//                    {
//                        diag_info->tail_errs++;
//                        diag_info->comm_diag.lost_frm_cnt++;
//                        diag_info->comm_diag.data_valid = 0;
//                    }
//                    else
//                    {
//                        uint32_t check_sum = 0;
//                        uint32_t *sum_data = (&(eth_data->app_head.timetag));
//                        for (int i = 0; i < (eth_data->app_head.dat_num / 2) + 2; ++i)
//                        {
//                            check_sum += sum_data[i];
//                        }
//                        check_sum = 0xffffffff - check_sum;
//                        if (check_sum != app_tail->check_sum)
//                        {
//                            diag_info->comm_diag.app_sum_errs++;
//                            diag_info->comm_diag.lost_frm_cnt++;
//                            diag_info->comm_diag.data_valid = 0;
//                        }
//                        else
//                        {
//                            memset(app_tail, 0, (GETH_MAX_LEN * 2) - (eth_data->app_head.dat_num * 2));
//                            diag_info->comm_diag.data_valid = 1;
//
//                            if (diag_info->comm_diag.data_valid && diag_info->comm_diag.last_data_valid &&
//                                (diag_info->comm_diag.last_time_tag == diag_info->comm_diag.time_tag))
//                            {
//                                diag_info->disconnected++;
//                                diag_info->comm_diag.lost_frm_cnt++;
//                                diag_info->comm_diag.data_valid = 0;
//                            }
//                            else
//                            {
//                                if (diag_info->comm_diag.last_data_valid &&
//                                    car->port_frame_cnt != (uint16_t)(g_pcie_geth_fbk.recv_frame_cnt[src_port] + 1))
//                                {
//                                    diag_info->comm_diag.lost_frm_cnt++;
//                                    diag_info->comm_diag.data_valid = 0;
//                                }
//                                else
//                                {
//                                    diag_info->comm_diag.data_oks++;
//
//                                    /*add fbg source code here*/
//                                    SETDWORD($TIME_TAG, eth_data->app_head.timetag);
//                                    SETWORD($APP_DAT_NUM, eth_data->app_head.dat_num);
//                                    SETWORD($FRM_INDX, eth_data->app_head.frm_idx);
//
//                                    SETWORD($X1,  eth_data->app_data[0]);
//                                    SETWORD($X2,  eth_data->app_data[1]);
//                                    SETWORD($X3,  eth_data->app_data[2]);
//                                    SETWORD($X4,  eth_data->app_data[3]);
//                                    SETWORD($X5,  eth_data->app_data[4]);
//                                    SETWORD($X6,  eth_data->app_data[5]);
//                                    SETWORD($X7,  eth_data->app_data[6]);
//                                    SETWORD($X8,  eth_data->app_data[7]);
//                                    SETWORD($X9,  eth_data->app_data[8]);
//                                    SETWORD($X10, eth_data->app_data[9]);
//                                    SETWORD($X11, eth_data->app_data[10]);
//                                    SETWORD($X12, eth_data->app_data[11]);
//                                    SETWORD($X13, eth_data->app_data[12]);
//                                    SETWORD($X14, eth_data->app_data[13]);
//                                    SETWORD($X15, eth_data->app_data[14]);
//                                    SETWORD($X16, eth_data->app_data[15]);
//                                    SETWORD($X17, eth_data->app_data[16]);
//                                    SETWORD($X18, eth_data->app_data[17]);
//                                    SETWORD($X19, eth_data->app_data[18]);
//                                    SETWORD($X20, eth_data->app_data[19]);
//                                    SETWORD($X21, eth_data->app_data[20]);
//                                    SETWORD($X22, eth_data->app_data[21]);
//                                    SETWORD($X23, eth_data->app_data[22]);
//                                    SETWORD($X24, eth_data->app_data[23]);
//                                    SETWORD($X25, eth_data->app_data[24]);
//                                    SETWORD($X26, eth_data->app_data[25]);
//                                    SETWORD($X27, eth_data->app_data[26]);
//                                    SETWORD($X28, eth_data->app_data[27]);
//                                    SETWORD($X29, eth_data->app_data[28]);
//                                    SETWORD($X30, eth_data->app_data[29]);
//                                    SETWORD($X31, eth_data->app_data[30]);
//                                    SETWORD($X32, eth_data->app_data[31]);
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//            else
//            {
//                diag_info->comm_diag.data_valid = 0;
//                diag_info->comm_diag.lost_frm_cnt++;
//            }
//            g_pcie_geth_fbk.recv_frame_cnt[src_port] = car->port_frame_cnt;
//            g_pcie_geth_fbk.car[src_port] = NULL;
//        }
//        else
//        {
//            diag_info->comm_diag.data_valid = 0;
//        }
//
//        SETDWORD($DTIMETAG, diag_info->comm_diag.time_tag);
//        SETBIT($DATAVALID, diag_info->comm_diag.data_valid);
//        SETDWORD($DATA_OKS, diag_info->comm_diag.data_oks);
//        SETDWORD($HEAD_ERRS, diag_info->comm_diag.head_sum_errs);
//        SETDWORD($CAR_ERRS, diag_info->comm_diag.car_sum_errs);
//        SETDWORD($APP_ERRS, diag_info->comm_diag.app_sum_errs);
//        SETDWORD($LOST_FRM, diag_info->comm_diag.lost_frm_cnt);
//
//        diag_info->comm_diag.last_data_valid = diag_info->comm_diag.data_valid;
//        diag_info->comm_diag.last_time_tag = diag_info->comm_diag.time_tag;
//    }
//    else
//    {
//        SETBIT($DATAVALID, 0);
//        SETWORD($APP_DAT_NUM, 0);
//
//        SETWORD($X1,  0);
//        SETWORD($X2,  0);
//        SETWORD($X3,  0);
//        SETWORD($X4,  0);
//        SETWORD($X5,  0);
//        SETWORD($X6,  0);
//        SETWORD($X7,  0);
//        SETWORD($X8,  0);
//        SETWORD($X9,  0);
//        SETWORD($X10, 0);
//        SETWORD($X11, 0);
//        SETWORD($X12, 0);
//        SETWORD($X13, 0);
//        SETWORD($X14, 0);
//        SETWORD($X15, 0);
//        SETWORD($X16, 0);
//        SETWORD($X17, 0);
//        SETWORD($X18, 0);
//        SETWORD($X19, 0);
//        SETWORD($X20, 0);
//        SETWORD($X21, 0);
//        SETWORD($X22, 0);
//        SETWORD($X23, 0);
//        SETWORD($X24, 0);
//        SETWORD($X25, 0);
//        SETWORD($X26, 0);
//        SETWORD($X27, 0);
//        SETWORD($X28, 0);
//        SETWORD($X29, 0);
//        SETWORD($X30, 0);
//        SETWORD($X31, 0);
//        SETWORD($X32, 0);
//    }

//    //Odd channel numbers are for receiving
//    uint8_t src_port = ((GETBYTE($SFP_NR) - 1));
//    if (src_port < GETH_PORT_NUM)
//    {
//        geth_diag_info_t *diag_info = &(g_pcie_geth_fbk.diag_info[src_port]);
//
//        if (g_pcie_geth_fbk.car[src_port] != NULL)
//        {
//            car_rx_t *car = g_pcie_geth_fbk.car[src_port];
//
//            diag_info->comm_diag.time_tag = car->time_cnt;
//
//            eth_data_2_t *eth_data = (eth_data_2_t *)&car->payload;
//            if ((car->eth_type == 0x88ec) &&
//                (eth_data->app_head.report_head == 0x0564))
//            {
//                if ((eth_data->app_head.tag_num < 1) || (eth_data->app_head.tag_num > 5))
//                {
//                    diag_info->dat_num_errs++;
//                    diag_info->comm_diag.app_sum_errs++;
//                    diag_info->comm_diag.lost_frm_cnt++;
//                    diag_info->comm_diag.data_valid = 0;
//                    g_pcie_geth_fbk.car[src_port] = NULL;
//                }
//                else
//                {
//                    yc_data_t *yc_data = (yc_data_t *)&eth_data->app_data;
//                    if ((yc_data->yc_tag != 0xA1 && yc_data->yc_tag != 0xA2))
//                    {
//                        diag_info->yc_tag_errs++;
//                        diag_info->comm_diag.lost_frm_cnt++;
//                        diag_info->comm_diag.data_valid = 0;
//                        g_pcie_geth_fbk.car[src_port] = NULL;
//                    }
//                    else
//                    {
//                        uint32_t check_sum = 0;
//                        uint32_t *sum_data = (uint32_t *)(&(eth_data->app_head));
//                        uint32_t sum_data_num = car->payload_len - sizeof(eth_protocol_t) - 4;;
//                        uint32_t temp_data = 0;
//                        if (0 != (sum_data_num % 4))
//                        {
//                            memcpy(&temp_data, (uint8_t *)&sum_data[sum_data_num / 4], sum_data_num % 4);
//                        }
//                        for (uint32_t i = 0; i < sum_data_num / 4; i++)
//                        {
//                            check_sum += sum_data[i];
//                        }
//                        check_sum += temp_data;
//                        if (check_sum != *((uint32_t *)((uint8_t *)eth_data + car->payload_len - 4)))
//                        {
//                            diag_info->comm_diag.app_sum_errs++;
//                            diag_info->comm_diag.lost_frm_cnt++;
//                            diag_info->comm_diag.data_valid = 0;
//                            g_pcie_geth_fbk.car[src_port] = NULL;
//                        }
//                        else
//                        {
//                            diag_info->comm_diag.data_valid = 1;
//                            if (diag_info->comm_diag.data_valid && diag_info->comm_diag.last_data_valid &&
//                                (diag_info->comm_diag.last_time_tag == diag_info->comm_diag.time_tag))
//                            {
//                                diag_info->disconnected++;
//                                diag_info->comm_diag.lost_frm_cnt++;
//                                diag_info->comm_diag.data_valid = 0;
//                                g_pcie_geth_fbk.car[src_port] = NULL;
//                            }
//                            else
//                            {
//                                if (diag_info->comm_diag.last_data_valid &&
//                                    car->port_frame_cnt != (uint16_t)(g_pcie_geth_fbk.recv_frame_cnt[src_port] + 5))
//                                {
//                                    diag_info->comm_diag.lost_frm_cnt++;
//                                    diag_info->comm_diag.data_valid = 0;
//                                    g_pcie_geth_fbk.car[src_port] = NULL;
//                                }
//                                else
//                                {
//                                    diag_info->comm_diag.data_oks++;
//
//                                    SETBYTE($APP_FRM_TYP, eth_data->app_head.app_frm_type);
//                                    SETBYTE($STA_WORD, eth_data->app_head.sta_word);
//                                    SETWORD($T_DLY_RATED, eth_data->app_head.delay_tine);
//                                    SETWORD($FRM_INDX, eth_data->app_head.frm_index);
//                                    SETBYTE($YC_TAG, yc_data->yc_tag);
//                                    SETBYTE($YC_DAT_NUM, yc_data->yc_data_num);
//                                    SETWORD($INDEX, yc_data->frm_indx);
//                                    SETDWORD($CH_VLD_0, yc_data->chan_vld[0]);
//                                    SETDWORD($CH_VLD_1, yc_data->chan_vld[1]);
//                                    SETDWORD($CH_VLD_2, yc_data->chan_vld[2]);
//                                    SETDWORD($CH_VLD_3, yc_data->chan_vld[3]);
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//            g_pcie_geth_fbk.recv_frame_cnt[src_port] = car->port_frame_cnt;
//        }
//
//        SETDWORD($DTIMETAG, diag_info->comm_diag.time_tag);
//        SETBIT($DATAVALID, diag_info->comm_diag.data_valid);
//        SETDWORD($DATA_OKS, diag_info->comm_diag.data_oks);
//        SETDWORD($HEAD_ERRS, diag_info->comm_diag.head_sum_errs);
//        SETDWORD($CAR_ERRS, diag_info->comm_diag.car_sum_errs);
//        SETDWORD($APP_ERRS, diag_info->comm_diag.app_sum_errs);
//        SETDWORD($LOST_FRM, diag_info->comm_diag.lost_frm_cnt);
//
//        diag_info->comm_diag.last_data_valid = diag_info->comm_diag.data_valid;
//        diag_info->comm_diag.last_time_tag = diag_info->comm_diag.time_tag;
//    }

//    SETBYTE($VLD_NUM, 0);
//    if (GETBYTE($SFP_NR) >= 1 && GETBYTE($SFP_NR) <= 8 &&
//        GETBYTE($BLK_NUM) >= 1 && GETBYTE($BLK_NUM) <= 16)
//    {
//        uint8_t src_port = ((GETBYTE($SFP_NR) - 1));
//        if (src_port < GETH_PORT_NUM)
//        {
//            if (g_pcie_geth_fbk.car[src_port] != NULL)
//            {
//                eth_data_2_t *eth_data = (eth_data_2_t *)&(g_pcie_geth_fbk.car[src_port]->payload);
//                yc_data_t *yc_data = (yc_data_t *)&eth_data->app_data;
//                uint8_t data_num = yc_data->yc_data_num;
//                uint32_t offset = (GETBYTE($BLK_NUM) - 1) * 8;
//
//                SETDINT($DATA1, yc_data->ana_data[offset + 0]);
//                SETDINT($DATA2, yc_data->ana_data[offset + 1]);
//                SETDINT($DATA3, yc_data->ana_data[offset + 2]);
//                SETDINT($DATA4, yc_data->ana_data[offset + 3]);
//                SETDINT($DATA5, yc_data->ana_data[offset + 4]);
//                SETDINT($DATA6, yc_data->ana_data[offset + 5]);
//                SETDINT($DATA7, yc_data->ana_data[offset + 6]);
//                SETDINT($DATA8, yc_data->ana_data[offset + 7]);
//
//                if (data_num == 0)
//                {
//                    SETBYTE($VLD_NUM, 0);
//                }
//                else if (GETBYTE($BLK_NUM) * 8 <= data_num)
//                {
//                    SETBYTE($VLD_NUM, 8);
//                }
//                else
//                {
//                    SETBYTE($VLD_NUM, data_num % 8);
//                }
//            }
//        }
//    }

}
