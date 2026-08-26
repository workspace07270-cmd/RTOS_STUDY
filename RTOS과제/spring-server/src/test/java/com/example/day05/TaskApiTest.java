package com.example.day05;

import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.*;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.*;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.AutoConfigureMockMvc;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;

@SpringBootTest
@AutoConfigureMockMvc
class TaskApiTest {
    @Autowired MockMvc mvc;

    @Test
    void requestAndCompleteTask() throws Exception {
        mvc.perform(post("/api/tasks").contentType(MediaType.APPLICATION_JSON)
                .content("{\"taskType\":\"PRINT_RECEIPT\",\"payload\":\"ORDER-1001\"}"))
                .andExpect(status().isCreated()).andExpect(jsonPath("$.status").value("PENDING"));
        mvc.perform(get("/api/tasks/pending")).andExpect(status().isOk())
                .andExpect(jsonPath("$[0].taskType").value("PRINT_RECEIPT"));
        mvc.perform(patch("/api/tasks/1/complete").contentType(MediaType.APPLICATION_JSON)
                .content("{\"result\":\"receipt printed: ORDER-1001\"}"))
                .andExpect(status().isOk()).andExpect(jsonPath("$.status").value("COMPLETED"));
    }
}

