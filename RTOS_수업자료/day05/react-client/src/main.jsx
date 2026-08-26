import React, { useEffect, useState } from 'react';
import { createRoot } from 'react-dom/client';
import './style.css';

const API = 'http://localhost:8080/api/commands';
const FAULT_API = 'http://localhost:8080/api/faults';

const commandOptions = [
  { type: 'PRINT_RECEIPT', title: '영수증 출력', hint: 'ORDER-1001|아메리카노 x 2|9000' },
  { type: 'LED_BLINK', title: '상태 LED 점멸', hint: '3' },
  { type: 'BUZZER_ON', title: '픽업 부저', hint: '1200' }
];

function App() {
  const [taskType, setTaskType] = useState('PRINT_RECEIPT');
  const [payload, setPayload] = useState(commandOptions[0].hint);
  const [current, setCurrent] = useState(null);
  const [history, setHistory] = useState([]);
  const [faults, setFaults] = useState([]);
  const [error, setError] = useState('');

  function selectCommand(option) {
    setTaskType(option.type);
    setPayload(option.hint);
  }

  async function loadHistory() {
    const response = await fetch(API);
    if (response.ok) setHistory(await response.json());
  }

  // 장애 목록은 RTOS가 서버로 올린 역방향 이벤트를 운영 화면에 반영합니다.
  async function loadFaults() {
    const response = await fetch(FAULT_API);
    if (response.ok) setFaults(await response.json());
  }

  // 확인은 사람이 알림을 보았다는 뜻이며 실제 장애 해결 상태로 바꾸지 않습니다.
  async function acknowledgeFault(id) {
    const response = await fetch(`${FAULT_API}/${id}/acknowledge`, { method: 'PATCH' });
    if (!response.ok) throw new Error(`장애 확인 실패: HTTP ${response.status}`);
    await loadFaults();
  }

  // 복구 버튼은 기존 명령 API를 통해 RTOS Handler에 장애 id를 전달합니다.
  async function requestRecovery(fault) {
    const response = await fetch(API, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ taskType: 'RECOVER_FAULT', payload: String(fault.id) })
    });
    if (!response.ok) throw new Error(`복구 명령 등록 실패: HTTP ${response.status}`);
    setCurrent(await response.json());
    await loadHistory();
  }

  async function sendCommand() {
    setError('');
    try {
      const response = await fetch(API, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ taskType, payload })
      });
      if (!response.ok) throw new Error(`명령 등록 실패: HTTP ${response.status}`);
      const command = await response.json();
      setCurrent(command);
      await loadHistory();
    } catch (e) { setError(e.message); }
  }

  useEffect(() => {
    loadHistory().catch(() => {});
    loadFaults().catch(() => {});
    // 간단한 폴링으로 RTOS 장애와 복구 결과를 화면에 지속적으로 동기화합니다.
    const timer = setInterval(() => loadFaults().catch(() => {}), 1000);
    return () => clearInterval(timer);
  }, []);

  useEffect(() => {
    if (!current || current.status !== 'PENDING') return;
    const timer = setInterval(async () => {
      try {
        const response = await fetch(`${API}/${current.id}`);
        if (!response.ok) return;
        const command = await response.json();
        setCurrent(command);
        if (command.status !== 'PENDING') {
          await loadHistory();
          await loadFaults();
        }
      } catch { setError('Spring Boot 연결을 확인하세요.'); }
    }, 1000);
    return () => clearInterval(timer);
  }, [current]);

  return <main>
    <header>
      <p className="eyebrow">DAY 05 · RTOS HANDLER MAPPING</p>
      <h1>키오스크 장치 명령 센터</h1>
      <p>하나의 API로 명령을 보내고 RTOS가 taskType에 맞는 Handler를 선택합니다.</p>
    </header>

    <div className="cards">
      {commandOptions.map(option => <button key={option.type}
        className={taskType === option.type ? 'card selected' : 'card'}
        onClick={() => selectCommand(option)}>
        <strong>{option.title}</strong><small>{option.type}</small>
      </button>)}
    </div>

    <section className="composer">
      <label>RTOS에 전달할 payload
        <input value={payload} onChange={event => setPayload(event.target.value)} />
      </label>
      <button className="send" onClick={sendCommand} disabled={!payload.trim()}>
        {taskType} 명령 보내기
      </button>
    </section>

    {current && <section className={`result ${current.status.toLowerCase()}`}>
      <strong>{current.status === 'PENDING' ? 'RTOS Handler 선택 대기 중…' : current.status}</strong>
      <span>#{current.id} · {current.taskType}</span>
      <p>{current.result ?? '아직 실행 결과가 없습니다.'}</p>
    </section>}
    {error && <p className="error">{error}</p>}

    <section className="faults">
      <div className="section-title">
        <div><p className="eyebrow">RTOS → SPRING → REACT</p><h2>장치 장애</h2></div>
        <span>{faults.filter(fault => fault.status !== 'RESOLVED').length}건 미해결</span>
      </div>
      {faults.length === 0 ? <p>RTOS에서 수신한 장애가 없습니다.</p> :
        faults.map(fault => <article className={`fault ${fault.status.toLowerCase()}`} key={fault.id}>
          <div>
            <strong>#{fault.id} {fault.faultType}</strong>
            <small>{fault.deviceId} · {fault.message}</small>
          </div>
          <div className="fault-actions">
            <b>{fault.status}</b>
            {fault.status === 'ACTIVE' && <button onClick={() =>
              acknowledgeFault(fault.id).catch(e => setError(e.message))}>확인</button>}
            {fault.status === 'ACKNOWLEDGED' && <button className="recover" onClick={() =>
              requestRecovery(fault).catch(e => setError(e.message))}>RTOS 복구 요청</button>}
          </div>
        </article>)}
    </section>

    <section className="history">
      <h2>최근 명령</h2>
      {history.length === 0 ? <p>등록된 명령이 없습니다.</p> :
        history.map(command => <article key={command.id}>
          <div><strong>#{command.id} {command.taskType}</strong><small>{command.payload}</small></div>
          <b className={command.status.toLowerCase()}>{command.status}</b>
        </article>)}
    </section>
  </main>;
}

createRoot(document.getElementById('root')).render(<App />);

