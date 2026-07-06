cmake_minimum_required(VERSION 3.22)
set(CMAKE_SYSTEM_NAME Generic)

# import custom cmake macro
include(./cmake/macros.cmake)

# specifie toolchain
include(./cmake/toolchain.cmake)

# add compile flag
include(./cmake/compile-flags.cmake)
set(CMAKE_C_STANDARD 99)

# When checking the compiler, the type of the compiled library
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# export compile command json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(tbl_qspi LANGUAGES C ASM)

# specifie linker script and add linker flag
file(GLOB LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/*.cmd)
include(./cmake/linker-flag.cmake)
add_link_options(-Wl,-m${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map -Wl,${LINKER_SCRIPT})

include_directories(.)
include_directories(inc)

header_directories(src)
header_directories(soc)

include_directories(../../CPHMC_1A/common)
include_directories(ti/csl/arch/r5)
include_directories(ti/boot/sbl/soc/k3)
include_directories(ti/freertos/include)
include_directories(ti/freertos/portable/TI_CGT/r5f)
include_directories(ti/freertos/config/j721e/r5f)
include_directories(common/middleware/file_system/include)
include_directories(common/middleware/ipc_common/include)
include_directories(common/middleware/irig_b/include)

include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/SmartPLC/platform/inc)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/SmartPLC/inc)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/inc)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/platform)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/net_interface)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/flash_interface)
include_directories(../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/platform/tcp)

add_compile_definitions(SOC_J721E j721e_evm BUILD_MCU BUILD_MCU1_0 BOOT_OSPI OSPI_ONLY SBL_LOG_LEVEL=3
        MPU1_HLOS_BOOT_ENABLED FREERTOS)


file(GLOB_RECURSE SOURCES "src/*.c" "src/*.h" "inc/*.h" "soc/*.c" "soc/*.asm" "soc/*h" "ti/*c" "ti/*h" "ti/*.asm"
                            "board/*.c" "board/*.h" "common/*.c" "common/*.h" "optional_lib/*.c" "optional_lib/*.h"
        "../../CPHMC_1A/empty_CPHMC_1A.c"
        "../../CPHMC_1A/common/middleware/platform/src/*.c"
        "../../CPHMC_1A/common/middleware/net_protocol_prog/*.c"
        "../../CPHMC_1A/common/middleware/file_system/*.c"
        "../../CPHMC_1A/common/middleware/enet_ipc/*.c"
        "../../CPHMC_1A/common/middleware/ipc_common/*.c"
        "../../CPHMC_1A/common/middleware/report_manage/*.c"
        "../../CPHMC_1A/common/middleware/setting_manage/*.c"
        "../../CPHMC_1A/common/middleware/irig_b/*.c"
        "../../CPHMC_1A/common/middleware/cfg_file_prase/*.c"
        "../../CPHMC_1A/optional_lib/SpecialFWLib/*.h"
        "../../CPHMC_1A/optional_lib/SpecialFWLib/*.c"
        "../../CPHMC_1A/optional_lib/PSCode_lib_prog/SmartPLC/*h"
        "../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/*.c"
        "../../CPHMC_1A/optional_lib/PSCode_lib_prog/PSCode/*.h"
        "../../CPHMC_1A/optional_lib/Time_Sync_lib_prog/*.c"
        "../../CPHMC_1A/optional_lib/Time_Sync_lib_prog/*.h"
)
add_executable(${PROJECT_NAME} ${SOURCES}  ${LINKER_SCRIPT})

file(GLOB_RECURSE LIBS "lib/*.aer5f")
target_link_libraries(${PROJECT_NAME} ${LIBS})


