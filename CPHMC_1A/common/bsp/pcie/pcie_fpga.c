/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pcie_train.c
 *@author     LiuRui
 *@date       2024.12.18
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2024.12.18  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include <string.h>
#include "pcie_fpga.h"
#include "debug_config.h"
#include "redun_switch.h"
#include <stdio.h>

#ifdef BUILD_MCU2_0
#include "enet_queue_common.h"
#endif

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

app_sys_param init_param;

// CPU->FPGA
train_tx_t slot0_1_train_tx[10] = {
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = 0,
        .car_num_s = CAR_NUM,
        .car_num_b = 0,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = 0,
        .car_size_b = CAR_LEN_B,
        .car_num_s = 0,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
};

train_tx_t slot3_train_tx[10] = {
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = 0,
        .car_num_s = CAR_NUM,
        .car_num_b = 0,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = 0,
        .car_size_b = CAR_LEN_B,
        .car_num_s = 0,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
};

// CPU->FPGA
train_tx_t slot2_train_tx[10] = {
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = 0,
        .car_num_s = CAR_NUM,
        .car_num_b = 0,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = CAR_LEN_B,
        .car_num_s = CAR_NUM,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = 0,
        .car_size_b = CAR_LEN_B,
        .car_num_s = 0,
        .car_num_b = CAR_NUM,
        .write_index_s = 0,
        .write_index_b = 0,
    },
};

// FPGA->CPU
train_rx_t slot0_1_train_rx[4] = {
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_S,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_S,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_B,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_B,
        .car_num = CAR_NUM,
    },
};

train_rx_t slot3_train_rx[4] = {
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_S,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_S,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_B,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_B,
        .car_num = CAR_NUM,
    },
};

// FPGA->CPU
train_rx_t slot2_train_rx[4] = {
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_S,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_S,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_B,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_B,
        .car_num = CAR_NUM,
    },
};

train_tx_t slot0_1_train_c66tx[1] = {
    {
        .header = NULL,
        .car_s = {NULL},
        .car_b = {NULL},
        .car_size_s = CAR_LEN_S,
        .car_size_b = 0,
        .car_num_s = CAR_NUM,
        .car_num_b = 0,
        .write_index_s = 0,
        .write_index_b = 0,
    },
};

train_rx_t slot0_1_train_c66rx[4] = {
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_S,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_S,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_B,
        .car_num = CAR_NUM,
    },
    {
        .header = NULL,
        .car0 = {NULL},
        .car1 = {NULL},
        .car_size = CAR_LEN_B,
        .car_num = CAR_NUM,
    },
};

#ifdef BUILD_MCU2_0
core_train_rx_t core_train_rx[8] = {
//    {.train_rx = &slot0_1_train_rx[1], .slot_num = 0}, // 双机冗余通信快速小列车
    {.train_rx = &slot0_1_train_rx[3], .slot_num = 0}, // B Code
    {.train_rx = &slot2_train_rx[3], .slot_num = 2},  // slot2 eth and B Code
    {.train_rx = &slot3_train_rx[3], .slot_num = 3},  // slot3 eth
    {.train_rx = NULL, .slot_num = 0},                // must be null
};

train_tx_t *core_train_tx[7] = {
//    &slot0_1_train_tx[2], // 双机冗余通信快速2号列车
    &slot0_1_train_tx[9],// B Code
    &slot2_train_tx[9],  // slot2 eth
    &slot3_train_tx[9],  // slot3 eth
    NULL,                // must be null
};

core_train_tx_ft3_t core_train_tx_ft3[5] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

core_train_tx_aurora_t core_train_tx_aurora[2] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

#elif defined(BUILD_MCU2_1)

core_train_rx_t core_train_rx[8] = {
    {.train_rx = NULL, .slot_num = 0},  // must be null
};

train_tx_t *core_train_tx[7] = {
    NULL,  // must be null
};

core_train_tx_ft3_t core_train_tx_ft3[5] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

core_train_tx_aurora_t core_train_tx_aurora[2] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

#elif defined(BUILD_MCU3_0)

core_train_rx_t core_train_rx[8] = {
    {.train_rx = NULL, .slot_num = 0},  // must be null
};

