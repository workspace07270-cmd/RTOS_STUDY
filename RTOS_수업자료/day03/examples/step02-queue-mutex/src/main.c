/* FreeRTOS 공통 자료형, 설정값, 상태 매크로를 제공합니다. */
#include "FreeRTOS.h"
/* Queue 생성·송신·수신 API를 제공합니다. */
#include "queue.h"
/* Mutex를 포함한 Semaphore 계열 API를 제공합니다. */
#include "semphr.h"
/* Task 생성·지연 및 scheduler 시작 API를 제공합니다. */
#include "task.h"
/* QEMU UART0를 통해 실행 과정을 콘솔에 출력합니다. */
#include "uart.h"

/*
 * Step 2 목표:
 *   Task 사이에 Queue로 이벤트를 전달하고, Mutex로 프린터 공유 자원을 보호합니다.
 *
 * 키오스크 흐름:
 *   ButtonTask  -> 주문 이벤트 생성
 *   PaymentTask -> 주문 이벤트 수신 후 출력 요청 생성
 *   PrinterTask -> 출력 요청 수신 후 프린터 사용
 */

/*
 * [개념 1] Queue란?
 *
 * Queue는 Task 사이에서 데이터를 안전하게 전달하는 FIFO 방식의 대기열입니다.
 * FIFO(First In, First Out)는 먼저 넣은 데이터가 먼저 나오는 구조입니다.
 *
 *   생산자 Task -> [이벤트 1][이벤트 2][이벤트 3] -> 소비자 Task
 *
 * xQueueSend()는 전달할 구조체의 주소만 보관하지 않고, xQueueCreate()에서 정한
 * item 크기만큼 Queue 내부 공간으로 값을 복사합니다. xQueueReceive()도 Queue의
 * 값을 수신 변수로 복사한 뒤 해당 item을 Queue에서 제거합니다.
 *
 * Queue를 쓰면 ButtonTask가 PaymentTask를 직접 호출하지 않아도 됩니다.
 * 두 Task는 서로의 실행 시점을 몰라도 Queue를 통해 느슨하게 연결됩니다.
 *
 * [개념 2] Mutex란?
 *
 * Mutex(Mutual Exclusion)는 여러 Task가 하나의 공유 자원을 동시에 사용하지
 * 못하도록 잠그는 객체입니다. 화장실 열쇠 하나를 먼저 가져간 사람만 안에
 * 들어갈 수 있는 것과 비슷합니다.
 *
 *   Task A -> Mutex 획득 -> 프린터 사용 -> Mutex 반환
 *   Task B -> Mutex가 반환될 때까지 대기 -> 획득 후 프린터 사용
 *
 * Mutex에는 소유권이 있습니다. 획득한 Task가 사용 후 반드시 반환해야 합니다.
 * FreeRTOS Mutex는 우선순위 역전의 영향을 줄이기 위한 priority inheritance도
 * 지원하므로, 단순한 상태 변수보다 공유 자원 보호에 적합합니다.
 *
 * Queue와 Mutex의 차이:
 *
 *   Queue : 데이터나 이벤트를 Task 사이에 전달
 *   Mutex : 프린터 같은 공유 자원을 한 번에 한 Task만 사용하도록 보호
 */

typedef enum {
    /* 주문 버튼이 눌렸음을 나타내며 숫자 값은 1입니다. */
    EVENT_ORDER_REQUESTED = 1,
    /* 결제가 끝나 영수증 출력이 필요함을 나타냅니다. */
    EVENT_PRINT_REQUESTED
} KioskEventType;

typedef struct {
    /* 수신 Task가 이벤트 종류를 구분할 때 사용합니다. */
    KioskEventType type;
    /*
     * 문자열 자체가 아니라 문자열의 주소를 저장합니다.
     * Queue는 이 포인터 값만 복사하며 문자열까지 깊은 복사하지 않습니다.
     * 현재는 프로그램 수명 동안 유지되는 문자열 리터럴이라 안전합니다.
     */
    const char *item;
} KioskEvent;

