/* FreeRTOS 기본 자료형, 설정값, 공통 매크로를 제공합니다. */
#include "FreeRTOS.h"
/* Task 생성·지연 및 scheduler 시작 API를 제공합니다. */
#include "task.h"
/* QEMU의 UART0를 통해 WSL 콘솔에 문자열을 출력합니다. */
#include "uart.h"

/*
 * Step 1 목표:
 *   FreeRTOS에서 Task를 생성하고 scheduler를 시작하는 최소 흐름을 확인합니다.
 *
 * 이 단계에서는 Queue, Mutex, Timer를 사용하지 않습니다.
 * 먼저 "Task는 일반 함수와 다르게 scheduler가 반복 실행을 관리한다"는 감각을
 * 잡는 것이 목적입니다.
 */

/*
 * [개념 1] Task란?
 *
 * Task는 FreeRTOS가 실행 순서를 관리하는 독립적인 작업 단위입니다.
 * 일반 함수는 호출한 함수가 직접 실행하지만, Task는 main()이 직접 반복 호출하지
 * 않고 scheduler가 실행 가능한 Task를 선택해 CPU 실행 시간을 나누어 줍니다.
 *
 * 키오스크에서는 기능별로 Task를 나눌 수 있습니다.
 *
 *   ButtonTask  : 버튼 입력 감지
 *   PaymentTask : 결제 처리
 *   PrinterTask : 영수증 출력
 *
 * Task 함수는 보통 for (;;) 무한 루프를 가지며 다음 상태를 오갑니다.
 *
 *   Ready   : CPU를 사용할 준비가 된 상태
 *   Running : 현재 CPU에서 실행 중인 상태
 *   Blocked : delay, Queue 대기 등으로 잠시 기다리는 상태
 *
 * [개념 2] Scheduler란?
 *
 * Scheduler는 Ready 상태의 Task 중 지금 실행할 Task를 선택하는 FreeRTOS의
 * 실행 관리자입니다. 우선순위가 높은 Task가 먼저 선택되며, 실행 중인 Task가
 * vTaskDelay() 등으로 Blocked 상태가 되면 다른 Task가 CPU를 사용할 수 있습니다.
 *
 * 이 예제의 흐름:
 *
 *   main()
 *     -> HeartbeatTask와 UiTask 생성
 *     -> scheduler 시작
 *     -> UiTask 또는 HeartbeatTask 실행
 *     -> 실행한 Task가 vTaskDelay()로 Blocked
 *     -> scheduler가 다른 Ready Task 실행
 */

/*
 * static 함수는 현재 main.c 안에서만 사용할 수 있습니다.
 * void *parameters는 xTaskCreate()의 네 번째 인자를 받는 통로입니다.
 */
static void HeartbeatTask(void *parameters);
static void UiTask(void *parameters);

