/************************************************* *****************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       device_init.c
 *@author     XuQuanbing
 *@date       2024.10.28
 *@brief      Device init function source file
 *@par        History
 *Date        Version   Author       Description
 *2024.10.28  1.0       XuQuanbing   Initial version
 ******************************************************************************/
/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "debug_config.h"
#include <ti/csl/soc/j721e/src/cslr_soc.h>
#include <ti/drv/i2c/I2C.h>
#include "ti/drv/i2c/soc/I2C_soc.h"
#include "bsp_init.h"
#include "i2c_bus_init.h"

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
I2C_Handle i2c0_handle;
I2C_Handle i2c1_handle;
I2C_Handle i2c2_handle;
I2C_Handle i2c6_handle;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

/*
 *  ======== Board_initI2C ========
 */
void board_init_i2c0(void)
{
    I2C_HwAttrs i2c_cfg;

    /* Get the default I2C init configurations */
    I2C_socGetInitCfg(I2C0_CSXID_INSTANCE, &i2c_cfg);

    /* Modify the default I2C configurations if necessary */
    i2c_cfg.baseAddr = CSL_I2C0_CFG_BASE;

    /* Set the default I2C init configurations */
    I2C_socSetInitCfg(I2C0_CSXID_INSTANCE, &i2c_cfg);

    Debug_logTag("I2C Test: Using Instance %d\n", I2C0_CSXID_INSTANCE);
}

void board_init_i2c1(void)
{
    I2C_HwAttrs i2c_cfg;

    /* Get the default I2C init configurations */
    I2C_socGetInitCfg(I2C1_EEPROM_INSTANCE, &i2c_cfg);

    /* Modify the default I2C configurations if necessary */
    i2c_cfg.baseAddr = CSL_I2C1_CFG_BASE;

    /* Set the default I2C init configurations */
    I2C_socSetInitCfg(I2C1_EEPROM_INSTANCE, &i2c_cfg);

    Debug_logTag("I2C Test: Using Instance %d\n", I2C1_EEPROM_INSTANCE);
}

void board_init_i2c2(void)
{
    I2C_HwAttrs i2c_cfg;

    /* Get the default I2C init configurations */
    I2C_socGetInitCfg(I2C2_SENSOR_INSTANCE, &i2c_cfg);

    /* Modify the default I2C configurations if necessary */
    i2c_cfg.baseAddr = CSL_I2C2_CFG_BASE;

    /* Set the default I2C init configurations */
    I2C_socSetInitCfg(I2C2_SENSOR_INSTANCE, &i2c_cfg);

    Debug_logTag("I2C Test: Using Instance %d\n", I2C2_SENSOR_INSTANCE);
}

void board_init_i2c6(void)
{
    I2C_HwAttrs i2c_cfg;

    /* Get the default I2C init configurations */
    I2C_socGetInitCfg(I2C6_LED_INSTANCE, &i2c_cfg);

    /* Modify the default I2C configurations if necessary */
    i2c_cfg.baseAddr = CSL_I2C6_CFG_BASE;

    /* Set the default I2C init configurations */
    I2C_socSetInitCfg(I2C6_LED_INSTANCE, &i2c_cfg);

    Debug_logTag("I2C Test: Using Instance %d\n", I2C6_LED_INSTANCE);
}

/* I2C0 BUS Init
 *  SlaveDevice: CSx ID
 */

void i2c0_bus_Init(void)
{
    I2C_Params params;

    I2C_Params_init(&params);
    board_init_i2c0();

    params.bitRate = I2C_400kHz;
    params.transferMode = I2C_MODE_BLOCKING;
    params.lockNumber = I2C0_SPINK_LOCK_ID;
    i2c0_handle = I2C_open(I2C0_CSXID_INSTANCE, &params);

    if (i2c0_handle == NULL)
    {
        Debug_logTag("I2C0 device open failed!\n");
        I2C_close(i2c0_handle);
    }
}

/* I2C1 BUS Init
 *  SlaveDevice: MainBoard ID  &  EEPROM
 */
void i2c1_bus_Init(void)
{
    I2C_Params params;
    I2C_Params_init(&params);
    board_init_i2c1();

    params.bitRate = I2C_400kHz;
    params.transferMode = I2C_MODE_BLOCKING;
    params.lockNumber = I2C1_SPINK_LOCK_ID;
    i2c1_handle = I2C_open(I2C1_EEPROM_INSTANCE, &params);

    if (i2c1_handle == NULL)
    {
        Debug_logTag("I2C1 device open failed!\n");
        I2C_close(i2c1_handle);
    }
}

/* I2C2 BUS Init
 *  SlaveDevice: TMP75Q1  &  DS1338
 */
void i2c2_bus_Init(void)
{
    I2C_Params params;
    I2C_Params_init(&params);
    board_init_i2c2();

    params.bitRate = I2C_400kHz;
    params.transferMode = I2C_MODE_BLOCKING;
    params.lockNumber = I2C2_SPINK_LOCK_ID;
    i2c2_handle = I2C_open(I2C2_SENSOR_INSTANCE, &params);

    if (NULL == i2c2_handle)
    {
        Debug_logTag("I2C1 device open failed!\n");
        I2C_close(i2c2_handle);
    }
}

/* I2C6 BUS Init
 *  SlaveDevice: LED
 */
void i2c6_bus_Init(void)
{
    I2C_Params params;
    I2C_Params_init(&params);
    board_init_i2c6();

    params.bitRate = I2C_400kHz;
    params.transferMode = I2C_MODE_BLOCKING;
    params.lockNumber = I2C6_SPINK_LOCK_ID;
    i2c6_handle = I2C_open(I2C6_LED_INSTANCE, &params);
    if (NULL == i2c6_handle)
    {
        Debug_logTag("I2C6 device open failed!\n");
        I2C_close(i2c6_handle);
    }
}
