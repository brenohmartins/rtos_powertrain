# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(FreeRTOS_Pico2W C CXX ASM)

set(PICO_CXX_ENABLE_RTTI 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ==================== NOVA ORGANIZAÇÃO DOS ARQUIVOS ====================
# Lista todos os arquivos fonte
set(SOURCES
    main.c
    src/motor_control.c
    src/wifi_comm.c
    freertos_callbacks.c
)

# Add executable com os fontes organizados
add_executable(FreeRTOS_Pico2W ${SOURCES})

pico_set_program_name(FreeRTOS_Pico2W "FreeRTOS_Pico2W")
pico_set_program_version(FreeRTOS_Pico2W "0.1")

# Generate PIO header (se ainda precisar)
pico_generate_pio_header(FreeRTOS_Pico2W ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(FreeRTOS_Pico2W 1)
pico_enable_stdio_usb(FreeRTOS_Pico2W 1)

# ==================== DIRETÓRIOS DE INCLUDE ====================
# Adiciona os diretórios de include (inc/ e a raiz)
target_include_directories(FreeRTOS_Pico2W PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}      # Para FreeRTOSConfig.h
    ${CMAKE_CURRENT_LIST_DIR}/inc   # Para nossos headers
)

# ==================== BIBLIOTECAS ====================
target_link_libraries(FreeRTOS_Pico2W 
    pico_stdlib
    hardware_spi
    hardware_i2c
    hardware_dma
    hardware_pio
    hardware_interp
    hardware_timer
    hardware_watchdog
    hardware_clocks
    hardware_pwm
    hardware_gpio
    pico_cyw43_arch_lwip_sys_freertos
)

pico_add_extra_outputs(FreeRTOS_Pico2W)

# ==================== FREERTOS ====================
set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Kernel-main")
include(FreeRTOS_Kernel_import.cmake)

# FREERTOS: FreeRTOSConfig.h needs to be in the include path
target_include_directories(FreeRTOS_Pico2W PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Add the FreeRTOS library to the build
target_link_libraries(FreeRTOS_Pico2W
    FreeRTOS-Kernel-Heap4
)