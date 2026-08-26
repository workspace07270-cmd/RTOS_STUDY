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

/*
 * Step 3 목표:
 *   Step 2의 주문/결제/출력 흐름에 Software Timer를 추가합니다.
 *
 * Software Timer는 특정 주기마다 callback을 실행합니다.
 * 키오스크에서는 장치 상태 점검, 서버 동기화, 결제 timeout 확인 같은
 * 주기 작업에 사용할 수 있습니다.
 */

/*
 * Step 3에서 함께 사용하는 FreeRTOS 개념:
 *
 * Task
 *   scheduler가 실행 순서를 관리하는 독립 작업 단위입니다.
 *   ButtonTask, PaymentTask, PrinterTask가 각각 자신의 역할을 반복합니다.
 *
 * Queue
 *   Task 사이에서 이벤트를 FIFO 순서로 전달하는 대기열입니다.
 *   이 예제에서는 주문 이벤트와 출력 이벤트를 구조체 값으로 복사해 전달합니다.
 *
 * Mutex
 *   하나뿐인 프린터를 여러 Task가 동시에 사용하지 못하도록 보호하는 잠금입니다.
 *   획득한 Task가 공유 자원 사용을 마치면 반드시 반환해야 합니다.
 *
 * Software Timer
 *   하드웨어 Timer 레지스터를 애플리케이션이 직접 제어하는 대신, FreeRTOS가
 *   지정한 시간이 지난 뒤 callback 실행을 요청하는 커널 객체입니다.
 *   별도의 사용자 Task를 하나 더 만들지 않고 짧은 주기 작업을 예약할 수 있습니다.
 *
 * 전체 흐름:
 *
 *   ButtonTask
 *     -> orderQueue
 *     -> PaymentTask
 *     -> printQueue
 *     -> PrinterTask
 *     -> printerMutex 획득
 *     -> 영수증 출력
 *     -> printerMutex 반환
 *
 *   Timer service task
 *     -> 2000ms마다 MonitorTimerCallback 실행
 */

typedef enum {
    /* 주문 접수 이벤트이며 명시적으로 숫자 1부터 시작합니다. */
    EVENT_ORDER_REQUESTED = 1,
    /* 결제 후 영수증 출력 요청 이벤트입니다. */
    EVENT_PRINT_REQUESTED
} KioskEventType;

