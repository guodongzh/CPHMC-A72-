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

project(sbl_qspi LANGUAGES C ASM)

# specifie linker script and add linker flag
file(GLOB LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/*.cmd)
include(./cmake/linker-flag.cmake)
add_link_options(-Wl,-m${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map -Wl,${LINKER_SCRIPT})


# add include path for source code
include_directories(inc)
include_directories(.)

header_directories(src)
header_directories(soc)

add_compile_definitions(SOC_J721E BUILD_MCU j721e_evm BUILD_MCU1_0 SBL_LOG_LEVEL=1
                        SBL_ENABLE_PLL SBL_ENABLE_CLOCKS SBL_ENABLE_DDR SBL_ENABLE_SERDES
                        SBL_USE_DMA=0 BOOT_OSPI MAX_APP_SIZE_EMMC=0x7D000
                        SBL_SCRATCH_MEM_START=0xC2000000 SBL_SCRATCH_MEM_SIZE=0x4000000
                        UART_DMA_ENABLE SPI_DMA_ENABLE BOARD_DDR_ENABLE_DDR_MEM_PRIME
)


#search source code files
file(GLOB_RECURSE SOURCES "src/*.c" "src/*.h" "inc/*.h" "soc/*.c" "soc/*.asm" "soc/*h"
    "ti/*.c" "ti/*.h" "ti/*.asm" "board/*.c" "board/*.h"
)
add_executable(${PROJECT_NAME} ${SOURCES}  ${LINKER_SCRIPT})

#search library
file(GLOB_RECURSE LIBS "lib/*.aer5f")
target_link_libraries(${PROJECT_NAME} ${LIBS})

#add_custom_command(TARGET ${PROJECT_NAME}  POST_BUILD
#        COMMAND make -f ../makefile all
#        COMMENT "generate image"
#)
