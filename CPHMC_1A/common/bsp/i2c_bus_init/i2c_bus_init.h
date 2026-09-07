/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       eeprom_at24c.h
 *@author     XuQuanbing
 *@date       2024.10.28
 *@brief      EEPROM function source file
 *@par        History
 *Date        Version   Author       Description
 *2024.10.28  1.0       XuQuanbing   Initial version
 ******************************************************************************/
#ifndef DEVICE_INIT_H_
#define DEVICE_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "ti/drv/i2c/I2C.h"
/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
#define I2C0_CSXID_INSTANCE    0
#define I2C1_EEPROM_INSTANCE   1
#define I2C2_SENSOR_INSTANCE   2
#define I2C6_LED_INSTANCE      6

extern I2C_Handle i2c0_handle;   // i2c0
extern I2C_Handle i2c1_handle;   // i2c1
extern I2C_Handle i2c2_handle;   // i2c2
extern I2C_Handle i2c6_handle;   // i2c6

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */
void i2c0_bus_Init(void);
void i2c1_bus_Init(void);
void i2c2_bus_Init(void);
void i2c6_bus_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef DEVICE_INIT_H_ */
