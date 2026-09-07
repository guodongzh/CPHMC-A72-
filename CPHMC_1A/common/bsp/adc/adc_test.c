/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       adc_test.c
 *@author     xqb
 *@date       2024.07.21
 *@brief      adc module test source.c file
 *@par        History
 *Date        Version   Author     Description
 *2024.07.21  1.0       xqb        example
 ******************************************************************************/

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "adc_test.h"
#include "debug_config.h"

#define ADC_REF_VOLTAGE 1800 /* 3300  Reference voltage 3.3v  1.8v */

uint32_t baseAddr = CONFIG_ADC0_BASE_ADDR;
volatile uint32_t isrFlag = 0U;
void ADCIntrISR(void *handle);


void ADC_Init(void)
{
    adcStepConfig_t adcConfig;

    /* start ADC */
    ADCPowerUp(baseAddr, TRUE);

    /* initiate ADC */
    ADCInit(baseAddr, FALSE, 0, 1);

    /* ADC configure */
    adcConfig.mode = ADC_OPERATION_MODE_SINGLE_SHOT;
    adcConfig.openDelay = 0;
    adcConfig.sampleDelay = 0;
    adcConfig.rangeCheckEnable = FALSE;
    adcConfig.averaging = ADC_AVERAGING_16_SAMPLES;
    adcConfig.fifoNum = ADC_FIFO_NUM_0;
    /* each of ADC channel configuration */
    adcConfig.channel = ADC_CHANNEL_1;
    ADCSetStepParams(baseAddr, ADC_STEP_1, &adcConfig);
    adcConfig.channel = ADC_CHANNEL_2;
    ADCSetStepParams(baseAddr, ADC_STEP_2, &adcConfig);
    adcConfig.channel = ADC_CHANNEL_3;
    ADCSetStepParams(baseAddr, ADC_STEP_3, &adcConfig);
    adcConfig.channel = ADC_CHANNEL_4;
    ADCSetStepParams(baseAddr, ADC_STEP_4, &adcConfig);
    adcConfig.channel = ADC_CHANNEL_5;
    ADCSetStepParams(baseAddr, ADC_STEP_5, &adcConfig);

    /* start step */
    ADCStepEnable(baseAddr, ADC_STEP_1, TRUE);
    ADCStepEnable(baseAddr, ADC_STEP_2, TRUE);
    ADCStepEnable(baseAddr, ADC_STEP_3, TRUE);
    ADCStepEnable(baseAddr, ADC_STEP_4, TRUE);
    ADCStepEnable(baseAddr, ADC_STEP_5, TRUE);
}

void ADCIntrISR(void *handle)
{
    uint32_t status;

    Debug_log("\r\nIn ISR...\n");
    status = ADCGetIntrStatus(baseAddr);
    ADCClearIntrStatus(baseAddr, status);
    if (ADC_INTR_SRC_END_OF_SEQUENCE == (status & ADC_INTR_SRC_END_OF_SEQUENCE))
    {
        Debug_log("\r\nEnd of sequence interrupt occurred.\n");
    }
    if (ADC_INTR_SRC_FIFO0_THRESHOLD == (status & ADC_INTR_SRC_FIFO0_THRESHOLD))
    {
        Debug_log("\r\nFIFO 0 threshold interrupt occurred.\n");
    }
    if (ADC_INTR_SRC_FIFO0_OVERRUN == (status & ADC_INTR_SRC_FIFO0_OVERRUN))
    {
        Debug_log("\r\nFIFO 0 overrun interrupt occurred.\n");
    }
    if (ADC_INTR_SRC_FIFO0_UNDERFLOW == (status & ADC_INTR_SRC_FIFO0_UNDERFLOW))
    {
        Debug_log("\r\nFIFO 0 underflow interrupt occurred.\n");
    }
    if (ADC_INTR_SRC_FIFO1_THRESHOLD == (status & ADC_INTR_SRC_FIFO1_THRESHOLD))
    {
        Debug_log("\r\nFIFO 1 threshold interrupt occurred.\n");
    }
    if (ADC_INTR_SRC_FIFO1_OVERRUN == (status & ADC_INTR_SRC_FIFO1_OVERRUN))
    {
        Debug_log("\r\nFIFO 1 overrun interrupt occurred.\n");
    }
    if (ADC_INTR_SRC_FIFO1_UNDERFLOW == (status & ADC_INTR_SRC_FIFO1_UNDERFLOW))
    {
        Debug_log("\r\nFIFO 1 underflow interrupt occurred.\n");
    }
    if (ADC_INTR_SRC_OUT_OF_RANGE == (status & ADC_INTR_SRC_OUT_OF_RANGE))
    {
        Debug_log("\r\nOut of range interrupt occurred.\n");
    }
    isrFlag++;
    ADCWriteEOI(baseAddr);
}

