# 실습: WSL2 + QEMU + ARM GCC 환경 셋팅

## 목적

2일차에는 FreeRTOS를 실행하지 않습니다. WSL Ubuntu에서 ARM GCC와 QEMU를 준비하고, bare-metal firmware가 QEMU에서 실행되는지만 확인합니다.

실제 FreeRTOS 실행은 3일차 `day03/examples/freertos-qemu-kiosk`에서 진행합니다.

## 1. WSL 상태 확인

PowerShell에서 실행합니다.

```powershell
wsl --status
wsl --list --verbose
```

Ubuntu가 없다면 설치합니다.

```powershell
wsl --install -d Ubuntu
```

## 2. Ubuntu 접속

```powershell
wsl -d Ubuntu
```

정상 접속 예:

```text
user@DESKTOP:~$
```

## 3. 작업 폴더 생성

```bash
mkdir -p ~/rtos-kiosk-course
cd ~/rtos-kiosk-course
pwd
```

Windows 작업 폴더를 직접 사용할 때는 `/mnt/c` 경로로 접근합니다.

```bash
cd /mnt/c/workspace/new_rtos_context
```

## 4. Ubuntu 업데이트

```bash
sudo apt update
sudo apt upgrade -y
```

## 5. 실습 도구 설치

```bash
sudo apt install -y build-essential git make
sudo apt install -y qemu-system-arm gcc-arm-none-eabi
```

GDB 실습을 추가로 진행한다면 선택적으로 설치합니다.

```bash
sudo apt install -y gdb-multiarch
```

## 6. 설치 확인

```bash
make --version
qemu-system-arm --version
arm-none-eabi-gcc --version
gdb-multiarch --version
```

## 7. 도구 역할 확인

| 이름 | 무엇을 하는가 |
|---|---|
| QEMU | 실제 하드웨어 없이 CPU와 보드를 에뮬레이션 |
| `qemu-system-arm` | QEMU에서 ARM 가상 보드 전체를 실행 |
| UART | firmware가 로그를 보내는 직렬 통신 장치 |
| Make | Makefile에 정의된 빌드·실행 명령 수행 |
| CMake | 다른 프로젝트에서 Make 또는 Ninja용 빌드 규칙 생성 |
| Ninja | CMake가 생성한 규칙을 빠르게 실행하는 선택 도구 |
| `arm-none-eabi-gcc` | ARM Cortex-M bare-metal firmware 컴파일·링크 |
| `gdb-multiarch` | QEMU에 연결하여 ARM firmware 디버깅 |

현재 샘플의 필수 흐름은 다음과 같습니다.

```text
Makefile -> make -> arm-none-eabi-gcc -> qemu_hello.elf
qemu_hello.elf -> qemu-system-arm -> UART -> WSL 터미널
```

CMake와 Ninja는 현재 Makefile 샘플에 필요하지 않으므로 설치 대상에서 제외합니다.

## 8. QEMU Machine 확인

```bash
qemu-system-arm -machine help
```

이번 과정의 기본 machine:

```text
lm3s6965evb
```

## 9. 첫 QEMU 샘플 빌드와 실행

이 샘플은 FreeRTOS가 아니라 bare-metal firmware입니다.

```bash
cd day02/examples/qemu-first-sample
make
make run
```

Windows 작업 폴더 기준:

```bash
cd /mnt/c/workspace/new_rtos_context/day02/examples/qemu-first-sample
make
make run
```

예상 로그:

```text
[BOOT] bare-metal qemu sample
[CHECK] uart output is working
[CHECK] qemu lm3s6965evb is running
[NEXT] day03 runs the real FreeRTOS kernel
```

QEMU 종료는 `Ctrl+A`를 누른 뒤 `X`를 누릅니다.

## 10. 3일차 준비 확인

3일차에는 FreeRTOS Kernel을 내려받습니다.

```bash
cd /mnt/c/workspace/new_rtos_context
mkdir -p third_party
git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git third_party/FreeRTOS-Kernel
```

확인:

```bash
ls third_party/FreeRTOS-Kernel/include/FreeRTOS.h
ls third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c
```
