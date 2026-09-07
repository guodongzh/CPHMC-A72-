/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       i2c_tmp75q1.h
*@author     xqb
*@date       2024.07.09
*@brief      i2c read tmp75q1 test source.h file
*@par        History
*Date        Version   Author     Description
2024.07.09   1.0       xqb        example
******************************************************************************/
#ifndef I2C_TMP75Q1_I2C_TMP75Q1_H_
#define I2C_TMP75Q1_I2C_TMP75Q1_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <ti/osal/osal.h>
#include <ti/drv/i2c/I2C.h>
#include <ti/drv/i2c/soc/I2C_soc.h>
#include <ti/csl/soc.h>
#include "board/board.h"
#include "debug_config.h"

/* TMP75Q1(I2C to Temperature Sensor */
#define I2C_TEST_INSTANCE           (2U)

#define TMP75Q1_I2C2_ADDR           (0x48)

/* Temperature result registers */
#define TMP75Q1_RESULT_REG          (0x0000U)
/* Temperature config registers */
#define TMP75Q1_CONFIG_REG          (0x0001U)

#define TMP75Q1_CFG_REG_SD_MASK     (0x01)
#define TMP75Q1_CFG_REG_SD_SHIFT    (0U)

#define TMP75Q1_CFG_REG_TM_MASK     (0x01)
#define TMP75Q1_CFG_REG_TM_SHIFT    (1U)

#define TMP75Q1_CFG_REG_POL_MASK    (0x01)
#define TMP75Q1_CFG_REG_POL_SHIFT   (2U)

#define TMP75Q1_CFG_REG_FQ_MASK     (0x03)
#define TMP75Q1_CFG_REG_FQ_SHIFT    (3U)

#define TMP75Q1_CFG_REG_CR_MASK     (0x03)
#define TMP75Q1_CFG_REG_CR_SHIFT    (5U)

#define TMP75Q1_CFG_REG_OS_MASK     (0x01)
#define TMP75Q1_CFG_REG_OS_SHIFT    (7U)

#define TMP75Q1_SD_ENABLE           (1U)
#define TMP75Q1_SD_DISABLE_DFT      (0U)

#define TMP75Q1_TM_ENABLE           (1U)
#define TMP75Q1_TM_DISABLE_DFT      (0U)

#define TMP75Q1_POL_HIGH            (1U)
#define TMP75Q1_POL_LOW_DFT         (0U)

#define TMP75Q1_FQ_ZERO_DFT         (0U)
#define TMP75Q1_FQ_ONE              (1U)
#define TMP75Q1_FQ_TWO              (2U)
#define TMP75Q1_FQ_THREE            (3U)

#define TMP75Q1_CR_ZERO_DFT         (0U)
#define TMP75Q1_CR_ONE              (1U)
#define TMP75Q1_CR_TWO              (2U)
#define TMP75Q1_CR_THREE            (3U)

#define TMP75Q1_OS_ENABLE           (1U)
#define TMP75Q1_OS_DISABLE_DFT      (0U)

#define TMP75Q1_CFG_REG_CONFIG(SD, TM, POL, FQ, CR, OS) \
                              ((((SD) & TMP75Q1_CFG_REG_SD_MASK) << TMP75Q1_CFG_REG_SD_SHIFT) | \
                              (((TM) & TMP75Q1_CFG_REG_TM_MASK) << TMP75Q1_CFG_REG_TM_SHIFT) | \
                              (((POL) & TMP75Q1_CFG_REG_POL_MASK) << TMP75Q1_CFG_REG_POL_SHIFT) | \
                              (((FQ) &TMP75Q1_CFG_REG_FQ_MASK) << TMP75Q1_CFG_REG_FQ_SHIFT) | \
                              (((CR) &TMP75Q1_CFG_REG_CR_MASK) << TMP75Q1_CFG_REG_CR_SHIFT) | \
                              (((OS) & TMP75Q1_CFG_REG_OS_MASK) << TMP75Q1_CFG_REG_OS_SHIFT))

extern float temperaturecelcius;

void tmp75q1_test(void);
int16_t tmp72q1_read_data();

#endif /* I2C_TMP75Q1_I2C_TMP75Q1_H_ */
