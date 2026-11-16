# Installing the NIGHT Pocket core

`iamghost.night` is an openfpgaOS core for the Analogue Pocket.  It runs the same `src/f` game layer as the SDL port
(`build/night`), on the Pocket, from the original game's own data repacked into APF data slots.

The game data is yours: nothing in this repo ships NIGHT.RES or the disc audio.  Everything below regenerates it
from the `TG_NIGHT.CUE` you already have, the image its own `FILE` line names, and the `NIGHT.RES` of your
install set. `make extract` puts all three in `extracted/` (docs/455); `scripts/pocket/gen_disc.py`,
`repack.py` and `mk_dist.py` take them by suffix through `scripts/disc_loc.py` -- the repo root, a
`NIGHT/` under it, or the directory you run them from, and `NIGHT.RES` from `extracted/` first -- so a
rip kept anywhere works as long as the CUE is on that search path. `NIGHT_EXE` names a particular
`NIGHT.EXE` when a run has to say which copy it measured.


> **Upgrading clears your menu settings (docs/394 3).**  `scripts/pocket/deploy.sh` deletes
> `Settings/iamghost.night/Interact/` on purpose: a stale `interact_persist.json` once carried "Music Volume 0", which
> the peripheral mirror read as a real choice and the core played as silence.  After a deploy, the in-menu knobs (Music
> Volume, Sound Volume, Screen Fit) return to the `night.ini` values shown above.  Your `Input` and `Video`
> settings are left alone.

## 0. 게임 데이터 준비 (한국어)

이 코어 패키지는 게임 데이터를 담지 않습니다.  `NIGHT.RES`와 음악은 직접 갖고 계신 게임 파일에서 만들어야 하고,
그 작업에 도스용 설치 프로그램(`SETUP.EXE`)이나 윈도우는 필요 없습니다.  저장매체에 있는 파일을 고르는 것만으로
끝납니다.

저장소 루트에서 실행합니다 (Python이 아니라 `uv` 하나면 됩니다):

```sh
uv run night-extractor --data TG_NIGHT.CUE TG_NIGHT.IMG -o scratch/nkx/disc
```

`--data` 뒤에는 어떤 형태를 놓아도 됩니다.  각각 다른 이름을 가진 같은 데이터일 뿐이고, 어떤 것인지 판별하는 일은
스크립트가 파일 내용을 보고 합니다:

| 놓는 것 | 결과 |
|---|---|
| 큐 시트 + 이미지 두 파일 (이름은 아무거나) | `NIGHT.RES`와 14개 트랙을 모두 만듭니다 |
| 압축 파일 하나 (`.zip`) | 압축을 풀지 않고 그 안에서 읽어냅니다 |
| 풀어 놓은 파일들 (`NIGHT.RES`, `MUSIC.RES`, `SOUND.RES`) | 팬 재배포판 형태입니다.  음악에 대한 주의는 아래 |
| 이미지 혼자 (큐 없음) | 이 포트가 가진 트랙 표를 사용합니다 |
| `NIGHT.RES` 혼자 | 게임은 뜨지만 음악은 없습니다 |

만들리는 것은 이렇습니다:

```
scratch/nkx/disc/
  NIGHT.RES            슬롯 4 — 게임이 읽어들이는 컨테이너, 원본 그대로
  cd/track02.bin …     슬롯 5, 6, 20–31 — 트랙 하나에 파일 하나, 2352바이트 프레임을 그대로 잘라낸 것
  slot_map.tsv         어느 파일을 어느 슬롯에 넣는지
  MANIFEST.tsv         각 파일의 바이트 수와 해시 — `--verify`로 다시 검사할 수 있습니다
```

SD 카드에는 이렇게 놓습니다.  `cd/` 접두사는 **벗깁니다** — 코어가 요구하는 이름은 `track02.bin`입니다:

