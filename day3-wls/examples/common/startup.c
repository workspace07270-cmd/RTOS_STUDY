#include <stdint.h>

/*
 * Cortex-M3 부팅 코드입니다.
 *
 * PC 프로그램은 운영체제가 main()을 호출해 주지만, MCU firmware는 그렇지 않습니다.
 * CPU가 reset되면 vector table을 먼저 읽고, 그 안에 있는 Reset_Handler 주소로
 * 점프합니다. 이 파일은 그 최소 부팅 흐름을 제공합니다.
 */

extern int main(void);

/*
 * 아래 심볼들은 linker.ld에서 만들어집니다.
 *
 * _estack:
 *   초기 stack pointer 값입니다.
 *
 * _sidata, _sdata, _edata:
 *   초기값이 있는 전역/static 변수(.data)를 FLASH에서 RAM으로 복사할 때 사용합니다.
 *
 * _sbss, _ebss:
 *   초기값이 0인 전역/static 변수(.bss)를 0으로 채울 때 사용합니다.
 */
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/*
 * FreeRTOS Cortex-M3 port가 제공하는 interrupt handler입니다.
 *
 * vPortSVCHandler:
 *   scheduler 시작 시 첫 Task로 전환하는 데 사용됩니다.
 *
 * xPortPendSVHandler:
 *   context switch에 사용됩니다.
 *
 * xPortSysTickHandler:
 *   tick interrupt를 처리하고, delay가 끝난 Task를 깨우는 데 사용됩니다.
 */
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

void Reset_Handler(void);
void Default_Handler(void);

/*
 * Cortex-M vector table입니다.
 *
 * 첫 번째 항목은 초기 stack pointer,
 * 두 번째 항목은 reset 후 실행할 handler입니다.
 *
 * FreeRTOS를 실제로 실행하려면 SVC, PendSV, SysTick 자리에
 * FreeRTOS port handler가 연결되어 있어야 합니다.
 */
__attribute__((section(".isr_vector")))
const uintptr_t vector_table[] = {
    (uintptr_t) &_estack,
    (uintptr_t) Reset_Handler,
    (uintptr_t) Default_Handler,
    (uintptr_t) Default_Handler,
    (uintptr_t) Default_Handler,
    (uintptr_t) Default_Handler,
    (uintptr_t) Default_Handler,
    0,
    0,
    0,
    0,
    (uintptr_t) vPortSVCHandler,
    (uintptr_t) Default_Handler,
    0,
    (uintptr_t) xPortPendSVHandler,
    (uintptr_t) xPortSysTickHandler,
};

void Reset_Handler(void) {
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    /*
     * .data 섹션 초기화:
     * 초기값이 있는 전역/static 변수는 FLASH에 초기값이 저장되어 있고,
     * 실행 중에는 RAM에 있어야 하므로 부팅 시 복사합니다.
     */
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /*
     * .bss 섹션 초기화:
     * C 언어 규칙상 초기값이 없는 전역/static 변수는 0으로 시작해야 합니다.
     */
    for (dst = &_sbss; dst < &_ebss; dst++) {
        *dst = 0u;
    }

    (void) main();

    /*
     * main()이 돌아오면 실행할 운영체제가 없으므로 멈춰 둡니다.
     */
    for (;;) {
    }
}

void Default_Handler(void) {
    /*
     * 아직 개별 interrupt handler를 만들지 않은 예외가 들어오면 여기서 멈춥니다.
     */
    for (;;) {
    }
}
