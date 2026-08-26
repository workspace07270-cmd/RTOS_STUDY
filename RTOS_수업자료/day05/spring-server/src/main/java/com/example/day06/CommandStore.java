package com.example.day06;

import java.time.Instant;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import org.springframework.stereotype.Service;

@Service
public class CommandStore {
    private final AtomicLong sequence = new AtomicLong();
    private final ConcurrentHashMap<Long, DeviceCommand> commands = new ConcurrentHashMap<>();

    public DeviceCommand create(String taskType, String payload) {
        long id = sequence.incrementAndGet();
        DeviceCommand command = new DeviceCommand(id, taskType, payload,
                DeviceCommand.Status.PENDING, null, Instant.now(), null);
        commands.put(id, command);
        return command;
    }

    public List<DeviceCommand> pending() {
        return commands.values().stream()
                .filter(command -> command.status() == DeviceCommand.Status.PENDING)
                .sorted(Comparator.comparingLong(DeviceCommand::id))
                .limit(1)
                .toList();
    }

    public DeviceCommand find(long id) {
        DeviceCommand command = commands.get(id);
        if (command == null) throw new CommandNotFoundException(id);
        return command;
    }

    public List<DeviceCommand> findAll() {
        return commands.values().stream()
                .sorted(Comparator.comparingLong(DeviceCommand::id).reversed())
                .toList();
    }

    public DeviceCommand finish(long id, DeviceCommand.Status status, String result) {
        if (status == DeviceCommand.Status.PENDING) {
            throw new IllegalArgumentException("finish status must be COMPLETED or FAILED");
        }
        return commands.compute(id, (key, command) -> {
            if (command == null) throw new CommandNotFoundException(id);
            return command.finish(status, result, Instant.now());
        });
    }
}

