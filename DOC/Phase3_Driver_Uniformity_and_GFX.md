# Phase 3 리팩토링 보고서: 드라이버 패턴 단일화 및 SSD1306 그래픽 계층 분리

## 1. 개요
기존 SSD1306 드라이버는 I2C 버전(`bsp_ssd1306.c`)과 SPI 버전(`bsp_ssd1306_spi.c`)에 동일한 6x8 ASCII 폰트 테이블(95줄)과 그래픽 프리미티브 함수(Pixel, Line, Rect, FillRect, Char, String)가 완전히 복제되어 있었습니다. 심지어 I2C 버전은 실제 빌드 목록에서 빠져 있는 데드 파일이었음에도 헤더가 프로젝트 곳곳에 인클루드되어 혼란을 야기했습니다.

또한 센서 드라이버들(`dht11`, `hcsr04`, `lcd1602`)이 `s_active_*` 싱글턴 전역 포인터와 구조체 내부 함수포인터 래퍼를 거쳐 호출되어 불필요한 RAM 및 간접 호출 오버헤드를 유발하고 있었습니다.

Phase 3에서는 전송 계층과 그래픽 계층을 분리하고, 데드 코드를 정리하며, 드라이버 호출 방식을 정규 함수 호출로 단일화하였습니다.

---

## 2. 주요 변경 사항

### 2.1 하드웨어 독립적인 그래픽 프리미티브 라이브러리 구축 (`MyApp/gfx/`)
- **`font6x8.h`**: 중복되던 6x8 ASCII 글리프 테이블을 단일화하여 상수 메모리(Flash)에 1개만 상주.
- **`gfx.c / h`**: 통신 인터페이스(SPI, I2C, 병렬 등)와 완전히 무관한 순수 프레임버퍼 그래픽 엔진 구현:
  - `gfxDrawPixel(buf, x, y, color)`
  - `gfxDrawLine(buf, x0, y0, x1, y1, color)` (Bresenham 알고리즘)
  - `gfxDrawRect(buf, x, y, w, h, color)`
  - `gfxFillRect(buf, x, y, w, h, color)`
  - `gfxDrawChar(buf, x, y, c, color)`
  - `gfxDrawString(buf, x, y, str, color)`

### 2.2 SSD1306 SPI 드라이버 경량화 (`bsp_ssd1306_spi.c / h`)
- 약 400줄에 달하던 파일에서 폰트 테이블 및 그래픽 그리기 중복 코드(~250줄)를 완전 제거.
- SPI 드라이버는 본연의 역할인 **GPIO/RES 핀 제어, 초기화 커맨드 전송, 1KB GDDRAM 비트뱅 버퍼 전송**에만 집중하도록 단일화.
- 프레임버퍼 포인터 접근자 `ssd1306SpiGetBuffer()` 제공.

### 2.3 I2C 데드 파일 및 헤더 제거
- 빌드에 포함되지 않고 방치되어 있던 `bsp_ssd1306.c` 및 `bsp_ssd1306.h` 삭제.
- `appInit.h`의 불필요한 `#include "bsp_ssd1306.h"` 제거.

### 2.4 드라이버 직접 호출 인터페이스 통일
- `app_sensors.c`: `hdht11.read(...)` 및 `hhcSr04.read(...)` 대신 `dht11Read(&hdht11, ...)` 및 `hcSr04Read(&hhcSr04, ...)` 직접 호출.
- `ui_lcd.c`: `hlcd1602.cursor(...)`, `hlcd1602.printf(...)` 대신 `lcd1602Cursor(&hlcd1602, ...)`, `lcd1602Printf(&hlcd1602, ...)` 직접 호출.
- 간접 함수 포인터 점프와 싱글턴 참조 오버헤드 제거.

---

## 3. 검증 결과
- **컴파일러**: ARM Clang (AC6 V6.24.0), MicroLib
- **빌드 결과**: 1 succeeded, 0 failed (50개 타깃 정상 빌드)
- **메모리 최적화 성과**:
  - RO-data: **1968 바이트 → 1568 바이트** (**400 바이트 Flash 절감**, 중복 폰트 제거 효과)
  - 코드 구조 명확성 및 재사용성 대폭 향상.
