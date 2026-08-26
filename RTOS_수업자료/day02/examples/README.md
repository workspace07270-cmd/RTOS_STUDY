# 2일차 수업 예제

## 예제 파일

| 파일/폴더 | 설명 |
|---|---|
| `windows_setup_checklist.md` | Windows 개발 환경 점검 체크리스트 |
| `tool_install_guide.md` | 실습 도구 설치와 확인 가이드 |
| `wsl_qemu_setup.md` | WSL2, Ubuntu, QEMU, ARM GCC 셋팅 체크리스트 |
| `qemu-first-sample/` | QEMU에서 바로 실행하는 bare-metal firmware 샘플 |

## 핵심 구분

2일차의 `qemu-first-sample/`은 FreeRTOS가 아닙니다.

이 샘플의 목적은 다음을 확인하는 것입니다.

- ARM GCC로 Cortex-M 펌웨어를 빌드할 수 있는가?
- QEMU에서 `lm3s6965evb` 보드를 실행할 수 있는가?
- UART 로그가 WSL 터미널에 출력되는가?

실제 FreeRTOS 실행은 3일차 `day03/examples/step01-task-basic/`부터 진행합니다.

## 실습 방법

1. PowerShell에서 WSL2 상태를 확인합니다.
2. Ubuntu를 실행하고 기본 업데이트를 수행합니다.
3. Make, QEMU와 ARM GCC를 설치합니다.
4. `qemu-system-arm -machine help`로 가상 보드 목록을 확인합니다.
5. `qemu-first-sample`에서 `make`, `make run`을 실행합니다.
6. UART 로그가 터미널에 출력되는지 확인합니다.
7. CMake와 Ninja는 현재 예제에 필요하지 않음을 확인합니다.
8. 실제 FreeRTOS 빌드는 3일차에 진행합니다.