static void ADCModuleDisable(void)
{
#if defined(SOC_TDA3XX) || defined(SOC_DRA78x)
    HW_WR_FIELD32(SOC_L4PER_CM_CORE_BASE + CM_L4PER2_ADC_CLKCTRL,
                  CM_L4PER2_ADC_CLKCTRL_MODULEMODE,
                  CM_L4PER2_ADC_CLKCTRL_MODULEMODE_DISABLED);
    while (CM_L4PER2_ADC_CLKCTRL_IDLEST_DISABLE !=
           HW_RD_FIELD32(SOC_L4PER_CM_CORE_BASE + CM_L4PER2_ADC_CLKCTRL,
                         CM_L4PER2_ADC_CLKCTRL_IDLEST))
    {
        /* Wait till ADC module is disabled */
    }
#elif defined(SOC_J721E)
    /* Disable ADC module */
    Sciclient_pmSetModuleState(TISCI_DEV_MCU_ADC12_16FFC0,
                               TISCI_MSG_VALUE_DEVICE_SW_STATE_AUTO_OFF,
                               TISCI_MSG_FLAG_AOP |
                                   TISCI_MSG_FLAG_DEVICE_EXCLUSIVE |
                                   TISCI_MSG_FLAG_DEVICE_RESET_ISO,
                               SCICLIENT_SERVICE_WAIT_FOREVER);
#elif defined(SOC_AM64X)
    /* Disable ADC module */
    Sciclient_pmSetModuleState(TISCI_DEV_ADC0,
                               TISCI_MSG_VALUE_DEVICE_SW_STATE_AUTO_OFF,
                               TISCI_MSG_FLAG_AOP |
                                   TISCI_MSG_FLAG_DEVICE_EXCLUSIVE |
                                   TISCI_MSG_FLAG_DEVICE_RESET_ISO,
                               SCICLIENT_SERVICE_WAIT_FOREVER);
#else
    /* Disable ADC module */
    Sciclient_pmSetModuleState(TISCI_DEV_MCU_ADC0,
                               TISCI_MSG_VALUE_DEVICE_SW_STATE_AUTO_OFF,
                               TISCI_MSG_FLAG_AOP |
                                   TISCI_MSG_FLAG_DEVICE_EXCLUSIVE |
                                   TISCI_MSG_FLAG_DEVICE_RESET_ISO,
                               SCICLIENT_SERVICE_WAIT_FOREVER);
#endif
}

