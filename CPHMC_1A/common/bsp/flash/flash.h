/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       flash.h
*@author     LiuRui
*@date       2024.08.22
*@brief      nor flash read and write
*@par        History
*Date        Version   Author     Description
*2024.08.22  1.0       LiuRui    first version
******************************************************************************/

#ifndef _FLASH_
#define _FLASH_

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <ti/drv/spi/SPI.h>
#include <ti/drv/spi/soc/SPI_soc.h>
#include "ti/osal/osal.h"
#include "debug_config.h"

#include "board/board.h"
#include "board/src/flash/include/board_flash.h"
#include <ti/drv/sciclient/sciclient.h>

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/
typedef  struct _flash_diag
{
    uint32_t wr_rd_oks;
    uint32_t wr_rd_err;
    uint32_t wr_rd_running;
    uint32_t status;
}flash_diag_t;

extern flash_diag_t flash_diag; 


extern Board_flashHandle flash_ctrl0_handle;
extern Board_flashHandle flash_ctrl1_handle;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
int flash_interface_init(void);
bool verify_data(uint8_t *expData, uint8_t *rxData, uint32_t length);
void flash_test(void);


#ifdef __cplusplus
}
#endif

#endif //_FLASH_
