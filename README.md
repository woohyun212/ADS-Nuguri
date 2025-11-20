# ADS-Nuguri
고급자료구조 팀과제

## 협업 가이드

### 1. 커밋 메시지 컨벤션

커밋 메시지는 다음 형식을 따릅니다:

```
[<타입>] <제목>

- <본문> (선택사항)

- <꼬리말> (선택사항)
```

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
  typedef struct Node {
      int data;
      struct Node* next;
  } Node;
  ```

#### 들여쓰기 및 포맷팅
- 들여쓰기: 4칸 스페이스 사용
- 중괄호 스타일: K&R 스타일
  ```c
  if (condition) {
      // code
  } else {
      // code
  }
  ```

#### 주석
- 함수 상단에 간단한 설명 주석 작성
  ```c
  // 노드를 삽입하는 함수
  void insert_node(Node* root, int value) {
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
