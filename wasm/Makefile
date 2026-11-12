# night — the WebAssembly (browser) port of the NIGHT port.  docs/457.
#
# Targets:
#   make build      compile build/night.{js,wasm}  (the game layer is compiled in place from ../src)
#   make stage      assemble dist/: the app + the data a browser needs, tracks symlinked (no copies)
#   make lite       same, without the 604 MB of CD-DA: the game boots and plays silent
#   make serve      stage + serve dist/ on http://127.0.0.1:8091  (Range-capable server: wasm + CD-DA)
#   make check      headless-Chrome smoke test: boots the page, waits, screenshots, prints the log
#   make size       wasm/js bytes, and what they mean
#   make clean
#
# Like pocket/, the game layer (src/f, src/sys, src/app/keymap.c, src/cd) is compiled IN PLACE from the
# repo — one copy of the fixture-tested code, no fork.  wasm/src/main_wasm.c is the browser host twin of
# src/app/main.c, and there is no SDL in this build at all (see the comment at the top of main_wasm.c).

APP        = night
ROOT       = $(realpath $(CURDIR))
REPO       = $(realpath $(ROOT)/..)
OBJ_DIR    = $(ROOT)/.obj
BUILD_DIR  = $(ROOT)/build
DIST       = $(ROOT)/dist
DATA       = $(DIST)/data
EXTRACTED  = $(REPO)/extracted
PORT      ?= 8091

# emscripten. Pinned-ish: the toolchain version decides the EM_JS argument convention (this host refers
# to EM_JS parameters by their plain names, which is what this emsdk emits), so keep it in sync with the
# version printed by `make size`.
EMSDK      ?= $(HOME)/emsdk
EMCC        = $(EMSDK)/upstream/emscripten/emcc
export EMSDK

# --- sources ----------------------------------------------------------------------
GAME_FULL = $(REPO)/src/f/f.c $(REPO)/src/f/fx_04794a.c $(REPO)/src/sys/sys.c $(REPO)/src/app/keymap.c \
            $(REPO)/src/cd/cd_iso.c $(REPO)/src/cd/cd_audit.c
APP_FULL  = src/main_wasm.c src/wasm_disc.c
OBJS      = $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(APP_FULL) $(GAME_FULL)))
VPATH       = src:$(REPO)/src/f:$(REPO)/src/sys:$(REPO)/src/app:$(REPO)/src/cd

# The warning diet is pocket/src/night/Makefile's: the reg-ABI port carries deliberate pointer-type
# mismatches and 64-bit constants, and newer clang promotes them to errors. Implicit function
# declarations stay errors on purpose -- a truncated pointer is the one bug class this repo has said
# cost a whole debugging session (docs/331 §7).
CFLAGS   += -std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter
CFLAGS   += -fcommon
CFLAGS   += -Wno-incompatible-pointer-types -Wno-int-conversion -Wno-implicit-int
CFLAGS   += -Wno-sign-compare -Wno-unused-label -Wno-constant-conversion
CFLAGS   += -Werror=implicit-function-declaration
CPPFLAGS += -Isrc -I$(REPO)/src
LDFLAGS  += -sENVIRONMENT=web
# The whole game main is one long-lived call that suspends ~60 times a second. ASYNCIFY_STACK_SIZE has to
# hold the C stack an unwind copies, so it travels with STACK_SIZE.
LDFLAGS  += -sASYNCIFY -sSTACK_SIZE=2MB -sASYNCIFY_STACK_SIZE=4MB
# 68 MB of the port's data is static (the flat model's arenas); the game's own heap arena is capped at
# 48 MB by the boot recipe, and the NIGHT.RES bytes live in the JS-side filesystem, not in wasm memory.
LDFLAGS  += -sALLOW_MEMORY_GROWTH=1 -sINITIAL_HEAP=48MB -sMAXIMUM_MEMORY=768MB
# The page loads NIGHT.RES into the in-memory filesystem before starting the game, so main() must wait to
# be called, and FS/callMain have to be reachable from JS.
LDFLAGS  += -sINVOKE_RUN=0 -sFORCE_FILESYSTEM=1 -sMODULARIZE=1 -sEXPORT_NAME=NightModule \
            -sEXPORTED_RUNTIME_METHODS=FS,callMain,HEAPU8,HEAPU32,HEAP16 \
            -sEXPORTED_FUNCTIONS=_main,_nk_block_addr,_nk_ring_addr,_nk_ring_frames,_nk_ring_marks_addr

