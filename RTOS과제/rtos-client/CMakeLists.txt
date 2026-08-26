cmake_minimum_required(VERSION 3.16)
project(day04_rtos_task_bridge C)
set(CMAKE_C_STANDARD 11)
if(DEFINED ENV{FREERTOS_KERNEL_PATH})
    set(FREERTOS_KERNEL_PATH "$ENV{FREERTOS_KERNEL_PATH}")
elseif(EXISTS "$ENV{HOME}/FreeRTOS-Kernel/CMakeLists.txt")
    set(FREERTOS_KERNEL_PATH "$ENV{HOME}/FreeRTOS-Kernel")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/FreeRTOS-Kernel/CMakeLists.txt")
    # WSL 배포 구조: rtos-kiosk-course/day5/rtos-client
    set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/FreeRTOS-Kernel")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party/FreeRTOS-Kernel/CMakeLists.txt")
    set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../third_party/FreeRTOS-Kernel")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/FreeRTOS-Kernel/CMakeLists.txt")
    set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/FreeRTOS-Kernel")
else()
    message(FATAL_ERROR
        "FreeRTOS-Kernel not found. Run:\n"
        "  export FREERTOS_KERNEL_PATH=/actual/path/to/FreeRTOS-Kernel\n"
        "Then verify:\n"
        "  test -f $FREERTOS_KERNEL_PATH/CMakeLists.txt && echo OK")
endif()
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config")
target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)
set(FREERTOS_PORT "GCC_POSIX" CACHE STRING "" FORCE)
set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
add_subdirectory("${FREERTOS_KERNEL_PATH}" freertos-kernel)
find_package(Threads REQUIRED)
add_executable(day04_rtos src/main.c src/http_client.c)
target_include_directories(day04_rtos PRIVATE src)
target_compile_options(day04_rtos PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(day04_rtos PRIVATE freertos_kernel freertos_config Threads::Threads)

