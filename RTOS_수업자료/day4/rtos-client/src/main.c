#include "http_client.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { uint32_t id; char task_type[32]; char payload[256]; } work_t;
static http_server_t server;
static volatile BaseType_t worker_running = pdFALSE;

static const char *json_value(const char *json, const char *key) {
    static char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *value = strstr(json, pattern);
    return value == NULL ? NULL : value + strlen(pattern);
}

static int json_string(const char *json, const char *key, char *out, size_t size) {
    const char *value = json_value(json, key);
    if (value == NULL || *value++ != '\"') return -1;
    size_t i = 0;
    while (*value && *value != '\"' && i + 1 < size) out[i++] = *value++;
    out[i] = '\0';
    return *value == '\"' ? 0 : -1;
}

static int parse_work(const char *json, work_t *work) {
    if (strstr(json, "[]") != NULL) return 0;
    const char *id = json_value(json, "id");
    if (id == NULL) return -1;
    work->id = (uint32_t) strtoul(id, NULL, 10);
    return json_string(json, "taskType", work->task_type, sizeof(work->task_type)) == 0 &&
           json_string(json, "payload", work->payload, sizeof(work->payload)) == 0 ? 1 : -1;
}

static void one_shot_task(void *parameter) {
    work_t work = *(work_t *) parameter;
    vPortFree(parameter);
    printf("[WorkerTask] 영수증 출력 태스크 시작: id=%u\n", work.id);

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
    vTaskDelay(pdMS_TO_TICKS(1500));

    char path[96], json[256], body[1024];
    http_response_t response = {0};
    snprintf(path, sizeof(path), "/api/tasks/%u/complete", work.id);
    snprintf(json, sizeof(json),
            "{\"result\":\"receipt printed successfully: %s\"}", order_id);
    if (http_request(&server, "PATCH", path, json, body, sizeof(body), &response) == 0 &&
            response.status_code == 200)
        printf("[WorkerTask] 완료 결과 전송: id=%u\n", work.id);
    else fprintf(stderr, "[WorkerTask] 결과 전송 실패: id=%u\n", work.id);
    worker_running = pdFALSE;
    vTaskDelete(NULL);
}

static void command_poll_task(void *parameter) {
    (void) parameter;
    for (;;) {
        if (!worker_running) {
            char body[1024]; http_response_t response = {0}; work_t parsed = {0};
            if (http_request(&server, "GET", "/api/tasks/pending", NULL,
                    body, sizeof(body), &response) == 0 && response.status_code == 200 &&
                    parse_work(body, &parsed) == 1) {
                work_t *work = pvPortMalloc(sizeof(*work));
                if (work != NULL) {
                    *work = parsed; worker_running = pdTRUE;
                    if (xTaskCreate(one_shot_task, "WorkerTask", 2048, work, 3, NULL) != pdPASS) {
                        worker_running = pdFALSE; vPortFree(work);
                    } else printf("[CommandPollTask] 태스크 등록: id=%u\n", parsed.id);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vApplicationMallocFailedHook(void) { abort(); }

int main(int argc, char **argv) {
    const char *url = argc >= 2 ? argv[1] : "http://localhost:8080";
    if (http_server_parse(url, &server) < 0) return EXIT_FAILURE;
    configASSERT(xTaskCreate(command_poll_task, "CommandPollTask", 2048, NULL, 2, NULL) == pdPASS);
    printf("[FreeRTOS] Spring Boot 명령 대기: %s\n", url);
    vTaskStartScheduler();
    return EXIT_FAILURE;
}
