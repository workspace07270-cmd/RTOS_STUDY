package com.example.day06;

import java.time.Instant;

/** React 요청부터 RTOS 결과까지 전달되는 공통 명령 모델입니다. */
public record DeviceCommand(
        long id,
        String taskType,
        String payload,
        Status status,
        String result,
        Instant requestedAt,
        Instant completedAt
) {
    public DeviceCommand finish(Status nextStatus, String result, Instant completedAt) {
        return new DeviceCommand(id, taskType, payload, nextStatus,
                result, requestedAt, completedAt);
    }

    public enum Status { PENDING, COMPLETED, FAILED }
}

