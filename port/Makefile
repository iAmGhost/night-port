# Night port — function-to-function build.
#   make test    — build and run the f-layer verification tests (no SDL needed)
#   make selftest — build the res-pipeline selftest (needs SDL3)
CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra -Wno-unused-parameter
SRC_F   = src/f/f.c src/f/fx_04794a.c
SRC_SYS = src/sys/sys.c
T       = test

.PHONY: all test selftest clean

all: test

$(T)/test_f1: $(T)/test_f1.c $(SRC_F) $(SRC_SYS) src/f/f.h src/sys/sys.h
	$(CC) $(CFLAGS) -o $@ $(T)/test_f1.c $(SRC_F) $(SRC_SYS)

test: $(T)/test_f1
	./$(T)/test_f1

# res pipeline (verified decoders) — needs SDL3 for png_out
selftest:
	pkg-config --exists sdl3 && $(CC) $(CFLAGS) -o $(T)/selftest \
		src/res/selftest.c src/res/res_container.c src/res/res_pcx.c \
		src/res/res_spr.c src/res/fli.c src/res/voc.c src/res/wav.c \
		src/res/png_out.c $(shell pkg-config --cflags --libs sdl3) \
		|| { echo "SDL3 not found; skipping res selftest"; exit 0; }
	./$(T)/selftest

clean:
	rm -f $(T)/test_f1 $(T)/selftest
