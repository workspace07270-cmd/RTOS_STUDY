# QEMU 첫 실행 샘플

이 샘플은 WSL2 Ubuntu에서 ARM Cortex-M3 가상 보드(`lm3s6965evb`)를 QEMU로 실행하는 최소 예제입니다.

중요: 이 샘플은 FreeRTOS가 아닙니다. 3일차 FreeRTOS 실습 전에 QEMU와 ARM GCC가 정상 동작하는지 확인하기 위한 bare-metal firmware입니다.

## 파일 구성

| 파일 | 설명 |
|---|---|
| `Makefile` | ARM firmware 빌드와 QEMU 실행 명령 |
| `startup.c` | Cortex-M3 vector table과 reset handler |
| `linker.ld` | FLASH/RAM 메모리 배치 |
| `src/main.c` | UART 로그 출력 예제 |

## 빌드

```bash
make
```

생성 파일:

```text
build/qemu_hello.elf
build/qemu_hello.bin
build/qemu_hello.map
```

## 실행

```bash
make run
```

직접 실행한다면:

```bash
qemu-system-arm \
  -M lm3s6965evb \
  -cpu cortex-m3 \
  -kernel build/qemu_hello.elf \
  -nographic \
  -monitor none \
  -serial stdio
```

## 예상 로그

```text
[BOOT] bare-metal qemu sample
[CHECK] uart output is working
[CHECK] qemu lm3s6965evb is running
[NEXT] day03 runs the real FreeRTOS kernel
```

## 종료

QEMU 종료는 `Ctrl+A`를 누른 뒤 `X`를 누릅니다.

## 다음 단계

3일차에는 `FreeRTOS-Kernel`을 받아서 실제 FreeRTOS API를 사용합니다.

```c
xTaskCreate(...);
xQueueCreate(...);
xSemaphoreCreateMutex();
xTimerCreate(...);
vTaskStartScheduler();
```
