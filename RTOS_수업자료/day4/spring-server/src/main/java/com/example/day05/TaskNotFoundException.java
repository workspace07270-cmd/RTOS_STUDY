package com.example.day04;

public class TaskNotFoundException extends RuntimeException {
    public TaskNotFoundException(long id) { super("task command not found: " + id); }
}

