package com.example.day05;

public class TaskNotFoundException extends RuntimeException {
    public TaskNotFoundException(long id) { super("task command not found: " + id); }
}

