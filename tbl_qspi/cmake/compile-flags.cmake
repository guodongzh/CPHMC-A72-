
add_compile_flags(BOTH -march=armv7r -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -mlittle-endian -marm)

add_compile_flags(C -ffunction-sections -fdata-sections -fno-common -Wno-gnu-variable-sized-type-not-at-end)

add_compile_flags(ASM -xti-asm)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_flags(C -Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_flags(C -Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_flags(C -Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_flags(C -O0)
endif ()