static void ADCModuleEnable(void)
{
#if defined(SOC_TDA3XX) || defined(SOC_DRA78x)
    HW_WR_FIELD32(SOC_L4PER_CM_CORE_BASE + CM_L4PER2_ADC_CLKCTRL,
                  CM_L4PER2_ADC_CLKCTRL_MODULEMODE,
                  CM_L4PER2_ADC_CLKCTRL_MODULEMODE_ENABLE);
    while (CM_L4PER2_ADC_CLKCTRL_IDLEST_FUNC !=
           HW_RD_FIELD32(SOC_L4PER_CM_CORE_BASE + CM_L4PER2_ADC_CLKCTRL,
                         CM_L4PER2_ADC_CLKCTRL_IDLEST))
    {
        /* Wait till ADC module is enabled */
    }
#elif defined(SOC_J721E)
    /* Enable ADC module */
    Sciclient_pmSetModuleState(TISCI_DEV_MCU_ADC12_16FFC0,
                               TISCI_MSG_VALUE_DEVICE_SW_STATE_ON,
                               TISCI_MSG_FLAG_AOP |
                               TISCI_MSG_FLAG_DEVICE_EXCLUSIVE |
                               TISCI_MSG_FLAG_DEVICE_RESET_ISO,
                               SCICLIENT_SERVICE_WAIT_FOREVER);
#elif defined(SOC_AM64X)
    /* Enable ADC module */
    Sciclient_pmSetModuleState(TISCI_DEV_ADC0,
                               TISCI_MSG_VALUE_DEVICE_SW_STATE_ON,
                               TISCI_MSG_FLAG_AOP |
                                   TISCI_MSG_FLAG_DEVICE_EXCLUSIVE |
                                   TISCI_MSG_FLAG_DEVICE_RESET_ISO,
                               SCICLIENT_SERVICE_WAIT_FOREVER);
#else
    /* Enable ADC module */
    Sciclient_pmSetModuleState(TISCI_DEV_MCU_ADC0,
                               TISCI_MSG_VALUE_DEVICE_SW_STATE_ON,
                               TISCI_MSG_FLAG_AOP |
                                   TISCI_MSG_FLAG_DEVICE_EXCLUSIVE |
                                   TISCI_MSG_FLAG_DEVICE_RESET_ISO,
                               SCICLIENT_SERVICE_WAIT_FOREVER);
#endif
}

static void ADCConfigureInterrupt(void)
{
#if defined(SOC_TDA3XX) || defined(SOC_DRA78x)
    /* XBar configuration */
    CSL_xbarIrqConfigure(XBAR_CPU, XBAR_INST, XBAR_INTR_SOURCE);

    /* Enable Error interrupt for ADC */
    Intc_Init();
    Intc_IntEnable(APP_ADC_INT);
    /* Register ISR */
    Intc_IntRegister(APP_ADC_INT, (IntrFuncPtr)AppADCIntrISR, 0);
    Intc_IntPrioritySet(APP_ADC_INT, 1, 0);
    Intc_SystemEnable(APP_ADC_INT);
#else
    OsalRegisterIntrParams_t intrPrms;
    OsalInterruptRetCode_e osalRetVal;
    HwiP_Handle hwiHandle;

    Osal_RegisterInterrupt_initParams(&intrPrms);
    intrPrms.corepacConfig.arg = (uintptr_t)0;
    intrPrms.corepacConfig.priority = 1U;
    intrPrms.corepacConfig.corepacEventNum = 0U; /* NOT USED */
#if defined(SOC_AM65XX)
    intrPrms.corepacConfig.intVecNum = CSL_MCU0_INTR_ADC0_GEN_LEVEL;
#elif defined(SOC_AM64X)
    intrPrms.corepacConfig.intVecNum = CSLR_R5FSS0_CORE0_INTR_ADC0_GEN_LEVEL_0;
#else
    intrPrms.corepacConfig.intVecNum = CSLR_MCU_R5FSS0_CORE0_INTR_MCU_ADC0_GEN_LEVEL_0;
#endif
    intrPrms.corepacConfig.isrRoutine = (void (*)(uintptr_t))(&ADCIntrISR);
    osalRetVal = Osal_RegisterInterrupt(&intrPrms, &hwiHandle);
    if (OSAL_INT_SUCCESS != osalRetVal)
    {
        Debug_log("\r\nError Could not register ISR !!!\r\n");
    }

#endif
}

/* Get the DEC voltage convert value */
uint32_t ADC_ReadVoltage(uint32_t baseAddr, uint32_t fifoNum)
{
    uint32_t adcvalue;
    int16_t res;

    adcvalue = ADCGetFIFOData(baseAddr, fifoNum);
    return (adcvalue * ADC_RANGE_MAX) / ADC_RANGE_MAX;
}

