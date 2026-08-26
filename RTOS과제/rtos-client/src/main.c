#include "http_client.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Spring Boot의 JSON 명령에서 RTOS가 실제로 사용할 값만 보관합니다. */
typedef struct { uint32_t id; char task_type[32]; char payload[256]; } work_t;
/* 모든 RTOS 태스크가 함께 사용하는 Spring Boot 접속 정보입니다. */
static http_server_t server;
/* 교육용 단일 작업 제어 플래그: 같은 PENDING 명령의 중복 태스크 생성을 막습니다. */
static volatile BaseType_t worker_running = pdFALSE;

static const char *json_value(const char *json, const char *key) {
    /* 예: key가 id이면 JSON 본문에서 \"id\": 바로 다음 위치를 찾습니다. */
    static char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *value = strstr(json, pattern);
    return value == NULL ? NULL : value + strlen(pattern);
}

static int json_string(const char *json, const char *key, char *out, size_t size) {
    /* 문자열 값을 여는 따옴표부터 닫는 따옴표 전까지 출력 배열에 복사합니다. */
    const char *value = json_value(json, key);
    if (value == NULL || *value++ != '\"') return -1;
    size_t i = 0;
    while (*value && *value != '\"' && i + 1 < size) out[i++] = *value++;
    out[i] = '\0';
    return *value == '\"' ? 0 : -1;
}

static int parse_work(const char *json, work_t *work) {
    /* 빈 배열은 Spring Boot에 대기 중인 명령이 없다는 의미입니다. */
    if (strstr(json, "[]") != NULL) return 0;
    const char *id = json_value(json, "id");
    if (id == NULL) return -1;
    work->id = (uint32_t) strtoul(id, NULL, 10);
    return json_string(json, "taskType", work->task_type, sizeof(work->task_type)) == 0 &&
           json_string(json, "payload", work->payload, sizeof(work->payload)) == 0 ? 1 : -1;
}

static void one_shot_task(void *parameter) {
    /* 생성자가 넘긴 동적 메모리의 값을 지역 변수에 복사한 뒤 즉시 반환합니다. */
    work_t work = *(work_t *) parameter;
    vPortFree(parameter);
    printf("[WorkerTask] 영수증 출력 태스크 시작: id=%u\n", work.id);

    /* React가 보낸 주문번호|주문상세|결제금액을 영수증 항목으로 분리합니다. */
    char receipt[sizeof(work.payload)];
    snprintf(receipt, sizeof(receipt), "%s", work.payload);
    char *order_id = strtok(receipt, "|");
    char *items = strtok(NULL, "|");
    char *amount = strtok(NULL, "|");
    if (order_id == NULL || items == NULL || amount == NULL) {
        order_id = "UNKNOWN";
        items = work.payload;
        amount = "0";
    }

    printf("\n");
    printf("+--------------------------------------+\n");
    printf("|          KIOSK RECEIPT               |\n");
    printf("+--------------------------------------+\n");
    printf("  주문 번호 : %s\n", order_id);
    printf("  주문 상세 : %s\n", items);
    printf("  결제 금액 : %s원\n", amount);
    printf("+--------------------------------------+\n");
    printf("|       이용해 주셔서 감사합니다       |\n");
    printf("+--------------------------------------+\n\n");
    fflush(stdout);
    /* 실제 프린터의 출력 시간을 표현하기 위한 1.5초 지연입니다. */
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* 출력 완료 결과를 같은 명령 ID의 complete API로 전송합니다. */
    char path[96], json[256], body[1024];
    http_response_t response = {0};
    snprintf(path, sizeof(path), "/api/tasks/%u/complete", work.id);
    snprintf(json, sizeof(json),
            "{\"result\":\"receipt printed successfully: %s\"}", order_id);
    if (http_request(&server, "PATCH", path, json, body, sizeof(body), &response) == 0 &&
            response.status_code == 200)
        printf("[WorkerTask] 완료 결과 전송: id=%u\n", work.id);
    else fprintf(stderr, "[WorkerTask] 결과 전송 실패: id=%u\n", work.id);
    /* 다음 명령을 받을 수 있게 표시하고, 한 번 실행한 현재 태스크를 삭제합니다. */
    worker_running = pdFALSE;
    vTaskDelete(NULL);
}

static void command_poll_task(void *parameter) {
    (void) parameter;
    /* 이 태스크는 프로그램이 실행되는 동안 계속 살아서 서버 명령을 감시합니다. */
    for (;;) {
        /* WorkerTask 실행 중에는 같은 PENDING 명령을 다시 가져오지 않습니다. */
        if (!worker_running) {
            char body[1024]; http_response_t response = {0}; work_t parsed = {0};
            if (http_request(&server, "GET", "/api/tasks/pending", NULL,
                    body, sizeof(body), &response) == 0 && response.status_code == 200 &&
                    parse_work(body, &parsed) == 1) {
                work_t *work = pvPortMalloc(sizeof(*work));
                if (work != NULL) {
                    /* 파싱한 명령을 동적 메모리에 복사해 새 태스크의 인자로 넘깁니다. */
                    *work = parsed; worker_running = pdTRUE;
                    /* 명령 한 건마다 우선순위 3의 일회성 작업 태스크를 등록합니다. */
                    if (xTaskCreate(one_shot_task, "WorkerTask", 2048, work, 3, NULL) != pdPASS) {
                        worker_running = pdFALSE; vPortFree(work);
                    } else printf("[CommandPollTask] 태스크 등록: id=%u\n", parsed.id);
                }
            }
        }
        /* CPU를 계속 점유하지 않고 다음 서버 조회까지 1초간 Blocked 상태로 쉽니다. */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vApplicationMallocFailedHook(void) { abort(); }

int main(int argc, char **argv) {
    /* 실행 인자가 없으면 같은 PC의 Spring Boot 8080 포트를 사용합니다. */
    const char *url = argc >= 2 ? argv[1] : "http://localhost:8080";
    if (http_server_parse(url, &server) < 0) return EXIT_FAILURE;
    /* 서버를 계속 감시할 상시 태스크를 먼저 만들고 FreeRTOS 스케줄러를 시작합니다. */
    configASSERT(xTaskCreate(command_poll_task, "CommandPollTask", 2048, NULL, 2, NULL) == pdPASS);
    printf("[FreeRTOS] Spring Boot 명령 대기: %s\n", url);
    vTaskStartScheduler();
    return EXIT_FAILURE;
}
