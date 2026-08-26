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
@RequestMapping("/api/faults")
public class FaultController {
    private static final Logger log = LoggerFactory.getLogger(FaultController.class);
    private final FaultStore store;

    public FaultController(FaultStore store) { this.store = store; }

    /** RTOS의 FaultMonitorTask가 센서에서 감지한 장애를 서버에 등록합니다. */
    @PostMapping
    @ResponseStatus(HttpStatus.CREATED)
    public DeviceFault create(@Valid @RequestBody CreateRequest request) {
        DeviceFault fault = store.create(request.deviceId(), request.faultType(),
                request.message(), request.severity());
        log.warn("[RTOS -> Spring] fault id={}, type={}, device={}",
                fault.id(), fault.faultType(), fault.deviceId());
        return fault;
    }

    /** React 운영 화면이 장애 목록과 현재 상태를 주기적으로 확인합니다. */
    @GetMapping
    public List<DeviceFault> findAll() { return store.findAll(); }

    /** 확인은 장애를 숨기지 않으며 ACKNOWLEDGED 상태로만 변경합니다. */
    @PatchMapping("/{id}/acknowledge")
    public DeviceFault acknowledge(@PathVariable long id) { return store.acknowledge(id); }

    /** 최종 해결은 센서를 재확인한 RTOS만 호출하는 것을 전제로 한 API입니다. */
    @PatchMapping("/{id}/resolve")
    public DeviceFault resolve(@PathVariable long id) {
        DeviceFault fault = store.resolve(id);
        log.info("[RTOS -> Spring] fault id={} resolved", id);
        return fault;
    }

    @ExceptionHandler(FaultNotFoundException.class)
    @ResponseStatus(HttpStatus.NOT_FOUND)
    public Map<String, String> notFound(FaultNotFoundException exception) {
        return Map.of("message", exception.getMessage());
    }

    public record CreateRequest(@NotBlank String deviceId, @NotBlank String faultType,
            @NotBlank String message, @NotNull DeviceFault.Severity severity) {}
}