typedef struct {
    /* 소비 Task가 이벤트의 목적을 판단하는 값입니다. */
    KioskEventType type;
    /*
     * 문자열 자체가 아니라 주소를 저장합니다.
     * Queue는 포인터 값만 복사하므로 지역 배열을 가리킬 때는 수명에 주의해야 합니다.
     * 이 예제의 문자열 리터럴은 프로그램이 끝날 때까지 유지됩니다.
     */
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
    /* 시작 코드에서 UART가 준비된 뒤 main()이 호출되므로 바로 출력할 수 있습니다. */
    uart_puts("[부팅] 3단계 키오스크 통합 예제를 시작합니다.\n");

    /*
     * Step 2와 같은 Queue/Mutex 구성입니다.
     * 여기까지는 주문 이벤트와 출력 이벤트를 Task 사이에 전달하는 구조입니다.
     * 각 Queue는 KioskEvent 사본을 최대 4개까지 FreeRTOS heap에 보관합니다.
     */
    orderQueue = xQueueCreate(4, sizeof(KioskEvent));
    printQueue = xQueueCreate(4, sizeof(KioskEvent));
    printerMutex = xSemaphoreCreateMutex();

    /*
     * xTimerCreate()는 Software Timer를 생성합니다.
     *
     * 매개변수 순서:
     *   1. Timer 이름
     *   2. Timer 주기
     *   3. 자동 반복 여부
     *   4. Timer ID
     *   5. 만료 시 실행할 callback 함수
     *
     * pdTRUE는 auto-reload timer를 의미합니다.
     * 즉, 2000ms마다 반복해서 callback이 실행됩니다.
     *
     * 네 번째 인자 Timer ID는 callback에서 여러 Timer를 구분할 때 쓸 수 있으며,
     * 여기서는 별도 데이터가 필요 없으므로 0을 전달합니다.
     *
     * pdFALSE를 사용하면 한 번 만료된 뒤 자동으로 다시 시작하지 않는
     * one-shot timer가 됩니다.
     */
    monitorTimer = xTimerCreate(
        "MonitorTimer",
        pdMS_TO_TICKS(2000),
        pdTRUE,
        0,
        MonitorTimerCallback
    );

    /* heap 부족 등으로 객체 생성이 실패했는지 Task 실행 전에 확인합니다. */
    configASSERT(orderQueue != 0);
    configASSERT(printQueue != 0);
    configASSERT(printerMutex != 0);
    configASSERT(monitorTimer != 0);

    /*
     * stack 256은 byte가 아니라 StackType_t 256개입니다.
     * ButtonTask의 우선순위 3은 나머지 Task의 2보다 높습니다.
     *
     * xTaskCreate(taskFunction, taskName, stackDepth,
     *             parameters, priority, createdTaskHandle)
     *
     * 실행 함수, 이름, stack 크기, 전달값, 우선순위, Handle 저장 위치 순으로
     * 전달합니다. 성공하면 pdPASS를 반환하며, 마지막 인자 0은 생성된 Task를
     * 나중에 직접 제어하기 위한 Handle을 받지 않겠다는 뜻입니다.
     */
    (void) xTaskCreate(ButtonTask, "Button", 256, 0, 3, 0);
    (void) xTaskCreate(PaymentTask, "Payment", 256, 0, 2, 0);
    (void) xTaskCreate(PrinterTask, "Printer", 256, 0, 2, 0);

    /*
     * xTimerStart()는 Timer를 시작하라는 명령을 FreeRTOS timer service task에
     * 전달합니다. scheduler가 시작된 뒤부터 주기적으로 callback이 실행됩니다.
     *
     * 사용 형식:
     *   BaseType_t xTimerStart(
     *       TimerHandle_t timer,
     *       TickType_t ticksToWait
     *   );
     *
     * 매개변수:
     *   timer       : xTimerCreate()로 생성한 Timer Handle
     *   ticksToWait : Timer 명령 Queue가 가득 찼을 때 기다릴 최대 Tick 수
     *
     * 반환값:
     *   pdPASS : 시작 명령을 Timer 명령 Queue에 넣는 데 성공
     *   pdFAIL : 제한 시간 안에 시작 명령을 넣지 못함
     *
     * 사용 방법:
     *   1. xTimerCreate()로 Timer를 만들고 Handle을 검사합니다.
     *   2. xTimerStart(timerHandle, 대기시간)로 시작을 요청합니다.
     *   3. scheduler가 실행되면 설정된 주기 후 callback이 호출됩니다.
     *
     * pdPASS는 callback 실행 완료가 아니라 '시작 명령 전달 성공'이라는 뜻입니다.
     * 즉, xTimerStart() 호출 위치에서 callback이 즉시 실행되는 것이 아닙니다.
     * timer command queue에 시작 요청이 전달되고 scheduler 시작 후 timer
     * service task가 그 요청과 만료 시점을 처리합니다.
     * scheduler 시작 전에는 호출 Task가 기다릴 수 없으므로 block time은 0입니다.
     */
    (void) xTimerStart(monitorTimer, 0);

    uart_puts("[FreeRTOS] Scheduler를 시작합니다.\n");
    vTaskStartScheduler();

    uart_puts("[FreeRTOS] Scheduler 시작에 실패했습니다.\n");
    for (;;) {
    }
}

