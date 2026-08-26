# 예제: Windows + WSL2 개발 환경 점검 체크리스트

## 1. 기본 정보

| 항목 | 작성 |
|---|---|
| 이름 |  |
| Windows 버전 |  |
| PowerShell 실행 여부 |  |
| WSL2 사용 가능 여부 |  |

## 2. WSL 확인

PowerShell에서 실행합니다.

```powershell
wsl --status
wsl --list --verbose
```

| 항목 | 결과 |
|---|---|
| WSL 설치 여부 |  |
| Ubuntu 설치 여부 |  |
| Ubuntu VERSION 2 여부 |  |

## 3. Ubuntu 접속 확인

PowerShell에서 실행합니다.

```powershell
wsl -d Ubuntu
```

접속 후 프롬프트를 확인합니다.

```text
user@DESKTOP:~$
```

| 항목 | 결과 |
|---|---|
| `wsl -d Ubuntu` 접속 성공 |  |
| Ubuntu 프롬프트 확인 |  |
| Linux 사용자 계정 설정 완료 |  |

## 4. Ubuntu 작업 폴더 생성

Ubuntu shell에서 실행합니다.

```bash
mkdir -p ~/rtos-kiosk-course
cd ~/rtos-kiosk-course
pwd
```

| 항목 | 결과 |
|---|---|
| 작업 폴더 생성 |  |
| 현재 경로가 Ubuntu 홈 아래 |  |
| `/mnt/c` 경로가 아님 |  |

## 5. Ubuntu 도구 설치 확인

Ubuntu에서 실행하고 결과를 기록합니다.

```bash
make --version
qemu-system-arm --version
arm-none-eabi-gcc --version
```

| 도구 | 결과 | 통과 여부 |
|---|---|---|
| Make |  |  |
| QEMU ARM |  |  |
| ARM GCC |  |  |

디버깅 실습을 진행한다면 선택 도구도 확인합니다.

```bash
gdb-multiarch --version
```

## 6. QEMU Machine 확인

```bash
qemu-system-arm -machine help
```

| Machine | 확인 |
|---|---|
| `lm3s6965evb` |  |
| `mps2-an385` |  |
| `mps2-an386` |  |

## 7. Ubuntu 종료 확인

Ubuntu shell 종료:

```bash
exit
```

WSL 전체 종료:

```powershell
wsl --shutdown
```

| 항목 | 확인 |
|---|---|
| `exit` 사용 가능 |  |
| `wsl --shutdown` 의미 이해 |  |

## 8. 2일차 실행 환경 확인

| 항목 | 확인 |
|---|---|
| WSL2 Ubuntu가 실행된다 |  |
| Ubuntu에 접속할 수 있다 |  |
| QEMU가 설치되어 있다 |  |
| ARM GCC가 설치되어 있다 |  |
| Make가 설치되어 있다 |  |
| `qemu-first-sample` 빌드에 성공했다 |  |
| QEMU에서 UART 로그를 확인했다 |  |
| 2일차 샘플에는 FreeRTOS Kernel이 없음을 이해했다 |  |

## 9. 문제 발생 시 기록

| 증상 | 시도한 해결 방법 | 해결 여부 |
|---|---|---|
|  |  |  |
|  |  |  |
