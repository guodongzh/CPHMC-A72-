/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       board_info.c
 *@author     LiuRui
 *@date       2026.04.07
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.04.07  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "board_info.h"
#include "gpio_ctrl.h"
#include "ti/csl/src/ip/vtm/V1/csl_vtm.h"
#include "ti/csl/soc/j721e/src/cslr_soc.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
power_status_t power_status[POWER_ID_MAX];
bool slot_status[SLOT_ID_MAX];
int32_t degree_temp_val[5];

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

void get_power_status(void)
{
    uint32_t val = 0;
    val = GPIO_read(0, PIN_NUM_PWRA_EX);
    power_status[PWRA_ID].is_online = !val;

    val = GPIO_read(0, PIN_NUM_PWRA_FALA);
    power_status[PWRA_ID].fala = !val;

    val = GPIO_read(0, PIN_NUM_PWRA_FALB);
    power_status[PWRA_ID].falb = !val;

    val = GPIO_read(0, PIN_NUM_PWRA_TMP);
    power_status[PWRA_ID].tmp = !val;

    val = GPIO_read(0, PIN_NUM_PWRB_EX);
    power_status[PWRB_ID].is_online = !val;

    val = GPIO_read(0, PIN_NUM_PWRB_FALA);
    power_status[PWRB_ID].fala = !val;

    val = GPIO_read(0, PIN_NUM_PWRB_FALB);
    power_status[PWRB_ID].falb = !val;

    val = GPIO_read(0, PIN_NUM_PWRB_TMP);
    power_status[PWRB_ID].tmp = !val;
}

void get_slot_ex(void)
{
    uint32_t val = 0;
    val = GPIO_read(0, PIN_NUM_SLOTA_EX);
    if (val == GPIO_PIN_LOW)
    {
        slot_status[SLOTA_ID] = true;
    }
    else
    {
        slot_status[SLOTA_ID] = false;
    }

    val = GPIO_read(0, PIN_NUM_SLOTB_EX);
    if (val == GPIO_PIN_LOW)
    {
        slot_status[SLOTB_ID] = true;
    }
    else
    {
        slot_status[SLOTB_ID] = false;
    }

    val = GPIO_read(0, PIN_NUM_SLOTC_EX);
    if (val == GPIO_PIN_LOW)
    {
        slot_status[SLOTC_ID] = true;
    }
    else
    {
        slot_status[SLOTC_ID] = false;
    }

    val = GPIO_read(0, PIN_NUM_SLOTD_EX);
    if (val == GPIO_PIN_LOW)
    {
        slot_status[SLOTD_ID] = true;
    }
    else
    {
        slot_status[SLOTD_ID] = false;
    }

    val = GPIO_read(0, PIN_NUM_PEXIST);
    if (val == GPIO_PIN_LOW)
    {
        slot_status[PEXIST_ID] = true;
    }
    else
    {
        slot_status[PEXIST_ID] = false;
    }
}

void get_pfunc(void)
{
    uint32_t val = 0;
    val = GPIO_read(0, PIN_NUM_PFUNC0);
    Debug_log("PFUNC0 : %d\r\n", val);
    val = GPIO_read(0, PIN_NUM_PFUNC1);
    Debug_log("PFUNC1 : %d\r\n\r\n", val);
}

void print_power_status(void)
{
    if (power_status[PWRA_ID].is_online)
    {
        Debug_logInfo("PWRA_EX online\r\n");
    }
    else
    {
        Debug_logInfo("PWRA_EX offline\r\n");
    }

    Debug_logInfo("PWRA_FALA : %d\r\n", power_status[PWRA_ID].fala);
    Debug_logInfo("PWRA_FALB : %d\r\n", power_status[PWRA_ID].falb);
    Debug_logInfo("PWRA_TMP : %d\r\n", power_status[PWRA_ID].tmp);

    if (power_status[PWRB_ID].is_online)
    {
        Debug_logInfo("PWRB_EX online\r\n");
    }
    else
    {
        Debug_logInfo("PWRB_EX offline\r\n");
    }

    Debug_logInfo("PWRB_FALA : %d\r\n", power_status[PWRB_ID].fala);
    Debug_logInfo("PWRB_FALB : %d\r\n", power_status[PWRB_ID].falb);
    Debug_logInfo("PWRB_TMP : %d\r\n", power_status[PWRB_ID].tmp);
}

void print_slot_status(void)
{
    if (slot_status[SLOTA_ID])
    {
        Debug_logInfo("SLOTA_EX online\r\n");
    }
    else
    {
        Debug_logInfo("SLOTA_EX offline\r\n");
    }

    if (slot_status[SLOTB_ID])
    {
        Debug_logInfo("SLOTB_EX online\r\n");
    }
    else
    {
        Debug_logInfo("SLOTB_EX offline\r\n");
    }

    if (slot_status[SLOTC_ID])
    {
        Debug_logInfo("SLOTC_EX online\r\n");
    }
    else
    {
        Debug_logInfo("SLOTC_EX offline\r\n");
    }

    if (slot_status[SLOTD_ID])
    {
        Debug_logInfo("SLOTD_EX online\r\n");
    }
    else
    {
        Debug_logInfo("SLOTD_EX offline\r\n");
    }

    if (slot_status[PEXIST_ID])
    {
        Debug_logInfo("PEXIST online\r\n");
    }
    else
    {
        Debug_logInfo("PEXIST offline\r\n");
    }
}

void get_all_temp_sensor_value(void)
{
    uint8_t id;
    CSL_vtm_cfg1Regs *p_vtm_cfg1_regs;
    CSL_vtm_info vtm_info;
    CSL_vtm_tsStat_val ts_stat_val;
    CSL_vtm_tsStat_read_ctrl read_ctrl;
    int32_t milli_degree_temp_val;

    p_vtm_cfg1_regs = (CSL_vtm_cfg1Regs *)CSL_WKUP_VTM0_MMR_VBUSP_CFG1_BASE;
    CSL_vtmGetVTMInfo(p_vtm_cfg1_regs, &vtm_info);
    for (id = 0; id < vtm_info.temp_sensors_cnt && id < sizeof(degree_temp_val)/sizeof(degree_temp_val[0]); id++)
    {
        read_ctrl = CSL_VTM_TS_READ_VD_MAP_VAL | CSL_VTM_TS_READ_ALL_THRESHOLD_ALERTS |
                    CSL_VTM_TS_READ_FIRST_TIME_EOC_BIT | CSL_VTM_TS_READ_DATA_VALID_BIT |
                    CSL_VTM_TS_READ_DATA_OUT_VAL;

        CSL_vtmTsGetSensorStat(p_vtm_cfg1_regs, &read_ctrl, id, &ts_stat_val);
        CSL_vtmTsConvADCToTemp(ts_stat_val.data_out, id, &milli_degree_temp_val);
        degree_temp_val[id] = milli_degree_temp_val / 1000;
    }
}

void print_all_temp_sensor_value(void)
{
    for (uint8_t i = 0; i < sizeof(degree_temp_val)/sizeof(degree_temp_val[0]); i++)
    {
        Debug_logInfo("Temperature Sensor %d: %d degree C\r\n", i, degree_temp_val[i]);
    }
}
