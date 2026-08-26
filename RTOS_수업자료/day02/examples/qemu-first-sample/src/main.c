#include <stdint.h>

/*
 * 2일차 QEMU 첫 실행 샘플입니다.
 *
 * 이 코드는 아직 FreeRTOS Kernel을 포함하지 않은 bare-metal firmware입니다.
 * 목적은 ARM GCC로 firmware를 빌드하고, QEMU lm3s6965evb 보드에서
 * UART 로그가 출력되는지 확인하는 것입니다.
 * https://www.ti.com/product/LM3S6965
 */

/*
 * UART(Universal Asynchronous Receiver/Transmitter)는 MCU가 외부 장치와
 * 문자를 한 바이트씩 주고받을 때 사용하는 직렬 통신 장치입니다.
 *
 * 실제 개발 보드에서는 UART 신호가 USB-UART 케이블 등을 거쳐 PC로
 * 전달됩니다. 이 예제에서는 QEMU가 UART 하드웨어를 가상으로 구현하고,
 * 실행 옵션 "-serial stdio"가 UART 출력을 WSL 터미널에 연결합니다.
 *
 * 이 예제의 출력 동작 순서:
 *
 * main()
 *   -> uart_puts("문자열")
 *   -> 문자열에서 문자 한 개를 꺼냄
 *   -> uart_putc(문자)
 *   -> UART_FR의 TXFF 비트로 송신 공간 확인
 *   -> UART_DR에 문자 한 바이트 기록
 *   -> QEMU 가상 UART가 레지스터 쓰기를 처리
 *   -> WSL 터미널에 문자 출력
 *
 * 위 과정을 문자열 끝의 '\0'을 만날 때까지 반복합니다.
 *
 * 간단히 말하면 애플리케이션은 송신 FIFO에 빈자리가 있는지 확인한 뒤
 * UART_DR에 문자 하나를 전달하는 일까지만 담당합니다. 그 이후에는
 * QEMU의 가상 UART가 FIFO와 UART_FR 상태를 관리하고, FIFO에서 문자를
 * 순서대로 꺼내 WSL 터미널로 전달합니다.
 *
 * FIFO(First In, First Out)는 먼저 들어온 데이터가 먼저 나가는 큐입니다.
 *
 *   입력: H -> e -> l -> l -> o
 *   출력: H -> e -> l -> l -> o
 *
 * UART_DR은 "Hello" 같은 문자열 전체를 저장하는 일반 메모리 버퍼가
 * 아닙니다. uart_puts()가 문자열에서 꺼낸 현재 문자 하나를 UART에
 * 전달하는 하드웨어 데이터 레지스터입니다.
 *
 *   QEMU: 애플리케이션 -> UART_DR -> 가상 FIFO -> QEMU -> WSL 터미널
 *   실물: 애플리케이션 -> UART_DR -> 물리 FIFO -> UART TX -> PC 터미널
 */

/* lm3s6965evb 보드의 UART0 레지스터 시작 주소입니다. */
#define UART0_BASE 0x4000C000u

/*
 * 0x000u의 의미:
 *   0x  : 뒤의 숫자를 16진수로 해석합니다.
 *   000 : 값은 0이며, UART0 기준 주소로부터의 거리(offset)를 뜻합니다.
 *   u   : 값을 부호 없는 정수(unsigned int)로 취급합니다.
 *
 * 따라서 UART_DR의 실제 주소는 다음과 같습니다.
 *
 *   UART0_BASE + 0x000u
 *   = 0x4000C000 + 0x000
 *   = 0x4000C000
 *
 * UART Data Register는 송신할 문자 한 바이트를 기록하는 레지스터입니다.
 *
 * 아래의 "volatile uint32_t *"는 다음 의미를 가집니다.
 *
 *   uint32_t : 부호 없는 32비트 정수 자료형
 *   volatile : 값이 하드웨어에 의해 언제든 바뀔 수 있으므로 접근할 때마다
 *              실제 주소를 읽거나 쓰도록 컴파일러에 지시
 *   *         : 계산한 주소에 연결된 레지스터 값을 실제로 접근하는 역참조
 *
 * 전체 표현식은 다음 순서로 동작합니다.
 *
 *   UART0_BASE + 0x000u
 *     -> 0x4000C000 주소 계산
 *     -> (volatile uint32_t *)로 32비트 레지스터 포인터 변환
 *     -> 앞의 *로 해당 주소의 UART_DR 레지스터 접근
 *
 * volatile은 잠금 장치가 아닙니다. 컴파일러가 레지스터 읽기나 쓰기를
 * 불필요하다고 판단해 생략하거나 이전 값을 재사용하지 못하게 합니다.
 */
#define UART_DR    (*(volatile uint32_t *)(UART0_BASE + 0x000u))

/*
 * UART_FR은 기준 주소에서 0x018바이트 떨어진 곳에 있습니다.
 *
 *   UART0_BASE + 0x018u
 *   = 0x4000C000 + 0x018
 *   = 0x4000C018
 *
 * UART Flag Register는 송수신 장치의 현재 상태를 확인하는 레지스터입니다.
 * volatile이 있으므로 while 반복마다 QEMU 또는 실제 UART 하드웨어가
 * 제공하는 최신 UART_FR 값을 0x4000C018 주소에서 다시 읽습니다.
 */
#define UART_FR    (*(volatile uint32_t *)(UART0_BASE + 0x018u))

/*
 * UART_FR_TXFF는 상태를 저장하거나 변경하는 변수가 아닙니다.
 * UART_FR의 5번 비트만 골라 읽기 위한 마스크(0x20)입니다.
 *
 * 실제 보드에서는 UART 하드웨어가, 이 예제에서는 QEMU의 가상 UART가
 * 송신 FIFO 상태에 따라 TXFF 비트를 제공합니다.
 *
 *   TXFF = 0: 송신 FIFO에 빈 공간이 있음
 *   TXFF = 1: 송신 FIFO가 가득 참
 */
#define UART_FR_TXFF (1u << 5)

static void uart_putc(char ch) {
    /*
     * 송신 FIFO가 가득 차 있으면 빈 공간이 생길 때까지 기다립니다.
     * volatile 레지스터이므로 반복할 때마다 UART의 실제 상태를 다시 읽습니다.
     */
    while ((UART_FR & UART_FR_TXFF) != 0u) {
    }

    /*
     * 송신 공간이 생기면 Data Register에 문자 바이트 한 개를 기록합니다.
     * 한글은 UTF-8 바이트 여러 개로 구성되며 uart_puts()가 이를 차례로 보냅니다.
     * uint8_t 변환은 0x80 이상인 UTF-8 바이트의 부호 확장을 방지합니다.
     * QEMU는 이 레지스터 쓰기를 감지해 해당 바이트를 터미널로 전달합니다.
     */
    UART_DR = (uint32_t) (uint8_t) ch;
}

static void uart_puts(const char *text) {
    /* C 문자열의 끝을 나타내는 null 문자('\0')까지 한 글자씩 전송합니다. */
    while (*text != '\0') {
        /*
         * 터미널 줄바꿈 호환을 위해 LF('\n') 앞에 CR('\r')을 먼저 보냅니다.
         * 따라서 '\n'은 UART로 CR과 LF 두 문자가 차례로 전송됩니다.
         */
        if (*text == '\n') {
            uart_putc('\r');
        }

        /* 현재 문자를 보낸 뒤 포인터를 다음 문자로 이동합니다. */
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
