# 3일차 수업 예제

3일차 예제는 실제 FreeRTOS Kernel을 사용하되, 이해하기 쉽도록 3단계로 나누었습니다.

## 예제 구조

| 경로 | 설명 |
|---|---|
| `common/` | 세 단계가 공유하는 FreeRTOS 설정, startup, linker, UART, 빌드 규칙 |
| `step01-task-basic/` | FreeRTOS Task 생성과 scheduler 시작 |
| `step02-queue-mutex/` | Queue 이벤트 전달과 Mutex 공유 자원 보호 |
| `step03-kiosk-integrated/` | 키오스크 통합 흐름과 Software Timer |

## FreeRTOS Kernel 준비

프로젝트 루트에서 실행합니다.

```bash
mkdir -p third_party
git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git third_party/FreeRTOS-Kernel
```

## Step 1 실행

```bash
cd day03/examples/step01-task-basic
make
make run
```

## Step 2 실행

```bash
cd day03/examples/step02-queue-mutex
make
make run
```

## Step 3 실행

```bash
cd day03/examples/step03-kiosk-integrated
make
make run
```

Windows 작업 폴더를 WSL에서 직접 접근하는 경우:

```bash
cd /mnt/c/workspace/new_rtos_context/day03/examples/step01-task-basic
make
make run
```

## 종료 방법

QEMU 실행 중 터미널에서 다음 키를 순서대로 누릅니다.

```text
Ctrl + A, X
```

## 읽는 순서

1. `step01-task-basic/src/main.c`
2. `step02-queue-mutex/src/main.c`
3. `step03-kiosk-integrated/src/main.c`
4. `common/FreeRTOSConfig.h`
5. `common/startup.c`
6. `common/freertos_qemu.mk`