/* ADC_CSL_Task --- way2 */
void ADC_CSL_Task(void *arg0, void *arg1)
{
    int32_t configStatus, testErrCount = 0;
    uint32_t loopcnt, fifoData, fifoWordCnt, stepID, voltageLvl;
    adcStepConfig_t adcConfig;

    Debug_log("\r\nADC get power voltage test started...\r\n");

    ADCConfigureInterrupt();

    ADCModuleEnable();

    /* Initialize ADC module */
    ADCInit(baseAddr, FALSE, 0, 1);

    /* ADC configure */
    adcConfig.mode = ADC_OPERATION_MODE_SINGLE_SHOT;
    adcConfig.openDelay = 0;
    adcConfig.sampleDelay = 0;
    adcConfig.rangeCheckEnable = FALSE;
    adcConfig.averaging = ADC_AVERAGING_16_SAMPLES;
    adcConfig.fifoNum = ADC_FIFO_NUM_0;

    /* Enable interrupts */
    ADCEnableIntr(CONFIG_ADC0_BASE_ADDR, (ADC_INTR_SRC_END_OF_SEQUENCE |
                                          ADC_INTR_SRC_FIFO0_THRESHOLD |
                                          ADC_INTR_SRC_FIFO0_OVERRUN |
                                          ADC_INTR_SRC_FIFO0_UNDERFLOW |
                                          ADC_INTR_SRC_FIFO1_THRESHOLD |
                                          ADC_INTR_SRC_FIFO1_OVERRUN |
                                          ADC_INTR_SRC_FIFO1_UNDERFLOW |
                                          ADC_INTR_SRC_OUT_OF_RANGE));
    /* Configure ADC */
    /* step 1 configuration */
    adcConfig.channel = ADC_CHANNEL_1;
    configStatus = ADCSetStepParams(CONFIG_ADC0_BASE_ADDR, ADC_STEP_1, &adcConfig);
    if (STW_SOK != configStatus)
    {
        Debug_log("\r\nError in ADC step configuration.\n");
        testErrCount++;
    }
    /* step 2 configuration */
    adcConfig.channel = ADC_CHANNEL_2;
    configStatus = ADCSetStepParams(CONFIG_ADC0_BASE_ADDR, ADC_STEP_2, &adcConfig);
    if (STW_SOK != configStatus)
    {
        Debug_log("\r\nError in ADC step configuration.\n");
        testErrCount++;
    }
    /* step 3 configuration */
    adcConfig.channel = ADC_CHANNEL_3;
    configStatus = ADCSetStepParams(CONFIG_ADC0_BASE_ADDR, ADC_STEP_3, &adcConfig);
    if (STW_SOK != configStatus)
    {
        Debug_log("\r\nError in ADC step configuration.\n");
        testErrCount++;
    }
    /* step 4 configuration */
    adcConfig.channel = ADC_CHANNEL_4;
    configStatus = ADCSetStepParams(CONFIG_ADC0_BASE_ADDR, ADC_STEP_4, &adcConfig);
    if (STW_SOK != configStatus)
    {
        Debug_log("\r\nError in ADC step configuration.\n");
    }
    /* step 5 configuration */
    adcConfig.channel = ADC_CHANNEL_5;
    configStatus = ADCSetStepParams(CONFIG_ADC0_BASE_ADDR, ADC_STEP_5, &adcConfig);
    if (STW_SOK != configStatus)
    {
        Debug_log("\r\nError in ADC step configuration.\n");
        testErrCount++;
    }
    configStatus = ADCSetCPUFIFOThresholdLevel(APP_ADC_MODULE, ADC_FIFO_NUM_0, 40U);
    if (STW_SOK != configStatus)
    {
        Debug_log("\r\nError in ADC CPU threshold configuration.\n");
        testErrCount++;
    }
    /* step enable */
    ADCStepEnable(CONFIG_ADC0_BASE_ADDR, ADC_STEP_1, TRUE);
    ADCStepEnable(CONFIG_ADC0_BASE_ADDR, ADC_STEP_2, TRUE);
    ADCStepEnable(CONFIG_ADC0_BASE_ADDR, ADC_STEP_3, TRUE);
    ADCStepEnable(CONFIG_ADC0_BASE_ADDR, ADC_STEP_4, TRUE);
    ADCStepEnable(CONFIG_ADC0_BASE_ADDR, ADC_STEP_5, TRUE);

    /* start the task loop*/
    while (1)
    {
        ADCStartConfig();

        while (0U == isrFlag)
        {
        }
        /* Get FIFO data */
        fifoWordCnt = ADCGetFIFOWordCount(CONFIG_ADC0_BASE_ADDR, ADC_FIFO_NUM_0);
        Debug_log("\r\nNumber of samples in FIFO:", (uint32_t)fifoWordCnt);
        Debug_log("\r\n");
        Debug_log("\r\nFIFO Data:\n");
        for (loopcnt = 0U; loopcnt < fifoWordCnt; loopcnt++)
        {
            fifoData = ADCGetFIFOData(CONFIG_ADC0_BASE_ADDR, ADC_FIFO_NUM_0);
            stepID = ((fifoData & ADC_FIFODATA_ADCCHNLID_MASK) >>
                      ADC_FIFODATA_ADCCHNLID_SHIFT);
            fifoData = ((fifoData & ADC_FIFODATA_ADCDATA_MASK) >>
                        ADC_FIFODATA_ADCDATA_SHIFT);
            voltageLvl = fifoData * (uint32_t)ADC_REF_VOLTAGE;
            voltageLvl /= (uint32_t)ADC_RANGE_MAX;
            Debug_log("\r\nStep ID:", (uint32_t)(stepID + 1U));
            Debug_log("\n");
            Debug_log("\r\nVoltage Level:", (uint32_t)voltageLvl);
            Debug_log("mV\n");
        }
        ADCStopConfig();
        /* Power down ADC */
        ADCPowerUp(CONFIG_ADC0_BASE_ADDR, FALSE);
        /* Disable ADC module */
        ADCModuleDisable();

        Debug_log("\r\nApplication is completed.\n");

        if (testErrCount == 0)
        {
            Debug_log("\r\n All tests have passed. \n");
#if defined(UNITY_INCLUDE_CONFIG_H)
            TEST_PASS();
#endif
        }
        else
        {
            Debug_log("\r\n ADC Test failed. \n");
#if defined(UNITY_INCLUDE_CONFIG_H)
            TEST_FAIL();
#endif
        }

        /* delay 3s to read again */
        Osal_delay(3000);
    }
}

