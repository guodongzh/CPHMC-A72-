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
#ifndef EEPROM_AT24C_H_
#define EEPROM_AT24C_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "ti/drv/i2c/I2C.h"
#include <ti/drv/i2c/soc/I2C_v1.h>
/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

#define EEPROM_PAGE_SIZE         (256U)
#define EEPROM_WR_BUF_SIZE       (2U + EEPROM_PAGE_SIZE)
#define EEPROM_AT24C_WRAP_OFFSET (64 * 1024U)

// EEPROM device address
#define EEPROM_Address 0x50


typedef  struct _eeprom_diag
{
    uint32_t wr_rd_oks;
    uint32_t wr_rd_err;
    uint32_t wr_rd_running;
    uint32_t status;
}eeprom_diag_t;

extern eeprom_diag_t eeprom_diag; 

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

int16_t EEPROM_AT24C_open(void);

void EEPROM_AT24C_close(I2C_Handle handle);

int16_t EEPROM_AT24C_read(I2C_Handle handle,
                          uint32_t   offset,
                          uint8_t   *buf,
                          uint32_t   len);

int16_t EEPROM_AT24C_write(I2C_Handle     handle,
                           uint32_t       offset,
                           const uint8_t *buf,
                           uint32_t       len);

void eeprom_test(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef EEPROM_AT24C_H_ */
