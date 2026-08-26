# 참고: WSL2 + Ubuntu RTOS 도구 설치 가이드

## Windows에서 확인할 도구

PowerShell에서 확인합니다.

```powershell
wsl --status
wsl --list --verbose
```

Ubuntu가 WSL2로 실행되어야 합니다.

```text
Ubuntu    Running    2
```

VS Code를 사용할 경우 Windows에 VS Code와 WSL 확장을 설치합니다.

## Ubuntu에서 설치할 도구

Ubuntu 터미널에서 실행합니다.

```bash
sudo apt update
sudo apt upgrade -y
sudo apt install -y build-essential git make
sudo apt install -y qemu-system-arm gcc-arm-none-eabi
```

GDB 디버깅 실습을 진행할 때만 선택적으로 설치합니다.

```bash
sudo apt install -y gdb-multiarch
```

## 설치 확인

```bash
git --version
make --version
qemu-system-arm --version
arm-none-eabi-gcc --version
```

## 도구 역할

| 도구 | 역할 | 현재 예제에서 필수 여부 |
|---|---|
| Git | 예제 코드 다운로드와 버전 관리 | 환경 준비에 사용 |
| Make | Makefile의 빌드·실행 규칙 수행 | 필수 |
| QEMU | CPU와 보드를 에뮬레이션하는 프로젝트 | 필수 |
| `qemu-system-arm` | ARM 가상 보드와 주변 장치 실행 | 필수 |
| UART | firmware의 직렬 로그 출력 통로 | 필수 개념 |
| `arm-none-eabi-gcc` | OS 없는 ARM Cortex-M용 firmware 컴파일·링크 | 필수 |
| `gdb-multiarch` | QEMU GDB 서버에 연결하여 중단점·레지스터 확인 | 디버깅 시 선택 |

## 빌드와 실행 흐름

```text
Makefile -> make -> arm-none-eabi-gcc -> ELF
ELF -> qemu-system-arm -> 가상 UART -> 터미널
```

QEMU는 실제 장비가 아니라 가상 보드를 실행하는 프로그램입니다. 실제 ARM 개발 보드에서는 같은 UART 출력 원리를 사용하되, QEMU 가상 UART 대신 실제 UART 하드웨어와 USB-UART 케이블 또는 보드 내장 디버거를 통해 PC 터미널에 연결합니다.

```text
QEMU: firmware -> UART register -> virtual UART -> WSL terminal
실물: firmware -> UART register -> physical UART -> USB-UART -> PC terminal
```

실제 보드에서는 register 주소, GPIO pin, clock, baud rate를 보드에 맞게 초기화하고 firmware를 Flash에 기록해야 합니다.

CMake와 Ninja를 사용하는 다른 프로젝트에서는 앞부분이 달라질 수 있습니다.

```text
CMakeLists.txt -> CMake -> Ninja build file -> Ninja -> arm-none-eabi-gcc
```

따라서 CMake와 Ninja는 현재 2일차 설치 대상에서 제외합니다. 개념 비교는 학생용 자료와 PPT에서만 다룹니다.

## Spring Boot 준비

4일차 Spring Boot 연동을 위해 JDK도 필요합니다. Windows 또는 WSL 중 수업 기준 환경에 맞추어 설치합니다.

Ubuntu에 설치하는 경우:

```bash
sudo apt install -y openjdk-17-jdk
java -version
```