static void ADCStopConfig(void)
{
    adcSequencerStatus_t status;

    /* Disable all/enabled steps */
    ADCStepEnable(CONFIG_ADC0_BASE_ADDR, ADC_STEP_1, FALSE);
    ADCStepEnable(CONFIG_ADC0_BASE_ADDR, ADC_STEP_2, FALSE);
    ADCStepEnable(CONFIG_ADC0_BASE_ADDR, ADC_STEP_3, FALSE);
    ADCStepEnable(CONFIG_ADC0_BASE_ADDR, ADC_STEP_4, FALSE);
    ADCStepEnable(CONFIG_ADC0_BASE_ADDR, ADC_STEP_5, FALSE);

    /* Wait for FSM to go IDLE */
    ADCGetSequencerStatus(CONFIG_ADC0_BASE_ADDR, &status);
    while ((ADC_ADCSTAT_FSM_BUSY_IDLE != status.fsmBusy) &&
           ADC_ADCSTAT_STEP_ID_IDLE != status.stepId)
    {
        ADCGetSequencerStatus(CONFIG_ADC0_BASE_ADDR, &status);
    }

    /* Stop ADC */
    ADCStart(CONFIG_ADC0_BASE_ADDR, FALSE);
    /* Wait for FSM to go IDLE */
    ADCGetSequencerStatus(CONFIG_ADC0_BASE_ADDR, &status);
    while ((ADC_ADCSTAT_FSM_BUSY_IDLE != status.fsmBusy) &&
           ADC_ADCSTAT_STEP_ID_IDLE != status.stepId)
    {
        ADCGetSequencerStatus(CONFIG_ADC0_BASE_ADDR, &status);
    }
}

static void ADCStartConfig(void)
{
    adcSequencerStatus_t status;

    /* Check if FSM is idle */
    ADCGetSequencerStatus(CONFIG_ADC0_BASE_ADDR, &status);
    while ((ADC_ADCSTAT_FSM_BUSY_IDLE != status.fsmBusy) &&
           ADC_ADCSTAT_STEP_ID_IDLE != status.stepId)
    {
        ADCGetSequencerStatus(CONFIG_ADC0_BASE_ADDR, &status);
    }
    /* Start ADC conversion */
    ADCStart(CONFIG_ADC0_BASE_ADDR, TRUE);
}
