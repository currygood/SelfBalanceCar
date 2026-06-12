cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Balance_Car)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
# Add components (BSP, Middlewares, etc.) so their CMakeLists are processed
add_subdirectory(components/BSP)
add_subdirectory(components/Middlewares)
add_subdirectory(freertos)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/components
    ${CMAKE_SOURCE_DIR}/components/BSP/Moitor
    ${CMAKE_SOURCE_DIR}/components/BSP/I2C
    ${CMAKE_SOURCE_DIR}/components/BSP/BLE_Serial
    ${CMAKE_SOURCE_DIR}/components/BSP/Ultrasonicwave_Communcation
    ${CMAKE_SOURCE_DIR}/components/Middlewares/OLED
    ${CMAKE_SOURCE_DIR}/components/Middlewares/BLE
    ${CMAKE_SOURCE_DIR}/components/Middlewares/MPU6050
    ${CMAKE_SOURCE_DIR}/components/Middlewares/PID
    ${CMAKE_SOURCE_DIR}/components/Middlewares/Ultrasonicwave_Avoid
)

# Collect component sources and add component include paths so IDE/compiler can find headers like I2C_Bus.h
file(GLOB_RECURSE COMPONENT_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/components/*.c)
if(COMPONENT_SOURCES)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${COMPONENT_SOURCES})
endif()

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)



# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    freertos
    bsp
    middlewares
    # Add user defined libraries
)
