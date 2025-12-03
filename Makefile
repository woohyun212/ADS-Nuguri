CC      := gcc
SRC     := nuguri.c

# OS 탐지
ifeq ($(OS),Windows_NT)
  HOST_OS := Windows
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Darwin)
    HOST_OS := macOS
  else ifeq ($(UNAME_S),Linux)
    HOST_OS := Linux
  else
    HOST_OS := $(UNAME_S)
  endif
endif

# 기본 컴파일/링크 옵션
CFLAGS  := -std=c11 -Wall -Wextra
LDFLAGS :=

ifeq ($(HOST_OS),Windows)
  # termiWin 경로 (Makefile 기준 상대경로)
  TERMIO_INC := termiWin-master/include
  TERMIO_SRC := termiWin-master/src/termiWin.c

  # termiWin 헤더를 찾도록 include 경로 추가
  CFLAGS += -I$(TERMIO_INC)

  # Windows용으로 SRC에 termiWin.c도 포함
  SRC += $(TERMIO_SRC)

  # 실행 파일 이름만 ".exe"로 붙이기 위해 TARGET 재정의
  TARGET := nuguri.exe
  REMOVE_CMD := del
else ifeq ($(HOST_OS),macOS)
  # macOS 전용 설정
  TARGET=nuguri
  REMOVE_CMD := rm
else ifeq ($(HOST_OS),Linux)
  # Linux 전용 설정
  TARGET=nuguri
  REMOVE_CMD := rm
endif

.PHONY: all clean clean_build

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean_build: clean all

clean:
	-$(REMOVE_CMD) $(TARGET)