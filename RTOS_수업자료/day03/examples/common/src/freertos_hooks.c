#include "uart.h"

/*
 * FreeRTOS hook 함수 모음입니다.
 *
 * FreeRTOSConfig.h에서 configUSE_MALLOC_FAILED_HOOK을 1로 켜면
 * heap 할당 실패 시 vApplicationMallocFailedHook()이 호출됩니다.
 */

void vApplicationMallocFailedHook(void) {
    uart_puts("[FreeRTOS] 메모리 할당에 실패했습니다.\n");

    /*
     * 임베디드 실습에서는 실패 원인을 로그로 확인할 수 있게 멈춰 둡니다.
     * 실제 제품이라면 재시작, 장애 상태 저장, 안전 모드 진입 등을 고려합니다.
     */
    for (;;) {
    }
}

void vAssertCalled(const char *file, unsigned long line) {
    /*
     * 지금은 UART 로그를 단순하게 유지하기 위해 file/line을 출력하지 않습니다.
     * 디버깅 수업에서는 숫자 출력 함수를 추가해 위치까지 출력할 수 있습니다.
     */
    (void) file;
    (void) line;

    uart_puts("[FreeRTOS] Assert 조건 검사에 실패했습니다.\n");
    for (;;) {
    }
}
