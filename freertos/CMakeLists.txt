cmake_minimum_required(VERSION 3.13)
project(freertos C ASM)

# 创建静态库目标
add_library(freertos STATIC)

set(FREERTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# 递归收集存在的 C 源
file(GLOB_RECURSE FREERTOS_C_SRCS
    "${FREERTOS_DIR}/src/*.c"
    "${FREERTOS_DIR}/por/*.c"
    "${FREERTOS_DIR}/*.c"
)

# 收集可能的 port 源（包括汇编）
file(GLOB_RECURSE FREERTOS_PORT_CANDIDATES
    "${FREERTOS_DIR}/**/port.*"
)

# 过滤 port 候选，只保留 .c .s .S .asm
set(FREERTOS_PORT_SRCS)
foreach(f IN LISTS FREERTOS_PORT_CANDIDATES)
    string(TOLOWER "${f}" _f_lower)
    if(_f_lower MATCHES "\\.c$" OR _f_lower MATCHES "\\.s$" OR _f_lower MATCHES "\\.S$" OR _f_lower MATCHES "\\.asm$")
        list(APPEND FREERTOS_PORT_SRCS "${f}")
    endif()
endforeach()

# 合并并去重
list(APPEND FREERTOS_SOURCES ${FREERTOS_C_SRCS} ${FREERTOS_PORT_SRCS})
list(REMOVE_DUPLICATES FREERTOS_SOURCES)

# 确保至少有一个源文件可用
if(NOT FREERTOS_SOURCES)
    message(FATAL_ERROR "No FreeRTOS source files found under ${FREERTOS_DIR}. Please check the FreeRTOS tree or adjust CMakeLists.txt.")
endif()

# 为汇编文件显式设置 LANGUAGE 属性（以便 CMake 正确处理）
set(FREERTOS_ASM_FILES)
foreach(f IN LISTS FREERTOS_SOURCES)
    string(TOLOWER "${f}" _f_lower)
    if(_f_lower MATCHES "\\.s$" OR _f_lower MATCHES "\\.asm$")
        list(APPEND FREERTOS_ASM_FILES "${f}")
    endif()
endforeach()

if(FREERTOS_ASM_FILES)
    set_source_files_properties(${FREERTOS_ASM_FILES} PROPERTIES LANGUAGE ASM)
endif()

target_sources(freertos PRIVATE ${FREERTOS_SOURCES})

# 仅添加存在的 include 路径
set(POSSIBLE_INCLUDES
    ${FREERTOS_DIR}/inc
    ${FREERTOS_DIR}/port
    ${FREERTOS_DIR}/src
    ${FREERTOS_DIR}
)
set(EXISTING_INCLUDES)
foreach(d IN LISTS POSSIBLE_INCLUDES)
    if(IS_DIRECTORY "${d}")
        list(APPEND EXISTING_INCLUDES "${d}")
    endif()
endforeach()

target_include_directories(freertos PUBLIC ${EXISTING_INCLUDES})

# 针对 ARM Cortex-M3 的编译选项（GCC/Clang）
target_compile_options(freertos PRIVATE
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>>:-mcpu=cortex-m3;-mthumb;-Os;-ffunction-sections;-fdata-sections>
)

# 静态库不需要 PIC
set_target_properties(freertos PROPERTIES POSITION_INDEPENDENT_CODE OFF)