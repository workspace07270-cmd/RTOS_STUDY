/* FreeRTOS 기본 자료형, 설정값, 공통 매크로를 제공합니다. */
#include "FreeRTOS.h"
/* Task 생성·지연 및 scheduler 시작 API를 제공합니다. */
#include "task.h"
/* QEMU의 UART0를 통해 WSL 콘솔에 문자열을 출력합니다. */
#include "uart.h"

static void HeartbeatTask(void *parameters);
static void UiTask(void *parameters);

int main(void) {
  
    uart_puts("[부팅] 1단계 Task 기본 예제를 시작합니다.\n");

   
    (void) xTaskCreate(HeartbeatTask, "Heartbeat", 256, 0, 1, 0);
  
    (void) xTaskCreate(UiTask, "UiTask", 256, 0, 2, 0);

 
    uart_puts("[FreeRTOS] Scheduler를 시작합니다.\n");
    vTaskStartScheduler();

  
    uart_puts("[FreeRTOS] Scheduler 시작에 실패했습니다.\n");
    for (;;) {
    }
}

static void HeartbeatTask(void *parameters) {
 
    (void) parameters;

 
    for (;;) {
        uart_puts("[HeartbeatTask] 시스템이 정상 동작 중입니다.\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void UiTask(void *parameters) {
    
    (void) parameters;

   
    for (;;) {
        uart_puts("[UiTask] 키오스크 입력을 기다리는 중입니다.\n");

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}