static void ButtonTask(void *parameters) {
    /* xTaskCreate()의 네 번째 인자가 0이므로 전달받은 설정값이 없습니다. */
    (void) parameters;

    for (;;) {
        /*
         * 실제 제품에서는 터치 또는 GPIO 입력 시 이벤트를 만들지만,
         * 예제에서는 1초마다 주문 버튼을 누른 상황을 반복해서 만듭니다.
         */
        KioskEvent event = {
            .type = EVENT_ORDER_REQUESTED,
            .item = "americano x1"
        };

        uart_puts("[ButtonTask] 주문 버튼이 눌렸습니다.\n");

        /*
         * 지역 변수 event의 KioskEvent 값을 orderQueue 내부로 복사합니다.
         *
         * xQueueSend(queue, itemAddress, ticksToWait)
         *   queue       : 목적지 Queue Handle
         *   itemAddress : 복사할 item의 주소
         *   ticksToWait : Queue가 가득 찼을 때 기다릴 최대 Tick 수
         *   반환값      : 성공 pdPASS, 공간을 얻지 못하면 errQUEUE_FULL
         *
         * Queue가 가득 차면 최대 100ms 동안 공간을 기다립니다.
         * 반환 후 지역 변수는 다음 반복에서 다시 만들어져도 Queue 사본은 유지됩니다.
         *
         * 이 예제는 흐름을 단순화하려고 결과를 버리지만, 실제 제품에서는
         * pdPASS인지 검사하여 주문 이벤트 유실에 대응해야 합니다.
         */
        (void) xQueueSend(orderQueue, &event, pdMS_TO_TICKS(100));

        /*
         * 현재 Task를 1초 동안 Blocked 상태로 바꿉니다.
         * 단순 빈 반복문과 달리 CPU는 PaymentTask, PrinterTask 등에 넘어갑니다.
         */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void PaymentTask(void *parameters) {
    /* 이 Task는 시작 매개변수를 사용하지 않습니다. */
    (void) parameters;

    for (;;) {
        /* orderQueue에서 수신한 구조체 사본을 담을 지역 변수입니다. */
        KioskEvent event;

        /*
         * 주문이 없으면 portMAX_DELAY 동안 Blocked 상태로 기다립니다.
         *
         * xQueueReceive(queue, receiveBuffer, ticksToWait)
         *   queue         : 원본 Queue Handle
         *   receiveBuffer : item 사본을 저장할 변수의 주소
         *   ticksToWait   : Queue가 비었을 때 기다릴 최대 Tick 수
         *   반환값        : 성공 pdPASS, 받지 못하면 errQUEUE_EMPTY
         *
         * 반환값이 pdPASS일 때만 수신 변수 event를 사용합니다.
         * 주문이 들어오면 FIFO의 가장 오래된 이벤트가 event에 복사되고
         * Queue에서는 제거됩니다. 대기 중에는 CPU를 소비하지 않습니다.
         */
        if (xQueueReceive(orderQueue, &event, portMAX_DELAY) == pdPASS) {
            uart_puts("[PaymentTask] 주문 정보를 받았습니다.\n");

            /*
             * 실제 시스템이라면 여기서 카드 단말기와 통신하고 승인 결과를
             * Spring Boot 서버에 전달합니다. 예제에서는 즉시 승인된 것으로 봅니다.
             */
            uart_puts("[PaymentTask] 결제가 승인되었습니다.\n");

            /* 결제 완료 후 PrinterTask에 넘길 별도의 출력 이벤트를 만듭니다. */
            KioskEvent printEvent = {
                .type = EVENT_PRINT_REQUESTED,
                /*
                 * 문자열 데이터 전체가 아니라 기존 문자열 리터럴의 주소를
                 * 다음 이벤트에 복사합니다.
                 */
                .item = event.item
            };

            /*
             * 출력 요청을 printQueue에 값으로 복사합니다.
             * Queue가 가득 차면 최대 100ms 기다리며, 실제 코드라면
             * 실패 반환값을 확인해 재시도 또는 오류 처리를 해야 합니다.
             */
            (void) xQueueSend(printQueue, &printEvent, pdMS_TO_TICKS(100));
        }
    }
}

static void PrinterTask(void *parameters) {
    /* 이 Task는 시작 매개변수를 사용하지 않습니다. */
    (void) parameters;

    for (;;) {
        /* printQueue에서 꺼낸 출력 요청의 사본을 저장합니다. */
        KioskEvent event;

        /*
         * 출력 요청이 생길 때까지 Blocked 상태로 기다립니다.
         * 이벤트 도착 전에는 계속 반복 검사하지 않으므로 CPU를 낭비하지 않습니다.
         */
        if (xQueueReceive(printQueue, &event, portMAX_DELAY) == pdPASS) {
            uart_puts("[PrinterTask] 출력 요청을 받았습니다.\n");

            /*
             * 하나뿐인 프린터를 사용하기 전에 Mutex 소유권을 획득합니다.
             *
             * xSemaphoreTake(mutex, ticksToWait)
             *   mutex       : 획득할 Mutex Handle
             *   ticksToWait : 다른 Task가 사용 중일 때 기다릴 최대 Tick 수
             *   반환값      : 획득 성공 pdTRUE, 제한 시간 초과 시 pdFALSE
             *
             * 이 코드의 pdPASS 비교도 값이 같은 성공 매크로이므로 동작합니다.
             * 반드시 획득에 성공한 경우에만 보호 대상인 프린터를 사용합니다.
             * 다른 Task가 이미 사용 중이라면 반환될 때까지 Blocked 상태가 됩니다.
             * FreeRTOS Mutex는 우선순위 역전을 줄이기 위한 우선순위 상속을 지원합니다.
             */
            if (xSemaphoreTake(printerMutex, portMAX_DELAY) == pdPASS) {
                uart_puts("[PrinterTask] 프린터 Mutex를 획득했습니다.\n");

                /*
                 * 실제 제품에서는 이 구간에서 UART, USB 등의 장치 드라이버로
                 * 영수증 데이터를 프린터에 전송합니다.
                 */
                uart_puts("[PrinterTask] 영수증을 출력했습니다.\n");

                /*
                 * xSemaphoreGive(mutex)는 현재 Task가 획득한 Mutex를 반환합니다.
                 * 성공하면 pdTRUE, 반환할 수 없으면 pdFALSE를 반환합니다.
                 * Take에 성공한 동일 Task가 공유 자원 사용 직후 호출해야 합니다.
                 *
                 * 공유 자원 사용이 끝났으므로 반드시 Mutex를 반환합니다.
                 * 획득한 Task가 반환하지 않으면 다음 출력 작업이 영원히 막힐 수 있습니다.
                 */
                xSemaphoreGive(printerMutex);
                uart_puts("[PrinterTask] 프린터 Mutex를 반환했습니다.\n");
            }
        }
    }
}

static void MonitorTimerCallback(TimerHandle_t timer) {
    /*
     * Timer callback은 FreeRTOS timer service task 문맥에서 실행됩니다.
     * 따라서 오래 걸리는 작업이나 오래 block되는 작업은 callback 안에서
     * 직접 처리하지 않는 것이 좋습니다.
     *
     * callback이 오래 실행되면 같은 timer service task가 관리하는 다른 Timer의
     * callback 실행도 늦어질 수 있습니다. 긴 작업이 필요하면 callback에서는
     * Queue나 Task notification으로 작업 Task에 알리고 빠르게 반환합니다.
     *
     * 인자 timer는 지금 만료된 Timer의 Handle입니다. Timer ID를 지정했다면
     * pvTimerGetTimerID(timer)로 callback 안에서 관련 데이터를 찾을 수 있습니다.
     */
    /* 이 예제에서는 Timer Handle을 직접 사용하지 않아 경고 방지용으로 버립니다. */
    (void) timer;

    /*
     * 학습용이라 짧은 UART 로그만 출력합니다. 실제 제품에서는 UART 출력 시간도
     * 고려하고, 오래 걸리는 장치 점검은 전용 Task에 요청하는 편이 안전합니다.
     */
    uart_puts("[MonitorTimer] 키오스크 장치가 정상 동작 중입니다.\n");
}
