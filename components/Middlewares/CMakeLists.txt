# CMake config for Middlewares components
cmake_minimum_required(VERSION 3.22)
project(middlewares C)

add_library(middlewares STATIC)

set(MW_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Collect C sources (recursive) and assembly files
file(GLOB_RECURSE MIDDLEWARES_C_SRCS
	"${MW_DIR}/*.c"
)
file(GLOB_RECURSE MIDDLEWARES_ASM_SRCS
	"${MW_DIR}/*.s"
	"${MW_DIR}/*.S"
)

list(APPEND MIDDLEWARES_SOURCES ${MIDDLEWARES_C_SRCS} ${MIDDLEWARES_ASM_SRCS})
list(REMOVE_DUPLICATES MIDDLEWARES_SOURCES)

if(NOT MIDDLEWARES_SOURCES)
	message(STATUS "No Middlewares sources found in ${MW_DIR}")
endif()

# Mark assembly files as ASM language so CMake handles them correctly
set(MW_ASM_FILES)
foreach(f IN LISTS MIDDLEWARES_SOURCES)
	string(TOLOWER "${f}" _f_lower)
	if(_f_lower MATCHES "\\.s$" OR _f_lower MATCHES "\\.S$")
		list(APPEND MW_ASM_FILES "${f}")
	endif()
endforeach()

if(MW_ASM_FILES)
	set_source_files_properties(${MW_ASM_FILES} PROPERTIES LANGUAGE ASM)
endif()

target_sources(middlewares PRIVATE ${MIDDLEWARES_SOURCES})

# Include directories from known middleware subfolders
set(POSSIBLE_INCLUDES
	${MW_DIR}
	${MW_DIR}/BLE
	${MW_DIR}/MPU6050
	${MW_DIR}/OLED
	${MW_DIR}/PID
	${MW_DIR}/Ultrasonicwave_Avoid
)
set(EXISTING_INCLUDES)
foreach(d IN LISTS POSSIBLE_INCLUDES)
	if(IS_DIRECTORY "${d}")
		list(APPEND EXISTING_INCLUDES "${d}")
	endif()
endforeach()

if(EXISTING_INCLUDES)
	target_include_directories(middlewares PUBLIC ${EXISTING_INCLUDES})
endif()


set_target_properties(middlewares PROPERTIES POSITION_INDEPENDENT_CODE OFF)

target_link_libraries(middlewares PRIVATE stm32cubemx bsp)