# QEMU 첫 실행 샘플용 Makefile입니다.
#
# 목표:
#   1. ARM Cortex-M3용 ELF firmware를 만든다.
#   2. 만든 ELF를 QEMU의 lm3s6965evb 가상 보드에서 실행한다.
#
# 사용:
#   make       -> build/qemu_hello.elf 생성
#   make run   -> QEMU에서 firmware 실행
#   make clean -> build 폴더 삭제

# 최종 결과물 이름입니다.
# build/qemu_hello.elf, build/qemu_hello.bin, build/qemu_hello.map이 생성됩니다.
TARGET := qemu_hello

# 컴파일 결과물을 모아둘 폴더입니다.
# 소스 코드와 빌드 결과를 분리하면 재빌드와 정리가 쉬워집니다.
BUILD_DIR := build

# ARM Cortex-M 같은 bare-metal 타깃은 PC용 gcc가 아니라 cross compiler를 사용합니다.
# arm-none-eabi-gcc:
#   arm  -> ARM 아키텍처
#   none -> 운영체제 ABI 없음
#   eabi -> embedded ABI
CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
QEMU := qemu-system-arm

# QEMU에서 실행할 보드가 Cortex-M3 계열이므로 CPU와 thumb 명령어 모드를 지정합니다.
CPU_FLAGS := -mcpu=cortex-m3 -mthumb

# -ffreestanding:
#   표준 OS 환경이 아니라는 뜻입니다. main 이전에 OS가 초기화해주는 환경이 없습니다.
# -nostdlib:
#   C 표준 라이브러리와 시작 파일을 자동으로 붙이지 않습니다.
#   그래서 startup.c와 linker.ld를 직접 제공합니다.
CFLAGS := $(CPU_FLAGS) -Wall -Wextra -O0 -g -ffreestanding -nostdlib

# -T linker.ld:
#   우리가 작성한 메모리 배치 규칙을 사용합니다.
# -Wl,-Map=...:
#   어떤 심볼과 섹션이 어디에 배치되었는지 map 파일로 남깁니다.
LDFLAGS := $(CPU_FLAGS) -T linker.ld -nostdlib -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

# startup.c:
#   Cortex-M 부팅에 필요한 vector table과 reset handler를 제공합니다.
# src/main.c:
#   UART로 로그를 출력하는 수업용 애플리케이션 코드입니다.
SRCS := startup.c src/main.c
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# all, run, clean은 실제 파일명이 아니라 명령 타깃입니다.
.PHONY: all run clean

# 기본 타깃입니다. 그냥 make를 입력하면 ELF와 BIN을 만듭니다.
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

# ELF는 QEMU가 -kernel 옵션으로 직접 실행할 수 있는 firmware 파일입니다.
$(BUILD_DIR)/$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

# BIN은 순수 바이너리 이미지입니다.
# 이번 수업에서는 ELF를 QEMU로 실행하지만, BIN도 firmware 결과물 이해를 위해 생성합니다.
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# 각 .c 파일을 build 폴더 아래의 .o 파일로 컴파일합니다.
# 예: src/main.c -> build/src/main.o
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# QEMU에서 firmware를 실행합니다.
# -M lm3s6965evb:
#   Luminary Micro LM3S6965 Evaluation Board를 에뮬레이션합니다.
# -cpu cortex-m3:
#   Cortex-M3 CPU로 실행합니다.
# -kernel:
#   방금 빌드한 ELF 파일을 로드합니다.
# -nographic:
#   별도 GUI 창 없이 터미널에서 실행합니다.
# -serial stdio:
#   UART 출력을 현재 터미널 표준 입출력에 연결합니다.
run: $(BUILD_DIR)/$(TARGET).elf
	$(QEMU) \
		-M lm3s6965evb \
		-cpu cortex-m3 \
		-kernel $(BUILD_DIR)/$(TARGET).elf \
		-nographic \
		-monitor none \
		-serial stdio

# 빌드 결과물을 삭제합니다.
# 현재 폴더가 qemu-first-sample인지 확인한 뒤 실행하세요.
clean:
	rm -rf $(BUILD_DIR)
