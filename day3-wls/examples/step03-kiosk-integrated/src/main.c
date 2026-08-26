/* FreeRTOS 공통 자료형, 설정값, 상태 매크로를 제공합니다. */
#include "FreeRTOS.h"
/* Task 사이의 FIFO 이벤트 전달에 사용하는 Queue API입니다. */
#include "queue.h"
/* 공유 프린터 보호에 사용하는 Mutex API입니다. */
#include "semphr.h"
/* Task 생성·지연 및 scheduler 시작 API입니다. */
#include "task.h"
/* Software Timer 생성·시작 API입니다. */
#include "timers.h"
/* QEMU UART0를 통해 WSL 콘솔에 로그를 출력합니다. */
#include "uart.h"

typedef enum {
    EVENT_ORDER_REQUESTED = 1,
    EVENT_PRINT_REQUESTED
} KioskEventType;

typedef struct {
    KioskEventType type;
    const char *item;
} KioskEvent;

static QueueHandle_t orderQueue;
static QueueHandle_t printQueue;
static SemaphoreHandle_t printerMutex;
static TimerHandle_t monitorTimer;

static void ButtonTask(void *parameters);
static void PaymentTask(void *parameters);
static void PrinterTask(void *parameters);
static void MonitorTimerCallback(TimerHandle_t timer);

int main(void) {
    uart_puts("[부팅] 3단계 키오스크 통합 예제를 시작합니다.\n");

    orderQueue = xQueueCreate(4, sizeof(KioskEvent));
    printQueue = xQueueCreate(4, sizeof(KioskEvent));
    printerMutex = xSemaphoreCreateMutex();

  
    monitorTimer = xTimerCreate(
        "MonitorTimer",
        pdMS_TO_TICKS(2000),
        pdTRUE,
        0,
        MonitorTimerCallback
    );

    configASSERT(orderQueue != 0);
    configASSERT(printQueue != 0);
    configASSERT(printerMutex != 0);
    configASSERT(monitorTimer != 0);

    
    (void) xTaskCreate(ButtonTask, "Button", 256, 0, 3, 0);
    (void) xTaskCreate(PaymentTask, "Payment", 256, 0, 2, 0);
    (void) xTaskCreate(PrinterTask, "Printer", 256, 0, 2, 0);

    (void) xTimerStart(monitorTimer, 0);

    uart_puts("[FreeRTOS] Scheduler를 시작합니다.\n");
    vTaskStartScheduler();

    uart_puts("[FreeRTOS] Scheduler 시작에 실패했습니다.\n");
    for (;;) {
    }
}

static void ButtonTask(void *parameters) {
 
    (void) parameters;

    for (;;) {
   
        KioskEvent event = {
            .type = EVENT_ORDER_REQUESTED,
            .item = "americano x1"
        };

        uart_puts("[ButtonTask] 주문 버튼이 눌렸습니다.\n");

        (void) xQueueSend(orderQueue, &event, pdMS_TO_TICKS(100));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void PaymentTask(void *parameters) {
    (void) parameters;

    for (;;) {
        KioskEvent event;

        if (xQueueReceive(orderQueue, &event, portMAX_DELAY) == pdPASS) {
            uart_puts("[PaymentTask] 주문 정보를 받았습니다.\n");

            uart_puts("[PaymentTask] 결제가 승인되었습니다.\n");

            KioskEvent printEvent = {
                .type = EVENT_PRINT_REQUESTED,
                .item = event.item
            };

            (void) xQueueSend(printQueue, &printEvent, pdMS_TO_TICKS(100));
        }
    }
}

static void PrinterTask(void *parameters) {
    (void) parameters;

    for (;;) {
        KioskEvent event;

        if (xQueueReceive(printQueue, &event, portMAX_DELAY) == pdPASS) {
            uart_puts("[PrinterTask] 출력 요청을 받았습니다.\n");

            if (xSemaphoreTake(printerMutex, portMAX_DELAY) == pdPASS) {
                uart_puts("[PrinterTask] 프린터 Mutex를 획득했습니다.\n");

                uart_puts("[PrinterTask] 영수증을 출력했습니다.\n");

                xSemaphoreGive(printerMutex);
                uart_puts("[PrinterTask] 프린터 Mutex를 반환했습니다.\n");
            }
        }
    }
}

static void MonitorTimerCallback(TimerHandle_t timer) {

    (void) timer;

    uart_puts("[MonitorTimer] 키오스크 장치가 정상 동작 중입니다.\n");
}
