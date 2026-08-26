#include "http_client.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESPONSE_CAPACITY 2048
#define RESULT_CAPACITY 256

/* Spring Boot 명령을 RTOS에서 사용하기 위한 공통 작업 구조체입니다. */
typedef struct {
    uint32_t id;
    char task_type[32];
    char payload[256];
} work_t;

/* 모든 Handler는 같은 입력과 출력 형식을 사용합니다. 0은 성공, -1은 실패입니다. */
typedef int (*task_handler_t)(const work_t *work, char *result, size_t result_size);

/* Spring HandlerMapping처럼 문자열 taskType과 실제 처리 함수를 연결합니다. */
typedef struct {
    const char *task_type;
    task_handler_t handler;
} task_handler_mapping_t;

static http_server_t spring_server;
static volatile BaseType_t worker_running = pdFALSE;

/* 장애 예제는 실제 GPIO 대신 시작 5초 뒤 프린터 용지 부족을 한 번 발생시킵니다. */
static volatile BaseType_t paper_empty = pdTRUE;

static const char *json_value(const char *json, const char *key) {
    static char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *value = strstr(json, pattern);
    return value == NULL ? NULL : value + strlen(pattern);
}

static int json_string(const char *json, const char *key, char *out, size_t size) {
    const char *value = json_value(json, key);
    if (value == NULL || *value++ != '\"') return -1;
    size_t index = 0;
    while (*value && *value != '\"' && index + 1 < size) out[index++] = *value++;
    out[index] = '\0';
    return *value == '\"' ? 0 : -1;
}

static int parse_work(const char *json, work_t *work) {
    if (strstr(json, "[]") != NULL) return 0;
    const char *id_value = json_value(json, "id");
    if (id_value == NULL) return -1;
    work->id = (uint32_t) strtoul(id_value, NULL, 10);
    return json_string(json, "taskType", work->task_type, sizeof(work->task_type)) == 0 &&
           json_string(json, "payload", work->payload, sizeof(work->payload)) == 0 ? 1 : -1;
}

static int handle_print_receipt(const work_t *work, char *result, size_t result_size) {
    char payload[sizeof(work->payload)];
    snprintf(payload, sizeof(payload), "%s", work->payload);
    char *order_id = strtok(payload, "|");
    char *items = strtok(NULL, "|");
    char *amount = strtok(NULL, "|");
    if (order_id == NULL || items == NULL || amount == NULL) {
        snprintf(result, result_size, "invalid receipt payload");
        return -1;
    }

    printf("\n+--------------------------------------+\n");
    printf("|          KIOSK RECEIPT               |\n");
    printf("+--------------------------------------+\n");
    printf("  주문 번호 : %s\n", order_id);
    printf("  주문 상세 : %s\n", items);
    printf("  결제 금액 : %s원\n", amount);
    printf("+--------------------------------------+\n\n");
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(1200));
    snprintf(result, result_size, "receipt printed: %s", order_id);
    return 0;
}

