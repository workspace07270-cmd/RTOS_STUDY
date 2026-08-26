#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*
 * QEMU lm3s6965evb Cortex-M3 실습용 FreeRTOS 설정 파일입니다.
 *
 * FreeRTOS Kernel은 프로젝트마다 이 파일을 필요로 합니다.
 * 어떤 기능을 켤지, tick 주기를 얼마로 할지, heap을 얼마나 둘지 같은
 * 정책을 여기서 정합니다.
 */

/* 선점형 scheduler를 사용합니다. 높은 우선순위 Task가 준비되면 선점될 수 있습니다. */
#define configUSE_PREEMPTION                    1

/* 이식 계층의 최적화 task 선택 기능은 이번 실습에서 사용하지 않습니다. */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/* QEMU lm3s6965evb 실습에서 가정하는 CPU clock입니다. */
#define configCPU_CLOCK_HZ                      50000000UL

/* 1초에 1000 tick, 즉 1 tick은 약 1ms입니다. */
#define configTICK_RATE_HZ                      1000

/* 사용할 수 있는 Task 우선순위 범위입니다. 0부터 4까지 사용할 수 있습니다. */
#define configMAX_PRIORITIES                    5

/* idle task 등 최소 Task stack 기준값입니다. 단위는 word입니다. */
#define configMINIMAL_STACK_SIZE                128

/* xTaskCreate, xQueueCreate 같은 동적 생성 API가 사용할 heap 크기입니다. */
#define configTOTAL_HEAP_SIZE                   (16 * 1024)

/* Task 이름 최대 길이입니다. 디버깅과 로그 이해에 도움을 줍니다. */
#define configMAX_TASK_NAME_LEN                 16

/* 32-bit tick을 사용합니다. */
#define configUSE_16_BIT_TICKS                  0

/* 같은 우선순위 Task 사이에서 idle task가 양보할지 정합니다. */
#define configIDLE_SHOULD_YIELD                 1

/* Mutex를 사용하기 위해 켭니다. Step 2, Step 3에서 필요합니다. */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0

/* Software Timer를 사용하기 위해 켭니다. Step 3에서 필요합니다. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                5
#define configTIMER_TASK_STACK_DEPTH            256

/* 이번 실습은 동적 할당 API를 사용합니다. */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* 수업 예제에서는 stack overflow hook은 끄고, malloc 실패 hook은 켭니다. */
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0

/*
 * Cortex-M interrupt priority 설정입니다.
 * FreeRTOS API를 interrupt 안에서 사용할 때 priority 제한과 관련됩니다.
 */
#define configPRIO_BITS                         3
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 7
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* 예제에서 사용하는 API만 켭니다. */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelete                     0
#define INCLUDE_vTaskSuspend                    0
#define INCLUDE_vTaskPrioritySet                0
#define INCLUDE_uxTaskPriorityGet               0
#define INCLUDE_xTaskGetSchedulerState          0
#define INCLUDE_xTimerPendFunctionCall          0

void vAssertCalled(const char *file, unsigned long line);

/* configASSERT가 실패하면 freertos_hooks.c의 vAssertCalled()로 이동합니다. */
#define configASSERT(x) if ((x) == 0) { vAssertCalled(__FILE__, __LINE__); }

#endif
