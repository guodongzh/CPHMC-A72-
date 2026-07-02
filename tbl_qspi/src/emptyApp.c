#include <string.h>
#include "pscGlobal.h"
#include <oem/rxfb_api.h>

/* --- Variable initialization --- */

/* Init function to apply initial values for connectors - for all tasks of all CFC files */
PSCBYTE __attribute__((weak)) Application_Init() { return kPscSuccess; }

/* --- Init mode (all tasks) --- */

/* Function declarations */

/* I mode function for the resource (all tasks) */
PSCBYTE __attribute__((weak)) Application_AllTasks_I()
{
    return kPscSuccess;
}

/* --- I- and T-tasks system and normal mode --- */

/* Function declarations */

/* I1S function for the resource */
PSCBYTE __attribute__((weak)) Application_I1_S() { return kPscSuccess; }

/* I1FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I1_FI() { return kPscSuccess; }
#endif

/* I1N function for the resource */
uint8_t open_pre = 0;
uint8_t open_cnt = 0;
PSCBYTE __attribute__((weak)) Application_I1_N()
{
    return kPscSuccess;
}

/* I1FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I1_FE() { return kPscSuccess; }
#endif

/* I2S function for the resource */
PSCBYTE __attribute__((weak)) Application_I2_S() { return kPscSuccess; }

/* I2FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I2_FI() { return kPscSuccess; }
#endif

/* I2N function for the resource */
PSCBYTE __attribute__((weak)) Application_I2_N() { return kPscSuccess; }

/* I2FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I2_FE() { return kPscSuccess; }
#endif

/* I3S function for the resource */
PSCBYTE __attribute__((weak)) Application_I3_S() { return kPscSuccess; }

/* I3FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I3_FI() { return kPscSuccess; }
#endif

/* I3N function for the resource */
PSCBYTE __attribute__((weak)) Application_I3_N() { return kPscSuccess; }

/* I3FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I3_FE() { return kPscSuccess; }
#endif

/* I4S function for the resource */
PSCBYTE __attribute__((weak)) Application_I4_S() { return kPscSuccess; }

/* I4FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I4_FI() { return kPscSuccess; }
#endif

/* I4N function for the resource */
PSCBYTE __attribute__((weak)) Application_I4_N() { return kPscSuccess; }

/* I4FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I4_FE() { return kPscSuccess; }
#endif

/* I5S function for the resource */
PSCBYTE __attribute__((weak)) Application_I5_S() { return kPscSuccess; }

/* I5FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I5_FI() { return kPscSuccess; }
#endif

/* I5N function for the resource */
PSCBYTE __attribute__((weak)) Application_I5_N() { return kPscSuccess; }

/* I5FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I5_FE() { return kPscSuccess; }
#endif

/* I6S function for the resource */
PSCBYTE __attribute__((weak)) Application_I6_S() { return kPscSuccess; }

/* I6FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I6_FI() { return kPscSuccess; }
#endif

/* I6N function for the resource */
PSCBYTE __attribute__((weak)) Application_I6_N() { return kPscSuccess; }

/* I6FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I6_FE() { return kPscSuccess; }
#endif

/* I7S function for the resource */
PSCBYTE __attribute__((weak)) Application_I7_S() { return kPscSuccess; }

/* I7FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I7_FI() { return kPscSuccess; }
#endif

/* I7N function for the resource */
PSCBYTE __attribute__((weak)) Application_I7_N() { return kPscSuccess; }

/* I7FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I7_FE() { return kPscSuccess; }
#endif

/* I8S function for the resource */
PSCBYTE __attribute__((weak)) Application_I8_S() { return kPscSuccess; }

/* I8FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I8_FI() { return kPscSuccess; }
#endif

/* I8N function for the resource */
PSCBYTE __attribute__((weak)) Application_I8_N() { return kPscSuccess; }

/* I8FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_I8_FE() { return kPscSuccess; }
#endif

/* T1S function for the resource */
PSCBYTE __attribute__((weak)) Application_T1_S() { return kPscSuccess; }

