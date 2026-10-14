# NIGHT SDL3 port — stage 1 vertical slice.
# Build: make  (deps: SDL3 via pkg-config; nothing else)
PORT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
SRC := $(wildcard $(PORT)src/*.c)
OBJ := $(patsubst $(PORT)src/%.c,$(PORT)build/%.o,$(SRC))
BIN := $(PORT)night

CC ?= cc
CFLAGS ?= -O2
CFLAGS += -std=c11 -Wall -Wextra -I$(PORT)src
SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl3 2>/dev/null)
ifeq ($(SDL_LIBS),)
  # Homebrew fallback
  SDL3 := /opt/homebrew/opt/sdl3
  SDL_CFLAGS += -I$(SDL3)/include
  SDL_LIBS   += -L$(SDL3)/lib -lSDL3
endif

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(SDL_LIBS)

$(PORT)build/%.o: $(PORT)src/%.c | $(PORT)build
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

$(PORT)build:
	mkdir -p $(PORT)build

run: $(BIN)
	$(BIN) --selftest

clean:
	rm -rf $(PORT)build $(BIN)

.PHONY: all run clean
