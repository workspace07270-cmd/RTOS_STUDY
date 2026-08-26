package com.example.day06;

public class FaultNotFoundException extends RuntimeException {
    public FaultNotFoundException(long id) { super("device fault not found: " + id); }
}
