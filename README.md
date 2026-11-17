# NIGHT — 어쩐지 좋은 일이 생길 것 같은 저녁 SDL/WASM/Pocket 포트

1997년 도스 게임 **《어쩐지 좋은 일이 생길 것 같은 저녁》**의 원본 `NIGHT.EXE`를
함수 단위로 해체해 현대 플랫폼에서 다시 만든 프로젝트입니다.
네이티브(SDL3), 브라우저(WASM), Analogue Pocket(openFPGA) 세 가지 호스트가
**동일한 게임 레이어 하나**(`src/f`)를 공유합니다.

---

## 1. 원본 게임

- **제목**: 어쩐지 좋은 일이 생길 것 같은 저녁 (약칭 "어쩐지 저녁")
- **개발/출시**: TG Entertainment / Korea Media Link, 1997, DOS
- **원작**: 주간 소년챔프 연재 만화 (이명진 작, 단행본 100만 부 돌파)
- **장르**: 좌우 스크롤 격투 액션 (벨트스크롤 배틀)
- **시스템**: 주인공 "남궁 건"이 EXP·근성치 시스템으로 기술/스피드/파워/체력을
성장시키며, 연결기·캔슬 등 60여 종의 기술로 에피소드(전 9개)를 클리어해 나가는
만화 원작의 격투 액션입니다.
- **기술 사양**: Watcom C/C++ 32비트 + Rational DOS/4G(DOS4GW) 도스 익스텐더,
SVGA(VESA 640×480x8 모드로 부팅, 게임 화면은 320×200), Sound Blaster/ SB16,
MSCDEX 기반 CD-DA 14트랙.
- **배포 파일**: `NIGHT.EXE`(311 KB, MZ+LE), `NIGHT.RES`(51 MB 자산 컨테이너,
444개 자산), `DOS4GW.EXE`, `TG_NIGHT.CUE`/`TG_NIGHT.IMG`(633 MB CD 이미지)

> 원본 게임 데이터(`TG_NIGHT.IMG`, `NIGHT.RES`)은 저작권자의 것이며 이 저장소에
> 포함되지 않습니다. 자체적으로 확보한 디스크 이미지를 `make extract`하여 실행해야 합니다.

## 2. 프로젝트 소개

- Analogue Pocket(+openfpgaOS)에서의 실행을 주 목적으로 한 SDL 포트입니다.

- 원본에 가까운 포팅을 목표로 합니다. 100% 테스트가 완료되지 않았으므로 부정확한 구현이나 버그가 존재할 수 있습니다.
- 접근법은 **function-to-function**입니다: `NIGHT.EXE`의 모든 함수를 Ghidra로 해석하여 1:1 대응하는 C 함수로 포팅하고, 각각을 개별 검증합니다.
- 418개 함수(코드 215,518바이트, 호출 간선 3,478개) 전체 역컴파일 → 작업 순위
(`analysis/rank.tsv`, L0\~L23) → 현재 **526개 항목 포팅 완료 (비게임 항목 2개 기록, 남은 항목 없음), 픽스처 스위트 약 3,080 PASS / 0 FAIL**.
- 디버깅은 "추측"이 아니라 **차등 검증**으로: 포트와 원본을 같은 입력으로 달리고
스크린/오디오/상태를 비교하는 리그(`scripts/rig/`)가 모든 결정의 근거가 됩니다.
ASan/UBSan 스윕, 커버리지 기반 퍼징까지 수행합니다.
- 460편이 넘는 분석 문서(`docs/`)가 모든 주장의 측정 근거를 기록합니다.
문서 번호는 불변이며 `docs/INDEX.md`가 색인입니다.
(시작점: `docs/367` 통합 포트 기록, `docs/330` Pocket 계획, `docs/412` CD-DA)

## 3. 사용된 도구