train_tx_t *core_train_tx[7] = {
    NULL,  // must be null
};

core_train_tx_ft3_t core_train_tx_ft3[5] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

core_train_tx_aurora_t core_train_tx_aurora[2] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

#elif defined(BUILD_MCU3_1)

core_train_rx_t core_train_rx[8] = {
    {.train_rx = &slot3_train_rx[0], .slot_num = 3},  // slot3 ft3
    {.train_rx = NULL, .slot_num = 0},                // must be null
};

train_tx_t *core_train_tx[7] = {
    &slot3_train_tx[0],  // slot3 ft3
    NULL,                // must be null
};

core_train_tx_ft3_t core_train_tx_ft3[5] = {
    {.train_tx = &slot3_train_tx[0], .slot_num = 3},  // slot3 ft3
    {.train_tx = NULL, .slot_num = 0},                // must be null
};

core_train_tx_aurora_t core_train_tx_aurora[2] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

#elif defined(BUILD_C66X_1)

core_train_rx_t core_train_rx[8] = {
    {.train_rx = &slot2_train_rx[0], .slot_num = 2},  // slot2 ft3
    {.train_rx = NULL, .slot_num = 0},  // must be null
};

core_train_rx_t core_train_c66rx[8] = {
    {.train_rx = &slot0_1_train_c66rx[0], .slot_num = 2},  // slot2 ft3
    {.train_rx = NULL, .slot_num = 0},  // must be null
};

train_tx_t *core_train_tx[7] = {
    &slot2_train_tx[0],  // slot2 ft3
    NULL,  // must be null
};

train_tx_t *core_train_c66tx[7] = {
    &slot0_1_train_c66tx[0],
    NULL,  // must be null
};

core_train_tx_ft3_t core_train_tx_ft3[5] = {
    {.train_tx = &slot0_1_train_c66tx[0], .slot_num = 2},  // slot2 ft3
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

core_train_tx_aurora_t core_train_tx_aurora[2] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

#elif defined(BUILD_C66X_2)

core_train_rx_t core_train_rx[8] = {
    {.train_rx = NULL, .slot_num = 0},  // must be null
};

core_train_rx_t core_train_c66rx[8] = {
    {.train_rx = NULL, .slot_num = 0},  // must be null
};

train_tx_t *core_train_tx[7] = {
    NULL,  // must be null
};

train_tx_t *core_train_c66tx[7] = {
    NULL,  // must be null
};

core_train_tx_ft3_t core_train_tx_ft3[5] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

core_train_tx_aurora_t core_train_tx_aurora[2] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

#elif defined(BUILD_C7X_1)
core_train_rx_t core_train_rx[8] = {
    {.train_rx = &slot0_1_train_rx[0], .slot_num = 1},  // slot1 ft3
    {.train_rx = &slot0_1_train_rx[2], .slot_num = 1},  // slot1 geth
    {.train_rx = NULL, .slot_num = 0},                  // must be null
};

train_tx_t *core_train_tx[7] = {
    &slot0_1_train_tx[1],  // slot1 geth
    &slot0_1_train_tx[0],  // slot0 squ
    NULL,                  // must be null
};

core_train_tx_ft3_t core_train_tx_ft3[5] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

core_train_tx_aurora_t core_train_tx_aurora[2] = {
//    {.train_tx = &slot0_1_train_tx[1], .slot_num = 1},  // slot1 aurora
    {.train_tx = NULL, .slot_num = 0},                  // must be null
};

#elif defined(BUILD_MCU1_1)

core_train_rx_t core_train_rx[8] = {
    {.train_rx = &slot0_1_train_rx[1], .slot_num = 0},// 双机冗余通信快速小列车
    {.train_rx = NULL, .slot_num = 0},  // must be null
};

train_tx_t *core_train_tx[7] = {
    &slot0_1_train_tx[2], // 双机冗余通信快速2号列车
    NULL,  // must be null
};

core_train_tx_ft3_t core_train_tx_ft3[5] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

core_train_tx_aurora_t core_train_tx_aurora[2] = {
    {.train_tx = NULL, .slot_num = 0},  // must be null
};

#endif

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * cfg interrupt period (us)
 * @note do not over 500us
 */
int train_irq_tim_cfg(sys_config_t *sys_config,
                      uint16_t t1_us,
                      uint16_t t2_us,
                      uint16_t t3_us,
                      uint16_t t4_us,
                      uint16_t t5_us,
                      uint16_t t6_us,
                      uint16_t t7_us,
                      uint16_t t8_us)
{
    if (sys_config == NULL)
        return -1;

    sys_config->ratio_irq.ratio_irq1 = 1000000 / t1_us;
    sys_config->ratio_irq.ratio_irq2 = 1000000 / t2_us;
    sys_config->ratio_irq.ratio_irq3 = 1000000 / t3_us;
    sys_config->ratio_irq.ratio_irq4 = 1000000 / t4_us;
    sys_config->ratio_irq.ratio_irq5 = 1000000 / t5_us;
    sys_config->ratio_irq.ratio_irq6 = 1000000 / t6_us;
    sys_config->ratio_irq.ratio_irq7 = 1000000 / t7_us;
    sys_config->ratio_irq.ratio_irq8 = 1000000 / t8_us;

    return 0;
}

/**
 * @brief  cfg cpu->fpga irq number
 */
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
                         uint8_t slow_irq)
{
    if (sys_config == NULL)
        return -1;

    sys_config->tx_car[0].irq_num = exp_irq;
    sys_config->tx_car[1].irq_num = fast1_irq;
    sys_config->tx_car[2].irq_num = fast2_irq;
    sys_config->tx_car[3].irq_num = fast3_irq;
    sys_config->tx_car[4].irq_num = fast4_irq;
    sys_config->tx_car[5].irq_num = fast5_irq;
    sys_config->tx_car[6].irq_num = fast6_irq;
    sys_config->tx_car[7].irq_num = fast7_irq;
    sys_config->tx_car[8].irq_num = fast8_irq;
    sys_config->tx_car[9].irq_num = slow_irq;

    return 0;
}

