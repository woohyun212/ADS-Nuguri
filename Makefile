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
CFLAGS  := -std=c99 -Wall -Wextra
LDFLAGS :=

ifeq ($(HOST_OS),Windows)
  TARGET      := nuguri.exe
  REMOVE_CMD  := del
else ifeq ($(HOST_OS),macOS)
  TARGET      := nuguri
  REMOVE_CMD  := rm -f
else ifeq ($(HOST_OS),Linux)
  TARGET      := nuguri
  REMOVE_CMD  := rm -f
endif

.PHONY: all clean clean_build

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean_build: clean all

clean:
	-$(REMOVE_CMD) $(TARGET)
