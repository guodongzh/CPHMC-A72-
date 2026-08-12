/**
 *  \file boot_app_main.c
 *
 *  \brief Main file for building boot app build
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include "boot_app_priv.h"
#include "app_version.h"

#include <ti/drv/sciclient/src/sciserver/j721e/sciserver_hwiData.h>
#include <ti/drv/sciclient/sciserver_tirtos.h>

#include "ospi/boot_app_ospi.h"
#include "debug_config.h"
#include "ti/boot/src/rprc/sbl_rprc.h"
#include "src/interrupt_priv.h"
#include "backtrace.h"
#include "flash.h"
#include "cfg_prase_app.h"
#include "i2c_bus_init.h"

#include "pscEnv.h"
#include "ti/osal/osal_config.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* Test application stack size */
#define APP_TASK_STACK (10U * 1024U)
/**< Task Priority Levels */
#define BOOT_TASK_PRIORITY (2)

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */
static void BootApp_TaskFxn(void *a0, void *a1);
static uint32_t Boot_App(uint8_t package_image);
static int32_t BootApp_RequestStageCores(uint8_t stageNum);
static int32_t BootApp_ReleaseStageCores(uint8_t stageNum);
static void BootApp_ArmR5PmuCntrInit();
static uint32_t BootApp_GetTimeInMicroSec(uint32_t pmuCntrVal);
static uint32_t BootApp_SetupSciServer(void);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* Stack for the Boot task */
static uint8_t gBootAppTaskStack[APP_TASK_STACK] __attribute__((aligned(32)));
TaskP_Handle gbootTask;
static uint64_t gtimeBootAppStart, gtimeBootAppFinish;

sblEntryPoint_t gK3xx_evmEntry;

#define SCISERVER_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
HwiP_Handle gSciserverHwiHandles[SCISERVER_HWI_NUM];
uint8_t sciHwiMasked = 1U;
int32_t main(void)
{
    Board_initCfg boardCfg;
    uint32_t ret = CSL_PASS;
    TaskP_Params bootTaskParams;

    gExptnHandlers.dabtExptnHandler = dabt_exptn_handler;
    gExptnHandlers.pabtExptnHandlerArgs = NULL;

    boardCfg = 0;
    Board_init(boardCfg);
    OS_init();

    UART_HwAttrs uart_cfg;
    UART_Params params;

    UART_socGetInitCfg(BOARD_UART_INSTANCE, &uart_cfg);
    /* Use UART fclk freq setup by ROM */
    uart_cfg.baseAddr = CSL_WKUP_UART0_BASE;
    uart_cfg.frequency = SBL_SYSFW_UART_MODULE_INPUT_CLK;
    /* Disable the UART interrupt */
    uart_cfg.enableInterrupt = FALSE;
    UART_socSetInitCfg(BOARD_UART_INSTANCE, &uart_cfg);
    /* Init UART for logging. */
    UART_Params_init(&params);
    params.readTimeout = 999999;
    UART_stdioInit2(BOARD_UART_INSTANCE, &params);
    putchar_ = UART_putc;

    UART_printf("\nMCU R5F Boot App build at 20%d-%d-%d - %s, APP VSC VER = 0x%08x, started at %d usecs\n",
                APP_BUILD_YEAR,
                APP_BUILD_MONTH,
                APP_BUILD_DAY,
                __TIME__,
                APP_VSC_VERSION,
                BootApp_GetTimeInMicroSec(CSL_armR5PmuReadCntr(CSL_ARM_R5_PMU_CYCLE_COUNTER_NUM)));

    ret = BootApp_SetupSciServer();
    if (ret != CSL_PASS)
    {
        printf_("\nFailed to setup sciserver for boot app\r\n");
        OS_stop();
    }

    /*clear app memory
     * sbl clears DDR memory before loading this image
     * */
    memset((void *)0x70000000U, 0, 7 * 1024 * 1024);
    CacheP_wb((void *)0x70000000U, 7 * 1024 * 1024);
    CacheP_wb((void *)0x70700000U, 512 * 1024);

    BootApp_ArmR5PmuCntrInit();

    /* Initialize the task params */
    TaskP_Params_init(&bootTaskParams);
    bootTaskParams.priority = BOOT_TASK_PRIORITY;
    bootTaskParams.stack = gBootAppTaskStack;
    bootTaskParams.stacksize = sizeof(gBootAppTaskStack);
    gbootTask = TaskP_create(&BootApp_TaskFxn, &bootTaskParams);
    if (NULL == gbootTask)
    {
        printf_("\nBoot Task creation failed\r\n");
        OS_stop();
    }

    OS_start(); /* does not return */

    return 0;
}

