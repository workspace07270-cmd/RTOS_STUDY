package com.example.day05;

import java.time.Instant;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import org.springframework.stereotype.Service;

@Service
public class TaskStore {
    // DB 대신 메모리에서 고유 ID와 작업 목록을 관리하는 교육용 저장소입니다.
    private final AtomicLong sequence = new AtomicLong();
    private final ConcurrentHashMap<Long, TaskCommand> commands = new ConcurrentHashMap<>();

    public TaskCommand create(String taskType, String payload) {
        // React 요청에는 ID가 없으므로 Spring Boot가 순서대로 ID를 발급합니다.
        long id = sequence.incrementAndGet();
        // RTOS가 아직 실행하지 않았으므로 PENDING이며 결과와 완료 시각은 null입니다.
        TaskCommand command = new TaskCommand(id, taskType, payload,
                TaskCommand.Status.PENDING, null, Instant.now(), null);
        commands.put(id, command);
        return command;
    }

    public List<TaskCommand> pending() {
        // RTOS가 한 번에 하나씩 처리하도록 가장 오래된 PENDING 명령 한 건만 반환합니다.
        return commands.values().stream()
                .filter(c -> c.status() == TaskCommand.Status.PENDING)
                .sorted(Comparator.comparingLong(TaskCommand::id)).limit(1).toList();
    }

    public TaskCommand find(long id) {
        // React가 자신의 명령 상태를 1초마다 조회할 때 사용합니다.
        TaskCommand command = commands.get(id);
        if (command == null) throw new TaskNotFoundException(id);
        return command;
    }

    public TaskCommand complete(long id, String result) {
        // RTOS가 보낸 결과를 저장하고 상태와 완료 시각을 함께 변경합니다.
        return commands.compute(id, (key, command) -> {
            if (command == null) throw new TaskNotFoundException(id);
            return command.complete(result, Instant.now());
        });
    }
}