int main(void) {
    /* 시작 코드는 이미 UART를 초기화했으므로 여기서는 바로 출력할 수 있습니다. */
    uart_puts("[부팅] 1단계 Task 기본 예제를 시작합니다.\n");

    /*
     * xTaskCreate()는 FreeRTOS Task를 생성합니다.
     *
     * 사용 형식:
     *   BaseType_t xTaskCreate(
     *       TaskFunction_t taskFunction,
     *       const char *taskName,
     *       configSTACK_DEPTH_TYPE stackDepth,
     *       void *parameters,
     *       UBaseType_t priority,
     *       TaskHandle_t *createdTask
     *   );
     *
     * 매개변수:
     *   taskFunction : scheduler가 실행할 Task 함수 주소
     *   taskName     : 디버깅할 때 구분하기 위한 Task 이름
     *   stackDepth   : Task 전용 stack의 크기. byte가 아닌 StackType_t 개수
     *   parameters   : Task 함수의 void *parameters로 전달할 값
     *   priority     : Task 우선순위. 숫자가 클수록 높은 우선순위
     *   createdTask  : 생성된 Task Handle을 받을 주소. 필요 없으면 NULL 또는 0
     *
     * 반환값:
     *   pdPASS       : Task 생성 성공
     *   errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY : heap 부족 등으로 생성 실패
     *
     * 사용 순서:
     *   1. void TaskName(void *parameters) 형태의 Task 함수를 작성합니다.
     *   2. scheduler 시작 전에 xTaskCreate()로 Task를 등록합니다.
     *   3. 반환값이 pdPASS인지 확인합니다.
     *   4. 필요한 Task를 모두 만든 뒤 vTaskStartScheduler()를 호출합니다.
     *
     * Task handle은 생성한 Task를 나중에 중지, 재개, 삭제하거나 상태를 조회할 때
     * 사용하는 식별자입니다.
     *
     * 여기서는 handle을 나중에 제어하지 않으므로 마지막 인자를 0으로 둡니다.
     *
     * stack 크기 256은 256 byte가 아니라 StackType_t 256개입니다.
     * Cortex-M3에서 StackType_t가 4 byte라면 약 1 KiB에 해당합니다.
     *
     * 네 번째 인자 0은 Task에 전달할 값이 없다는 뜻입니다.
     * xTaskCreate()는 성공하면 pdPASS를 반환하지만, 이 최소 예제에서는
     * 반환값을 사용하지 않아 (void)로 명시적으로 버립니다.
     */
    (void) xTaskCreate(HeartbeatTask, "Heartbeat", 256, 0, 1, 0);
    /*
     * UiTask의 우선순위 2는 HeartbeatTask의 1보다 높습니다.
     * 둘 다 Ready 상태라면 UiTask가 먼저 실행되지만, UiTask가 delay로
     * Blocked 상태가 되면 HeartbeatTask도 실행 기회를 얻습니다.
     */
    (void) xTaskCreate(UiTask, "UiTask", 256, 0, 2, 0);

    /*
     * vTaskStartScheduler()를 호출하면 FreeRTOS scheduler가 시작됩니다.
     * 이 이후부터는 main()이 직접 순서대로 함수를 호출하는 구조가 아니라,
     * scheduler가 준비된 Task 중 실행할 Task를 선택합니다.
     * 시스템 유지에 필요한 Idle Task도 FreeRTOS 내부에서 함께 실행됩니다.
     */
    uart_puts("[FreeRTOS] Scheduler를 시작합니다.\n");
    vTaskStartScheduler();

    /*
     * 정상적인 경우 여기까지 돌아오지 않습니다.
     * 만약 heap 부족 등으로 scheduler 시작에 실패하면 아래 로그가 보입니다.
     * bare-metal 프로그램은 return할 운영체제가 없으므로 무한 루프로 정지합니다.
     */
    uart_puts("[FreeRTOS] Scheduler 시작에 실패했습니다.\n");
    for (;;) {
    }
}

static void HeartbeatTask(void *parameters) {
    /*
     * FreeRTOS Task 함수는 void * parameter 하나를 받는 형태입니다.
     * 이 예제에서는 parameter를 사용하지 않으므로 경고 방지를 위해 버립니다.
     */
    (void) parameters;

    /*
     * Task는 보통 무한 루프 구조를 가집니다.
     * 한 번 실행하고 끝나는 함수가 아니라, 자신의 일을 반복해서 수행합니다.
     */
    for (;;) {
        uart_puts("[HeartbeatTask] 시스템이 정상 동작 중입니다.\n");

        /*
         * vTaskDelay()는 현재 Task를 지정된 tick 동안 blocked 상태로 보냅니다.
         * blocked 상태가 되면 CPU를 다른 Task가 사용할 수 있습니다.
         * 빈 while 반복으로 시간을 보내는 busy waiting과 달리 CPU를 점유하지
         * 않으므로 여러 Task가 함께 동작할 수 있습니다.
         *
         * pdMS_TO_TICKS(1000)은 1000ms를 FreeRTOS tick 단위로 변환합니다.
         * 변환 결과는 FreeRTOSConfig.h의 configTICK_RATE_HZ 값에 따라 달라집니다.
         */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void UiTask(void *parameters) {
    /* 전달받은 parameter가 없으므로 미사용 경고를 막기 위해 명시적으로 버립니다. */
    (void) parameters;

    /* Task 함수는 보통 return하지 않고 자신의 역할을 계속 반복합니다. */
    for (;;) {
        uart_puts("[UiTask] 키오스크 입력을 기다리는 중입니다.\n");

        /*
         * UiTask는 1500ms마다 로그를 출력합니다.
         * HeartbeatTask와 delay 값이 다르기 때문에 두 Task 로그가 서로 다른
         * 주기로 출력되는 것을 볼 수 있습니다.
         */
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}