/**
 * @brief  FPGA starts reading data after xdma time after gpio interruption
 */
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
                        uint16_t time10)
{
    if (sys_config == NULL)
        return -1;

    sys_config->tx_car[0].time = time1 * 125;
    sys_config->tx_car[1].time = time2 * 125;
    sys_config->tx_car[2].time = time3 * 125;
    sys_config->tx_car[3].time = time4 * 125;
    sys_config->tx_car[4].time = time5 * 125;
    sys_config->tx_car[5].time = time6 * 125;
    sys_config->tx_car[6].time = time7 * 125;
    sys_config->tx_car[7].time = time8 * 125;
    sys_config->tx_car[8].time = time9 * 125;
    sys_config->tx_car[9].time = time10 * 125;

    return 0;
}

/**
 * @brief cfg fpga->cpu irq number
 */
int train_rx_irq_num_cfg(sys_config_t *sys_config,
                         uint8_t exp_irq,
                         uint8_t fast_irq_s,
                         uint8_t fast_irq_b,
                         uint8_t slow_irq)
{
    if (sys_config == NULL)
        return -1;

    sys_config->rx_car[0].irq_num = exp_irq;
    sys_config->rx_car[1].irq_num = fast_irq_s;
    sys_config->rx_car[2].irq_num = fast_irq_b;
    sys_config->rx_car[3].irq_num = slow_irq;

    return 0;
}

/**
 * @brief allocate CPU to FPGA buffer address
 * @param tx_car_info
 * @param train_tx
 * @param train_num
 * @param _ram_addr
 * @param _pcie_addr
 * @return end addr
 */