| 도구                                                               | 용도                                                                                                                      |
| ---------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| **Ghidra 12** (+ LE/DOS 익스텐더 로더, `watcom:LE:32:default` 컴파일러 스펙) | `NIGHT.EXE` 정적 해석. 전체 함수 역컴파일 덤프는 `ghidra_scripts/DumpAll.java`                                                         |
| **Ghidra MCP**                                                   | 에이전트가 실행 중인 Ghidra 세션에 직접 질의(`get_current_program_info`, `run_ghidra_script` 등). `scripts/ghidra_scripts/*.java`로 대량 덤프 |
| **capstone** (Python)                                            | 바이너리 직접 역어셈블 감사(`scripts/audit/disasm.py`) — Ghidra가 놓친 간접 호출·전환 테이블 확인                                                 |
| **dosbox-staging**                                               | 원본 게임 플레이/상호작용 테스트                                                                                                      |
| **dosbox-automation + REST API (Linux 컨테이너 + Xvfb)**             | 원본의 프레임을 GUI 없이 headless로 캡처 — `/api/v1/video/frame?format=raw`는 포트 아레나 바이트와 직접 비교 가능 (`docs/46`)                       |
| **uv + Python**                                                  | 자산 디코더·추출기·차등 리그 (`scripts/`, `scripts/ref/`의 차등 레퍼런스 모델)                                                               |
| **SDL3**                                                         | 네이티브 호스트                                                                                                                |
| **Emscripten (emsdk)**                                           | WASM 포트                                                                                                                                |
| **openfpgaOS SDK (ThinkElastic)**                                | Analogue Pocket 코어 (RISC-V 32비트 ELF)                                                                                                   |


## 4. 아키텍처

핵심 원칙: **게임 레이어는 하나, 세 가지 호스트 환경 지원.** `src/f`는 픽스처로 검증된 유일한
본체이며, 세 포트 모두 이를 제자리에서 그대로 컴파일합니다(포크 없음).

```
┌─────────────────── 게임 레이어 (플랫폼 중립, SDL 없음) ───────────────────┐
│ src/f/f.c          NIGHT.EXE 함수들의 1:1 포트 (~29,000줄)               │
│ src/sys/sys.c      DOS4GW 대체 — 평탄(flat) 메모리 모델, DOS 인터페이스  │
│ src/res/           NIGHT.RES 디코더 (컨테이너, PCX, SPR, FLI/Cinepak,   │
│                    VOC→WAV, SGI 8-SVX)                                   │
│ src/cd/cd_iso.c    MSCDEX(Int 21h/2Eh) 에뮬 + 리모트 바이트 소스 추상화  │
│ src/gen/           생성 헤더 — 디스크 TOC, 함수 테이블 등                │
│ src/app/keymap.c   DOS 스캔코드 키맵(게임패드 레이아웃 포함)              │
└────────────────────────────┬─────────────────────────────────────────────┘
         ┌───────────────────┼────────────────────────┐
    src/app/            wasm/src/                pocket/src/night/
    SDL3 호스트          브라우저 호스트           openfpgaOS RISC-V 호스트
    build/night          WASM + JS                 iamghost.night 코어
```

**검증 체계**

- `make test` — 픽스처 스위트(`test/test_f1.c` + `test/vectors`).
Python 레퍼런스 모델(`scripts/ref/`)과 C 포트를 같은 벡터로 대조하는 차등 테스트.
`make test`는 앱 바이너리까지 함께 빌드하여 오래된(stale) 바이너리로 GREEN이 나오는 것을 차단합니다.
- `scripts/rig/` — 원본(dosbox-automation)과 포트를 같은 드라이브로 구동하고
스크린 점수·오디오·상태 셀·엔티티 목록을 비교하는 차등 리그.
`NIGHT_CD_AUDIT`, `NIGHT_NO_DISC`, `NIGHT_REQUIRE_DISC` 등의 환경 변수로
디스크 읽기를 감시·재현합니다.
- ASan/UBSan(양쪽 최적화 레벨), `-finstrument-functions` 카운스(FUZZ 커버리지) 빌드.
- 원본 레퍼런스 재현: `make extract`(≈3분, 570 MB) 후 `make extract-verify`.

## 5. 세 가지 포트

### 네이티브 포트 (`make app` → `build/night`)

SDL3 호스트(`src/app/main.c`). **디스크 이미지 없이** 실행됩니다: `make extract`가
`extracted/`에 `NIGHT.RES`와 트랙별 raw 파일을 만들고, 컴파일 내장 TOC
(`src/gen/disc_toc.h`)가 디스크 게이트를 통과시킵니다. 인자 없이 `./build/night`만
실행하면 됩니다. 게임패드 지원(왼쪽 스틱 = 방향키), 리깅/진단 스위치
(`NIGHT_CD_AUDIT` 등), ASan 빌드(`make asanapp`) 포함.

### WASM 포트 (`make -C wasm`)

브라우저용 완전 단일 스레드 포트(`wasm/src/main_wasm.c`). SDL 의존성이 없으며
모든 경계(boundary)가 메모리를 통해 통신합니다 — PCM 링(windowed-sinc 리샘플러 포함), 키 입력
블록, HUD 등을 포함합니다. 특징:

