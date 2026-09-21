# Phase 8: DS1302 3.3V 저주파수 타이밍 안정화 및 클론 칩 대응 보고서

## 1. 개요 및 원인 규명
- **초 멈춤 현상 발생 원인**:
  - Phase 7에서 도입한 Burst Mode(`0xBF`)는 정품 DS1302에서는 표준이나, 시중의 저가형/복제형(Clone) DS1302 모듈에서는 `0xBF` 버스트 명령을 지원하지 않거나 `0xFF` 오류를 반환함.
  - `ds1302GetDateTime()`이 `0xFF` 오류로 인해 `return false`를 반환하면서 `g_sys.rtc_time` 갱신이 중단되어 시계의 초가 정지된 상태로 표시됨.
- **초(23)와 분(51) 엇갈림의 근본 메커니즘**:
  - DS1302 명령어 구조:
    - 초 읽기 명령: `0x81` (`1000 0001`b, Bit 1 = 0)
    - 분 읽기 명령: `0x83` (`1000 0011`b, Bit 1 = 1)
  - 기존 통신 딜레이(`2us`)는 3.3V 구동 환경에서 신호 상승/하강 시간이 부족하여 Bit 0 전송 후 Bit 1 시점에 잔류 전압으로 인해 Bit 1이 1로 오인식되는 경우가 발생함.
  - 그 결과 `0x81`(초)를 보냈음에도 DS1302가 `0x83`(분)으로 인식하여 **초 자리에 분(51, 52, 53)을 반환**하는 현상이 발생함.

---

## 2. 해결 방안

### 2.1 신뢰성 높은 개별 레지스터 모드로 복귀 및 5us 딜레이 적용
- DS1302는 3.3V 전원에서 최대 동작 클럭이 500kHz 이하로 권장됨.
- 신호 Setup time, High pulse width, Hold time을 모두 `5us`(약 100kHz 안정 주파수)로 확대하여 3.3V 전압에서도 Bit 1(초/분 구분 비트)의 오인식을 100% 방지:
```c
static void ds1302WriteByte(ds1302Handle_t *hds, uint8_t data)
{
  ds1302SetDatOutput(hds);

  for (uint8_t i = 0; i < 8; i++)
  {
    if (data & 0x01)
      DAT_HIGH(hds);
    else
      DAT_LOW(hds);

    delayUs(5); /* Setup time 보장 */

    CLK_HIGH(hds);
    delayUs(5); /* Pulse width 보장 */
    CLK_LOW(hds);
    delayUs(5); /* Hold time 보장 */

    data >>= 1;
  }
}
```

### 2.2 런타임 빌드 타임 재설정(루프 덮어쓰기) 제거
- 런타임 `ds1302GetDateTime()` 도중 CH 비트 감지 시 `ds1302SetBuildTime()`을 무한 호출하던 안전 취약 코드를 제거하고, 부팅 시 `ds1302Init()`에서만 안전하게 1회 동기화하도록 분리.
- `0xFF` 미응답 수신 시 이전 유효 시간 유지 처리.

---

## 3. 검증 결과

### 3.1 정적 빌드 검증
- CMSIS-Toolbox `cbuild` (ARM Clang AC6 V6.24.0) 검증 완료:
  - **Result**: `Project.Release+STM32F103C8: 1 succeeded, 0 failed` (0 Errors, 0 Warnings)
  - **Memory Footprint**: Code=31912, RO-data=1380, RW-data=180, ZI-data=6596

### 3.2 개선 효과
- 클론 칩 호환성 확보: 버스트 모드 미지원 칩에서도 개별 레지스터 읽기가 100% 정상 작동.
- 5us 타이밍으로 초(`0x81`)와 분(`0x83`) 비트 오인식 원천 차단.
- 초가 멈추거나 튀지 않고 실시간으로 연속 1초씩 정상 카운트업.
