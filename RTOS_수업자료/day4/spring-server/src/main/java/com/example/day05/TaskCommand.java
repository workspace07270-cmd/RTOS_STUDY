package com.example.day04;

import java.time.Instant;

public record TaskCommand(
        long id, String taskType, String payload, Status status,
        String result, Instant requestedAt, Instant completedAt
) {
    public TaskCommand complete(String result, Instant completedAt) {
        return new TaskCommand(id, taskType, payload, Status.COMPLETED,
                result, requestedAt, completedAt);
    }

    public enum Status { PENDING, COMPLETED }
}

