#ifndef UART_H
#define UART_H

/*
 * QEMU UART 출력 함수입니다.
 * printf 대신 이 함수를 사용하면 libc 없이도 터미널 로그를 볼 수 있습니다.
 */

void uart_putc(char ch);
void uart_puts(const char *text);

/* UART RX에 문자가 들어올 때까지 기다린 뒤 한 byte를 반환합니다. */
char uart_getc(void);

#endif
