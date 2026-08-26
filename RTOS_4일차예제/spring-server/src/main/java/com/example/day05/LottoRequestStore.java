package com.example.day05;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.IntStream;

import org.springframework.stereotype.Service;

@Service
public class LottoRequestStore {

    private final AtomicLong sequence = new AtomicLong();
    private final ConcurrentHashMap<Long, LottoRequest> requests =
            new ConcurrentHashMap<>();

    public LottoRequest create() {
        long id = sequence.incrementAndGet();

        LottoRequest request = new LottoRequest(
                id,
                generateLottoSets(),
                LottoRequest.Status.PENDING,
                Instant.now(),
                null
        );

        requests.put(id, request);
        return request;
    }

    public List<LottoRequest> pending() {
        return requests.values().stream()
                .filter(request ->
                        request.status() == LottoRequest.Status.PENDING)
                .sorted(Comparator.comparingLong(LottoRequest::id))
                .limit(1)
                .toList();
    }

    public LottoRequest find(long id) {
        LottoRequest request = requests.get(id);

        if (request == null) {
            throw new TaskNotFoundException(id);
        }

        return request;
    }

    public LottoRequest complete(long id) {
        return requests.compute(id, (key, request) -> {
            if (request == null) {
                throw new TaskNotFoundException(id);
            }

            return request.complete(Instant.now());
        });
    }

    private List<List<Integer>> generateLottoSets() {
        List<List<Integer>> lottoSets = new ArrayList<>();

        for (int i = 0; i < 5; i++) {
            lottoSets.add(generateLottoSet());
        }

        return lottoSets;
    }

    private List<Integer> generateLottoSet() {
        List<Integer> numbers = new ArrayList<>(
                IntStream.rangeClosed(1, 45)
                        .boxed()
                        .toList()
        );

        Collections.shuffle(numbers);

        return numbers.stream()
                .limit(6)
                .sorted()
                .toList();
    }
}