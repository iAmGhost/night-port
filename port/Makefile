# Night port — function-to-function build.
#   make test    — build and run the f-layer verification tests (no SDL needed)
#   make selftest — build the res-pipeline selftest (needs SDL3)
CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra -Wno-unused-parameter
SRC_F   = src/f/f.c src/f/fx_04794a.c
SRC_SYS = src/sys/sys.c
SRC_CD  = src/cd/cd_iso.c
SRC_KEY = src/app/keymap.c
SRC_RIG = src/app/rig_sink.c src/app/rig_census.c src/gen/fn_table.c
T       = test

.PHONY: all test selftest app run clean asanapp-o2

all: test

$(T)/test_f1: $(T)/test_f1.c $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SRC_KEY) src/f/f.h src/sys/sys.h src/cd/cd_iso.h src/app/keymap.h
	$(CC) $(CFLAGS) -Isrc -o $@ $(T)/test_f1.c $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SRC_KEY)

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
$(T)/night: src/app/main.c $(SRC_RIG) src/f/rig_table.h src/f/rig_cells.h $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD) src/app/keymap.h src/f/f.h src/sys/sys.h src/cd/cd_iso.h
	$(CC) $(CFLAGS) -Isrc $(SDL_CFLAGS) -o $@ src/app/main.c $(SRC_RIG) $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SDL_LIBS) -lm
# sanitizers at the APP's optimisation level (docs/146): the -O1 asanapp survives some drives that the -O2
# app faults on, so the optimisation level -- not the sanitizer -- decides whether the fault appears. This
# build is the one that reports them with a stack.
asanapp-o2: $(T)/night-asan-o2
$(T)/night-asan-o2: src/app/main.c $(SRC_RIG) src/f/rig_table.h src/f/rig_cells.h $(SRC_KEY) $(SRC_F) \
                 $(SRC_SYS) $(SRC_CD)
	$(CC) $(CFLAGS) -O2 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc $(SDL_CFLAGS) -o $@ \
		src/app/main.c $(SRC_RIG) $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SDL_LIBS) -lm

# instrumented build for coverage-guided fuzzing (docs/120): every function entry is counted and keyed to
# the shipped guest address that names it.
$(T)/night-census: src/gen/fn_table.h \
                   $(SRC_RIG) src/f/rig_table.h src/f/rig_cells.h $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD)
	$(CC) $(CFLAGS) -finstrument-functions -Isrc $(SDL_CFLAGS) -o $@ src/app/main.c \
		$(SRC_RIG) $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SDL_LIBS)

# address- sanitizer build of the SDL host, for fuzz-crash triage
$(T)/night-asan: src/app/main.c $(SRC_RIG) src/f/rig_table.h src/f/rig_cells.h $(SRC_KEY) $(SRC_F) \
                 $(SRC_SYS) $(SRC_CD)
	$(CC) $(CFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc $(SDL_CFLAGS) -o $@ \
		src/app/main.c $(SRC_RIG) $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SDL_LIBS)

asanapp: $(T)/night-asan

censusapp: $(T)/night-census

run: app
	./$(T)/night

clean:
	rm -f $(T)/test_f1 $(T)/selftest $(T)/night