uint32_t train_tx_pcie_addr_init(train_car_info_t *tx_car_info,
                                 train_tx_t *train_tx,
                                 uint32_t train_num,
                                 uint32_t _pcie_addr)
{
    uint32_t pcie_addr = _pcie_addr;
    uint32_t scar_len, bcar_len, header_len;

    if (tx_car_info == NULL || train_tx == NULL)
        return 0;

    for (int i = 0; i < train_num; ++i)
    {
        scar_len = train_tx[i].car_num_s * train_tx[i].car_size_s;
        bcar_len = train_tx[i].car_num_b * train_tx[i].car_size_b;
        if (scar_len == 0 && bcar_len == 0)
            continue;

        /* header */
        tx_car_info[i].header_addr = pcie_addr;
        if (train_tx[i].car_num_s != 0)
        {
            header_len = 0;

            /* get scar addr*/
            tx_car_info[i].car_addr_s0 = pcie_addr + TRAIN_HEADER_LEN;
            tx_car_info[i].car_addr_s1 = pcie_addr + TRAIN_HEADER_LEN + scar_len;
            pcie_addr = pcie_addr + TRAIN_HEADER_LEN + (scar_len * 2);
        }
        else
        {
            header_len = TRAIN_HEADER_LEN;
            tx_car_info[i].car_addr_s0 = 0;
            tx_car_info[i].car_addr_s1 = 0;
        }

        if (train_tx[i].car_num_b != 0)
        {
            /* get bcar addr*/
            tx_car_info[i].car_addr_b0 = pcie_addr + header_len;
            tx_car_info[i].car_addr_b1 = pcie_addr + header_len + bcar_len;
            pcie_addr = pcie_addr + header_len + (bcar_len * 2);
        }
        else
        {
            tx_car_info[i].car_addr_b0 = 0;
            tx_car_info[i].car_addr_b1 = 0;
        }
    }
    return pcie_addr;
}

uint64_t train_tx_ram_addr_init(train_tx_t *train_tx, uint32_t train_num, uint64_t _ram_addr)
{
    uint64_t ram_addr = _ram_addr, base_addr1, base_addr2;
    uint32_t scar_len, bcar_len, header_len;

    if (train_tx == NULL)
        return 0;

    for (int i = 0; i < train_num; ++i)
    {
        scar_len = train_tx[i].car_num_s * train_tx[i].car_size_s;
        bcar_len = train_tx[i].car_num_b * train_tx[i].car_size_b;
        if (scar_len == 0 && bcar_len == 0)
            continue;

        /* header */
        train_tx[i].header = (train_header_tx_t *)ram_addr; /* ram addr */
        if (train_tx[i].car_num_s != 0)
        {
            header_len = 0;

            /* get ram addr*/
            base_addr1 = ram_addr + TRAIN_HEADER_LEN;
            base_addr2 = ram_addr + TRAIN_HEADER_LEN + scar_len;
            for (int j = 0; j < train_tx[i].car_num_s; ++j)
            {
                train_tx[i].car_s[0][j] = (car_tx_t *)(base_addr1 + (train_tx[i].car_size_s * j));
                train_tx[i].car_s[1][j] = (car_tx_t *)(base_addr2 + (train_tx[i].car_size_s * j));
            }
            /* update pcie base addr*/
            ram_addr = ram_addr + TRAIN_HEADER_LEN + (scar_len * 2);
        }
        else
        {
            header_len = TRAIN_HEADER_LEN;
        }

        if (train_tx[i].car_num_b != 0)
        {
            /* get ram addr*/
            base_addr1 = ram_addr + header_len;
            base_addr2 = ram_addr + header_len + bcar_len;
            for (int j = 0; j < train_tx[i].car_num_b; ++j)
            {
                train_tx[i].car_b[0][j] = (car_tx_t *)(base_addr1 + (train_tx[i].car_size_b * j));
                train_tx[i].car_b[1][j] = (car_tx_t *)(base_addr2 + (train_tx[i].car_size_b * j));
            }
            /* update pcie base addr*/
            ram_addr = ram_addr + header_len + (bcar_len * 2);
        }
    }
    return ram_addr;
}

/**
 * @brief allocate FPGA to CPU buffer address
 * @param rx_car
 * @param train_rx
 * @param train_num
 * @return end addr
 */
