# NIGHT — WebAssembly 포트

TG Entertainment의 1994년 DOS 격투 게임 NIGHT를 SDL 없이 WebAssembly로 포팅한 버전입니다.

## 재생 방법

### 직접 호스팅하기

아래 명령어로 أي HTTP 서버로 실행:

    python3 -m http.server 8000 --directory dist-codeonly

그런 다음 브라우저에서 http://localhost:8000/ 열기

### 직접 게임 데이터 사용하기

이 포트에는 게임 데이터가 포함되지 않습니다. `NIGHT.RES` alone, `CUE` + `IMG` 쌍, 또는 이들을 포함한 zip 파일을 선택하면 됩니다.
페이지가 로컬에서 파일을 읽습니다 — 아무것도 업로드되지 않습니다.

## 조작법

- 방향키 — 이동
- Z — 펀치
- X — 발차기
- Space — 웅크리기
- Enter — 시작 / 메뉴

모바일에서 터치 컨트롤 사용 가능.
