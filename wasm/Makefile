# night — the WebAssembly (browser) port of the NIGHT port.  docs/457.
#
# Targets:
#   make build      compile build/night.{js,wasm}  (the game layer is compiled in place from ../src)
#   make probe      the host tool that certifies an encoded track (docs/463): decode, clock, seeks
#   make audio      encode the disc's CD-DA to MP3 and write the index the port reads (needs ffmpeg)
#   make audio-check  drive the MP3 Red Book reader with MSCDEX-shaped reads against the sectors it replaces
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
            $(REPO)/src/cd/cd_iso.c $(REPO)/src/cd/cd_audit.c $(REPO)/src/cd/cd_res.c
# src/mp3_stream.c is the lossy CD-DA decoder and src/wasm_mp3.c is the Red Book reader on top of it;
# both also compile natively -- analysis/probes/nk_mp3_probe.c links the same mp3_stream.c to certify the
# files `make audio` writes (docs/463).
APP_FULL  = src/main_wasm.c src/wasm_disc.c src/wasm_mp3.c src/mp3_stream.c
OBJS      = $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(APP_FULL) $(GAME_FULL)))
VPATH       = src:$(REPO)/src/f:$(REPO)/src/sys:$(REPO)/src/app:$(REPO)/src/cd

# The warning diet is pocket/src/night/Makefile's: the reg-ABI port carries deliberate pointer-type
# mismatches and 64-bit constants, and newer clang promotes them to errors. Implicit function
# declarations stay errors on purpose -- a truncated pointer is the one bug class this repo has said
# cost a whole debugging session (docs/331 §7).
# -O2, and that is a measured choice rather than a default: -O3 across the game moved present by 2-4% and
# cost 20 KB of download. The game's own logic is step=0..1 us per tick -- it is not CPU-bound -- so the one
# place optimization pays is the decoder's float loop, and that gets its own rule below.
CFLAGS   += -std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter
CFLAGS   += -fcommon
CFLAGS   += -Wno-incompatible-pointer-types -Wno-int-conversion -Wno-implicit-int
CFLAGS   += -Wno-sign-compare -Wno-unused-label -Wno-constant-conversion
CFLAGS   += -Werror=implicit-function-declaration
CPPFLAGS += -Isrc -I$(REPO)/src -Ithird_party/minimp3
LDFLAGS  += -sENVIRONMENT=web
# The whole game main is one long-lived call that suspends ~60 times a second. ASYNCIFY_STACK_SIZE has to
# hold the C stack an unwind copies, so it travels with STACK_SIZE.
LDFLAGS  += -sASYNCIFY -sSTACK_SIZE=2097152 -sASYNCIFY_STACK_SIZE=4194304
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

.PHONY: all build stage lite serve check resample probe audio size clean

all: build

build: $(BUILD_DIR)/$(APP).js

# Make does not know a compile flag is an input. Nothing here says "when CFLAGS changes, rebuild", so an
# optimization experiment -- the -O3 that this Makefile's mp3 rule exists to apply -- silently compares a
# build against itself and prints identical numbers. The stamp carries the flags; the objects carry the
# stamp; change a flag and everything rebuilds, change nothing and nothing does.
FLAGS_STAMP := $(OBJ_DIR)/.flags
$(FLAGS_STAMP): Makefile
	@mkdir -p $(OBJ_DIR); new='$(CFLAGS) $(CPPFLAGS) $(LDFLAGS)'; \
	 if [ ! -f $@ ] || ! printf '%s\n' "$$new" | cmp -s - $@; then printf '%s\n' "$$new" > $@; fi

