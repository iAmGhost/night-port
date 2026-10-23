# Night port — function-to-function build.
#   make test    — build and run the f-layer verification tests (no SDL needed)
#   make selftest — build the res-pipeline selftest (needs SDL3)
CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra -Wno-unused-parameter
SRC_F   = src/f/f.c src/f/fx_04794a.c
SRC_SYS = src/sys/sys.c
SRC_CD  = src/cd/cd_iso.c
T       = test

.PHONY: all test selftest app run clean

all: test

$(T)/test_f1: $(T)/test_f1.c $(SRC_F) $(SRC_SYS) $(SRC_CD) src/f/f.h src/sys/sys.h src/cd/cd_iso.h
	$(CC) $(CFLAGS) -o $@ $(T)/test_f1.c $(SRC_F) $(SRC_SYS) $(SRC_CD)

test: $(T)/test_f1
	./$(T)/test_f1

# res pipeline (verified decoders) — needs SDL3 for png_out
selftest: $(T)/selftest
	SHOTS=../scratch/shots ./$(T)/selftest   # byte-exact refs (python FLI frame, CDDA wav) live in scratch
$(T)/selftest: src/res/selftest.c src/res/res_container.c src/res/res_pcx.c \
                src/res/res_spr.c src/res/fli.c src/res/voc.c src/res/wav.c \
                src/res/png_out.c src/res/res.h
	$(CC) $(CFLAGS) -Isrc -DRES_SELFTEST_MAIN $(SDL_CFLAGS) -o $@ src/res/selftest.c src/res/res_container.c \
		src/res/res_pcx.c src/res/res_spr.c src/res/fli.c src/res/voc.c src/res/wav.c \
		src/res/png_out.c $(SDL_LIBS) -lm


# the SDL host (needs SDL3: brew install sdl3)
SDL_CFLAGS ?= -I/opt/homebrew/include
SDL_LIBS   ?= -L/opt/homebrew/lib -lSDL3
app: $(T)/night
$(T)/night: src/app/main.c $(SRC_F) $(SRC_SYS) $(SRC_CD) src/f/f.h src/sys/sys.h src/cd/cd_iso.h
	$(CC) $(CFLAGS) -Isrc $(SDL_CFLAGS) -o $@ src/app/main.c $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SDL_LIBS) -lm
run: app
	./$(T)/night

clean:
	rm -f $(T)/test_f1 $(T)/selftest $(T)/night
