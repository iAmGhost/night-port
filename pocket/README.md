# Pocket 코어 빌드 가이드

이 문서는 Analogue Pocket용 `iamghost.night` 코어를 직접 빌드하는 방법을 설명합니다. 코어 자체는 별도로 배포되지만, 개발 및 커스텀 빌드가 필요한 경우 이 가이드를 따르십시오.

## 1. 사전 요구 사항

코어를 빌드하려면 다음 도구들이 설치되어 있어야 합니다.

- **Python 3**: 스크립트 실행용
- **uv**: Python 패키지 및 환경 관리용 (권장)
- **make**: 빌드 자동화 도구
- **RISC-V GCC Toolchain**: Pocket의 RISC-V 아키텍처용 컴파일러

## 2. 코어 빌드 절차

코어 빌드는 `pocket/src/night` 디렉토리에서 수행됩니다.

### 2.1 기본 빌드

가장 기본적인 빌드 과정은 다음과 같습니다.

```sh
cd pocket/src/night
make all
```

이 명령은 다음을 수행합니다:
- RISC-V 코어 바이너리 (`app.elf`) 생성
- 코어 트리(Core Tree) 생성
- 필요한 헤더 파일 자동 생성

### 2.2 최적화 및 실험적 빌드 (BRAM 설정)

성능 측정을 위해 BRAM 사용 방식을 변경해야 하는 경우, `BRAM_SET` 옵션을 사용하여 빌드할 수 있습니다.

- **BRAM 펌프 모드 빌드**:
  ```sh
  make BRAM_SET=pump all
  ```

## 3. 게임 데이터 준비 (사용자 가이드)

코어 빌드와 별개로, 게임을 실행하기 위한 데이터(Assets)를 준비해야 합니다. 코어는 게임 데이터를 포함하지 않으므로, 사용자가 직접 원본 게임 파일에서 추출해야 합니다.

**주의: 코어 바이너리와 게임 데이터는 별개입니다.**

### 데이터 추출 방법

저장소 루트에서 `uv`를 사용하여 원본 게임 데이터를 추출합니다.

```sh
# CUE 파일과 이미지 파일을 사용하여 데이터 추출
uv run night-extractor --data TG_NIGHT.CUE TG_NIGHT.IMG -o scratch/nkx/disc
```

추출된 결과물(`scratch/nkx/disc/`)은 다음과 같이 구성됩니다:
- `NIGHT.RES`: 게임 컨테이너 파일 (슬롯 4)
- `track*.bin`: CD-DA 음악 트랙 파일들 (슬롯 5, 6, 20-31 등)
- `slot_map.tsv`: 파일과 슬롯 번호 매핑 정보

### SD 카드 배치

추출된 파일들을 Pocket SD 카드의 정해진 위치에 복사하십시오. `cd/` 접두사는 제거해야 합니다.

```sh
# 예시: Assets 디렉토리에 복사
mkdir -p /Volumes/Pocket/Assets/night/common
cp scratch/nkx/disc/NIGHT.RES /Volumes/Pocket/Assets/night/common/
cp scratch/nkx/disc/track*.bin /Volumes/Pocket/Assets/night/common/
```

## 4. 요약

| 항목 | 위치/명령어 | 설명 |
|---|---|---|
| 코어 빌드 | `pocket/src/night` | `make all` 실행 |
| 데이터 추출 | 프로젝트 루트 | `uv run night-extractor ...` 실행 |
| 배포 (Deploy) | `pocket/src/night` | `make deploy` (SD 카드 마운트 필요) |
