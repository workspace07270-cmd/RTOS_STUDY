package com.example.day06;

import jakarta.validation.Valid;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import java.util.List;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.*;

@CrossOrigin(origins = "http://localhost:5173")
@RestController
@RequestMapping("/api/commands")
public class CommandController {
    private static final Logger log = LoggerFactory.getLogger(CommandController.class);
    private final CommandStore store;

    public CommandController(CommandStore store) { this.store = store; }

    /** React가 taskType과 payload가 다른 여러 장치 명령을 같은 API로 등록합니다. */
    @PostMapping
    @ResponseStatus(HttpStatus.CREATED)
    public DeviceCommand create(@Valid @RequestBody CreateRequest request) {
        DeviceCommand command = store.create(request.taskType(), request.payload());
        log.info("[React -> Spring] id={}, taskType={}, payload={}",
                command.id(), command.taskType(), command.payload());
        return command;
    }

    /** RTOS의 CommandPollTask가 처리할 가장 오래된 명령 한 건을 조회합니다. */
    @GetMapping("/pending")
    public List<DeviceCommand> pending() { return store.pending(); }

    /** React가 자신의 명령 상태를 확인합니다. */
    @GetMapping("/{id}")
    public DeviceCommand find(@PathVariable long id) { return store.find(id); }

    /** React 화면에서 최근 명령들을 함께 보여주기 위한 조회 API입니다. */
    @GetMapping
    public List<DeviceCommand> findAll() { return store.findAll(); }

    /** RTOS Handler의 성공 또는 실패 결과를 저장합니다. */
    @PatchMapping("/{id}/finish")
    public DeviceCommand finish(@PathVariable long id,
            @Valid @RequestBody FinishRequest request) {
        DeviceCommand command = store.finish(id, request.status(), request.result());
        log.info("[RTOS -> Spring] id={}, status={}, result={}",
                id, request.status(), request.result());
        return command;
    }

    @ExceptionHandler(CommandNotFoundException.class)
    @ResponseStatus(HttpStatus.NOT_FOUND)
    public Map<String, String> notFound(CommandNotFoundException exception) {
        return Map.of("message", exception.getMessage());
    }

    public record CreateRequest(@NotBlank String taskType, @NotBlank String payload) {}
    public record FinishRequest(@NotNull DeviceCommand.Status status,
                                @NotBlank String result) {}
}

