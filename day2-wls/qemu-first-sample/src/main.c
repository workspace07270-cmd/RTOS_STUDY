#include <stdint.h>

#define UART0_BASE 0x4000C000u

#define UART_DR    (*(volatile uint32_t *)(UART0_BASE + 0x000u))

#define UART_FR    (*(volatile uint32_t *)(UART0_BASE + 0x018u))

#define UART_FR_TXFF (1u << 5)

static void uart_putc(char ch) {
    
    while ((UART_FR & UART_FR_TXFF) != 0u) {
    }

    UART_DR = (uint32_t) (uint8_t) ch;
}

static void uart_puts(const char *text) {
  
    while (*text != '\0') {
    
        if (*text == '\n') {
            uart_putc('\r');
        }

        uart_putc(*text++);
    }
}

int main(void) {
    uart_puts("[부팅] Bare-metal QEMU 예제를 시작합니다.\n");
    uart_puts("[확인] UART 출력이 정상적으로 동작합니다.\n");
    uart_puts("[확인] QEMU lm3s6965evb 가상 보드가 실행 중입니다.\n");
    uart_puts("[다음] 3일차에는 실제 FreeRTOS 커널을 실행합니다.\n");

    /*
     * bare-metal firmware는 main()이 끝난 뒤 돌아갈 운영체제가 없습니다.
     * QEMU가 계속 실행 상태를 유지하도록 무한 루프에 머뭅니다.
     */
    for (;;) {
    }
}
