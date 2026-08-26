package com.example.day05;

import jakarta.validation.Valid;
import jakarta.validation.constraints.NotBlank;
import java.util.List;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.*;

// 개발 서버(5173)의 React가 다른 포트(8080)의 API를 호출할 수 있게 허용합니다.
@CrossOrigin(origins = "http://localhost:5173")
@RestController
@RequestMapping("/api/tasks")
public class TaskController {
    private static final Logger log = LoggerFactory.getLogger(TaskController.class);
    private final TaskStore store;

    public TaskController(TaskStore store) { this.store = store; }

    // React -> Spring Boot: 영수증 출력 작업을 PENDING 상태로 등록합니다.
    @PostMapping
    @ResponseStatus(HttpStatus.CREATED)
    public TaskCommand create(@Valid @RequestBody CreateRequest request) {
        TaskCommand command = store.create(request.taskType(), request.payload());
        log.info(request.payload());
        log.info("[React -> Spring] 작업 요청 id={}, type={}", command.id(), command.taskType());
        return command;
    }

    // RTOS -> Spring Boot: 아직 실행되지 않은 작업 한 건을 가져갑니다.
    @GetMapping("/pending")
    public List<TaskCommand> pending() { return store.pending(); }

    // React -> Spring Boot: 등록한 작업의 현재 상태와 결과를 확인합니다.
    @GetMapping("/{id}")
    public TaskCommand find(@PathVariable long id) { return store.find(id); }

    // RTOS -> Spring Boot: 영수증 출력 결과를 보고하고 COMPLETED로 변경합니다.
    @PatchMapping("/{id}/complete")
    public TaskCommand complete(@PathVariable long id, @Valid @RequestBody CompleteRequest request) {
        TaskCommand command = store.complete(id, request.result());
        log.info(command.payload());
        log.info("[RTOS -> Spring] 작업 완료 id={}, result={}", id, request.result());
        return command;
    }

    @ExceptionHandler(TaskNotFoundException.class)
    @ResponseStatus(HttpStatus.NOT_FOUND)
    public Map<String, String> notFound(TaskNotFoundException exception) {
        return Map.of("message", exception.getMessage());
    }

    // 외부 요청 DTO를 별도로 두어 비어 있는 입력을 API 입구에서 차단합니다.
    public record CreateRequest(@NotBlank String taskType, @NotBlank String payload) {}
    public record CompleteRequest(@NotBlank String result) {}
}

