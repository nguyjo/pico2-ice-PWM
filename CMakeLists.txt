cmake_minimum_required(VERSION 3.13)

include(${CMAKE_CURRENT_SOURCE_DIR}/pico-ice-sdk/cmake/preinit_pico_ice_sdk.cmake)

# import the pico-sdk
set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk/)
include(pico_sdk_import.cmake)

# configure the pico-sdk project
project(rp2_ice_PWM C CXX ASM) # project(projectName, Languages...)
pico_sdk_init()

# add the pico-ice-sdk
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pico-ice-sdk)

# add the local files
add_executable(${CMAKE_PROJECT_NAME}
    main.c
    )
target_link_libraries(${CMAKE_PROJECT_NAME}
    pico_ice_sdk
    pico_stdlib
    hardware_spi    # Required for SPI communication
    hardware_i2c    # Required for I2C communicatio
    m
    )
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )
pico_add_extra_outputs(${CMAKE_PROJECT_NAME})
pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 0)
pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 0)

set(SV
    ${CMAKE_CURRENT_LIST_DIR}/top.sv
    ${CMAKE_CURRENT_LIST_DIR}/spi_slave.sv
)
add_custom_target(bitstream.h
    DEPENDS ${SV}
    COMMAND yosys -p 'synth_ice40 -abc9 -top top -json bitstream.json' ${SV}
    COMMAND nextpnr-ice40  --package sg48 --up5k --freq 48 --top top --pcf ${CMAKE_SOURCE_DIR}/pico_2ice.pcf --json bitstream.json --asc bitstream.asc
    COMMAND icepack bitstream.asc bitstream.bin
    COMMAND python ${CMAKE_CURRENT_LIST_DIR}/bin2header.py bitstream.bin >bitstream.h
    )
add_dependencies(${CMAKE_PROJECT_NAME} bitstream.h)
