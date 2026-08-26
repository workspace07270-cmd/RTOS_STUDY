import React, { useEffect, useState } from 'react';
import { createRoot } from 'react-dom/client';
import './style.css';

const API = 'http://localhost:8080/api/lotto-requests';

function App() {
  const [request, setRequest] = useState(null);
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [error, setError] = useState('');

  const isPending = request?.status === 'PENDING';

  async function issueLottoNumbers() {
    setError('');
    setIsSubmitting(true);

    try {
      const response = await fetch(API, {
        method: 'POST'
      });

      if (!response.ok) {
        throw new Error(`발급 요청 실패: HTTP ${response.status}`);
      }

      const data = await response.json();
      setRequest(data);
    } catch (exception) {
      setError(exception.message);
    } finally {
      setIsSubmitting(false);
    }
  }

  useEffect(() => {
    if (!request || request.status === 'COMPLETED') {
      return;
    }

    const timer = setInterval(async () => {
      try {
        const response = await fetch(`${API}/${request.id}`);

        if (!response.ok) {
          throw new Error(`상태 조회 실패: HTTP ${response.status}`);
        }

        const data = await response.json();
        setRequest(data);
        setError('');
      } catch (exception) {
        setError(exception.message);
      }
    }, 1000);

    return () => clearInterval(timer);
  }, [request?.id, request?.status]);

  return (
    <main>
      <p className="eyebrow">REACT · SPRING BOOT · RTOS</p>
      <h1>로또번호 발급 시스템</h1>

      <p className="description">
        로또번호 5세트를 생성하고 RTOS에 출력 작업을 요청합니다.
      </p>

      <button
        onClick={issueLottoNumbers}
        disabled={isSubmitting || isPending}
      >
        {isSubmitting
          ? '발급 요청 중…'
          : isPending
            ? 'RTOS 처리 대기 중…'
            : '로또번호 5세트 발급하기'}
      </button>

      {request && (
        <section className={request.status.toLowerCase()}>
          <div className="status">
            {request.status === 'PENDING'
              ? 'RTOS가 출력할 때까지 기다리는 중입니다.'
              : 'RTOS 출력이 완료됐습니다.'}
          </div>

          <p className="request-id">요청 ID: {request.id}</p>

          <ol className="lotto-list">
            {request.lottoSets.map((lottoSet, index) => (
              <li key={index}>
                <span>{index + 1}세트</span>

                <div className="numbers">
                  {lottoSet.map(number => (
                    <strong key={number}>{number}</strong>
                  ))}
                </div>
              </li>
            ))}
          </ol>
        </section>
      )}

      {error && <p className="error">{error}</p>}
    </main>
  );
}

createRoot(document.getElementById('root')).render(<App />);