static void BootApp_TaskFxn(void *a0, void *a1)
{
    uint8_t package_image;
    int32_t retVal;

    I2C_init();
    i2c0_bus_Init();
    i2c1_bus_Init();
    i2c2_bus_Init();
    i2c6_bus_Init();

    retVal = BootApp_OSPI_Init();
    if (retVal < 0)
    {
        printf_("Failure during BootApp_OSPI_Init\n\n");
        return;
    }

    printf_("Please select the startup image\n");
    printf_("0: app image (default)\n");
    printf_("1: back up image\n");
    printf_("2: debug image\n");
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 800; ++j)
        {
            for (int k = 0; k < 2000; ++k)
            {
                if (UARTCharGetNonBlocking2(CSL_WKUP_UART0_BASE, &package_image) == UTRUE)
                {
                    goto boot;
                }
            }
        }
        printf_("%d s ... ", i + 1);
    }

boot:
    printf_("\n");

    gtimeBootAppStart = BootApp_GetTimeInMicroSec(CSL_armR5PmuReadCntr(CSL_ARM_R5_PMU_CYCLE_COUNTER_NUM));

    if (package_image == '1')
    {
        printf_("loading bakup image\n");
        Boot_App(1);
    }
    else if (package_image == '2')
    {
        printf_("loading debug image\n");
        Boot_App(2);
    }
    else
    {
        printf_("loading default image\n");

         file_system_init();
         boot_cfg_read_main();
        for (int i = 0; i < MAX_CORES_PER_STAGE; ++i)
        {
            if (boot_cfg[i] == 1)
            {
                main_boot_flash_images[0][i] = debug_boot_flash_images[0][i];
                Debug_logWarn("core[%d] load debug flash addr=0x%08x\n", i, main_boot_flash_images[0][i]);
            }
            else
            {
                Debug_logInfo("core[%d] load app flash addr=0x%08x\n", i, main_boot_flash_images[0][i]);
            }
        }
        Boot_App(0);
    }

    gtimeBootAppFinish = BootApp_GetTimeInMicroSec(CSL_armR5PmuReadCntr(CSL_ARM_R5_PMU_CYCLE_COUNTER_NUM));

    /*modify magic numbers to redirect C66x core to L2 RAM program*/
    volatile uint32_t *c66_magic;

    c66_magic = (uint32_t *)0xa6300000;
    *c66_magic = 0x5f5f5f5f;

    c66_magic = (uint32_t *)0xa7300000;
    *c66_magic = 0x5f5f5f5f;

    printf_("\nMCU Boot Task started at %d usecs and finished at %d usecs\r\n",
            (uint32_t)gtimeBootAppStart, (uint32_t)gtimeBootAppFinish);

    while (1)
    {
        /* PSCode main scan */
//        Application_I1_FI();
//        Application_I1_N();
//        Application_I1_FE();
        Osal_delay(1);
    }
}

