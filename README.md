Project Repository : [https://github.com/woohyun212/ADS-Nuguri](https://github.com/woohyun212/ADS-Nuguri)
---

# ADS-Nuguri
고급자료구조 팀과제

---

## 팀소개
|<img width="100" height="100" alt="image" src="https://github.com/user-attachments/assets/4ebb2ffb-6c01-40dc-a3cc-64f9fad0d79c" />|<img width="100" height="100" alt="image" src="https://github.com/user-attachments/assets/fbbd67c2-97f8-4286-b09b-b312d1ec1f9a" />|<img width="100" height="100" alt="image" src="https://github.com/user-attachments/assets/666b4018-1b87-4b7b-8229-df4e29eb2cb2" />|
|:-:|:-:|:-:|
|박아정|박우현|이준서|
|20243103|20213097|20223129|

---

## OS별 컴파일 및 실행 방법 가이드

- gcc compiler 기준으로 안내한다.

### 3.2 Windows (gcc / MinGW 기준)

#### (1) 직접 컴파일

MinGW-w64 또는 유사한 gcc 환경에서:

```bash
gcc -std=c99 -Wall -o nuguri.exe nuguri.c
nuguri.exe
```

PowerShell 기준:

```powershell
gcc -std=c99 -Wall -o nuguri.exe nuguri.c
.\nuguri.exe
```

#### (2) Makefile 사용

```powershell
# 빌드
make          # 또는 mingw32-make 등 환경에 맞는 make 명령

# 실행
.\nuguri.exe

# 정리 및 재빌드
make clean
make          # 또는 make clean_build
```

> `Makefile`에서 기본 `TARGET`은 `nuguri`으로 정의되어 있으며,  
> Windows에서는 실행 시 `.exe` 확장자가 붙는다.

---

### 3.3 Linux

#### (1) 직접 컴파일

```bash
gcc -std=c99 -Wall -o nuguri nuguri.c
./nuguri
```

#### (2) Makefile 사용

`Makefile`은 OS를 감지하여 Linux인 경우 별도의 타깃 이름을 사용하도록 설정되어 있다.

```bash
# 빌드
make          # HOST_OS=Linux 인 경우 TARGET=MAKEPIC 으로 설정됨

# 실행
./MAKEPIC     # 또는 Makefile에 설정된 TARGET 이름

# 정리 및 재빌드
make clean
make          # 또는 make clean_build
```

> 실제 제출/실행 시에는 `TARGET` 이름을 `nuguri`으로 맞추거나,  
> README에 명시된 실행 파일 이름을 기준으로 조정하면 된다.

---

### 3.4 macOS

#### (1) 직접 컴파일

gcc가 설치 후

```bash
gcc -std=c99 -Wall -o nuguri nuguri.c
./nuguri
```

#### (2) Makefile 사용

```bash
make         # HOST_OS=macOS 로 인식됨
./nuguri   # 또는 Makefile에서 지정한 TARGET

make clean
make         # 또는 make clean_build
```

---

## 구현 기능 리스트 및 게임 스크린샷

### 구현 기능 리스트

1. 동적 맵 로딩 및 적과 코인 등 Stage Object 자동 인식 기능 구현
2. 스폰 위치 저장 및 리스폰 시스템 구현 (체력 및 점수 패널티 적용)
3. 체력(하트 UI) 시스템 도입 및 게임오버 화면 연출 추가
4. 코인 획득 판정 및 점수 반영 로직 개선 (효과음 추가)
5. 화면 버퍼링을 통한 깜빡임 최소화 및 출력 최적화
6. 적 이동 속도 제어를 통해 게임 난이도 조절 기능 추가
7. 캐릭터 및 객체 색상 출력으로 시인성 강화
8. 입력 처리 방식 개선 및 Crossplatform(Windows/Linux/macOS 환경) 지원
9. 오프닝/엔딩 화면 및 커서 제어 등 UX 향상 요소 추가
10. 동적으로 할당된 메모리 해제 루틴 구현으로 메모리 자원 관리 안정성 강화 

---

### 게임 스크린샷 

<!-- 게임 스크린샷 캡쳐하여 추가-->

|빌드 & 오프닝|플레이|
|:-:|:-:|
|![build&opening](https://github.com/user-attachments/assets/6b4c8e07-b2e7-4771-ae9b-b4062293854f)|![gameplay](https://github.com/user-attachments/assets/3c5b8624-e6e3-494f-ac12-bde35c42c3f8)|
|게임 클리어|게임 오버|
|![ending](https://github.com/user-attachments/assets/d0a58e27-925c-4c0b-b116-340a6bf2e802)|![gameover](https://github.com/user-attachments/assets/5ee5213d-4679-4b6e-9118-d29953f99c67)|


---

## 협업 가이드

### 1. 커밋 메시지 컨벤션

커밋 메시지는 다음 형식을 따릅니다:

```
[<타입>] <제목>

- <본문> (선택사항)

- <꼬리말> (선택사항)
```
한글을 사용해도 좋습니다.

#### 타입 (Type)
- **feat**: 새로운 기능 추가
- **fix**: 버그 수정
- **docs**: 문서 수정
- **style**: 코드 포맷팅, 세미콜론 누락 등 (기능 변경 없음)
- **refactor**: 코드 리팩토링
- **test**: 테스트 코드 추가 또는 수정
- **chore**: 빌드 업무 수정, 패키지 매니저 수정 등

#### 제목 작성 규칙
- 50자 이내로 작성
- 첫 글자는 대문자로 작성
- 마침표를 붙이지 않음
- 명령문으로 작성 (과거형 사용 금지)

#### 예시
```
feat: 이진 탐색 트리 구현

- 재귀 함수를 통한 탐색 구현
```

### 2. 코드 컨벤션

#### 네이밍 규칙
- **함수명**: snake_case 사용
  ```c
  void insert_node(Node* root, int value);
  int find_maximum(int arr[], int size);
  ```
- **변수명**: snake_case 사용
  ```c
  int node_count;
  char* file_name;
  ```
- **상수명**: UPPER_SNAKE_CASE 사용
  ```c
  #define MAX_SIZE 100
  #define DEFAULT_CAPACITY 10
  ```
- **구조체명**: PascalCase 사용
  ```c
  typedef struct Node 
  {
      int data;
      struct Node* next;
  } Node;
  ```

#### 들여쓰기 및 포맷팅
- 들여쓰기: 4칸 스페이스 사용
- 중괄호 스타일: BSD 스타일
  ```c
  if (condition)
  {
      // code
  }
  else
  {
      // code
  }
  ```

#### 주석
- 함수 상단에 간단한 설명 주석 작성
  ```c
  // 노드를 삽입하는 함수
  void insert_node(Node* root, int value) 
  {
      // implementation
  }
  ```
- 복잡한 알고리즘의 경우 핵심 로직에 주석 추가
- 한글 또는 영어로 작성 가능

#### 파일 구조
- 헤더 파일(.h): 함수 선언, 구조체 정의
- 소스 파일(.c): 함수 구현
- 파일명: snake_case 사용 (예: binary_tree.c, hash_table.h)

#### 기타 규칙
- 한 줄은 80자를 넘지 않도록 작성
- 각 함수는 한 가지 기능만 수행하도록 작성
- 매직 넘버 사용 금지 (상수로 정의)
- 메모리 할당 후 반드시 해제 확인

### 3. 브랜치 전략
- **main**: 안정된 코드
- **feature/<기능명>/\<name>**: 새로운 기능 개발 브랜치
- **issue/<이슈 번호>/\<name>**: 이슈 수정 브랜치
