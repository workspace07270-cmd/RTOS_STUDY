package com.example.day06;

import java.time.Instant;

/** RTOS가 감지하고 운영자가 확인한 뒤 RTOS가 정상 여부를 확정하는 장애 모델입니다. */
public record DeviceFault(
        long id,
        String deviceId,
        String faultType,
        String message,
        Severity severity,
        Status status,
        Instant detectedAt,
        Instant acknowledgedAt,
        Instant resolvedAt
) {
    public DeviceFault acknowledge(Instant now) {
        if (status == Status.RESOLVED) return this;
        return new DeviceFault(id, deviceId, faultType, message, severity,
                Status.ACKNOWLEDGED, detectedAt, now, null);
    }

    public DeviceFault resolve(Instant now) {
        return new DeviceFault(id, deviceId, faultType, message, severity,
                Status.RESOLVED, detectedAt, acknowledgedAt, now);
    }

    public enum Severity { WARNING, CRITICAL }
    public enum Status { ACTIVE, ACKNOWLEDGED, RESOLVED }
}