- **CD-DA는 MP3 사전 다운로드**: 546 MB CD-DA를 50 MB MP3로 인코딩해
HTTP Range가 아닌 게이트 후 선다운로드로 공급(`make -C wasm audio`).
- 음악을 꺼둘 경우(`play the music` 체크 해제) 음악 데이터를 요구하지 않습니다.
- `cd_iso.c`의 리모트 바이트 소스 추상화로 디스크 읽기는 HTTP Range(스트림 모드).
- headless Chrome 하네스(`make -C wasm check`)가 프레임/디스크/오디오/키를 단언.
- 산출물 약 513 KB `.wasm` + 68 KB `.js`. URL 레버(`?music=`, `?cache=`, `?audio=` 등).
- 상세: `docs/457`, `docs/463`.

### Pocket 포트 (`make -C pocket/src/night`)

Analogue Pocket용 openFPGA 코어 **`iamghost.night`** (openfpgaOS SDK, RISC-V
32비트 ELF). `src/f`를 그대로 컴파일한 앱(`main_of.c` 약 2,100줄)과 FPGA 코어로
구성되며, 게임 자산을 APF 데이터 슬롯 28개로 재패킹합니다 (`NIGHT.RES` + 트랙별
`.bin`, 약 600 MB).

- 게임 내부 PIT 분주비가 낳는 490 Hz 틱 펌프, 하드웨어 믹서(`VARIANT=os25`)
위로 CD-DA를 2채널 링(4초)·75섹터/s 디스크 클럭으로 갭리스 재생.
- SD 카드 한 번에 설치: `make -C pocket/src/night data` → `deploy`
(자세한 절차는 `pocket/INSTALL.md`, 계획/예산은 `docs/330`,
통합 기록과 남은 결함 목록은 `docs/367`).

## 6. 저장소 구조


| 경로           | 내용                                                                                                     |
| ------------ | ------------------------------------------------------------------------------------------------------ |
| `src/`       | 게임 레이어 + SDL 호스트                                                                                       |
| `wasm/`      | 브라우저 포트 (호스트 + MP3 리딩 + 하네스)                                                                           |
| `pocket/`    | openfpgaOS SDK 기반 Pocket 코어                                                                            |
| `test/`      | 픽스처 스위트와 차등 벡터                                                                                         |
| `scripts/`   | 추출/디코더(`extract_disc.py`, `extract_res.py`), 레퍼런스(`ref/`), 생성기(`gen/`), 차등 리그(`rig/`), 분석(`analysis/`) |
| `analysis/`  | 문서가 인용하는 근거 — 역어셈블, 의사코드, 노트, 프로브                                                                      |
| `docs/`      | 460여 편의 분석·포팅 기록 (불변 번호, `INDEX.md`가 색인)                                                               |
| `extracted/` | `make extract` 산출물 (git-ignored, 재생성 가능)                                                               |
| `build/`     | 모든 빌드 산출물 (git-ignored)                                                                                                               |


## 7. 빠른 시작

```sh
# 1) 디스크 이미지에서 실행 파일 세트 추출 (TG_NIGHT.CUE/IMG 필요)
make extract              # → extracted/ (NIGHT.RES, 트랙별 raw, TOC)

# 2) 네이티브 포트
make app && ./build/night

# 3) 검증 스위트 (~3,080 체크)
make test

# 4) 브라우저 포트 (~/.local/share/... 또는 ~/emsdk 필요)
make -C wasm stage && make -C wasm serve   # http://127.0.0.1:8091
make -C wasm check                         # headless Chrome 스모크

# 5) Pocket (SD 카드 마운트 후)
make -C pocket/src/night data
make -C pocket/src/night deploy            # 코어 트리 + 슬롯 파일 → /Volumes/Pocket
```

## 8. 라이선스 / 면책

- 이 저장소의 포팅 코드와 분석 문서는 프로젝트 라이선스를 따릅습니다
(Pocket 트리는 Apache-2.0, `pocket/LICENSE` 참조).
- 원본 게임의 실행 파일·자산·음악은 각각 저작권자의 소유이며 이 저장소에
포함되지 않습니다. 원본 디스크를 직접 소유하고 있는 경우에만 추출·실행이
가능합니다.

## 9. 유기농 슬롭 안내

- 이 저장소의 코드는 공개된 가중치를 사용한 로컬 LLM 환경에서 개발되었습니다.
  - Qwen3.8-27B
  - Qwen3.8-35B-A3B
  - Qwen3.8-Flash-Next