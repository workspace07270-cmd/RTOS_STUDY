# 공통 FreeRTOS + QEMU 빌드 규칙입니다.
#
# 각 단계 예제의 Makefile은 TARGET과 APP_SRCS만 정하고 이 파일을 include합니다.
#
# 기본 폴더 구조:
#   rtos-kiosk-course
#     third_party/FreeRTOS-Kernel
#     day03/examples/common
#     day03/examples/step01-task-basic
#
# 만약 실습 폴더를 day03/step01-task-basic처럼 examples 없이 복사했다면
# make 실행 시 아래처럼 FreeRTOS Kernel 경로를 직접 넘길 수 있습니다.
#
#   make FREERTOS_KERNEL=../../third_party/FreeRTOS-Kernel

BUILD_DIR := build
# 다른 일차 예제에서도 공통 startup/linker/UART 코드를 재사용할 수 있도록
# 호출하는 Makefile이 COMMON_DIR을 지정한 경우 그 값을 유지합니다.
COMMON_DIR ?= ../common
FREERTOS_KERNEL ?= ../../third_party/FreeRTOS-Kernel

CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
QEMU := qemu-system-arm

# Cortex-M3는 Thumb 명령어 세트를 사용합니다.
CPU_FLAGS := -mcpu=cortex-m3 -mthumb

# -ffreestanding:
#   일반 운영체제 위의 C 프로그램이 아니라 bare-metal firmware임을 의미합니다.
# -fdata-sections, -ffunction-sections:
#   사용하지 않는 함수/데이터를 linker가 제거할 수 있게 섹션을 나눕니다.
CFLAGS := $(CPU_FLAGS) -Wall -Wextra -O0 -g -ffreestanding -fdata-sections -ffunction-sections
CFLAGS += -I.
CFLAGS += -Isrc
CFLAGS += -I$(COMMON_DIR)
CFLAGS += -I$(COMMON_DIR)/src
CFLAGS += -I$(FREERTOS_KERNEL)/include
CFLAGS += -I$(FREERTOS_KERNEL)/portable/GCC/ARM_CM3

# linker.ld는 QEMU 보드의 FLASH/RAM 배치를 정의합니다.
LDFLAGS := $(CPU_FLAGS) -T $(COMMON_DIR)/linker.ld -nostdlib -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map
# Cortex-M3에 하드웨어 나눗셈 명령이 없어 compiler helper가 필요할 수 있습니다.
LDLIBS ?= -lgcc

# 각 step의 application source입니다.
APP_OBJS := $(patsubst %.c,$(BUILD_DIR)/app/%.o,$(APP_SRCS))

# 세 단계가 공통으로 사용하는 부팅 코드, UART, hook, libc stub입니다.
COMMON_OBJS := $(BUILD_DIR)/common/startup.o \
               $(BUILD_DIR)/common/uart.o \
               $(BUILD_DIR)/common/libc_stubs.o \
               $(BUILD_DIR)/common/freertos_hooks.o

# 실제 FreeRTOS Kernel source입니다.
# 이 파일들이 빌드에 들어가기 때문에 3일차 예제는 진짜 FreeRTOS 실행입니다.
FREERTOS_OBJS := $(BUILD_DIR)/freertos/tasks.o \
                 $(BUILD_DIR)/freertos/queue.o \
                 $(BUILD_DIR)/freertos/list.o \
                 $(BUILD_DIR)/freertos/timers.o \
                 $(BUILD_DIR)/freertos/port.o \
                 $(BUILD_DIR)/freertos/heap_4.o

OBJS := $(APP_OBJS) $(COMMON_OBJS) $(FREERTOS_OBJS)

.PHONY: all run clean check-kernel

all: check-kernel $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

check-kernel:
	@test -f $(FREERTOS_KERNEL)/include/FreeRTOS.h || (echo "FreeRTOS Kernel not found: $(FREERTOS_KERNEL)" && exit 1)
	@test -f $(FREERTOS_KERNEL)/portable/GCC/ARM_CM3/port.c || (echo "ARM_CM3 port not found under $(FREERTOS_KERNEL)" && exit 1)

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(COMMON_DIR)/linker.ld
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/app/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common/startup.o: $(COMMON_DIR)/startup.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common/uart.o: $(COMMON_DIR)/src/uart.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common/libc_stubs.o: $(COMMON_DIR)/src/libc_stubs.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/common/freertos_hooks.o: $(COMMON_DIR)/src/freertos_hooks.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/freertos/tasks.o: $(FREERTOS_KERNEL)/tasks.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/freertos/queue.o: $(FREERTOS_KERNEL)/queue.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/freertos/list.o: $(FREERTOS_KERNEL)/list.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/freertos/timers.o: $(FREERTOS_KERNEL)/timers.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/freertos/port.o: $(FREERTOS_KERNEL)/portable/GCC/ARM_CM3/port.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/freertos/heap_4.o: $(FREERTOS_KERNEL)/portable/MemMang/heap_4.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BUILD_DIR)/$(TARGET).elf
	$(QEMU) \
		-M lm3s6965evb \
		-cpu cortex-m3 \
		-kernel $(BUILD_DIR)/$(TARGET).elf \
		-nographic \
		-monitor none \
		-serial stdio

clean:
	rm -rf $(BUILD_DIR)
