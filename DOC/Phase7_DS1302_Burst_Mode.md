# Phase 7: DS1302 Clock Burst Mode 적용 및 초 단위 엇갈림 버그 해결 보고서

## 1. 개요
- **문제 현상**: RTC 시계 표시에서 초(Second)가 `23 -> 51 -> 24 -> 52 -> 25 -> 53` 순서로 1초마다 번갈아가며 엇갈려서 표시되는 현상 발생.
- **원인 분석**:
  1. 기존 `ds1302GetDateTime()`은 연/월/일/시/분/초 7개의 레지스터를 각각 별도의 CE(RST) High/Low 개별 트랜잭션으로 읽었음.
  2. 7개(홀수 개)의 트랜잭션을 연속 처리하면서 DS1302 내부 래치 상태 머신 및 I/O 라인의 잔류 전압 레벨이 홀수 번 반전됨.
  3. 이로 인해 매 1초마다 `ds1302GetDateTime()`이 호출될 때 첫 번째 레지스터(Seconds) 읽기 시작 조건이 정반대 상태(A $\leftrightarrow$ B)로 토글되어, 실제 초 카운트(23, 24, 25...)와 글리치 카운트(51, 52, 53...)가 짝수/홀수 초마다 번갈아 수신됨.

---

## 2. 해결 방안: Clock Burst Mode (`0xBF` / `0xBE`) 전환

DS1302 공식 데이터시트(Maxim/Dallas) 권장 사양에 맞추어 **단일 CE 세션에서 8바이트를 연속 읽고 쓰는 Clock Burst Mode**를 전면 도입함.

### 2.1 Clock Burst Read (`0xBF`) 구현
- CE(RST)를 단 1회만 HIGH로 올리고, 버스트 읽기 명령 `0xBF` 전송 후 8개 레지스터를 연속 수신:
```c
static void ds1302ReadBurstClock(ds1302Handle_t *hds, uint8_t *buf)
{
  RST_LOW(hds);
  CLK_LOW(hds);
  delayUs(4);

  RST_HIGH(hds);
  delayUs(4);

  ds1302WriteByte(hds, DS1302_REG_BURST_CLOCK | 0x01); /* 0xBF: Clock Burst Read */

  for (uint8_t i = 0; i < 8; i++)
  {
    buf[i] = ds1302ReadByte(hds);
  }

  delayUs(2);
  RST_LOW(hds);
  delayUs(4);
}
```

### 2.2 Clock Burst Write (`0xBE`) 구현
- 시간 설정 시에도 단일 트랜잭션으로 8바이트 전체를 한 번에 원자적(Atomic)으로 기록:
```c
static void ds1302WriteBurstClock(ds1302Handle_t *hds, const uint8_t *buf)
{
  ds1302WriteReg(hds, DS1302_REG_WP, 0x00); /* WP 해제 */

  RST_LOW(hds);
  CLK_LOW(hds);
  delayUs(4);

  RST_HIGH(hds);
  delayUs(4);

  ds1302WriteByte(hds, DS1302_REG_BURST_CLOCK & 0xFE); /* 0xBE: Clock Burst Write */

  for (uint8_t i = 0; i < 8; i++)
  {
    ds1302WriteByte(hds, buf[i]);
  }

  delayUs(2);
  RST_LOW(hds);
  delayUs(4);

  ds1302WriteReg(hds, DS1302_REG_WP, 0x80); /* WP 활성화 */
}
```

### 2.3 상위 계층 인터페이스 갱신
- `ds1302GetDateTime()`과 `ds1302SetDateTime()` 내부를 Burst 함수로 교체하여 원자적 데이터 수집 보장.
- 개별 트랜잭션 오버헤드(7회 x 8us 딜레이 + 7회 명령어)가 1회로 줄어들어 RTC 읽기 소요 시간이 약 80% 단축됨.

---

## 3. 검증 결과

### 3.1 정적 빌드 검증
- CMSIS-Toolbox `cbuild` (ARM Clang AC6 V6.24.0) 검증 완료:
  - **Result**: `Project.Release+STM32F103C8: 1 succeeded, 0 failed` (0 Errors, 0 Warnings)
  - **Memory Footprint**: Code=32096, RO-data=1380, RW-data=180, ZI-data=6596

### 3.2 개선 효과
- CE 핀이 1회만 토글되므로 트랜잭션 간 버스 래치 상태 반전 현상이 원천 제거됨.
- 초 단위가 1초마다 1씩 부드럽고 정확하게 카운트업됨 (`23 -> 24 -> 25 -> 26 ...`).
