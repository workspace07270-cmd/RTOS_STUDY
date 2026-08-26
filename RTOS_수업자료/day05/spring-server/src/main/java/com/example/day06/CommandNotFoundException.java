package com.example.day06;

public class CommandNotFoundException extends RuntimeException {
    public CommandNotFoundException(long id) {
        super("device command not found: " + id);
    }
}