```sh
mkdir -p /Volumes/Pocket/Assets/night/common
cp scratch/nkx/disc/NIGHT.RES /Volumes/Pocket/Assets/night/common/
cp scratch/nkx/disc/cd/track*.bin /Volumes/Pocket/Assets/night/common/
```

코어 본체(`Cores/iamghost.night/`)는 아래 1번 절차로 카드에 넣게 되어 있고, 여기 만든 파일들만 같은 자리에 가면 됩니다.
`slot_map.tsv`의 슬롯 번호는 코어가 `data.json`에 묶어 놓은 번호와 같은 표에서 나왔습니다.

음악이 원본 CD와 **바이트까지 같은지는 보장하지 않습니다.**  팬 재배포판의 음악 파일은 이 디스크의 음악과 같은
녹음인데도, 트랙 안에서 어떤 구간은 완전히 같고 어떤 구간부터는 다른 데서 읽어옵니다 (그쪽 이미지가 디스크의
헤더 174개 섹터만 들고 나머지 파일은 담지 않기 때문입니다).  그래서 카드에는 그쪽 이미지를 그대로 잘라 넣게 되고,
게임이 묻는 위치는 디스크의 위치라 제목 화면의 테마 같은 짧은 재생은 맞지만 긴 트랙은 중간에 어긋날 수 있습니다.
같은 데이터에서 `--wav`를 붙이면 각 트랙을 44.1 kHz WAV로도 써 주니, 사람이 직접 들어보고 판단할 수 있습니다.
원본 디스크 이미지(`TG_NIGHT.CUE` + 이미지)를 쓰면 14개 트랙이 전부 원본과 바이트 단위로 같은 것으로 확인되었습니다.

만든 파일이 맞는지는 같은 명령에 `--verify`를 붙이면 다시 검사합니다 (각 파일의 해시를 `MANIFEST.tsv`와 대조).
개발자가 저장소 안에서 같은 일을 할 때는 `make -C pocket/src/night data`를 쓰면 되는데, 이쪽도 결국 위와 같은
입력 단계를 지나갑니다.  어느 형태에서 무엇이 나왔고 음악이 어디까지 같은지 측정한 표는 `docs/466`에 있습니다.


## 1. One command, on a mounted SD card

```sh
make -C pocket/src/night all      # the RISC-V core (app.elf) + the core tree + the generated headers
make -C pocket/src/night data     # scratch/pocket/: NIGHT.RES + one .bin per CDDA track (~600 MB, regenerable)
make -C pocket/src/night deploy   # core tree + slot files -> /Volumes/Pocket
```

`deploy` is the install step.  It writes

```
/Volumes/Pocket/Cores/iamghost.night/   core.json  variants.json  video.json  audio.json  input.json
                                         interact.json  data.json  loader.bin  os25.rbf_r
/Volumes/Pocket/Assets/night/common/    night.elf  os.bin  bank.ofsf  night.ini  README.md
                                         NIGHT.RES  track02.bin .. track15.bin
/Volumes/Pocket/Saves/night/common/     (created by the core: nightlog.sav, and only while `LOG=1`)
```

`make copy` is the SDK's own copy step and carries the **core tree only** -- no slot files.  Run `deploy`, and run it
last: `copy.sh` writes macOS AppleDouble sidecars (`._night.elf`) that `deploy` strips again.

Slot bindings are generated, not typed: `scripts/pocket/repack.py` + `scripts/pocket/mk_dist.py` write `data.json`,
`Assets/night/iamghost.night/NIGHT.json` and `src/cd/disc_table.h` from the CUE, and
`scripts/pocket/check_slots.py` proves the three agree and that no audio file sits in the firmware's save range
(slots 10-19).  `data.json` declares 28 slots with ids 0..31, inside Analogue's 32-slot ceiling; every filename is
under the Pocket's 23-character limit.  The table of filenames and slots lives in the deployed
`Assets/night/common/README.md` and is the thing to check a card against.

## 2. Settings

