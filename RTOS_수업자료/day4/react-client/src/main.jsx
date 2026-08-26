import React, { useEffect, useState } from "react";
import { createRoot } from "react-dom/client";
import "./style.css";

const API = "http://localhost:8080/api/tasks";

function App() {
  const [orderId, setOrderId] = useState("ORDER-1001");
  const [items, setItems] = useState("아메리카노 x 2, 치즈케이크 x 1");
  const [amount, setAmount] = useState("13500");

  const [task, setTask] = useState(null);
  const [error, setError] = useState("");

  async function sendSignal() {
    setError("");
    try {
      const response = await fetch(API, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          taskType: "PRINT_RECEIPT",
          payload: `${orderId}|${items}|${amount}`,
        }),
      });
      if (!response.ok) throw new Error(`요청 실패: HTTP ${response.status}`);
      setTask(await response.json());
    } catch (e) {
      setError(e.message);
    }
  }

  useEffect(() => {
    if (!task || task.status === "COMPLETED") return;

    const timer = setInterval(async () => {
      try {
        const response = await fetch(`${API}/${task.id}`);
        if (response.ok) setTask(await response.json());
      } catch {
        setError("Spring Boot 연결을 확인하세요.");
      }
    }, 1000);
    return () => clearInterval(timer);
  }, [task]);

  return (
    <main>
      <p className="eyebrow">DAY 04 · RTOS TASK BRIDGE</p>
      <h1>키오스크 영수증 출력</h1>
      <p>영수증 상세 내용을 전달해서 RTOS에서 해당 영수증을 출력합니다.</p>
      <label>
        주문 번호
        <input value={orderId} onChange={(e) => setOrderId(e.target.value)} />
      </label>
      <label>
        주문 상세
        <input value={items} onChange={(e) => setItems(e.target.value)} />
      </label>
      <label>
        결제 금액
        <input
          type="number"
          value={amount}
          onChange={(e) => setAmount(e.target.value)}
        />
      </label>
      <button
        onClick={sendSignal}
        disabled={!orderId.trim() || !items.trim() || !amount}
      >
        영수증 상세 내용 전달 및 RTOS 출력
      </button>
      {task && (
        <section className={task.status.toLowerCase()}>
          <strong>
            {task.status === "PENDING" ? "RTOS 실행 대기 중…" : "실행 완료"}
          </strong>
          <dl>
            <dt>명령 ID</dt>
            <dd>{task.id}</dd>
            <dt>작업</dt>
            <dd>{task.taskType}</dd>
            <dt>영수증</dt>
            <dd>{task.payload}</dd>
            <dt>결과</dt>
            <dd>{task.result ?? "아직 결과 없음"}</dd>
          </dl>
        </section>
      )}
      {error && <p className="error">{error}</p>}
    </main>
  );
}

createRoot(document.getElementById("root")).render(<App />);
