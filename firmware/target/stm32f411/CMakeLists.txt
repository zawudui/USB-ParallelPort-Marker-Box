cmake_minimum_required(VERSION 3.16)
project(marker_box_target C)
if(NOT CMAKE_C_COMPILER MATCHES "arm-none-eabi-gcc")
    message(FATAL_ERROR "Configure with -DCMAKE_TOOLCHAIN_FILE=firmware/target/stm32f411/toolchain.cmake")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
add_executable(marker_box
    main.c
    startup.c
    bringup.c
    board_clock.c
    board_crit.c
    board_systick.c
    board_time.c
    ../../core/time_us.c
    board_gpio.c
    board_iwdg.c
    board_stop.c
    usb_descriptors.c
    usb_setup.c
    usb_dcd.c
    ../../core/power_sequence.c
    ../../core/runtime.c
    ../../src/marker_fsm.c
    ../../src/protocol_parser.c
    ../../src/protocol_dispatch.c)
set_target_properties(marker_box PROPERTIES SUFFIX ".elf" OUTPUT_NAME "marker_box")
option(MKR_USB_ONLY_BRINGUP "Keep isolation and marker outputs off and disable sleep for USB bring-up" OFF)
option(MKR_FUNCTIONAL_BRINGUP "Enable real power/marker functions for controlled bench tests; no RTC/IWDG/sleep" OFF)
option(MKR_BENCH_IWDG "Restore RTC/IWDG in functional bench mode, keeping the core awake" OFF)
option(MKR_BENCH_STOP "Restore Stop/wakeup in watchdog bench mode" OFF)
if(MKR_BENCH_STOP AND NOT MKR_BENCH_IWDG)
    message(FATAL_ERROR "MKR_BENCH_STOP requires MKR_BENCH_IWDG")
endif()
if(MKR_BENCH_IWDG AND NOT MKR_FUNCTIONAL_BRINGUP)
    message(FATAL_ERROR "MKR_BENCH_IWDG requires MKR_FUNCTIONAL_BRINGUP")
endif()
if(MKR_USB_ONLY_BRINGUP AND MKR_FUNCTIONAL_BRINGUP)
    message(FATAL_ERROR "Select only one bench bring-up mode")
endif()
if(MKR_USB_ONLY_BRINGUP)
    target_compile_definitions(marker_box PRIVATE MKR_USB_ONLY_BRINGUP=1)
endif()
if(MKR_FUNCTIONAL_BRINGUP)
    target_compile_definitions(marker_box PRIVATE MKR_FUNCTIONAL_BRINGUP=1)
endif()
if(MKR_BENCH_IWDG)
    target_compile_definitions(marker_box PRIVATE MKR_BENCH_IWDG=1)
endif()
if(MKR_BENCH_STOP)
    target_compile_definitions(marker_box PRIVATE MKR_BENCH_STOP=1)
endif()
target_include_directories(marker_box PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cmsis
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
target_compile_options(marker_box PRIVATE
    -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
    -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns
    -ffunction-sections -fdata-sections
    -Wall -Wextra -Werror -Wstrict-prototypes)
target_link_options(marker_box PRIVATE
    -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
    -T${CMAKE_CURRENT_SOURCE_DIR}/stm32f411ceux.ld
    -nostdlib -lgcc
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE_DIR:marker_box>/marker_box.map)
find_program(MKR_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(MKR_SIZE arm-none-eabi-size REQUIRED)
add_custom_command(TARGET marker_box POST_BUILD
    COMMAND ${MKR_OBJCOPY} -O ihex $<TARGET_FILE:marker_box> $<TARGET_FILE_DIR:marker_box>/marker_box.hex
    COMMAND ${MKR_OBJCOPY} -O binary $<TARGET_FILE:marker_box> $<TARGET_FILE_DIR:marker_box>/marker_box.bin
    COMMAND ${MKR_SIZE} $<TARGET_FILE:marker_box>
    VERBATIM)