# `make DEBUG=1` for a build that checks its stack and asserts, which is what the first runs of the port
# need (an asyncify overflow looks exactly like a hang).
ifeq ($(DEBUG),1)
CFLAGS   += -O1 -g -DDEBUG
LDFLAGS  += -g -sASSERTIONS=1 -sSTACK_OVERFLOW_CHECK=2 -sASYNCIFY_DEBUG=0
else
LDFLAGS  += -sASSERTIONS=0
endif

.PHONY: all build stage lite serve check resample size clean

all: build

build: $(BUILD_DIR)/$(APP).js

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(EMCC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(APP).js: $(OBJS) | $(BUILD_DIR)
	$(EMCC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

$(OBJ_DIR) $(BUILD_DIR) $(DATA) $(DATA)/cd:
	mkdir -p $@

# --- the data a browser needs ------------------------------------------------------
# NIGHT.RES ships gzipped and the page inflates it (DecompressionStream) into the in-memory filesystem:
# 22 MB down instead of 51. The CUE is the real one — the per-track `cd/disc.cue` puts every track at its
# own INDEX 00:00:00, which would land the whole disc on one LBA.
$(DATA)/NIGHT.RES.gz: $(EXTRACTED)/NIGHT.RES | $(DATA)
	gzip -6 -c $< > $@
$(DATA)/TG_NIGHT.CUE: $(EXTRACTED)/TG_NIGHT.CUE | $(DATA)
	cp $< $@
$(DATA)/cd/track%.bin: $(EXTRACTED)/cd/track%.bin | $(DATA)/cd
	ln -f -s $< $@

TRACK_NAMES := 02 03 04 05 06 07 08 09 10 11 12 13 14 15
TRACK_FILES := $(patsubst %,$(DATA)/cd/track%.bin,$(TRACK_NAMES))

runtime := $(wildcard runtime/*) scripts/serve.py

stage: $(DATA)/NIGHT.RES.gz $(DATA)/TG_NIGHT.CUE $(TRACK_FILES) $(BUILD_DIR)/$(APP).js
	cp $(runtime) $(DIST)/
	cp $(BUILD_DIR)/$(APP).js $(BUILD_DIR)/$(APP).wasm $(DIST)/
	@du -sh $(DATA) | sed 's/^/data:   /'
	@echo "staged: $(DIST)"

# No CD-DA: the game boots and plays silent, and the whole download is the app plus NIGHT.RES.gz.
lite: $(DATA)/NIGHT.RES.gz $(DATA)/TG_NIGHT.CUE $(BUILD_DIR)/$(APP).js
	cp $(runtime) $(DIST)/
	cp $(BUILD_DIR)/$(APP).js $(BUILD_DIR)/$(APP).wasm $(DIST)/
	@echo "staged (no tracks): $(DIST)"

serve: stage
	python3 scripts/serve.py --dir $(DIST) --port $(PORT)

# Stage, serve, drive in Chrome with injected keys, assert, stop. The assertions are in headless.js's
# NIGHT_CHECK branch: loop driven, clock advancing, disc answering, rings draining, keys landing.
check: stage
	NIGHT_KEYS='$(if $(KEYS),$(KEYS),Space:150 Enter:150 ArrowRight:150 KeyZ:150 Space:150)' \
	PORT=$(PORT) WAIT=$(if $(WAIT),$(WAIT),22000) DIST=$(DIST) node scripts/check.js

# What the converter does to the board's samples: images, gain, the rate mark, and the producer running
# ahead. No browser and no game: this is the measurement behind "the effects sound less wrong now".
resample:
	node scripts/resample-test.js

size: build
	@ls -l $(BUILD_DIR)/$(APP).wasm $(BUILD_DIR)/$(APP).js | awk '{print $$5, $$9}'
	@$(EMCC) --version | head -1

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR) $(DIST)