On the device, **Core Options** carries three settings -- Music Volume, SFX Volume, Screen Fit.  They are three
fields of one bridge word (`0xF700000C`, mirrored into the peripheral register `0x4000006c` that the core can read)
and they survive an eject and a reboot.  `L1+B` cycles the panel fit live.  Why the menu reaches the game through
that route, and why not through `of_interact_get()`, is docs/350 - docs/362.

The rest are developer keys in `Assets/night/common/night.ini` under `[night]` -- the deployed README lists every one
with its default.  The ones that matter for a report:

| key | ships as | set it to |
|---|---|---|
| `LOG` | `0` (no log file) | `1` to write `Saves/night/common/nightlog.sav`, the diagnostic trail the port was measured with |
| `DIAG` | `0` (a player's build performs no experimental bus probes) | `1` peripheral scan, `2` analogizer probe, `4` interact census |
| `FB_MODE` | `1` (native 640x480 surface, row memcpy) | Hardware-measured, and `0` is a trap: 307 200 B / 15 771 us = 19.5 MB/s against 76 800 B / 82 657 us = 0.93 MB/s for the host 2x2 downscale (~1 us per output pixel). `0` was shipped for two builds and brought the logo freeze back on hardware (docs/401, docs/404 §5) |
| `FIT` | `1` (stretch) | `0` letterbox, `2` the core scaler's 320x200 mode |
| `SCREEN` | `0` | `0` = the game alone, which is what ships; `2` = a nine-line diagnostic panel in white text over it (version, frame, view + picture signature, ticks/s and frames/s, present cost, palette and arena use, CD position, disc reads, ring headroom), and `1` terminal text only. `2` was the default once and cost a 1 Hz flicker over the game (docs/422), so the overlay is a switch, not the product |
| `TICK_HZ` | `490` | a cap on the game's fixed step rate, for slow-motion experiments |
| `SFX_GAIN` / `CDDA_GAIN` | `200` / `150` | 100-212 %, through a knee that cannot clip and cannot attenuate (docs/361) |

There are **no game saves**, and that is deliberate (docs/449): the shipped game has no save concept at all —
`src/f` names no save file and the shipped game ships none (docs/374) — so saves were the port inventing a feature, and the
masked restore that came with them only ever half-worked (HP bars restored, stage position and enemies did not).
They were removed in `0.2.23`, together with the `L1+Select` / `L1+Start` / `L1+A` combos that carried them; those
combos also pressed Esc and Enter on the game, so every save was a stray key into the shipped pause menu.
`L1+Up` (trace window) and `L1+B` (panel fit cycle) remain.

The data-slot manifest still *declares* save slots `10`–`16` (`night_0.sav … night_6.sav`) next to the
log slot `17`. **Nothing ever opens them.** They are there because the OS does not appear to attach a
save slot the save table never reaches: declaring only `17` made every write to the log succeed, from
the app's side, and land nowhere, for six boots (docs/454). The cores on the card that do write saves
(Quake, Doom) both declare `10`–`19` contiguously, so this is the shape the platform expects. A build
whose log slot sits below a hole in the table is a build that is blind.

The card ships **`LOG=0 CONSOLE=0`**: no file is written, nothing is printed, and a boot leaves nothing on
the card behind. That is the player shape and the default -- and since 0.4.5 it is also not the shape that
stutters (below). Logging is a switch, not a state: flip it with `scripts/pocket/logswitch.sh ram` before a
diagnostic boot and `off` after (the script prints what the card now says, because `deploy.sh` regenerates
`night.ini` from the dist tree and has quietly reverted hand-edits before: docs/428).

`ram` (`LOG=3`) keeps **every** line in an SDRAM ring and commits it to the card as one append every
`LOG_FLUSH_MS` (default 30 s, so ~2 writes a minute) -- the most trail per card event available, and
measured smooth (docs/456 13). `on` (`LOG=1`) writes every line as it happens, a quarter-megabyte file
committed ~0.6 times a second, which is not the build a feel judgement is valid on (docs/452).

A hitch hunt wants one more line than "log on": `scripts/pocket/logswitch.sh ram --stall 120`.
`STALL_MS` is report-only (`main_of.c:3207` -- a frame gap that passes it testifies once, naming the resource
in flight), and the shipped `400` sees only the stage-entry stalls docs/433 chased. A hitch you can feel is
smaller than that, and because the flag is part of the script the shape survives being argued about; a
hand-edit to the ini does not survive the next deploy (docs/456).

## 3. Reporting a hardware run

Power the Pocket **fully off** -- not sleep -- before a boot you intend to believe, and eject cleanly
(`diskutil eject /Volumes/Pocket`) before mounting it to read a log. A warm resume relaunches the cached app,
and a half-flushed exFAT read hands the device yesterday's bytes; both produce a boot that ran bits the card
already replaced, which is how a "0.3.3 is stuttering" report turned out to be a 0.3.2 boot (docs/456).

The shipped ini is `LOG=0 CONSOLE=0`, and getting back here cost twenty-six sections (docs/456). `LOG=0`
used to *be* the stutter: same ELF, `LOG=1` called smooth in three boots, `LOG=0` called stuttering -- so
0.3.7 shipped a player shape that wrote a summary log instead of nothing. The confound was a single line:
when no log can be opened, every diagnostic line `printf`s instead (`main_of.c:3225`), and on hardware
`printf` is a `write(1)` syscall into the SDK's 40x30 text console -- which in FRAMEBUFFER display mode is
not displayed at all. Read as a pair on 0.4.4 with one key between them: console on, 11 of 12 ordinary
seconds carried a gap over 80 ms and the machine delivered 32 frames/s; console off, 1 of 20 and 39
frames/s. The silent shape is therefore no longer the stuttering shape, and the player default is blind
again by deliberate choice -- `logswitch.sh ram` gets the full trail back, `logswitch.sh off` returns the
card to shipped.

Then archive the log with the tool that checks it, rather than copying it by hand:

```sh
./scripts/pocket/logsave.sh --note stutter-hunt     # names the file by the version the core stamped in its
                                                    # own boot marker, and asserts that banner against the
                                                    # card's core.json -- a mismatch prints VOID BOOT and
                                                    # refuses the comparison (docs/428, docs/456)
```

Once it is archived, read it apart by display branch instead of as one fps number -- the 640x480 slide path
and the game view cost 25 ms and 4 ms a present, and a single average hides both:

```sh
python3 scripts/pocket/branchfps.py scratch/pocket/logs/<file>.log   # per-branch medians, every window under
                                                                     # 25 fps, and two labels that stop
                                                                     # misreadings: HOLD (the stage-entry
                                                                     # card, which is allowed to run 1 fps)
                                                                     # and CLAMPED, where aud+disc overlap and
                                                                     # `rest` has frozen at 0 (docs/456 9-10)
```

Or read it directly, remembering that `nightlog.sav` is a ring:

```sh
cp /Volumes/Pocket/Saves/night/common/nightlog.sav /tmp/nightlog.sav
grep -E '^(settings|clock|memstat|perf|stages|budget|audio|view|video CHANGED|gap worst|STALL)' /tmp/nightlog.sav | tail -40
strings /Volumes/Pocket/Assets/night/common/night.elf | grep -E '^0\.[0-9]+\.[0-9]+$' | head -1   # the version actually on the card
md5 -q /Volumes/Pocket/Assets/night/common/night.elf
```

The `budget` line is what carries the device's own SD-read timings (`disc reads=… mean=…us max=…us seeks=…
fails=…`) and the mixer's underrun counters; the desktop shim cannot produce the SD half of it (docs/364 §5).  A
screenshot from `Memories/Screenshots/` helps when the complaint is about the picture.

**If the complaint is the music, send the whole file rather than the grep.**  The `audio voice live=…` line added
in 0.1.89 is the only thing in the log that asks the mixer instead of the port's own model, and it has to be read
against the `audio cdda=` and `perf` lines around it (docs/408).  Note that `nightlog.sav` is a fixed-size slot:
new data starts at the beginning and the previous boot's tail survives below it, so look for the point where the
cumulative counters go *down* and cut there (docs/402 §1) — everything below that line is an older run.
For a music pause, the two fields that decide it are `e_live` (0 while the voice is sounding = the voice is reading silence) and `behind`/`ahead` (both near half the ring = healthy; `behind` near `RING` = the voice is about to run off the written frontier). See docs/409.

## 4. Measuring performance (two boots, one ini line apart)

The perf questions this port has open can only be answered on the device, because BRAM and SD cost are no-ops on the
desktop shim.  Set `LOG=1` in `Assets/night/common/night.ini` (the `budget`/`stages`/`perf` lines live in the log),
then run the same ~60 seconds of play twice:

```sh
# variant A: the shipping placement -- per-frame copy path in the 14 KB BRAM
cd pocket/src/night && make deploy

# variant B: tick_pump in BRAM instead, copy path back in SDRAM (docs/372 3)
cd pocket/src/night && make BRAM_SET=pump && make deploy
```

Compare `grep -E '^(stages|perf|budget)' nightlog.sav` between the two.  A third axis is `PUMP_AUDIO_TICK=1`, which
gates the board's DMA drain to the 490 Hz tick instead of every pump entry (docs/369 §5) -- judge it on `sfx=…/…/…`
and `dry` staying flat.  Whichever wins is what stays default.

The three disc levers need no rebuild at all, only an ini line (docs/375, docs/376), and they are read from the same
`budget` line's `disc ops=/sectors=/max_op=` and `cdda fills=` columns:

| key | default | what it changes |
|---|---|---|
| `DISC_BATCH` | 27 | sectors per slot read -- `27 x 2352 = 63 504 B`, the largest single op this OS moves (docs/406); 1 = one 2 352-byte read at a time |
| `DISC_MIN_WANT` | 12 | how long the pump waits for a chunk to be due; 1 = read whatever the disc clock says is due |
| `DISC_PREFILL_FIRST` | 150 | sectors read before a track's voices start; 300 = the old whole-ring prefill |

`scripts/pocket/ab.sh` sets these variants up for you (`diag`, `bram-pump`, `audio-tick`, `player`, `show`) and prints
the grep to run after each boot.

## 5. Desktop verification (no hardware needed)

The desktop shim now takes the rig's key lines directly (docs/377 §5):

```sh
./pocket/src/night/app_pc --frames 3000 --script scripts/playtest/route_show_dense.txt --rig-dump /tmp/run
```

Only `key` lines are honoured -- the anchored `wait res` lines belong to the SDL host's reader (docs/55) -- and keys
reach the game on the direct key path, so a route file can drive the Pocket code path without a warp.

```sh
make test                                    # the fixture suite against src/f (repo root)
cd pocket/src/night && make test             # app_pc: the same src/f under the SDK's SDL2 shim
NIGHT_CD_DIR=scratch/pocket/cd ./app_pc --warp-stage 1 --frames 2000 --rig-dump /tmp/b --rig-witness 0 | grep '^budget'
FRAMES=4500 scripts/pocket/sweep_episodes.sh && python3 scripts/pocket/sweep_report.py /tmp/pocket_sweep
python3 scripts/pocket/diff_hosts.py /tmp/xpoc/s1 /tmp/xsdl/s1          # Pocket shim vs SDL host, same route
```

Analysis and evidence: `docs/330` (budgets and the mapping) through `docs/372` (the BRAM sets), `scripts/pocket/`
(repack, mk_dist, deploy, check_slots, check_interact, sim_menu, sweep_*, diff_hosts), `scripts/analysis/gain_scan.py`
(the audio measurements behind the gain curve).