/*
 * orderQueue:
 *   ButtonTask가 만든 주문 이벤트를 PaymentTask로 전달합니다.
 *
 * printQueue:
 *   PaymentTask가 만든 출력 요청을 PrinterTask로 전달합니다.
 *
 * printerMutex:
 *   프린터는 하나뿐인 공유 자원이라고 가정합니다.
 *   동시에 여러 Task가 프린터를 쓰지 못하도록 Mutex로 보호합니다.
 */
static QueueHandle_t orderQueue;
static QueueHandle_t printQueue;
static SemaphoreHandle_t printerMutex;

static void ButtonTask(void *parameters);
static void PaymentTask(void *parameters);
static void PrinterTask(void *parameters);

int main(void) {
    /* Scheduler를 시작하기 전에 필요한 RTOS 객체를 모두 생성합니다. */
    uart_puts("[부팅] 2단계 Queue와 Mutex 예제를 시작합니다.\n");

    /*
     * xQueueCreate(queue_length, item_size)
     *
     * queue_length:
     *   Queue가 최대 몇 개의 item을 저장할 수 있는지 정합니다.
     *   여기서는 KioskEvent를 최대 4개까지 순서대로 보관합니다.
     *
     * item_size:
     *   Queue에 들어가는 item 하나의 크기입니다.
     *   여기서는 KioskEvent 구조체를 통째로 복사해서 전달합니다.
     *
     * Queue 저장 공간은 FreeRTOS heap에서 확보됩니다. 구조체 안의 item은
     * 포인터이므로 포인터가 가리키는 문자열 데이터까지 복사되지는 않습니다.
     */
    orderQueue = xQueueCreate(4, sizeof(KioskEvent));
    printQueue = xQueueCreate(4, sizeof(KioskEvent));

    /*
     * xSemaphoreCreateMutex()는 Mutex 객체를 만듭니다.
     * FreeRTOS에서 Mutex도 semaphore 계열 API로 다룹니다.
     *
     * Semaphore는 자원의 개수나 사건 발생을 알리는 용도로도 쓰지만,
     * Mutex는 특히 "소유자가 있는 공유 자원 잠금"을 목적으로 사용합니다.
     */
    printerMutex = xSemaphoreCreateMutex();

    /*
     * configASSERT()는 생성 실패를 빠르게 확인하기 위한 안전장치입니다.
     * Queue나 Mutex 생성이 실패하면 보통 heap 부족을 의심합니다.
     * 조건이 거짓이면 잘못된 handle을 사용하기 전에 실행을 멈춥니다.
     */
    configASSERT(orderQueue != 0);
    configASSERT(printQueue != 0);
    configASSERT(printerMutex != 0);

    /*
     * 우선순위 숫자가 높을수록 더 높은 우선순위입니다.
     * ButtonTask는 입력 반응성을 위해 3, 나머지는 2로 둡니다.
     * 높은 우선순위 Task도 Queue 대기나 delay로 Blocked 상태가 되어야
     * 낮은 우선순위 Task가 실행될 수 있습니다.
     *
     * xTaskCreate() 사용 형식:
     *   xTaskCreate(taskFunction, taskName, stackDepth,
     *               parameters, priority, createdTaskHandle);
     *
     * 현재 호출 해석:
     *   ButtonTask : 실행 함수
     *   "Button"   : Task 이름
     *   256        : StackType_t 256개 크기의 stack
     *   0          : 전달할 parameter 없음
     *   3          : 우선순위 3
     *   0          : 생성된 Task Handle을 따로 받지 않음
     *
     * 성공하면 pdPASS, heap 부족 등으로 실패하면 오류 값을 반환합니다.
     * 실제 제품에서는 반환값을 검사하거나 configASSERT로 확인해야 합니다.
     */
    (void) xTaskCreate(ButtonTask, "Button", 256, 0, 3, 0);
    (void) xTaskCreate(PaymentTask, "Payment", 256, 0, 2, 0);
    (void) xTaskCreate(PrinterTask, "Printer", 256, 0, 2, 0);

    uart_puts("[FreeRTOS] Scheduler를 시작합니다.\n");
    vTaskStartScheduler();

    uart_puts("[FreeRTOS] Scheduler 시작에 실패했습니다.\n");
    for (;;) {
    }
}

