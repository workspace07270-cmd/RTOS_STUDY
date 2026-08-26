#include <stdint.h>

/*
 * Cortex-M 계열 MCU는 전원이 켜지면 vector table을 먼저 읽습니다.
 *
 * vector table의 첫 번째 값:
 *   초기 stack pointer 값
 *
 * vector table의 두 번째 값:
 *   reset handler 주소
 *
 * 즉, 일반 PC 프로그램처럼 main()부터 바로 실행되지 않습니다.
 * CPU가 reset handler로 점프하고, reset handler 안에서 main()을 호출합니다.
 */

/* main은 src/main.c에 정의되어 있습니다. */
extern int main(void);

/*
 * _estack은 linker.ld에서 정의합니다.
 * RAM의 끝 주소를 stack 시작 위치로 사용합니다.
 */
extern uint32_t _estack;

void Reset_Handler(void);
void Default_Handler(void);

/*
 * .isr_vector 섹션에 vector table을 강제로 배치합니다.
 * linker.ld는 이 섹션을 FLASH 시작 주소 0x00000000에 놓습니다.
 *
 * KEEP은 linker가 이 테이블을 "사용하지 않는 코드"로 오해하고 제거하지 않게 합니다.
 */
__attribute__((section(".isr_vector")))
const uintptr_t vector_table[] = {
    /* 초기 stack pointer */
    (uintptr_t) &_estack,

    /* reset 후 가장 먼저 실행될 함수 */
    (uintptr_t) Reset_Handler,
};

void Reset_Handler(void) {
    /*
     * 실제 제품 코드에서는 여기서 .data 복사, .bss 초기화, clock 설정 등을 수행합니다.
     * 이 첫 샘플은 QEMU에서 UART 로그를 보는 것이 목적이라 main()만 호출합니다.
     */
    (void) main();

    /* main이 반환되면 갈 곳이 없으므로 무한 루프에 머뭅니다. */
    for (;;) {
    }
}

void Default_Handler(void) {
    /*
     * 아직 개별 interrupt handler를 만들지 않았으므로 기본 handler를 둡니다.
     * 예기치 않은 interrupt가 오면 여기서 멈추게 됩니다.
     */
    for (;;) {
    }
}
