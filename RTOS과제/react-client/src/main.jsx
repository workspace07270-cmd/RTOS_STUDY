import React, { useEffect, useState } from 'react';
import { createRoot } from 'react-dom/client';
import './style.css';

const API = 'http://localhost:8080/api/tasks';

function App() {
  // 사용자가 입력한 영수증 상세정보입니다. 각 input의 value와 연결됩니다.
  const [orderId, setOrderId] = useState('ORDER-1001');
  const [items, setItems] = useState('아메리카노 x 2, 치즈케이크 x 1');
  const [amount, setAmount] = useState('13500');

  // Spring Boot가 반환한 명령의 현재 상태(PENDING/COMPLETED)를 저장합니다.
  const [task, setTask] = useState(null);
  const [error, setError] = useState('');

  // 버튼을 누르면 영수증 상세정보를 Spring Boot의 작업 명령으로 등록합니다.
  async function sendSignal() {
    setError('');
    try {
      const response = await fetch(API, {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          // RTOS는 이 값으로 실행할 작업의 종류를 구분합니다.
          taskType: 'PRINT_RECEIPT',
          // 교육용 C 코드에서 쉽게 분리하도록 세 값을 | 문자로 연결합니다.
          payload: `${orderId}|${items}|${amount}`
        })
      });
      if (!response.ok) throw new Error(`요청 실패: HTTP ${response.status}`);
      // 최초 응답은 일반적으로 status=PENDING, result=null입니다.
      setTask(await response.json());
    } catch (e) { setError(e.message); }
  }

  useEffect(() => {
    // 아직 요청하지 않았거나 완료된 작업이면 상태 조회 타이머가 필요 없습니다.
    if (!task || task.status === 'COMPLETED') return;

    // RTOS의 처리가 끝났는지 Spring Boot에 1초마다 물어봅니다.
    const timer = setInterval(async () => {
      try {
        const response = await fetch(`${API}/${task.id}`);
        if (response.ok) setTask(await response.json());
      } catch { setError('Spring Boot 연결을 확인하세요.'); }
    }, 1000);
    // 컴포넌트가 사라지거나 task가 변경되면 이전 타이머를 정리합니다.
    return () => clearInterval(timer);
  }, [task]);

  return <main>
    <p className="eyebrow">DAY 04 · RTOS TASK BRIDGE</p>
    <h1>키오스크 영수증 출력</h1>
    <p>영수증 상세 내용을 전달해서 RTOS에서 해당 영수증을 출력합니다.</p>
    <label>주문 번호<input value={orderId} onChange={e => setOrderId(e.target.value)} /></label>
    <label>주문 상세<input value={items} onChange={e => setItems(e.target.value)} /></label>
    <label>결제 금액<input type="number" value={amount} onChange={e => setAmount(e.target.value)} /></label>
    <button onClick={sendSignal} disabled={!orderId.trim() || !items.trim() || !amount}>
      영수증 상세 내용 전달 및 RTOS 출력
    </button>
    {/* 서버에서 명령을 하나라도 받은 뒤부터 처리 상태와 결과를 표시합니다. */}
    {task && <section className={task.status.toLowerCase()}>
      <strong>{task.status === 'PENDING' ? 'RTOS 실행 대기 중…' : '실행 완료'}</strong>
      <dl><dt>명령 ID</dt><dd>{task.id}</dd><dt>작업</dt><dd>{task.taskType}</dd>
        <dt>영수증</dt><dd>{task.payload}</dd>
        <dt>결과</dt><dd>{task.result ?? '아직 결과 없음'}</dd></dl>
    </section>}
    {error && <p className="error">{error}</p>}
  </main>;
}

createRoot(document.getElementById('root')).render(<App />);

