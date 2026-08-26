package com.example.day06;

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
class CommandApiTest {
    @Autowired MockMvc mvc;

    @Test
    void commandCanFinishWithRtosResult() throws Exception {
        mvc.perform(post("/api/commands").contentType(MediaType.APPLICATION_JSON)
                .content("{\"taskType\":\"LED_BLINK\",\"payload\":\"3\"}"))
                .andExpect(status().isCreated())
                .andExpect(jsonPath("$.status").value("PENDING"));

        mvc.perform(get("/api/commands/pending"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$[0].taskType").value("LED_BLINK"));

        mvc.perform(patch("/api/commands/1/finish").contentType(MediaType.APPLICATION_JSON)
                .content("{\"status\":\"COMPLETED\",\"result\":\"LED blink completed: 3 times\"}"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.status").value("COMPLETED"));
    }

    @Test
    void faultIsAcknowledgedByOperatorAndResolvedByRtos() throws Exception {
        mvc.perform(post("/api/faults").contentType(MediaType.APPLICATION_JSON)
                .content("{\"deviceId\":\"PRINTER-01\",\"faultType\":\"PAPER_EMPTY\","
                        + "\"message\":\"paper empty\",\"severity\":\"CRITICAL\"}"))
                .andExpect(status().isCreated())
                .andExpect(jsonPath("$.status").value("ACTIVE"));

        mvc.perform(patch("/api/faults/1/acknowledge"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.status").value("ACKNOWLEDGED"));

        mvc.perform(patch("/api/faults/1/resolve"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.status").value("RESOLVED"));
    }
}

