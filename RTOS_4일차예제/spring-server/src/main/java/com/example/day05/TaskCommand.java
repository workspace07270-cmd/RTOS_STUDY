package com.example.day05;

import java.time.Instant;

public record TaskCommand(
        // React 요청부터 RTOS 완료 보고까지 같은 작업을 찾는 식별자입니다.
        long id, String taskType, String payload, Status status,
        String result, Instant requestedAt, Instant completedAt
) {
    // record는 불변 객체이므로 기존 값을 수정하지 않고 완료 상태의 새 객체를 만듭니다.
    public TaskCommand complete(String result, Instant completedAt) {
        return new TaskCommand(id, taskType, payload, Status.COMPLETED,
                result, requestedAt, completedAt);
    }

    // PENDING: RTOS 실행 전, COMPLETED: RTOS가 결과를 보고한 뒤
    public enum Status { PENDING, COMPLETED }
}

