package com.example.day05;

import java.util.List;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.*;

@CrossOrigin(originPatterns = {
        "http://localhost:*",
        "http://127.0.0.1:*"
})
@RestController
@RequestMapping("/api/lotto-requests")
public class LottoRequestController {

    private static final Logger log = LoggerFactory.getLogger(LottoRequestController.class);

    private final LottoRequestStore store;

    public LottoRequestController(LottoRequestStore store) {
        this.store = store;
    }

    // React가 로또번호 5세트 생성을 요청
    @PostMapping
    @ResponseStatus(HttpStatus.CREATED)
    public LottoRequest create() {
        LottoRequest request = store.create();

        log.info(
                "[React -> Spring] 로또 발급 요청 id={}, numbers={}",
                request.id(),
                request.lottoSets());

        return request;
    }

    // RTOS가 처리할 가장 오래된 PENDING 요청 한 건 조회
    @GetMapping("/pending")
    public List<LottoRequest> pending() {
        List<LottoRequest> requests = store.pending();

        if (!requests.isEmpty()) {
            log.info(
                    "[RTOS -> Spring] 대기 요청 조회 id={}",
                    requests.get(0).id());
        }

        return requests;
    }

    // React가 요청 상태와 결과 조회
    @GetMapping("/{id}")
    public LottoRequest find(@PathVariable long id) {
        return store.find(id);
    }

    // RTOS가 출력 완료 신호 전송
    @PatchMapping("/{id}/complete")
    public LottoRequest complete(@PathVariable long id) {
        LottoRequest request = store.complete(id);

        log.info(
                "[RTOS -> Spring] 출력 완료 id={}, status={}",
                request.id(),
                request.status());

        return request;
    }

    @ExceptionHandler(TaskNotFoundException.class)
    @ResponseStatus(HttpStatus.NOT_FOUND)
    public Map<String, String> notFound(
            TaskNotFoundException exception) {
        return Map.of("message", exception.getMessage());
    }
}