/* T1FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_T1_FI() { return kPscSuccess; }
#endif

/* T1CI function for the resource */
#ifdef USE_DATA_CONSISTENCY_SHM
PSCPUBLIC32 PSCBYTE PSCPUBLIC __attribute__((weak)) Application_T1_CI() { return kPscSuccess; }
#endif

/* T1N function for the resource */
PSCBYTE __attribute__((weak)) Application_T1_N() { return kPscSuccess; }

/* T1CE function for the resource */
#ifdef USE_DATA_CONSISTENCY_SHM
PSCPUBLIC32 PSCBYTE PSCPUBLIC __attribute__((weak)) Application_T1_CE() { return kPscSuccess; }
#endif

/* T1FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_T1_FE() { return kPscSuccess; }
#endif

/* T2S function for the resource */
PSCBYTE __attribute__((weak)) Application_T2_S() { return kPscSuccess; }

/* T2FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_T2_FI() { return kPscSuccess; }
#endif

/* T2CI function for the resource */
#ifdef USE_DATA_CONSISTENCY_SHM
PSCPUBLIC32 PSCBYTE PSCPUBLIC __attribute__((weak)) Application_T2_CI() { return kPscSuccess; }
#endif

/* T2N function for the resource */
PSCBYTE __attribute__((weak)) Application_T2_N() { return kPscSuccess; }

/* T2CE function for the resource */
#ifdef USE_DATA_CONSISTENCY_SHM
PSCPUBLIC32 PSCBYTE PSCPUBLIC __attribute__((weak)) Application_T2_CE() { return kPscSuccess; }
#endif

/* T2FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_T2_FE() { return kPscSuccess; }
#endif

/* T3S function for the resource */
PSCBYTE __attribute__((weak)) Application_T3_S() { return kPscSuccess; }

/* T3FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_T3_FI() { return kPscSuccess; }
#endif

/* T3CI function for the resource */
#ifdef USE_DATA_CONSISTENCY_SHM
PSCPUBLIC32 PSCBYTE PSCPUBLIC __attribute__((weak)) Application_T3_CI() { return kPscSuccess; }
#endif

/* T3N function for the resource */
PSCBYTE __attribute__((weak)) Application_T3_N() { return kPscSuccess; }

/* T3CE function for the resource */
#ifdef USE_DATA_CONSISTENCY_SHM
PSCPUBLIC32 PSCBYTE PSCPUBLIC __attribute__((weak)) Application_T3_CE() { return kPscSuccess; }
#endif

/* T3FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_T3_FE() { return kPscSuccess; }
#endif

/* T4S function for the resource */
PSCBYTE __attribute__((weak)) Application_T4_S() { return kPscSuccess; }

/* T4FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_T4_FI() { return kPscSuccess; }
#endif

/* T4CI function for the resource */
#ifdef USE_DATA_CONSISTENCY_SHM
PSCPUBLIC32 PSCBYTE PSCPUBLIC __attribute__((weak)) Application_T4_CI() { return kPscSuccess; }
#endif

/* T4N function for the resource */
PSCBYTE __attribute__((weak)) Application_T4_N() { return kPscSuccess; }

/* T4CE function for the resource */
#ifdef USE_DATA_CONSISTENCY_SHM
PSCPUBLIC32 PSCBYTE PSCPUBLIC __attribute__((weak)) Application_T4_CE() { return kPscSuccess; }
#endif

/* T4FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_T4_FE() { return kPscSuccess; }
#endif

/* T5S function for the resource */
PSCBYTE __attribute__((weak)) Application_T5_S() { return kPscSuccess; }

/* T5FI function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_T5_FI() { return kPscSuccess; }
#endif

/* T5CI function for the resource */
#ifdef USE_DATA_CONSISTENCY_SHM
PSCPUBLIC32 PSCBYTE PSCPUBLIC __attribute__((weak)) Application_T5_CI() { return kPscSuccess; }
#endif

/* T5N function for the resource */
PSCBYTE __attribute__((weak)) Application_T5_N() { return kPscSuccess; }

/* T5CE function for the resource */
#ifdef USE_DATA_CONSISTENCY_SHM
PSCPUBLIC32 PSCBYTE PSCPUBLIC __attribute__((weak)) Application_T5_CE() { return kPscSuccess; }
#endif

