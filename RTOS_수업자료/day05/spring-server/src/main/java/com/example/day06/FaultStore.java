package com.example.day06;

import java.time.Instant;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import org.springframework.stereotype.Service;

@Service
public class FaultStore {
    private final AtomicLong sequence = new AtomicLong();
    private final ConcurrentHashMap<Long, DeviceFault> faults = new ConcurrentHashMap<>();

    public DeviceFault create(String deviceId, String faultType, String message,
            DeviceFault.Severity severity) {
        long id = sequence.incrementAndGet();
        DeviceFault fault = new DeviceFault(id, deviceId, faultType, message, severity,
                DeviceFault.Status.ACTIVE, Instant.now(), null, null);
        faults.put(id, fault);
        return fault;
    }

    public List<DeviceFault> findAll() {
        return faults.values().stream()
                .sorted(Comparator.comparingLong(DeviceFault::id).reversed()).toList();
    }

    public DeviceFault acknowledge(long id) {
        return faults.compute(id, (key, fault) -> {
            if (fault == null) throw new FaultNotFoundException(id);
            return fault.acknowledge(Instant.now());
        });
    }

    public DeviceFault resolve(long id) {
        return faults.compute(id, (key, fault) -> {
            if (fault == null) throw new FaultNotFoundException(id);
            return fault.resolve(Instant.now());
        });
    }
}