static int handle_led_blink(const work_t *work, char *result, size_t result_size) {
    int count = atoi(work->payload);
    if (count < 1 || count > 10) {
        snprintf(result, result_size, "LED count must be 1..10");
        return -1;
    }
    for (int index = 1; index <= count; index++) {
        printf("[LED Handler] ON  (%d/%d)\n", index, count);
        vTaskDelay(pdMS_TO_TICKS(250));
        printf("[LED Handler] OFF (%d/%d)\n", index, count);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    snprintf(result, result_size, "LED blink completed: %d times", count);
    return 0;
}

static int handle_buzzer_on(const work_t *work, char *result, size_t result_size) {
    int duration_ms = atoi(work->payload);
    if (duration_ms < 100 || duration_ms > 5000) {
        snprintf(result, result_size, "buzzer duration must be 100..5000 ms");
        return -1;
    }
    printf("[Buzzer Handler] ON (%d ms)\n", duration_ms);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    printf("[Buzzer Handler] OFF\n");
    snprintf(result, result_size, "buzzer completed: %d ms", duration_ms);
    return 0;
}


static int handle_recover_fault(const work_t *work, char *result, size_t result_size) {
    unsigned long fault_id = strtoul(work->payload, NULL, 10);
    if (fault_id == 0) {
        snprintf(result, result_size, "fault id must be a positive number");
        return -1;
    }

    printf("[Recovery Handler] 프린터 용지 보충 및 센서 재확인 중...\n");
    vTaskDelay(pdMS_TO_TICKS(1200));
    paper_empty = pdFALSE;

    if (paper_empty) {
        snprintf(result, result_size, "fault remains active: %lu", fault_id);
        return -1;
    }

    char path[96], body[RESPONSE_CAPACITY];
    http_response_t response = {0};
    snprintf(path, sizeof(path), "/api/faults/%lu/resolve", fault_id);
    if (http_request(&spring_server, "PATCH", path, "{}",
            body, sizeof(body), &response) < 0 || response.status_code != 200) {
        snprintf(result, result_size, "failed to report resolved fault: %lu", fault_id);
        return -1;
    }

    snprintf(result, result_size, "fault resolved after sensor check: %lu", fault_id);
    return 0;
}

/* 새 작업을 추가할 때 Handler를 구현하고 이 테이블에 한 줄 등록합니다. */
static const task_handler_mapping_t task_handlers[] = {
    {"PRINT_RECEIPT", handle_print_receipt},
    {"LED_BLINK", handle_led_blink},
    {"BUZZER_ON", handle_buzzer_on},
    {"RECOVER_FAULT", handle_recover_fault},
};

static task_handler_t find_task_handler(const char *task_type) {
    size_t count = sizeof(task_handlers) / sizeof(task_handlers[0]);
    for (size_t index = 0; index < count; index++) {
        if (strcmp(task_handlers[index].task_type, task_type) == 0)
            return task_handlers[index].handler;
    }
    return NULL;
}

static void report_result(const work_t *work, const char *status, const char *result) {
    char path[96], json[512], response_body[RESPONSE_CAPACITY];
    http_response_t response = {0};
    snprintf(path, sizeof(path), "/api/commands/%u/finish", work->id);
    snprintf(json, sizeof(json), "{\"status\":\"%s\",\"result\":\"%s\"}",
            status, result);
    if (http_request(&spring_server, "PATCH", path, json,
            response_body, sizeof(response_body), &response) == 0 &&
            response.status_code == 200) {
        printf("[WorkerTask -> Spring] id=%u, status=%s, result=%s\n",
                work->id, status, result);
    } else {
        fprintf(stderr, "[WorkerTask] 결과 보고 실패: id=%u\n", work->id);
    }
}

static void worker_task(void *parameter) {
    work_t work = *(work_t *) parameter;
    vPortFree(parameter);
    char result[RESULT_CAPACITY] = {0};

    printf("[WorkerTask] Handler 검색: id=%u, taskType=%s\n",
            work.id, work.task_type);
    task_handler_t handler = find_task_handler(work.task_type);

    if (handler == NULL) {
        snprintf(result, sizeof(result), "unsupported taskType: %s", work.task_type);
        report_result(&work, "FAILED", result);
    } else if (handler(&work, result, sizeof(result)) == 0) {
        report_result(&work, "COMPLETED", result);
    } else {
        report_result(&work, "FAILED", result);
    }

    worker_running = pdFALSE;
    vTaskDelete(NULL);
}

static void command_poll_task(void *parameter) {
    (void) parameter;
    for (;;) {
        if (!worker_running) {
            char body[RESPONSE_CAPACITY];
            http_response_t response = {0};
            work_t parsed = {0};
            int request_ok = http_request(&spring_server, "GET", "/api/commands/pending",
                    NULL, body, sizeof(body), &response);
            if (request_ok == 0 && response.status_code == 200 &&
                    parse_work(body, &parsed) == 1) {
                work_t *work = pvPortMalloc(sizeof(*work));
                if (work != NULL) {
                    *work = parsed;
                    worker_running = pdTRUE;
                    if (xTaskCreate(worker_task, "WorkerTask", 2048,
                            work, 3, NULL) != pdPASS) {
                        worker_running = pdFALSE;
                        vPortFree(work);
                    } else {
                        printf("[CommandPollTask] WorkerTask 등록: id=%u\n", parsed.id);
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void fault_monitor_task(void *parameter) {
    (void) parameter;
    vTaskDelay(pdMS_TO_TICKS(5000));

    if (paper_empty) {
        const char *json = "{\"deviceId\":\"PRINTER-01\","
                "\"faultType\":\"PAPER_EMPTY\","
                "\"message\":\"Printer paper is empty\","
                "\"severity\":\"CRITICAL\"}";
        char body[RESPONSE_CAPACITY];
        http_response_t response = {0};
        if (http_request(&spring_server, "POST", "/api/faults", json,
                body, sizeof(body), &response) == 0 && response.status_code == 201) {
            printf("[FaultMonitorTask -> Spring] PAPER_EMPTY 장애 전송 완료\n");
        } else {

            fprintf(stderr, "[FaultMonitorTask] 장애 전송 실패: HTTP %d, body=%s\n",
                    response.status_code, body);
        }
    }

    vTaskDelete(NULL);
}

void vApplicationMallocFailedHook(void) { abort(); }

int main(int argc, char **argv) {
    const char *url = argc >= 2 ? argv[1] : "http://localhost:8080";
    if (http_server_parse(url, &spring_server) < 0) return EXIT_FAILURE;
    configASSERT(xTaskCreate(command_poll_task, "CommandPollTask",
            2048, NULL, 2, NULL) == pdPASS);
    configASSERT(xTaskCreate(fault_monitor_task, "FaultMonitorTask",
            2048, NULL, 2, NULL) == pdPASS);
    printf("[FreeRTOS] 양방향 명령/장애 예제 시작: %s\n", url);
    vTaskStartScheduler();
    return EXIT_FAILURE;
}