uint32_t train_rx_pcie_addr_init(train_car_info_t *rx_car,
                                 train_rx_t *train_rx,
                                 uint32_t train_num,
                                 uint32_t _pcie_addr)
{
    uint32_t pcie_addr = _pcie_addr;
    uint32_t car_len;

    if (rx_car == NULL || train_rx == NULL)
        return 0;

    for (int i = 0; i < train_num; ++i)
    {
        car_len = train_rx[i].car_size * train_rx[i].car_num;
        if (car_len == 0)
            continue;

        /* header */
        rx_car[i].header_addr = pcie_addr;
        if (train_rx[i].car_size == CAR_LEN_B)
        {
            rx_car[i].car_addr_s0 = 0;
            rx_car[i].car_addr_s1 = 0;
            rx_car[i].car_addr_b0 = pcie_addr + TRAIN_HEADER_LEN;
            rx_car[i].car_addr_b1 = pcie_addr + TRAIN_HEADER_LEN + car_len;
        }
        else
        {
            rx_car[i].car_addr_s0 = pcie_addr + TRAIN_HEADER_LEN;
            rx_car[i].car_addr_s1 = pcie_addr + TRAIN_HEADER_LEN + car_len;
            rx_car[i].car_addr_b0 = 0;
            rx_car[i].car_addr_b1 = 0;
        }
        /* update pcie base addr*/
        pcie_addr = pcie_addr + TRAIN_HEADER_LEN + (car_len * 2);
    }
    return pcie_addr;
}

uint32_t train_rx_ram_addr_init(train_rx_t *train_rx, uint32_t train_num, uint64_t _ram_addr)
{
    uint64_t ram_addr = _ram_addr, base_addr1, base_addr2;
    uint32_t car_len;

    if (train_rx == NULL)
        return 0;

    for (int i = 0; i < train_num; ++i)
    {
        car_len = train_rx[i].car_size * train_rx[i].car_num;
        if (car_len == 0)
            continue;

        /* header */
        train_rx[i].header = (train_header_rx_t *)ram_addr;
        base_addr1 = ram_addr + TRAIN_HEADER_LEN;
        base_addr2 = ram_addr + TRAIN_HEADER_LEN + car_len;
        for (int j = 0; j < train_rx[i].car_num; ++j)
        {
            train_rx[i].car0[j] = (car_rx_t *)(base_addr1 + (train_rx[i].car_size * j));
            train_rx[i].car1[j] = (car_rx_t *)(base_addr2 + (train_rx[i].car_size * j));
        }
        /* update ram base addr*/
        ram_addr = ram_addr + TRAIN_HEADER_LEN + (car_len * 2);
    }
    return ram_addr;
}


train_recv_handler train_recv_handler_tbl[FRAME_TYPE_AURORA + 1] = {0};
int train_reg_recv_handler(uint8_t frame_type, train_recv_handler handler)
{
    if (frame_type > FRAME_TYPE_AURORA)
        return -1;
    train_recv_handler_tbl[frame_type] = handler;
    return 0;
}