static void ButtonTask(void *parameters) {
    /* xTaskCreate()에서 네 번째 인자로 0을 전달했으므로 사용할 값이 없습니다. */
    (void) parameters;

    for (;;) {
        /*
         * 실제 키오스크에서는 GPIO, 터치 이벤트, 버튼 interrupt 등으로
         * 주문 입력이 발생합니다. 여기서는 1초마다 주문 버튼이 눌렸다고
         * 가정해서 이벤트를 만듭니다.
         */
        KioskEvent event = {
            .type = EVENT_ORDER_REQUESTED,
            .item = "americano x1"
        };

        uart_puts("[ButtonTask] 주문 버튼이 눌렸습니다.\n");

        /*
         * xQueueSend()는 Queue의 뒤쪽에 item을 복사해서 넣습니다.
         *
         * 사용 형식:
         *   BaseType_t xQueueSend(
         *       QueueHandle_t queue,
         *       const void *itemToQueue,
         *       TickType_t ticksToWait
         *   );
         *
         * 매개변수:
         *   queue       : 데이터를 넣을 Queue Handle
         *   itemToQueue : 복사할 데이터의 주소. 여기서는 &event
         *   ticksToWait : Queue가 가득 찼을 때 공간을 기다릴 최대 Tick 수
         *
         * 반환값:
         *   pdPASS         : 전송 성공
         *   errQUEUE_FULL  : 대기 시간이 지나도 Queue가 가득 차 전송 실패
         *
         * 사용 방법:
         *   KioskEvent event를 만든 뒤 xQueueSend(orderQueue, &event, 대기시간)
         *   형태로 호출합니다. 두 번째 인자에는 값이 아니라 값의 주소를 줍니다.
         *
         * 따라서 이 함수가 반환된 뒤 지역 변수 event의 수명이 끝나도 Queue에
         * 복사된 KioskEvent 값은 유지됩니다.
         *
         * 세 번째 인자 pdMS_TO_TICKS(100)은 Queue가 가득 찼을 때
         * 최대 100ms까지 기다리겠다는 뜻입니다.
         * 성공 시 pdPASS, 시간이 지나도 공간이 없으면 실패 값을 반환합니다.
         * 실전에서는 반환값을 검사해 재시도하거나 유실을 기록해야 합니다.
         */
        (void) xQueueSend(orderQueue, &event, pdMS_TO_TICKS(100));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void PaymentTask(void *parameters) {
    /* Task 시작 시 별도로 전달받은 설정 데이터가 없습니다. */
    (void) parameters;

    for (;;) {
        /* Queue에서 받은 KioskEvent 사본이 저장될 지역 변수입니다. */
        KioskEvent event;

        /*
         * xQueueReceive()는 Queue의 앞쪽 item을 수신 변수로 복사한 후 제거합니다.
         *
         * 사용 형식:
         *   BaseType_t xQueueReceive(
         *       QueueHandle_t queue,
         *       void *receiveBuffer,
         *       TickType_t ticksToWait
         *   );
         *
         * 매개변수:
         *   queue         : 데이터를 받을 Queue Handle
         *   receiveBuffer : 수신 데이터를 저장할 공간의 주소. 여기서는 &event
         *   ticksToWait   : Queue가 비었을 때 데이터를 기다릴 최대 Tick 수
         *
         * 반환값:
         *   pdPASS        : 수신 성공
         *   errQUEUE_EMPTY: 대기 시간이 지나도 데이터가 없어 수신 실패
         *
         * 사용 방법:
         *   수신할 구조체 변수를 먼저 준비하고 &event처럼 주소를 전달합니다.
         *   반환값이 pdPASS일 때만 event의 내용을 사용해야 합니다.
         *
         * portMAX_DELAY를 주면 이벤트가 들어올 때까지 계속 기다립니다.
         *
         * 기다리는 동안 이 Task는 blocked 상태가 되므로 CPU를 낭비하지 않습니다.
         * 수신에 성공하면 가장 먼저 들어온 item이 event로 복사되고 Queue에서
         * 제거됩니다.
         */
        if (xQueueReceive(orderQueue, &event, portMAX_DELAY) == pdPASS) {
            uart_puts("[PaymentTask] 주문 정보를 받았습니다.\n");
            uart_puts("[PaymentTask] 결제가 승인되었습니다.\n");

            /*
             * 결제가 끝나면 PrinterTask가 처리할 출력 요청 이벤트를 만듭니다.
             * 주문 Queue와 출력 Queue를 분리하면 각 Task가 자신이 처리할
             * 이벤트만 받게 되어 흐름이 단순해집니다.
             */
            KioskEvent printEvent = {
                .type = EVENT_PRINT_REQUESTED,
                /*
                 * 문자열을 새로 복사하는 대신 기존 문자열 리터럴의 주소를
                 * 다음 이벤트에도 전달합니다.
                 */
                .item = event.item
            };

            (void) xQueueSend(printQueue, &printEvent, pdMS_TO_TICKS(100));
        }
    }
}

static void PrinterTask(void *parameters) {
    /* Task 시작 시 별도로 전달받은 설정 데이터가 없습니다. */
    (void) parameters;

    for (;;) {
        /* printQueue에서 꺼낸 출력 요청을 보관할 지역 변수입니다. */
        KioskEvent event;

        /* 출력 요청이 없으면 무한 대기하며 CPU 실행 시간을 소비하지 않습니다. */
        if (xQueueReceive(printQueue, &event, portMAX_DELAY) == pdPASS) {
            uart_puts("[PrinterTask] 출력 요청을 받았습니다.\n");

            /*
             * xSemaphoreTake()로 Mutex의 소유권을 획득합니다.
             *
             * 사용 형식:
             *   BaseType_t xSemaphoreTake(
             *       SemaphoreHandle_t mutex,
             *       TickType_t ticksToWait
             *   );
             *
             * 매개변수:
             *   mutex       : 획득할 Mutex Handle
             *   ticksToWait : 사용 중일 때 반환을 기다릴 최대 Tick 수
             *
             * 반환값:
             *   pdTRUE : Mutex 획득 성공. 이제 공유 자원을 사용할 수 있음
             *   pdFALSE: 제한 시간 안에 Mutex를 얻지 못함
             *
             * 사용 방법:
             *   반환값이 성공인 블록 안에서만 공유 자원을 사용합니다.
             *   이 예제의 pdPASS 비교도 값이 같은 성공 매크로라 동작합니다.
             * 이미 다른 Task가 프린터를 사용 중이면 여기서 기다립니다.
             * portMAX_DELAY는 Mutex를 얻을 수 있을 때까지 현재 PrinterTask를
             * Blocked 상태로 두라는 뜻입니다.
             */
            if (xSemaphoreTake(printerMutex, portMAX_DELAY) == pdPASS) {
                uart_puts("[PrinterTask] 프린터 Mutex를 획득했습니다.\n");
                uart_puts("[PrinterTask] 영수증을 출력했습니다.\n");

                /*
                 * xSemaphoreGive()는 획득했던 Mutex의 소유권을 반환합니다.
                 *
                 * 사용 형식:
                 *   BaseType_t xSemaphoreGive(SemaphoreHandle_t mutex);
                 *
                 * 매개변수:
                 *   mutex : 반환할 Mutex Handle
                 *
                 * 반환값:
                 *   pdTRUE : 정상적으로 반환함
                 *   pdFALSE: 반환할 수 없음
                 *
                 * 사용 방법:
                 *   xSemaphoreTake()에 성공한 Task가 공유 자원 사용을 끝낸 직후
                 *   같은 Mutex Handle을 xSemaphoreGive()에 전달합니다.
                 *
                 * 공유 자원 사용이 끝나면 반드시 Mutex를 반환해야 합니다.
                 * 반환하지 않으면 이후 출력 요청이 계속 막힐 수 있습니다.
                 * Mutex는 획득한 동일 Task가 반환해야 합니다.
                 */
                xSemaphoreGive(printerMutex);
                uart_puts("[PrinterTask] 프린터 Mutex를 반환했습니다.\n");
            }
        }
    }
}
