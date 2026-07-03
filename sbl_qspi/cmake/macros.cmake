

#macro：define cache value
macro(global_set Name Value)
    set(${Name} "${Value}" CACHE STRING "NoDesc" FORCE)
endmacro()


#macro：Recursive addition of header files
macro(header_directories parent)
    file(GLOB_RECURSE newList ${parent}/*.h)
    set(dir_list "")
    foreach (file_path ${newList})
        get_filename_component(dir_path ${file_path} DIRECTORY)
        set(dir_list ${dir_list} ${dir_path})
    endforeach ()
    list(REMOVE_DUPLICATES dir_list)

    include_directories(${dir_list})
endmacro()

function(JOIN VALUES GLUE OUTPUT)
    string(REGEX REPLACE "([^\\]|^);" "\\1${GLUE}" _TMP_STR "${VALUES}")
    string(REGEX REPLACE "[\\](.)" "\\1" _TMP_STR "${_TMP_STR}") #fixes escaping
    set(${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

# Empty default variable
global_set(CMAKE_C_FLAGS "")
global_set(CMAKE_CXX_FLAGS "")
global_set(CMAKE_ASM_FLAGS "")
global_set(CMAKE_EXE_LINKER_FLAGS "")

#macro add compile flag
macro(add_compile_flags WHERE)
    JOIN("${ARGN}" " " STRING_ARGS)
    if (${WHERE} STREQUAL C)
        global_set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STRING_ARGS}")
    elseif (${WHERE} STREQUAL ASM)
        global_set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${STRING_ARGS}")
    elseif (${WHERE} STREQUAL LD)
        global_set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${STRING_ARGS}")
    elseif (${WHERE} STREQUAL BOTH)
        add_compile_flags(C ${ARGN})
        add_compile_flags(ASM ${ARGN})
    elseif (${WHERE} STREQUAL DEFINE)
        JOIN("${ARGN}" " -D" STRING_ARGS)
        set(STRING_ARGS "-D${STRING_ARGS}")
        add_compile_flags(C ${STRING_ARGS})
    else ()
        message(FATAL_ERROR "add_compile_flags - only support: C, BOTH, LD, DEFINE")
    endif ()
endmacro()
