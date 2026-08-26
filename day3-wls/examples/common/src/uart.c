#include <stdint.h>

#include "uart.h"

/*
 * QEMU lm3s6965evb 보드의 UART0 register 주소입니다.
 * QEMU 실행 옵션에 -serial stdio를 주면 UART0 출력이 터미널에 연결됩니다.
 */
#define UART0_BASE 0x4000C000u
#define UART_DR    (*(volatile uint32_t *)(UART0_BASE + 0x000u))
#define UART_FR    (*(volatile uint32_t *)(UART0_BASE + 0x018u))

/* UART Flag Register의 TXFF bit입니다. 1이면 transmit FIFO가 가득 찬 상태입니다. */
#define UART_FR_TXFF (1u << 5)
/* RXFE bit가 1이면 receive FIFO가 비어 있습니다. */
#define UART_FR_RXFE (1u << 4)

void uart_putc(char ch) {
    /*
     * FIFO가 가득 차 있으면 빈 공간이 생길 때까지 기다립니다.
     * volatile register를 읽기 때문에 compiler가 이 loop를 제거하지 않습니다.
     */
    while ((UART_FR & UART_FR_TXFF) != 0u) {
    }

    /*
     * 한글 문자열은 UTF-8 바이트 여러 개로 저장됩니다.
     * uint8_t로 변환하면 0x80 이상인 바이트가 부호 확장되지 않고
     * 원래 8-bit 값 그대로 UART Data Register에 기록됩니다.
     */
    UART_DR = (uint32_t) (uint8_t) ch;
}

void uart_puts(const char *text) {
    while (*text != '\0') {
        /*
         * 많은 terminal은 줄바꿈을 CRLF 조합으로 기대합니다.
         * '\n' 앞에 '\r'을 추가해서 로그 줄 정렬을 안정적으로 만듭니다.
         */
        if (*text == '\n') {
            uart_putc('\r');
        }

        uart_putc(*text++);
    }
}

char uart_getc(void) {
    while ((UART_FR & UART_FR_RXFE) != 0u) {
    }

    return (char) (UART_DR & 0xffu);
}