uint32_t Boot_App(uint8_t package_image)
{
    int32_t retVal;
    cpu_core_id_t core_id;
    uint32_t (*flash_image)[NUM_BOOT_STAGES][MAX_CORES_PER_STAGE];

    /* Initialize the entry point array to 0. */
    for (core_id = MPU1_CPU0_ID; core_id < NUM_CORES; core_id++)
        (&gK3xx_evmEntry)->CpuEntryPoint[core_id] = SBL_INVALID_ENTRY_ADDR;

    for (int i = 0; i < 2; i++)
    {
        retVal = BootApp_RequestStageCores(i);

        if (retVal != CSL_PASS)
        {
            printf_("Failed to request all late cores in Stage %d\n\n", i);
            BootApp_ReleaseStageCores(i);
        }
        else
        {
            /*get app entry and copy image from flash*/
            if (package_image == 1)
            {
                flash_image = &back_up_boot_flash_images;
            }
            else if (package_image == 2)
            {
                back_up_boot_flash_images[0][0] = ALL_CORES_APPS_NUL_FLASH_ADDR;
                flash_image = &back_up_boot_flash_images;
            }
            else
            {
                flash_image = &main_boot_flash_images;
            }
            for (int j = 0; j < MAX_CORES_PER_STAGE; ++j)
            {
                if ((*flash_image)[i][j] != 0)
                {
                    printf_("\nloading image from 0x%x\n", (*flash_image)[i][j]);
                    retVal = BootApp_OSPI_StageImage(&gK3xx_evmEntry, (*flash_image)[i][j]);
                    printf_("BootImage completed, status = %d\n", retVal);
                    if (retVal != CSL_PASS)
                    {
                        printf_("Failure during image copy and parsing\n\n");
                    }
                }
            }
            if (retVal != CSL_PASS)
            {
                printf_("Failure during image copy and parsing\n\n");
            }
            else
            {
                retVal = BootApp_ReleaseStageCores(i);
                if (retVal != CSL_PASS)
                {
                    printf_("Failed to release all late cores\n\n");
                }
            }
        }
    }

    if (retVal == CSL_PASS)
    {
        /* Start the individual cores for the boot stage */
        for (int i = 0; i < sizeof(boot_array) / sizeof(boot_array[0]); i++)
        {
            core_id = boot_array[i];
            /* Try booting all cores other than the cluster running the SBL */
            if ((gK3xx_evmEntry.CpuEntryPoint[core_id] != SBL_INVALID_ENTRY_ADDR) &&
                ((core_id != MCU1_CPU1_ID) && (core_id != MCU1_CPU0_ID)))
            {
                SBL_SlaveCoreBoot(core_id, 0, &gK3xx_evmEntry, SBL_REQUEST_CORE);
                printf_("SBL_SlaveCoreBoot completed for Core ID#%d, Entry point is 0x%x\n",
                        core_id, gK3xx_evmEntry.CpuEntryPoint[core_id]);
                booted_core_ids[num_booted_cores] = core_id;
                time_boot_core_finish[num_booted_cores] = BootApp_GetTimeInMicroSec(CSL_armR5PmuReadCntr(CSL_ARM_R5_PMU_CYCLE_COUNTER_NUM));
                num_booted_cores++;
            }
        }
    }

    /* Delay print out of boot log to avoid prints by other tasks */
    TaskP_sleep(4000);

    if (retVal == CSL_PASS)
    {
        /* Print boot log, including all gathered timestamps */
        printf_("\nBoot App: Started at %d usec\n", (uint32_t)gtimeBootAppStart);
        printf_("Boot App: Total Num booted cores = %d\n", num_booted_cores);

        for (core_id = 0; core_id < num_booted_cores; core_id++)
        {
            printf_("Boot App: Booted Core ID #%d at %d usecs\n",
                    booted_core_ids[core_id], (uint32_t)time_boot_core_finish[core_id]);
        }
    }
    else
    {
        printf_("Boot App: Failure occurred in boot sequence\n");
    }

    return (retVal);
}

static int32_t BootApp_RequestStageCores(uint8_t stageNum)
{
    uint32_t i;
    int32_t status = CSL_EFAIL;
    uint8_t stage = stageNum;

    for (i = 0; i < MAX_CORES_PER_STAGE; i++)
    {
        if (sbl_late_slave_core_stages_info[stage][i].tisci_proc_id != SBL_INVALID_ID)
        {
            status = Sciclient_procBootRequestProcessor(sbl_late_slave_core_stages_info[stage][i].tisci_proc_id,
                                                        SCICLIENT_SERVICE_WAIT_FOREVER);
            if (status != CSL_PASS)
            {
                printf_("Sciclient_procBootRequestProcessor, ProcId 0x%x...FAILED \n",
                        sbl_late_slave_core_stages_info[stage][i].tisci_proc_id);
                break;
            }
        }
    }

    return (status);
}

