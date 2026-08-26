#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <assert.h>

/*
 * FreeRTOS 커널의 기능과 크기를 정하는 설정 파일입니다.
 * 1은 사용, 0은 사용하지 않음을 뜻하는 항목이 많습니다.
 */

/* 우선순위가 높은 태스크가 낮은 태스크를 선점할 수 있습니다. */
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configCPU_CLOCK_HZ                      1000000000UL
/* 1 tick을 1ms로 사용합니다(1000Hz). vTaskDelay 계산의 기준입니다. */
#define configTICK_RATE_HZ                      1000
/* 사용할 수 있는 태스크 우선순위는 0부터 6까지입니다. */
#define configMAX_PRIORITIES                    7
#define configMINIMAL_STACK_SIZE                256
#define configMAX_TASK_NAME_LEN                 24
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIMERS                        0
/* Queue와 태스크를 실행 중에 만들 수 있도록 동적 할당을 사용합니다. */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
/* FreeRTOS 객체가 함께 사용하는 Heap 크기입니다. POSIX 학습 환경이라 넉넉히 둡니다. */
#define configTOTAL_HEAP_SIZE                   (1024 * 1024)
#define configCHECK_FOR_STACK_OVERFLOW          0
/* 메모리 할당 실패 시 main.c의 vApplicationMallocFailedHook을 호출합니다. */
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_APPLICATION_TASK_TAG          0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0
#define configUSE_NEWLIB_REENTRANT               0
#define configUSE_POSIX_ERRNO                    0
#define configENABLE_BACKWARD_COMPATIBILITY     1
/* 아래 설정이 1이어야 해당 FreeRTOS API가 커널에 포함됩니다. */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskPrioritySet                0
#define INCLUDE_uxTaskPriorityGet               0
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_xTimerPendFunctionCall          0
#define configASSERT(condition) assert(condition)

#endif
