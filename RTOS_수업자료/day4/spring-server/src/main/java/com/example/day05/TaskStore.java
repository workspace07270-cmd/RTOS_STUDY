package com.example.day04;

import java.time.Instant;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import org.springframework.stereotype.Service;

@Service
public class TaskStore {
    private final AtomicLong sequence = new AtomicLong();
    private final ConcurrentHashMap<Long, TaskCommand> commands = new ConcurrentHashMap<>();

    public TaskCommand create(String taskType, String payload) {
        long id = sequence.incrementAndGet();

        TaskCommand command = new TaskCommand(id, taskType, payload,
                TaskCommand.Status.PENDING, null, Instant.now(), null);
        commands.put(id, command);
        return command;
    }

    public List<TaskCommand> pending() {
        return commands.values().stream()
                .filter(c -> c.status() == TaskCommand.Status.PENDING)
                .sorted(Comparator.comparingLong(TaskCommand::id)).limit(1).toList();
    }

    public TaskCommand find(long id) {
        TaskCommand command = commands.get(id);
        if (command == null) throw new TaskNotFoundException(id);
        return command;
    }

    public TaskCommand complete(long id, String result) {
        return commands.compute(id, (key, command) -> {
            if (command == null) throw new TaskNotFoundException(id);
            return command.complete(result, Instant.now());
        });
    }
}

