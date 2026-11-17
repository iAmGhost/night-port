# 게임 데이터 준비 가이드 (Pocket 사용자용)

`iamghost.night` 코어는 게임 데이터를 포함하지 않습니다. 게임을 실행하려면 사용자가 직접 소유한 원본 게임 파일(CD 이미지 및 리소스)로부터 데이터를 추출하여 SD 카드에 준비해야 합니다.

**주의: 코어 바이너리는 별도로 제공됩니다. 이 가이드는 게임 실행을 위한 "데이터(Assets)"를 만드는 방법만을 다룹니다.**

## 1. 준비물

- **원본 게임 파일**: `TG_NIGHT.CUE` 및 `TG_NIGHT.IMG` (또는 이에 준하는 이미지와 큐 시트)
- **도구**: `uv`가 설치된 환경

## 2. 데이터 추출 단계

저장소의 루트 디렉토리에서 다음 명령어를 실행하여 게임 데이터를 추출합니다.

```sh
uv run night-extractor --data TG_NIGHT.CUE TG_NIGHT.IMG -o scratch/nkx/disc
```

`--data` 옵션 뒤에는 다음과 같은 다양한 형태를 넣을 수 있습니다:
- **큐 시트 + 이미지 파일**: `NIGHT.RES`와 모든 음악 트랙을 생성합니다. (권장)
- **압축 파일 (.zip)**: 압축을 풀지 않고 내부 파일을 직접 읽습니다.
- **풀려 있는 파일들**: `NIGHT.RES`, `MUSIC.RES`, `SOUND.RES`가 있다면 팬 재배포판 형태로 인식합니다.
- **이미지 파일만**: 큐 시트가 없으면 포트의 기본 트랙 표를 사용합니다.

### 추출된 결과물 구조 (`scratch/nkx/disc/`)

추출이 완료되면 다음과 같은 파일들이 생성됩니다:

| 파일명 | 설명 | 슬롯 번호 |
|---|---|---|
| `NIGHT.RES` | 게임의 핵심 리소스 컨테이너 | 4 |
| `track*.bin` | CD-DA 음악 트랙 파일들 | 5, 6, 20–31 등 |
| `slot_map.tsv` | 파일과 슬롯 번호의 매핑 정보 | - |

## 3. SD 카드에 복사하기

추출된 파일들을 Analogue Pocket SD 카드의 `Assets/night/common/` 폴더에 넣어야 합니다. 

**중요: 추출된 파일 경로에서 `cd/` 접두사는 제거해야 합니다.** (예: `scratch/nkx/disc/cd/track02.bin` $\rightarrow$ `track02.bin`)

```sh
# 예시 명령 (macOS 기준)
mkdir -p /Volumes/Pocket/Assets/night/common
cp scratch/nkx/disc/NIGHT.RES /Volumes/Pocket/Assets/night/common/
cp scratch/nkx/disc/track*.bin /Volumes/Pocket/Assets/night/common/
```

## 4. 주의 사항

- **음악 데이터**: 팬 재배포판(압축 파일 등)에서 추출한 음악은 원본 CD와 바이트 단위로 완벽히 일치하지 않을 수 있으며, 이로 인해 음악 재생 위치가 미세하게 어긋날 수 있습니다. 가장 정확한 방법은 원본 `CUE`와 `IMG`를 사용하는 것입니다.
- **파일 이름 제한**: 모든 파일 이름은 Pocket의 23자 제한을 준수해야 합니다.
- **검증**: 추출된 파일이 정확한지 확인하려면 명령 뒤에 `--verify`를 붙여 실행하십시오.
