#include "http_client.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOTTO_SET_COUNT 5
#define LOTTO_NUMBER_COUNT 6

/* Spring Boot에서 받은 요청 ID와 로또번호 5세트를 보관합니다. */
typedef struct {
    uint32_t id;
    int lotto_sets[LOTTO_SET_COUNT][LOTTO_NUMBER_COUNT];
} lotto_work_t;

/* 모든 RTOS 태스크가 함께 사용하는 Spring Boot 접속 정보입니다. */
static http_server_t server;
/* 출력 태스크가 실행 중인지 표시하여 같은 PENDING 요청의 중복 실행을 막습니다. */
static volatile BaseType_t worker_running = pdFALSE;

/* JSON에서 "key": 다음 값이 시작되는 위치를 찾는 간단한 도우미입니다. */
static const char *json_value(const char *json, const char *key) {
    static char pattern[64];

    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    const char *value = strstr(json, pattern);

    return value == NULL ? NULL : value + strlen(pattern);
}

static int parse_lotto_work(
        const char *json,
        lotto_work_t *work
) {
    /* 빈 배열은 Spring Boot에 처리할 PENDING 요청이 없다는 뜻입니다. */
    if (strstr(json, "[]") != NULL) {
        return 0;
    }

    const char *id_value = json_value(json, "id");

    if (id_value == NULL) {
        return -1;
    }

    /* 문자열에 있는 요청 ID를 정수로 변환합니다. */
    work->id = (uint32_t)strtoul(id_value, NULL, 10);

    const char *numbers = json_value(json, "lottoSets");

    if (numbers == NULL) {
        return -1;
    }

    /* lottoSets 배열에서 숫자 30개(5세트 x 6개)를 순서대로 읽습니다. */
    for (int set = 0; set < LOTTO_SET_COUNT; set++) {
        for (int index = 0; index < LOTTO_NUMBER_COUNT; index++) {
            /* 쉼표와 대괄호를 건너뛰고 다음 숫자의 첫 글자를 찾습니다. */
            while (*numbers != '\0' &&
                   (*numbers < '0' || *numbers > '9')) {
                numbers++;
            }

            if (*numbers == '\0') {
                return -1;
            }

            work->lotto_sets[set][index] =
                    (int)strtol(numbers, (char **)&numbers, 10);
        }
    }

    return 1;
}

static void lotto_print_task(void *parameter) {
    /* 생성 태스크가 넘겨준 데이터를 지역 변수에 복사한 뒤 동적 메모리를 해제합니다. */
    lotto_work_t work = *(lotto_work_t *)parameter;

    vPortFree(parameter);

    printf("\n");
    printf("========================================\n");
    printf("       RTOS LOTTO NUMBER OUTPUT\n");
    printf("========================================\n");
    printf("Request ID: %u\n\n", work.id);

    /* 로또번호 5세트를 한 세트씩 콘솔에 출력합니다. */
    for (int set = 0; set < LOTTO_SET_COUNT; set++) {
        printf("%d set: ", set + 1);

        for (int index = 0; index < LOTTO_NUMBER_COUNT; index++) {
            printf(
                    "%2d ",
                    work.lotto_sets[set][index]
            );
        }

        printf("\n");
        /* 실제 장치가 출력하는 모습을 표현하고 다른 태스크에도 실행 시간을 줍니다. */
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    printf("========================================\n");
    printf("All lotto numbers printed.\n\n");
    fflush(stdout);

    char path[128];
    char body[1024];
    http_response_t response = {0};

    /* 현재 요청 ID를 이용해 RTOS 완료 신호 API 경로를 만듭니다. */
    snprintf(
            path,
            sizeof(path),
            "/api/lotto-requests/%u/complete",
            work.id
    );

    /* 모든 번호 출력이 끝났으므로 Spring Boot에 PATCH 완료 요청을 보냅니다. */
    if (http_request(
            &server,
            "PATCH",
            path,
            NULL,
            body,
            sizeof(body),
            &response
        ) == 0 &&
        response.status_code == 200) {

        printf(
                "[LottoPrintTask] Complete signal sent: id=%u\n",
                work.id
        );
    } else {
        fprintf(
                stderr,
                "[LottoPrintTask] Complete signal failed: id=%u\n",
                work.id
        );
    }

    /* 다음 요청을 받을 수 있게 한 뒤 현재 일회성 태스크를 삭제합니다. */
    worker_running = pdFALSE;
    vTaskDelete(NULL);
}

static void command_poll_task(void *parameter) {
    (void)parameter;

    /* 프로그램이 실행되는 동안 Spring Boot의 대기 요청을 계속 감시합니다. */
    for (;;) {
        /* 출력 태스크 실행 중에는 같은 요청을 다시 조회해 실행하지 않습니다. */
        if (!worker_running) {
            char body[2048];
            http_response_t response = {0};
            lotto_work_t parsed = {0};

            /* RTOS가 처리할 가장 오래된 PENDING 요청 한 건을 조회합니다. */
            int result = http_request(
                    &server,
                    "GET",
                    "/api/lotto-requests/pending",
                    NULL,
                    body,
                    sizeof(body),
                    &response
            );

            if (result == 0 &&
                response.status_code == 200 &&
                parse_lotto_work(body, &parsed) == 1) {

                /* 새 태스크에 데이터를 안전하게 넘기기 위해 FreeRTOS 힙을 사용합니다. */
                lotto_work_t *work =
                        pvPortMalloc(sizeof(*work));

                if (work != NULL) {
                    *work = parsed;
                    worker_running = pdTRUE;

                    /* 요청 한 건마다 우선순위 3의 일회성 출력 태스크를 생성합니다. */
                    if (xTaskCreate(
                            lotto_print_task,
                            "LottoPrintTask",
                            2048,
                            work,
                            3,
                            NULL
                        ) != pdPASS) {

                        worker_running = pdFALSE;
                        vPortFree(work);
                    } else {
                        printf(
                                "[CommandPollTask] Lotto task created: id=%u\n",
                                parsed.id
                        );
                    }
                }
            }
        }

        /* CPU를 계속 점유하지 않고 다음 조회까지 1초 동안 Blocked 상태로 쉽니다. */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vApplicationMallocFailedHook(void) {
    /* FreeRTOS 동적 메모리 할당 실패 시 즉시 프로그램을 종료합니다. */
    abort();
}

int main(int argc, char **argv) {
    /* 실행 인자가 없으면 같은 PC의 Spring Boot 8080 포트를 사용합니다. */
    const char *url =
            argc >= 2
            ? argv[1]
            : "http://localhost:8080";

    if (http_server_parse(url, &server) < 0) {
        return EXIT_FAILURE;
    }

    /* 서버를 계속 감시할 상시 폴링 태스크를 먼저 생성합니다. */
    configASSERT(
            xTaskCreate(
                    command_poll_task,
                    "CommandPollTask",
                    2048,
                    NULL,
                    2,
                    NULL
            ) == pdPASS
    );

    printf(
            "[FreeRTOS] Waiting for lotto requests: %s\n",
            url
    );

    /* FreeRTOS 스케줄러를 시작하면 생성된 태스크들이 우선순위에 따라 실행됩니다. */
    vTaskStartScheduler();

    return EXIT_FAILURE;
}
