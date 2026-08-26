package com.example.day05;

import java.time.Instant;
import java.util.List;

public record LottoRequest(
        long id,
        List<List<Integer>> lottoSets,
        Status status,
        Instant requestedAt,
        Instant completedAt
) {
    public LottoRequest complete(Instant completedAt) {
        return new LottoRequest(
                id,
                lottoSets,
                Status.COMPLETED,
                requestedAt,
                completedAt
        );
    }

    public enum Status {
        PENDING,
        COMPLETED
    }
}