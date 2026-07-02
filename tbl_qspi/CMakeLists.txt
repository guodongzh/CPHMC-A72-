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

project(boot_app LANGUAGES C ASM)

# specifie linker script and add linker flag
file(GLOB LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/*.cmd)
include(./cmake/linker-flag.cmake)
add_link_options(-Wl,-m${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map -Wl,${LINKER_SCRIPT})


set(PDK_PATH C:/ti/ti-processor-sdk-rtos-j721e-evm-09_02_00_05/pdk_jacinto_09_02_00_30/packages)
include_directories(${PDK_PATH})
include_directories(ti/csl/arch/r5)
include_directories(ti/boot/sbl/soc/k3)
include_directories(ti/freertos/include)
include_directories(ti/freertos/portable/TI_CGT/r5f)
include_directories(ti/freertos/config/j721e/r5f)
include_directories(inc)
include_directories(common/middleware/file_system/include)
include_directories(common/middleware/ipc_common/include)
include_directories(common/middleware/irig_b/include)
include_directories(.)

include_directories(./optional_lib/PSCode_lib_prog)
include_directories(./optional_lib/PSCode_lib_prog/SmartPLC/platform/inc)
include_directories(./optional_lib/PSCode_lib_prog/SmartPLC/inc)
include_directories(./optional_lib/PSCode_lib_prog/PSCode)
include_directories(./optional_lib/PSCode_lib_prog/PSCode/inc)
include_directories(./optional_lib/PSCode_lib_prog/PSCode/platform)
include_directories(./optional_lib/PSCode_lib_prog/PSCode/net_interface)
include_directories(./optional_lib/PSCode_lib_prog/PSCode/flash_interface)
include_directories(./optional_lib/PSCode_lib_prog/PSCode/platform/tcp)

header_directories(common)
header_directories(src)
header_directories(soc)

add_compile_definitions(SOC_J721E j721e_evm BUILD_MCU BUILD_MCU1_0 BOOT_OSPI OSPI_ONLY SBL_LOG_LEVEL=3
        MPU1_HLOS_BOOT_ENABLED FREERTOS)


file(GLOB_RECURSE SOURCES "src/*.c" "src/*.h" "inc/*.h" "soc/*.c" "soc/*.asm" "soc/*h" "ti/*c" "ti/*h" "ti/*.asm"
                            "board/*.c" "board/*.h" "common/*.c" "common/*.h" "optional_lib/*.c" "optional_lib/*.h"
)
add_executable(${PROJECT_NAME} ${SOURCES}  ${LINKER_SCRIPT})

file(GLOB_RECURSE LIBS "lib/*.aer5f")
target_link_libraries(${PROJECT_NAME} ${LIBS})