#ifdef BUILD_MCU2_0
int train_recv(train_rx_t *train_rx, uint8_t src_slot)
{
    car_rx_t **car = NULL;
    uint32_t *sum_data = NULL;
    uint32_t check_sum = 0;
    car_ctrl_t enet_ctrl = {0};
    if (train_rx == NULL)
        return -1;

    CacheP_Inv(train_rx->header, TRAIN_HEADER_LEN);
    sum_data = (uint32_t *)train_rx->header;
    for (int i = 0; i < 31; ++i)
    {
        check_sum += sum_data[i];
    }

    /*is sum_data ok?*/
    if (check_sum == train_rx->header->check_sum)
    {
        extern uint32_t g_rx_slow_train_header_vld;
        g_rx_slow_train_header_vld = 1;
        /*get base addr*/
        if (train_rx->header->car_id == 1)
        {
            car = train_rx->car0;
        }
        else
        {
            car = train_rx->car1;
        }
        if (train_rx->header->car_num >= CAR_NUM)
        {
            train_rx->header->car_num = CAR_NUM;
        }

        if (train_rx->header->car_num == 0)
        {
            train_rx->car_num_errs++;
        }
        if (train_rx->header->car_num > 0)
        {
            train_rx->car_num_ok_cnt++;
        }
        for (int i = 0; i < train_rx->header->car_num; ++i)
        {
            CacheP_Inv(car[i], CAR_HEADER_LEN);

            /*align 16 bytes*/
            sum_data = (uint32_t *)car[i];
            uint16_t len_pakg, len_sum, sum_index;
            if (car[i]->payload_len >= train_rx->car_size || car[i]->payload_len == 0)
            {
                train_rx->car_len_errs[i]++;
                continue;
            }
            else
            {
                len_pakg = car[i]->payload_len + CAR_HEADER_LEN + CHECK_SUM_LEN;
                uint16_t diff = len_pakg % ALIGNED_SIZE;
                if (diff != 0)
                {
                    diff = ALIGNED_SIZE - diff;
                }
                uint8_t *base = (uint8_t *)(car[i]);
                base += CAR_HEADER_LEN;
                len_sum = len_pakg + diff;
                CacheP_Inv(base, len_sum - CAR_HEADER_LEN);
                sum_index = (len_sum / 4) - 1;
            }

            /*calc check sum*/
            check_sum = 0;
            for (int j = 0; j < sum_index; ++j)
            {
                check_sum += sum_data[j];
            }

            /*is sum_data ok?*/
            if (check_sum == sum_data[sum_index])
            {
                if (car[i]->port_type == FRAME_TYPE_ETH)
                {
                    train_rx->rx_eth_fpga++;
                    enet_ctrl.car[enet_ctrl.frame_cnt] = (uint8_t *)car[i];
                    enet_ctrl.frame_cnt++;
                }
                #if defined(BUILD_MCU1_1)
                else if (car[i]->port_type == FRAME_TYPE_REDUNDANCY)
                {
                    redun_switch_recv_handler(train_rx, src_slot, car[i]);
                }
                #endif
            }
            else
            {
                train_rx->car_sum_errs[i]++;
            }
        }
        for (int i = 0; i < enet_ctrl.frame_cnt; ++i)
        {
            pcie_to_enet_rxque(enet_ctrl.car[i], i, enet_ctrl.frame_cnt);
        }
        return 0;
    }
    else
    {
        train_rx->header_sum_err++;
        return -1;
    }
}

#else
int train_recv(train_rx_t *train_rx, uint8_t src_slot)
{
    car_rx_t **car = NULL;
    uint32_t *sum_data = NULL;
    uint32_t check_sum = 0;
    if (train_rx == NULL)
        return -1;

    CacheP_Inv(train_rx->header, TRAIN_HEADER_LEN);
    sum_data = (uint32_t *)train_rx->header;
    for (int i = 0; i < 31; ++i)
    {
        check_sum += sum_data[i];
    }

    /*is sum_data ok?*/
    if (check_sum == train_rx->header->check_sum)
    {
        /*get base addr*/
        if (train_rx->header->car_id == 1)
        {
            car = train_rx->car0;
        }
        else
        {
            car = train_rx->car1;
        }
        if (train_rx->header->car_num >= CAR_NUM)
        {
            train_rx->header->car_num = CAR_NUM;
        }

        for (int i = 0; i < train_rx->header->car_num; ++i)
        {
            CacheP_Inv(car[i], train_rx->car_size);

            /*align 16 bytes*/
            sum_data = (uint32_t *)car[i];
            uint16_t length;
            if (car[i]->payload_len >= train_rx->car_size || car[i]->payload_len == 0)
            {
                train_rx->car_len_errs[i]++;
                continue;
            }
            else
            {
                length = CAR_HEADER_LEN + CHECK_SUM_LEN + car[i]->payload_len;
                uint16_t diff = length % ALIGNED_SIZE;
                if (diff != 0)
                {
                    diff = ALIGNED_SIZE - diff;
                }
                length = ((length + diff) / 4) - 1;
            }

            /*calc check sum*/
            check_sum = 0;
            for (int j = 0; j < length; ++j)
            {
                check_sum += sum_data[j];
            }

            /*is sum_data ok?*/
            if (check_sum == sum_data[length])
            {
                if (car[i]->port_type == FRAME_TYPE_FT3)
                {
                    train_rx->rx_ft3_fpga++;
                    if (train_recv_handler_tbl[FRAME_TYPE_FT3] != NULL)
                    {
                        train_recv_handler_tbl[FRAME_TYPE_FT3](train_rx, src_slot, car[i]);
                    }
                }
                else if (car[i]->port_type == FRAME_TYPE_GETH)
                {
                    train_rx->rx_geth_fpga++;
                    if (train_recv_handler_tbl[FRAME_TYPE_GETH] != NULL)
                    {
                        train_recv_handler_tbl[FRAME_TYPE_GETH](train_rx, src_slot, car[i]);
                    }
                }
                else if (car[i]->port_type == FRAME_TYPE_SFP)
                {
                }
                else if (car[i]->port_type == FRAME_TYPE_AURORA)
                {
                    train_rx->rx_aurora_fpga++;
                    if (train_recv_handler_tbl[FRAME_TYPE_AURORA] != NULL)
                    {
                        train_recv_handler_tbl[FRAME_TYPE_AURORA](train_rx, src_slot, car[i]);
                    }
                }
                else if (car[i]->port_type == FRAME_TYPE_REDUNDANCY)
                {
                    redun_switch_recv_handler(train_rx, src_slot, car[i]);
                }
            }
            else
            {
                train_rx->car_sum_errs[i]++;
            }
        }
        return 0;
    }
    else
    {
        train_rx->header_sum_err++;
        return -1;
    }
}
#endif