static int32_t BootApp_ReleaseStageCores(uint8_t stageNum)
{
    uint32_t i;
    int32_t status = CSL_EFAIL;
    uint8_t stage = stageNum;

    for (i = 0; i < MAX_CORES_PER_STAGE; i++)
    {
        if (sbl_late_slave_core_stages_info[stage][i].tisci_proc_id != SBL_INVALID_ID)
        {
            status = Sciclient_procBootReleaseProcessor(sbl_late_slave_core_stages_info[stage][i].tisci_proc_id,
                                                        TISCI_MSG_FLAG_AOP,
                                                        SCICLIENT_SERVICE_WAIT_FOREVER);
            if (status != CSL_PASS)
            {
                printf_("Sciclient_procBootReleaseProcessor, ProcId 0x%x...FAILED \n",
                        sbl_late_slave_core_stages_info[stage][i].tisci_proc_id);
                break;
            }
        }
    }

    return (status);
}

void BootApp_ArmR5PmuCntrInit()
{
    uint32_t val;
    CSL_armR5PmuCfg(0, 0, 1);
    /* Clear the overflow */
    val = CSL_armR5PmuReadCntrOverflowStatus();
    val &= 0x80000000;
    CSL_armR5PmuClearCntrOverflowStatus(val);
    CSL_armR5PmuCfgCntr(CSL_ARM_R5_PMU_CYCLE_COUNTER_NUM, CSL_ARM_R5_PMU_EVENT_TYPE_CYCLE_CNT);
    CSL_armR5PmuEnableAllCntrs(1);
    CSL_armR5PmuEnableCntr(CSL_ARM_R5_PMU_CYCLE_COUNTER_NUM, 1);
}

uint32_t BootApp_GetTimeInMicroSec(uint32_t pmuCntrVal)
{
    uint64_t mcu_clk_freq = SBL_MCU1_CPU0_FREQ_HZ;
    uint32_t cycles_per_usec = (mcu_clk_freq / 1000000);
    return (pmuCntrVal / cycles_per_usec);
}

/* Function to clean the MCU R5 cache for a given start address and given memory size */
void BootApp_McuDCacheClean(void *addr, uint32_t size)
{
    /* Invalidate by MVA */
    CSL_armR5CacheWbInv((const void *)addr, uint32_to_int32(size), (bool)TRUE);
}

void Sciserver_userMsgTask(void *arg0)
{
    uintptr_t key;
    uint32_t i = 0U;
    int32_t ret;
    Sciserver_taskData *utd = (Sciserver_taskData *) arg0;

    /* Set the pending State first */
    utd->state->state = SCISERVER_TASK_PENDING;

    /* Enter critical section */
    key = HwiP_disable();

    /*
     * Here we check if all Hwi are still masked. We do this in order to control
     * when we enable the interrupts for the secure proxy messages in the case
     * there are pending messages queued prior to the task starting up. This
     * only should be done once, so we protect access with the global state.
     */
    if (sciHwiMasked == 1U)
    {
        sciHwiMasked = 0;
        for (i = 0U; i < SCISERVER_ARRAY_SIZE(sciserver_hwi_list); i++) {
            Osal_EnableInterrupt(0,sciserver_hwi_list[i].irq_num);
        }
    }

    /* Leave critical section */
    HwiP_restore(key);
    printf_("Sciserver_userMsgTask, Task ID: %d\n", utd->task_id);
    ret = Sciserver_processtask(utd);
    if (ret != CSL_PASS)
    {

    }
    else
    {
        /*
         * This is a bit of a hack... using the task ID to pick the offset
         * for the gloabl interrupt data array. This is functional but can
         * be cleaned up.
         */
        Osal_EnableInterrupt(0U, sciserver_hwi_list[(2 * utd->task_id) +
                                                    ((int32_t) utd->state->current_buffer_idx)].irq_num);
    }
}