/* T5FE function for the resource */
#ifdef USE_SHARED_MEMORY
PSCBYTE __attribute__((weak)) Application_T5_FE() { return kPscSuccess; }
#endif

/* --- System initialization --- */

/* Init function to apply task settings */
PSCBYTE __attribute__((weak)) Application_InitTaskSettings()
{
    taskConfig_g.dwT0Interval = 1000;

    taskConfig_g.dwT1Factor = 1;
    taskConfig_g.dwT2Factor = 2;
    taskConfig_g.dwT3Factor = 4;
    taskConfig_g.dwT4Factor = 8;
    taskConfig_g.dwT5Factor = 16;

    taskConfig_g.dwI1SamplingTime = 100000;
    taskConfig_g.bI1Source = 0;
    taskConfig_g.dwI2SamplingTime = 100000;
    taskConfig_g.bI2Source = 0;
    taskConfig_g.dwI3SamplingTime = 100000;
    taskConfig_g.bI3Source = 0;
    taskConfig_g.dwI4SamplingTime = 100000;
    taskConfig_g.bI4Source = 0;
    taskConfig_g.dwI5SamplingTime = 100000;
    taskConfig_g.bI5Source = 0;
    taskConfig_g.dwI6SamplingTime = 100000;
    taskConfig_g.bI6Source = 0;
    taskConfig_g.dwI7SamplingTime = 100000;
    taskConfig_g.bI7Source = 0;
    taskConfig_g.dwI8SamplingTime = 100000;
    taskConfig_g.bI8Source = 0;

    return kPscSuccess;
}

/* Init function to apply resource version information */
PSCBYTE __attribute__((weak)) Application_InitResVersion()
{
    strncpy(resVersion_g.m_szPrjName, "Empty", SIZE_PROJECT_NAME);
    strncpy(resVersion_g.m_szResName, "MS1", SIZE_RESOURCE_NAME);

    resVersion_g.m_PlcVer.m_TimeStamp = 0;
    resVersion_g.m_dwBuildDate = 0;
    resVersion_g.m_dwLoadDate = 0;

#ifdef USE_PLATFORM_NAME
    strncpy(resVersion_g.m_szPlatformName, "XJ_MS3000", SIZE_PLATFORM_NAME);
#endif

#ifdef USE_PROJECT_PASSWORD
    strncpy(resVersion_g.m_szPasswordDeveloper, "", SIZE_PASSWORD);
    strncpy(resVersion_g.m_szPasswordProject, "", SIZE_PASSWORD);
    strncpy(resVersion_g.m_szPasswordCustomer, "", SIZE_PASSWORD);
#endif

    resVersion_g.m_dwNumAppLibs = 0;
    resVersion_g.m_pAppLibInfo = PSCNULL;

    return kPscSuccess;
}

/* Init function to set data consistency control variables */
PSCBYTE __attribute__((weak)) Application_InitDataConsistency()
{
#ifdef USE_DATA_CONSISTENCY
    array_dwOutputBufferSize[kI1Task] = 0U;
    array_dwOutputBufferSize[kI2Task] = 0U;
    array_dwOutputBufferSize[kI3Task] = 0U;
    array_dwOutputBufferSize[kI4Task] = 0U;
    array_dwOutputBufferSize[kI5Task] = 0U;
    array_dwOutputBufferSize[kI6Task] = 0U;
    array_dwOutputBufferSize[kI7Task] = 0U;
    array_dwOutputBufferSize[kI8Task] = 0U;
    array_dwOutputBufferSize[kT1Task] = 0U;
    array_dwOutputBufferSize[kT2Task] = 0U;
    array_dwOutputBufferSize[kT3Task] = 0U;
    array_dwOutputBufferSize[kT4Task] = 0U;
    array_dwOutputBufferSize[kT5Task] = 0U;

    fUsesDataConsistency = PSCFALSE;
#endif

    return kPscSuccess;
}

/* --- Shared memory configuration --- */

#ifdef USE_DATA_CONSISTENCY_SHM
PSCBYTE __attribute__((weak)) Application_InitSharedMemoryDataConsistency() { return kPscSuccess; }
#endif