/**
 * @brief get a tx buff
 * @param train_tx a pointer of train_tx
 * @param car_type can be CAR_TYPE_S or CAR_TYPE_B
 * @param buff [out] pointer of current carriage buffer
 * @return The current carriage number, it can be used
 *          to update the length of carriage data
 */
int train_tx_get_buff(train_tx_t *train_tx, uint8_t car_type, car_tx_t **buff)
{
    int revt;
    if (train_tx == NULL)
        return -1;

    *buff = NULL;
    if (car_type == CAR_TYPE_S)
    {
        if (train_tx->write_index_s == train_tx->car_num_s)
            return -1;

        if (train_tx->header->car_id == 1)
            *buff = train_tx->car_s[1][train_tx->write_index_s];
        else
            *buff = train_tx->car_s[0][train_tx->write_index_s];
        revt = train_tx->write_index_s;
        train_tx->write_index_s++;
    }
    else
    {
        if (train_tx->write_index_b == train_tx->car_num_b)
            return -1;
        if (train_tx->header->car_id == 1)
            *buff = train_tx->car_b[1][train_tx->write_index_b];
        else
            *buff = train_tx->car_b[0][train_tx->write_index_b];
        revt = train_tx->write_index_b;
        train_tx->write_index_b++;
    }

    return revt;
}

/**
 * @brief update train_tx header info
 * @param train_tx a pointer of train_tx
 * @return 0: successful
 *        -1: failed
 */
int train_tx_update_header(train_tx_t *train_tx)
{
    int i;
    if (train_tx == NULL)
        return -1;
    if (train_tx->header == NULL)
        return -1;

    // set size
    train_tx->header->fream_head = CPU_TO_FPGA;
    train_tx->header->heart_beat_cnt += 1;
    train_tx->header->car_num_s = train_tx->write_index_s;
    train_tx->header->car_num_b = train_tx->write_index_b;
    train_tx->write_index_b = 0;
    train_tx->write_index_s = 0;

    // change car_id
    if (train_tx->header->car_id == 0 || train_tx->header->car_id == 2)
    {
        train_tx->header->car_id = 1;
    }
    else
    {
        train_tx->header->car_id = 2;
    }

    // set car len and CacheP_wb data
    memset(train_tx->header->car_len_s, 0, sizeof(train_tx->header->car_len_s));
    for (i = 0; i < train_tx->header->car_num_s; ++i)
    {
        train_tx->header->car_len_s[i] = train_tx->car_len_s[i];
        if (train_tx->header->car_id == 1)
            CacheP_wb(train_tx->car_s[0][i], train_tx->car_len_s[i]);
        else
            CacheP_wb(train_tx->car_s[1][i], train_tx->car_len_s[i]);
    }
    memset(train_tx->car_len_s, 0, sizeof(train_tx->car_len_s));

    // set car len and CacheP_wb data
    memset(train_tx->header->car_len_b, 0, sizeof(train_tx->header->car_len_b));
    for (i = 0; i < train_tx->header->car_num_b; ++i)
    {
        train_tx->header->car_len_b[i] = train_tx->car_len_b[i];
        if (train_tx->header->car_id == 1)
            CacheP_wb(train_tx->car_b[0][i], train_tx->car_len_b[i]);
        else
            CacheP_wb(train_tx->car_b[1][i], train_tx->car_len_b[i]);
    }
    memset(train_tx->car_len_b, 0, sizeof(train_tx->car_len_b));

    // calc sum
    uint32_t *sum_data = (uint32_t *)train_tx->header;
    uint32_t sum_check = 0;
    for (i = 0; i < (sizeof(train_header_tx_t) / 4) - 1; ++i)
    {
        sum_check += sum_data[i];
    }
    train_tx->header->check_sum = sum_check;

    // CacheP_wb
    CacheP_wb(train_tx->header, sizeof(train_header_tx_t));

    return 0;
}