void Sciserver_userMsgHwiFxn(uintptr_t arg)
{
    Sciserver_hwiData *uhd = (Sciserver_hwiData *) arg;
    int32_t ret = CSL_PASS;
    bool soft_error = false;

    Osal_DisableInterrupt(0, uhd->irq_num);

    ret = Sciserver_interruptHandler(uhd, &soft_error);

    if ((ret != CSL_PASS) && (soft_error == true))
    {
        Osal_EnableInterrupt(0, uhd->irq_num);
    }
    else
    {
        Sciserver_userMsgTask(uhd);
    }

    Osal_ClearInterrupt(0, uhd->irq_num);
}

static int32_t Sciserver_initHwis(void)
{
    uint32_t i = 0U;
    int32_t ret = CSL_PASS;

    for (i = 0U; i < SCISERVER_ARRAY_SIZE(sciserver_hwi_list); i++) {
        OsalRegisterIntrParams_t    intrPrms;
        Osal_RegisterInterrupt_initParams(&intrPrms);
        intrPrms.corepacConfig.arg  = (uintptr_t) &sciserver_hwi_list[i];
        intrPrms.corepacConfig.isrRoutine = &Sciserver_userMsgHwiFxn;
        intrPrms.corepacConfig.enableIntr = FALSE;
        intrPrms.corepacConfig.corepacEventNum  = 0;
        intrPrms.corepacConfig.intVecNum = sciserver_hwi_list[i].irq_num;
        /* Register interrupts */
        ret = Osal_RegisterInterrupt(&intrPrms,&gSciserverHwiHandles[i]);
        if(OSAL_INT_SUCCESS != ret) {
            gSciserverHwiHandles[i] = NULL_PTR;
            break;
        }
    }

    return ret;
}

uint32_t BootApp_SetupSciServer(void)
{
    Sciserver_TirtosCfgPrms_t appPrms;
    Sciclient_ConfigPrms_t clientPrms;
    uint32_t ret = CSL_PASS;

    appPrms.taskPriority[SCISERVER_TASK_USER_LO] = 1;
    appPrms.taskPriority[SCISERVER_TASK_USER_HI] = 4;

    /* Sciclient needs to be initialized before Sciserver. Sciserver depends on
     * Sciclient API to execute message forwarding */
    ret = Sciclient_configPrmsInit(&clientPrms);
    if (ret == CSL_PASS)
    {
        ret = Sciclient_init(&clientPrms);
    }

#if 1
    if (ret == CSL_PASS)
    {
        ret = Sciserver_tirtosInit(&appPrms);
    }
    
#else
    Sciserver_CfgPrms_t prms;

    /* Initialize the Init Parameters for the Sciserver */
    if (ret == CSL_PASS)
    {
        ret = Sciserver_initPrms_Init(&prms);
    }
    /* Initialize the Sciserver */
    if (ret == CSL_PASS)
    {
        ret = Sciserver_init(&prms);
    }

    /* hwi initialization */
    if (ret == CSL_PASS) {
        ret = Sciserver_initHwis();
    }

    /* Set the process State */
    if (ret == CSL_PASS)
    {
        Sciserver_setCtrlState(SCISERVER_PROCESS_STATE_RUN);
        if (Sciserver_getCtrlState() != (uint8_t)SCISERVER_PROCESS_STATE_RUN)
        {
            ret = CSL_EFAIL;
        }
    }
#endif

    if (ret == CSL_PASS)
    {
        printf_("\nStarting Sciserver..... PASSED\n");
    }
    else
    {
        printf_("Starting Sciserver..... FAILED\n");
    }
    return ret;
}

Board_STATUS Board_pmPowerOff(uint32_t slaveAddr)
{
    return BOARD_SOK;
}
