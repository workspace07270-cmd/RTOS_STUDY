package com.example.day04;

import jakarta.validation.Valid;
import jakarta.validation.constraints.NotBlank;
import java.util.List;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.*;

@CrossOrigin(origins = "http://localhost:5173")
@RestController
@RequestMapping("/api/tasks")
public class TaskController {
    private static final Logger log = LoggerFactory.getLogger(TaskController.class);
    private final TaskStore store;

    public TaskController(TaskStore store) { this.store = store; }

    @PostMapping
    @ResponseStatus(HttpStatus.CREATED)
    public TaskCommand create(@Valid @RequestBody CreateRequest request) {
        TaskCommand command = store.create(request.taskType(), request.payload());
        log.info("[React -> Spring] 작업 요청 id={}, type={}", command.id(), command.taskType());
        return command;
    }

    @GetMapping("/pending")
    public List<TaskCommand> pending() { return store.pending(); }

    @GetMapping("/{id}")
    public TaskCommand find(@PathVariable long id) { return store.find(id); }

    @PatchMapping("/{id}/complete")
    public TaskCommand complete(@PathVariable long id, @Valid @RequestBody CompleteRequest request) {
        TaskCommand command = store.complete(id, request.result());
        log.info("[RTOS -> Spring] 작업 완료 id={}, result={}", id, request.result());
        return command;
    }

    @ExceptionHandler(TaskNotFoundException.class)
    @ResponseStatus(HttpStatus.NOT_FOUND)
    public Map<String, String> notFound(TaskNotFoundException exception) {
        return Map.of("message", exception.getMessage());
    }

    public record CreateRequest(@NotBlank String taskType, @NotBlank String payload) {}
    public record CompleteRequest(@NotBlank String result) {}
}

