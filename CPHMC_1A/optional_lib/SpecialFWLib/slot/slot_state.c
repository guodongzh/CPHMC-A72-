/*
 * slot_status.c
 *
 *  Created on: 2026年7月31日
 *      Author: yaojiakang
 */

#include "slot_state.h"

uint8_t* slot_header(void)
{
    uint8_t* p = NULL;
#if defined(BUILD_C7X_1)
    p = &slot0_1_train_rx[0].header->resv1[0];
#elif defined(BUILD_C66X_1)
    p = &slot2_train_rx[0].header->resv1[0];
#elif defined(BUILD_MCU3_1)
    p = &slot3_train_rx[0].header->resv1[0];
#endif

    return p;
}