$(OBJ_DIR)/%.o: %.c $(FLAGS_STAMP) | $(OBJ_DIR)
	$(EMCC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(APP).js: $(OBJS) $(FLAGS_STAMP) | $(BUILD_DIR)
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

# The visitor's own game data (docs/464).  Nothing but code is served here: a dist with no `data/`, and a
# zip handed to the browser's file input over the DevTools protocol.  The default zip holds NIGHT.RES and no
# image, so this is the file-without-music shape -- the page must unpack it into the in-memory filesystem,
# say in its log that NIGHT.RES came from the file, and boot the game with no game byte from the server.
#
# `Z=<path>` runs a different game-data file.  A zip of TG_NIGHT.CUE and TG_NIGHT.IMG and nothing else
# (`zip -0 -X orig.zip TG_NIGHT.CUE TG_NIGHT.IMG`, stored so it can be seeked) is the original rip, which
# carries no NIGHT.RES at all -- so the page has to find it inside the disc's installer payload before the
# game can start (docs/465).
BYOD_DIST := $(DIST)-codeonly
check-byod: $(BUILD_DIR)/$(APP).js
ifeq ($(Z),)
	@rm -f $(BUILD_DIR)/byod.zip
	cd $(EXTRACTED) && zip -q -6 -X $(abspath $(BUILD_DIR)/byod.zip) NIGHT.RES
	@du -h $(BUILD_DIR)/byod.zip | sed 's/^/byod: /'
endif
	@rm -rf $(BYOD_DIST) && mkdir -p $(BYOD_DIST)
	cp $(runtime) $(BUILD_DIR)/$(APP).js $(BUILD_DIR)/$(APP).wasm $(BYOD_DIST)/
	rm -rf $(BUILD_DIR)/byod-profile
	NIGHT_KEYS='$(if $(KEYS),$(KEYS),Space:150 Enter:150 ArrowRight:150 KeyZ:150 Space:150)' \
	NIGHT_BYOD='$(if $(Z),$(Z),$(BUILD_DIR)/byod.zip)' \
	NIGHT_PROFILE=$(BUILD_DIR)/byod-profile \
	PORT=$(if $(PORT),$(PORT),8096) WAIT=$(if $(WAIT),$(WAIT),22000) DIST=$(BYOD_DIST) node scripts/check.js

# The IndexedDB asset cache, both ways, in one Chrome profile that survives between them: the cold pass
# empties the cache and must download everything; the warm pass is the same run again and must fetch zero
# bytes (it still sends a HEAD per file -- a cached byte that is not validated is the wrong music). A cache
# only ever tested empty is not a cache, it is a download with extra steps.
#
# There is no separate "check-preload": pre-download is the only mode the page offers, so the ordinary
# `make check` above already is that run -- its assertions include fetched=0, fails=0, holes=0.

# The IndexedDB asset cache, both ways, in one Chrome profile that survives between them: the cold pass
# empties the cache and must download everything; the warm pass is the same run again and must fetch zero
# bytes (it still sends a HEAD per file -- a cached byte that is not validated is the wrong music). A cache
# only ever tested empty is not a cache, it is a download with extra steps.
check-cache: stage
	rm -rf $(BUILD_DIR)/cdp-profile
	NIGHT_KEYS='$(if $(KEYS),$(KEYS),Space:150 Enter:150 ArrowRight:150 KeyZ:150 Space:150)' \
	NIGHT_PRELOAD=1 NIGHT_CACHE_COLD=1 NIGHT_PROFILE=$(BUILD_DIR)/cdp-profile \
	NIGHT_URL_EXTRA='&auto=1&preload=1&cache=clear' \
	PORT=$(PORT) WAIT=$(if $(WAIT),$(WAIT),22000) DIST=$(DIST) node scripts/check.js
	NIGHT_KEYS='$(if $(KEYS),$(KEYS),Space:150 Enter:150 ArrowRight:150 KeyZ:150 Space:150)' \
	NIGHT_PRELOAD=1 NIGHT_CACHE_WARM=1 NIGHT_PROFILE=$(BUILD_DIR)/cdp-profile \
	NIGHT_URL_EXTRA='&auto=1&preload=1' \
	PORT=$(PORT) WAIT=$(if $(WAIT),$(WAIT),22000) DIST=$(DIST) node scripts/check.js

# What the converter does to the board's samples: images, gain, the rate mark, and the producer running
# ahead. No browser and no game: this is the measurement behind "the effects sound less wrong now".
resample:
	node scripts/resample-test.js

# --- the lossy CD-DA (docs/463) ----------------------------------------------------
# The probe is a host tool, not a wasm one: it decodes a track with the SAME mp3_stream.c the port links
# and answers the questions the lossy path rests on -- is the file really constant bitrate, how far off is
# the byte clock, does a seek land on the sample it promised. encode_audio.py runs it per track and puts
# what it says into tracks.tsv, which is the only thing wasm/src/wasm_mp3.c trusts about track geometry.
probe: $(REPO)/build/nk_mp3_probe

$(REPO)/build/nk_mp3_probe: src/mp3_stream.c src/mp3_stream.h third_party/minimp3/minimp3.h \
                            $(REPO)/analysis/probes/nk_mp3_probe.c | $(BUILD_DIR)
	cd $(REPO) && cc -O2 -std=gnu11 -D_DARWIN_C_SOURCE -Wall -Wextra -Wno-unused-parameter \
	  -I wasm/src -I wasm/third_party/minimp3 -o build/nk_mp3_probe \
	  analysis/probes/nk_mp3_probe.c wasm/src/mp3_stream.c

# KBPS=96 TRACKS="02 03" FORCE=1 make audio
audio: probe
	python3 scripts/encode_audio.py $(if $(KBPS),--kbps $(KBPS),) $(if $(TRACKS),--tracks "$(TRACKS)",) \
	  $(if $(FORCE),--force,) $(if $(NO_PROBE),--no-probe,)

# The other half of the evidence, and the one that faces the game: nk_mp3_disc drives nk_mp3_pread -- the
# function cd_iso.c calls for a sector -- with the reads MSCDEX makes, and compares a seek against what a
# straight run through the same track read. Track 02 only, unless TRACKS says otherwise.
audio-check: $(REPO)/build/nk_mp3_disc
	cd $(REPO) && ./build/nk_mp3_disc wasm/dist/data/audio extracted/cd $(if $(TRACKS),$(TRACKS),02)

$(REPO)/build/nk_mp3_disc: src/wasm_mp3.c src/mp3_stream.c src/mp3_stream.h src/wasm_disc.h \
                            third_party/minimp3/minimp3.h $(REPO)/analysis/probes/nk_mp3_disc.c | $(BUILD_DIR)
	cd $(REPO) && cc -O2 -std=gnu11 -D_DARWIN_C_SOURCE -Wall -Wextra -Wno-unused-parameter \
	  -I wasm/src -I wasm/third_party/minimp3 -o build/nk_mp3_disc \
	  analysis/probes/nk_mp3_disc.c wasm/src/wasm_mp3.c wasm/src/mp3_stream.c

# The decoder is the only translation unit in this build that is a hot loop over data rather than a state
# machine over frames, and at -O2 wasm costs it roughly 8x what the same C costs the desktop CPU. -O3 plus
# "no math errno, no trapping math" is free for MP3 (no FP exception behaviour is being depended on) and
# buys back a slice of that.
$(OBJ_DIR)/mp3_stream.o: CFLAGS += -O3 -fno-math-errno -fno-trapping-math

size: build
	@ls -l $(BUILD_DIR)/$(APP).wasm $(BUILD_DIR)/$(APP).js | awk '{print $$5, $$9}'
	@$(EMCC) --version | head -1

clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR) $(DIST)
