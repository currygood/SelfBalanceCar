# CMake config for BSP components
cmake_minimum_required(VERSION 3.22)
project(bsp C)

add_library(bsp STATIC
	DWT/DWT.h)

set(BSP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Collect C sources under BSP (recursive)
file(GLOB_RECURSE BSP_C_SRCS
	"${BSP_DIR}/*.c"
)
list(REMOVE_DUPLICATES BSP_C_SRCS)

if(NOT BSP_C_SRCS)
	message(STATUS "No BSP C sources found in ${BSP_DIR}")
endif()

target_sources(bsp PRIVATE ${BSP_C_SRCS})

# Collect existing include directories from well-known subfolders
set(POSSIBLE_INCLUDES
	${BSP_DIR}
	${BSP_DIR}/BLE_Serial
	${BSP_DIR}/I2C
	${BSP_DIR}/Moitor
	${BSP_DIR}/Ultrasonicwave_Communcation
	${BSP_DIR}/DWT
	${BSP_DIR}/RTOS_Communication
)
set(EXISTING_INCLUDES)
foreach(d IN LISTS POSSIBLE_INCLUDES)
	if(IS_DIRECTORY "${d}")
		list(APPEND EXISTING_INCLUDES "${d}")
	endif()
endforeach()

if(EXISTING_INCLUDES)
	target_include_directories(bsp PUBLIC ${EXISTING_INCLUDES})
endif()


set_target_properties(bsp PROPERTIES POSITION_INDEPENDENT_CODE OFF)

target_link_libraries(bsp PRIVATE stm32cubemx middlewares freertos)
