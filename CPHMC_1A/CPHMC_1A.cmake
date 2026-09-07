

#search source code files

if (CORE_TYPE STREQUAL "R5F")
    file(GLOB_RECURSE CORE_SRC
            "../../CPHMC_1A/common/board/*.c"
            "../../CPHMC_1A/common/board/*.h"
            "../../CPHMC_1A/common/ti/drv/*.c"
            "../../CPHMC_1A/common/ti/drv/*.h"
    )
    include_directories(../../CPHMC_1A/common/ti/csl/arch/r5)

elseif (CORE_TYPE STREQUAL "C66x")
    file(GLOB_RECURSE CORE_SRC
            "../../CPHMC_1A/common/ti/csl/arch/c66x/*.c" 
            "../../CPHMC_1A/common/ti/csl/arch/c66x/*.asm"
            "../../CPHMC_1A/common/ti/osal/arch/*.c" 
            "../../CPHMC_1A/common/ti/osal/src/nonos/*.c"
    )
    include_directories(../../CPHMC_1A/common/ti/csl/arch/c66x)

elseif (CORE_TYPE STREQUAL "C71x")
    file(GLOB_RECURSE CORE_SRC
            "../../CPHMC_1A/common/ti/csl/arch/c7x/*.asm" 
            "../../CPHMC_1A/common/ti/csl/arch/c7x/*.c"
            "../../CPHMC_1A/common/ti/osal/arch/core/Core_utils.c" 
            "../../CPHMC_1A/common/ti/osal/arch/core/c7x/*.c"
            "../../CPHMC_1A/common/ti/freertos/*.asm" 
            "../../CPHMC_1A/common/ti/freertos/*.c"
            "../../CPHMC_1A/common/ti/osal/src/freertos/*.c" 
            "../../CPHMC_1A/common/board/*.c"
            "../../CPHMC_1A/common/board/*.h" 
            "../../CPHMC_1A/common/ti/drv/*.c" 
            "../../CPHMC_1A/common/ti/drv/*.h"
    )
    include_directories(../../CPHMC_1A/common/ti/csl/arch/c7x)

endif ()



# add include path for source code
include_directories(../../CPHMC_1A/common)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/SmartPLC/platform/inc)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/SmartPLC/inc)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/inc)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/platform)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/net_interface)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/flash_interface)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/platform/tcp)