void train_all_ram_addr_init()
{
    uint64_t end_addr;

#if USE_C66x_RAM
#if defined(BUILD_C7X)
    end_addr = train_tx_ram_addr_init(&slot0_1_train_tx[0], 1, PCIE0_IB1_RAM_ADDR);
    train_rx_ram_addr_init(&slot0_1_train_rx[0], 1, end_addr);
#endif
    end_addr = train_tx_ram_addr_init(&slot0_1_train_tx[1],
                                      ((sizeof(slot0_1_train_tx) / sizeof(train_tx_t))) - 1,
                                      (uint32_t)pcie0_ib_space);
    train_rx_ram_addr_init(&slot0_1_train_rx[1],
                           ((sizeof(slot0_1_train_rx) / sizeof(train_rx_t))) - 1,
                           end_addr);
#else
    end_addr = train_tx_ram_addr_init(&slot0_1_train_tx[0],
                                      (sizeof(slot0_1_train_tx) / sizeof(train_tx_t)),
                                      (uint32_t)pcie0_ib_space);
    train_rx_ram_addr_init(&slot0_1_train_rx[0],
                           (sizeof(slot0_1_train_rx) / sizeof(train_rx_t)),
                           end_addr);
#endif

    end_addr = train_tx_ram_addr_init(&slot2_train_tx[0],
                                      (sizeof(slot2_train_tx) / sizeof(train_tx_t)),
                                      (uint32_t)pcie3_ib_space);

    train_rx_ram_addr_init(&slot2_train_rx[0],
                           (sizeof(slot2_train_rx) / sizeof(train_rx_t)),
                           end_addr);

    end_addr = train_tx_ram_addr_init(&slot3_train_tx[0],
                                      (sizeof(slot3_train_tx) / sizeof(train_tx_t)),
                                      (uint32_t)pcie2_ib_space);

    train_rx_ram_addr_init(&slot3_train_rx[0],
                           (sizeof(slot3_train_rx) / sizeof(train_rx_t)),
                           end_addr);
}

void slot_fpga_init_comm()
{
    init_param.core0_main_intr_time = 200;
    init_param.core1_main_intr_time = 200;
    init_param.core2_main_intr_time = 1000;
    init_param.core3_main_intr_time = 1000;
    init_param.core4_main_intr_time = 100;
    init_param.core5_main_intr_time = 100;
    init_param.core6_main_intr_time = 50;    //41:24KHZ
    init_param.core7_main_intr_time = 100;

    init_param.core0_swi_time_interval = 1000 / init_param.core0_main_intr_time;
    init_param.core1_swi_time_interval = 1000 / init_param.core1_main_intr_time;
    init_param.core2_swi_time_interval = 1000 / init_param.core2_main_intr_time;
    init_param.core3_swi_time_interval = 1000 / init_param.core3_main_intr_time;
    init_param.core4_swi_time_interval = 1000 / init_param.core4_main_intr_time;
    init_param.core5_swi_time_interval = 1000 / init_param.core5_main_intr_time;
    init_param.core6_swi_time_interval = 1000 / init_param.core6_main_intr_time;
    init_param.core7_swi_time_interval = 1000 / init_param.core7_main_intr_time;
}
