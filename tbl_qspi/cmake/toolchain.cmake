message(STATUS "Checking toolchain ...")

if(NOT TOOLCHAIN)
    find_path(_TOOLCHAIN tiarmclang.exe)
    global_set(TOOLCHAIN "${_TOOLCHAIN}")
elseif(NOT "${TOOLCHAIN}" MATCHES "/$")
    global_set(TOOLCHAIN "${TOOLCHAIN}")
endif()

if(NOT TOOLCHAIN)
    message(FATAL_ERROR "bin folder of toolchain must add to PATH environment variable")
endif()

message(STATUS "Using ${TOOLCHAIN} tiarmclang toolchain")

global_set(CMAKE_C_COMPILER tiarmclang)
global_set(CMAKE_CXX_COMPILER tiarmclang)
global_set(CMAKE_ASM_COMPILER tiarmclang)
global_set(CMAKE_AR tiarmar)
global_set(CMAKE_LINKER tiarmlnk)
global_set(CMAKE_OBJCOPY tiarmobjcopy)
