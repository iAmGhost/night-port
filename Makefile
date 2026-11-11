# Night port — function-to-function build.
#   make test    — build and run the f-layer verification tests (no SDL needed)
#   make selftest — build the res-pipeline selftest (needs SDL3)
#   make extract  — unpack the disc image into extracted/ (no installer, no IMG needed to run: docs/455)
#   make disc-toc — regenerate src/gen/disc_toc.h, the compiled-in TOC that starts the game with no disc
#
# LAYOUT (run make from the repo root): src/ and test/ hold sources only (test/ = the fixture suite
#         and its vector data); every build output -- test_f1, night, night-asan, night-asan-o2,
#         night-census, selftest and their .dSYM bundles -- lands in build/, which is git-ignored.
CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra -Wno-unused-parameter
SRC_F   = src/f/f.c src/f/fx_04794a.c
SRC_SYS = src/sys/sys.c
SRC_CD  = src/cd/cd_iso.c src/cd/cd_audit.c
SRC_KEY = src/app/keymap.c
SRC_RIG = src/app/rig_sink.c src/app/rig_census.c src/gen/fn_table.c
# T is the test SOURCE dir (test_f1.c, vectors/); B is where every build output goes.
# (no trailing comment on these two lines: GNU make 3.81 mis-parses an assignment line that carries one.)
T       = test
B       = build

.PHONY: all test selftest app run clean asanapp asanapp-o2 censusapp extract extract-verify disc-toc

all: test

$(B)/test_f1: $(T)/test_f1.c $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SRC_KEY) src/f/f.h src/sys/sys.h src/cd/cd_iso.h src/gen/disc_toc.h src/app/keymap.h
	mkdir -p $(B)
	$(CC) $(CFLAGS) -Isrc -o $@ $(T)/test_f1.c $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SRC_KEY)

# docs/181 §5: `test` used to build ONLY the fixture binary.  An app edit (f.c, main.c) therefore left
# the app binary stale -- measured as 4 hours older than src/f/f.c -- and `make test` still printed
# ALL PASS while every rig dump kept replaying the previous binary.  The app binary is now a prerequisite, so
# a green suite cannot be produced from a stale app.
test: $(B)/test_f1 $(B)/night
	./$(B)/test_f1

# res pipeline (verified decoders) — needs SDL3 for png_out
selftest: $(B)/selftest
	SHOTS=scratch/shots ./$(B)/selftest   # byte-exact refs (python FLI frame, CDDA wav) live in scratch
$(B)/selftest: src/res/selftest.c src/res/res_container.c src/res/res_pcx.c \
                src/res/res_spr.c src/res/fli.c src/res/voc.c src/res/wav.c \
                src/res/png_out.c src/res/res.h
	mkdir -p $(B)
	$(CC) $(CFLAGS) -Isrc -DRES_SELFTEST_MAIN $(SDL_CFLAGS) -o $@ src/res/selftest.c src/res/res_container.c \
		src/res/res_pcx.c src/res/res_spr.c src/res/fli.c src/res/voc.c src/res/wav.c \
		src/res/png_out.c $(SDL_LIBS) -lm


# the SDL host (needs SDL3: brew install sdl3)
SDL_CFLAGS ?= -I/opt/homebrew/include
SDL_LIBS   ?= -L/opt/homebrew/lib -lSDL3
app: $(B)/night
$(B)/night: src/app/main.c $(SRC_RIG) src/f/rig_table.h src/f/rig_cells.h $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD) src/app/keymap.h src/f/f.h src/sys/sys.h src/cd/cd_iso.h src/gen/disc_toc.h
	mkdir -p $(B)
	$(CC) $(CFLAGS) -Isrc $(SDL_CFLAGS) -o $@ src/app/main.c $(SRC_RIG) $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SDL_LIBS) -lm
# sanitizers at the APP's optimisation level (docs/146): the -O1 asanapp survives some drives that the -O2
# app faults on, so the optimisation level -- not the sanitizer -- decides whether the fault appears. This
# build is the one that reports them with a stack.
asanapp-o2: $(B)/night-asan-o2
$(B)/night-asan-o2: src/app/main.c $(SRC_RIG) src/f/rig_table.h src/f/rig_cells.h $(SRC_KEY) $(SRC_F) \
                 $(SRC_SYS) $(SRC_CD) src/gen/disc_toc.h
	mkdir -p $(B)
	$(CC) $(CFLAGS) -O2 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc $(SDL_CFLAGS) -o $@ \
		src/app/main.c $(SRC_RIG) $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SDL_LIBS) -lm

# instrumented build for coverage-guided fuzzing (docs/120): every function entry is counted and keyed to
# the shipped guest address that names it.
$(B)/night-census: src/gen/fn_table.h src/gen/disc_toc.h \
                   $(SRC_RIG) src/f/rig_table.h src/f/rig_cells.h $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD)
	mkdir -p $(B)
	$(CC) $(CFLAGS) -finstrument-functions -Isrc $(SDL_CFLAGS) -o $@ src/app/main.c \
		$(SRC_RIG) $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SDL_LIBS)

# address- sanitizer build of the SDL host, for fuzz-crash triage
$(B)/night-asan: src/app/main.c $(SRC_RIG) src/f/rig_table.h src/f/rig_cells.h $(SRC_KEY) $(SRC_F) \
                 $(SRC_SYS) $(SRC_CD) src/gen/disc_toc.h
	mkdir -p $(B)
	$(CC) $(CFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc $(SDL_CFLAGS) -o $@ \
		src/app/main.c $(SRC_RIG) $(SRC_KEY) $(SRC_F) $(SRC_SYS) $(SRC_CD) $(SDL_LIBS)

asanapp: $(B)/night-asan

censusapp: $(B)/night-census

# Replace the DOS installer and cut the music out of the disc image: extracted/ holds the install set,
# the CUE, and one raw-frame file per CD-DA track -- the format the Pocket's data slots already use
# (docs/455).  Regenerable and git-ignored; idempotent, so a re-run costs one stat per file, and
# extract-verify re-hashes the tree against the manifest the script wrote.
#
# No --cue on purpose: the tool takes the first .cue in the current directory and the image from that
# cue's own FILE line, which is the repo root's TG_NIGHT.CUE/TG_NIGHT.IMG today and a rip directory's
# whatever-it-is-called tomorrow.  `uv run night-extractor` is the same code as an installed command
# (scripts/night-extractor); the script here keeps working with no install and no venv.
extract:
	python3 scripts/extract_disc.py

extract-verify:
	python3 scripts/extract_disc.py --verify

# Rebuild src/gen/disc_toc.h, the compiled-in disc geometry that lets the game start with no CUE, no
# image and no track file (the disc-present gate passes, CD-DA stays silent -- docs/455).  Needed only
# to re-derive the numbers from a different pressing: it reads TG_NIGHT.CUE and needs the image for one
# number (where the LAST track ends, which a CUE sheet does not state).  `make test` covers the header
# against the gate's own condition, so a checked-in table cannot silently go stale.
disc-toc:
	python3 scripts/gen/gen_disc_toc.py

run: app
	./$(B)/night

clean:
	rm -rf $(B)
