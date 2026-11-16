# 465 — Neither host needs the extracted directory anymore: the disc itself carries the game, and a cue sheet can be named anything

`docs/455` built `extracted/` and made both hosts read from it: the install set — `NIGHT.RES`,
`TG_NIGHT.CUE`, `cd/trackNN.bin` — produced by `scripts/night-extractor` before the program ever ran.
That directory is what a visitor does not have, and a directory of the visitor's own files is not obliged to
spell `TG_NIGHT.CUE`. So the reading that `scripts/night-extractor` did moved into the port: the native host
finds `NIGHT.RES` inside the disc image at open time (`src/cd/cd_res.c`), and the browser host does the same
job in JS (`wasm/runtime/byod.js`), which also stopped caring what anything is called.

A boot with no extracted directory, from the rip alone:

```sh
mkdir -p build/disc-only && ln -s ../../TG_NIGHT.CUE build/disc-only/ && ln -s ../../TG_NIGHT.IMG build/disc-only/
./build/night --res-dir build/disc-only --frames 400 --no-audio
```

```
disc: 15 tracks, 269453 sectors, lead-out MSF-derived, audio from build/disc-only/TG_NIGHT.IMG
res: no night.res in build/disc-only -- reading it off the disc: NIGHT.DAT@11780 +579613 (night.res, 50964894 B of 6 entries)
frames presented: 400
```

Four hundred frames, and CD-DA read from the same 633 MB file (`cdda=576240B tracks=1` in the pump line),
with nothing in that directory but a cue sheet and an image.

## What the disc actually carries

The disc does not contain `NIGHT.RES`. It contains the installer: one ISO9660 file, `NIGHT/NIGHT.DAT`, which
is the payload `SETUP.EXE` installed from, measured against the rip at the project root:

| | |
|---|---|
| `NIGHT.DAT` | volume LBA 11780, 51,563,830 bytes |
| its container's index | 6 entries: `dos4gw.exe`, `night.exe`, `night.ico`, `night.res`, `night.bat`, `readme.txt` |
| `night.res` inside it | offset 579,613, 50,964,894 bytes |

The index is obfuscated, which is why a hex editor finds nothing: the 20-byte-per-entry block is a reverse
XOR chain — `b[i] ^= b[i-1]` walking backwards from the end, then `b[0] ^= 0x20` — the same transform the
game's own loader `fn_0366a4` applies (`docs/02`). `docs/02` is the format note; the numbers above come from
reading them here, in C, and from `scripts/night-extractor` agreeing with them to the byte.

Two geometry facts carry the rest of the work:

* A MODE1/2352 frame carries 2,048 bytes of user data behind a 16-byte header, so the file's byte *N* lives
  at `(first + lba + N/2048)*2352 + 16 + N%2048`. Mapping file bytes linearly onto the raw image — the
  mistake the first version of `scripts/extract_disc.py` made (`docs/455`) — yields a file of exactly the
  right size and wrong bytes, 16 bytes early per sector travelled. Everything in both readers hangs off
  crossing that boundary in exactly one place: `cd_res_pread` for the port, `isoStream` for the page.
* The root directory record is not at a fixed offset. In this image the `CD001` signature sits at byte 1 of
  sector 16, one byte early, so the root is found by *shape*: a record whose little-endian and big-endian LBA
  and size words agree, and whose name length matches its own record length. Sectors 16 and 17 are both
  checked; the primary volume descriptor is not parsed at all.

## The native seam

`src/cd/cd_res.c` (~200 lines) answers one question: where are the bytes of `NIGHT.RES`. It opens the
volume, walks the directory breadth-first and case-insensitively to `NIGHT/NIGHT.DAT`, de-obfuscates the
container index, and exposes `cd_res_pread(offset, dst, n)`, which maps an offset in `NIGHT.RES` onto sectors
through `cd_iso_read_sector()`. The fixture suite reads the whole 50,964,894 bytes that way and hashes them:

```
cd_res: the installer's payload off the disc, when NIGHT.RES is not on disk
  [PASS] opening the rip's cue opens its image too -- the frames to look in
  [PASS] NIGHT.RES's slot found inside the payload
  [note] NIGHT.DAT@11780 +579613 (night.res, 50964894 B of 6 entries)
  [note] 50964894 bytes read, crc f578a854
  [PASS] gathered sector by sector it is the NIGHT.RES this port was verified against
  [note] disc f578a854, extracted f578a854
  [PASS] and identical to the copy scripts/night-extractor wrote
```

The second note is the assertion that matters: the same CRC-32 over the bytes the port read off the image and
over the file `scripts/night-extractor` wrote. A size that matches and a hash that does not is the failure
mode this feature invites.

The seam the game crosses is `sys_file_open` (`src/sys/sys.c`), which already resolves the guest's own
spelling — `fn_03690b` opens `"night.res"`, lower case, on the install directory — against a real directory,
case-insensitively. When that directory has no such file *and* a disc image is open, the open is answered
from the payload instead, and the handle the game receives is an ordinary one: `sys_file_read`,
`sys_file_lseek` and `sys_file_close` carry a disc-backed handle exactly as they carry a `FILE *`. Nothing
above the seam knows a disc was involved. Two guards keep it narrow:

* only that one name is answered — the payload holds `night.exe` too, and the port does not need it, so an
  open for anything else still returns the DOS not-found;
* an extracted install set never reaches the fallback, because the file it looks for is there.

`sys_file_disc_opened()` reports how many opens were answered off the disc, which is what makes the log line
above honest, and `sys_file_real_dir()` resets the disc-backed handles along with the real ones.

## A cue sheet may be called anything

`--res-dir` used to imply `<dir>/TG_NIGHT.CUE`. Now the disc is chosen in the order someone would ask for
it:

1. `--cue FILE`, named outright;
2. `<res-dir>/TG_NIGHT.CUE`, if it is there (the shape `make extract` builds, unchanged);
3. any `*.cue` in `<res-dir>` — the rip's own name first, after that a sheet whose image is actually present
   beats one whose image is not;
4. failing that, any `*.cue` in the current directory;
5. and failing that, nothing: the game boots on the compiled-in table of contents and the music is absent
   (`docs/455`), which is a run with no music, not an error.

The image needs no name rule at all, because a cue sheet states its image itself in its `FILE` line and
`cd_iso.c` resolves that name against the cue's own directory. A directory spelled differently works for the
same reason:

```sh
sed 's/TG_NIGHT.IMG/My Night Disc.bin/' TG_NIGHT.CUE > "build/renamed/My Night Disc.cue"
ln -s ../../TG_NIGHT.IMG "build/renamed/My Night Disc.bin"
./build/night --res-dir "build/renamed" --frames 250 --no-audio
```

```
disc: 15 tracks, 269453 sectors, lead-out MSF-derived, audio from build/renamed/My Night Disc.bin
res: no night.res in build/renamed -- reading it off the disc: NIGHT.DAT@11780 +579613 (night.res, 50964894 B of 6 entries)
frames presented: 250
```

## The same reader in the page, and a container that is not a zip

`docs/464` gave the browser a zip and read `NIGHT.RES` from a member. Three things were wrong with that for
a visitor who has the rip: the file has no `NIGHT.RES` member to read; the pair is usually two files, not one
archive; and the names inside are nobody's but the repacker's.

**A container is a zip or a set of files.** The `<input type=file>` takes `multiple`, so `TG_NIGHT.CUE` and
`TG_NIGHT.IMG` can be picked together, and a member and a picked file are now the same shape: each carries
the byte source to slice and the offset where its bytes begin (`0` for a picked file, past the local header
for a zip member). A picked file states no CRC, so the zip's CRC-32 check is skipped there and the container
header is checked instead.

**The cue names its image, and that is the authority.** `.cue`, `.img`, `.bin`, `.iso` are the patterns;
`SOUND.RES` and `MUSIC.RES` are aliases in the rule rather than the rule; `TG_NIGHT.CUE`/`TG_NIGHT.IMG` are
only preferred ties. What decides is the cue's own `FILE "…"` line, matched against what is present:

```
byod: handed the page TG_NIGHT.CUE + TG_NIGHT.IMG
byod: TG_NIGHT.CUE +1 -- 2 files, NIGHT.RES 50964894 (inside the disc image), image TG_NIGHT.IMG, cue TG_NIGHT.CUE host data none
byod: staged 50964894 resource bytes, 14 tracks (disc, 546113232 B of sectors)
CHECK: PASS (1038 frames, 47.2 fps, 70 keys, 2 CD tracks played)
```

**Fan or disc is settled by the bytes, because the cue sheet says no such thing.** `docs/464` told the fan
repack's image from the disc's by its name, `MUSIC.RES`. That name is now just a tie-break, and the cue's
shape does not help either: the fan's `SOUND.RES` copies the disc's layout line for line, including
`TRACK 01 MODE1/2352` / `INDEX 01 00:00:00`, which is a data track the fan image does not contain. The
honest test is the one this section is about — look for the ISO9660 volume where the data track is supposed
to be. The disc's image has it; a fan rip's begins at the disc's track 02, so its first frame is a frame of
music and nothing there can be a volume header. That single probe answers both questions at once: whether the
image carries the installer's payload, and which geometry to slice it by. The offsets measured for the fan
image (`docs/464`) still apply only to the bytes they were measured against — the CRC guard is unchanged.

**The gate stops offering the music box** while a file is in play. The box (`docs/463`) is a bandwidth
choice about *hosted* bytes -- unchecked, the run asks the server for none -- and a run that reads its
soundtrack from the visitor's disk has no such choice to make. Worse than a useless control, it would have
been an invisible one: a tick from last visit would still have muted the run. So the file answers the only
question left, which is whether it holds an image; silence means picking a `NIGHT.RES` alone, and `?music=0`
remains the lever for a run that wants it. `make check` asserts the box is offered on the hosted path, and
`make -C wasm check-byod` asserts it is not offered with a file.

**A bare image, with no cue at all,** is offered the port's own table of contents (`discCue()`, the JS twin
of `cd_iso_toc_cue()` from `docs/464`) — but only for an image within 2% of this disc's 269,303 sectors,
because guessing an image's geometry is worse than asking for its cue.

## The gate is Korean, and it names the game

The first screen used to say `NIGHT` and `WebAssembly port`, which is the port's name and not the game's.
It now says **어쩐지 좋은 일이 생길 것 같은 저녁** — the title it was sold under, TG Entertainment 1997
(`eojjeonji`, the Internet Archive's romanisation, is that title read into Latin letters). Under it: one
line about what the port is, the year and developer, two short buttons (`시작`, `데이터 파일 선택`), and a
third shape that is a link (`데이터 내려받기`) to
[archive.org/details/eojjeonji-definitive-for-dosbox](https://archive.org/details/eojjeonji-definitive-for-dosbox) —
the item whose `NIGHT.RES` the fixture already treats as the preferred one (docs/464). The shapes the file
input takes are listed under the buttons, and the third of them is called **비공식 배포판 zip** rather than
a repack, because it is one: fan-made, and the reason the page reads a disc image *or* a `MUSIC.RES` rather
than assuming which one the visitor happened to own.

Two things stay English on purpose. The log panel and the HUD are the harness's contract — `make check` and
`make -C wasm check-byod` assert against lines like `data file: … -- from your file`, `CD tracks`, `frames` —
so translating them would be translating the tests' eyes; and the game's own screens are what the original
drew. What was translated is the screen before the game, the one screen this port writes itself. The control
line keeps to one line at 10px (`white-space: nowrap`), and hands the wrap back below 460px, where clipped
text would be worse than two lines.

The shape list also knows how much attention it deserves: one line naming the three shapes where the host
supplies the data and a file is only an alternative, the full list where a file is the only way in and its
shapes are the difference between a game and a 404.

## What the runs cost

`scripts/byod-test.js` drives the page's reader outside a browser, over real bytes, four shapes:

```sh
node wasm/scripts/byod-test.js build/byod.zip            # NIGHT.RES alone: the game, silent
node wasm/scripts/byod-test.js /tmp/nkzip/orig.zip       # a zip of TG_NIGHT.CUE + TG_NIGHT.IMG
node wasm/scripts/byod-test.js TG_NIGHT.CUE TG_NIGHT.IMG # the same two files, picked, no zip
```

plus two cases it builds itself — the same pair renamed to `Eojjeonji Disc.cue`/`Eojjeonji Disc.bin`, and the
bare image alone — all of which stage `NIGHT.RES` at `md5 09383a0c6e6b144ca93de2cd14c4e38b`, the file this
port was verified against, and plan 14 audio tracks with no two tracks claiming the same bytes.

In the browser, the three shapes the page can be handed:

```sh
make -C wasm check-byod                                    # NIGHT.RES only: 0 tracks, silent, no game byte from the host
make -C wasm check-byod Z=/tmp/nkzip/orig.zip              # the rip in a zip: 14 tracks, 2 CD tracks played
make -C wasm check-byod Z="../TG_NIGHT.CUE ../TG_NIGHT.IMG"  # the rip picked: same numbers
```

`make -C wasm check` still passes on the host-stages-everything path; `make test` passes including the
`cd_res:` fixture.

## Three bugs, all of one family

**A reader that returns the right number of bytes from the wrong place.** `cd_res_pread` was written taking
an offset in `NIGHT.RES`, and forgot to add the entry's offset within the payload file — so every read,
including the first, returned the payload's container header. The size assertions passed and the game would
have loaded a header as a resource table. The fixture caught it by comparing the CRC against the extracted
file rather than against a number typed into the test.

**The same bug with an argument named after it.** `isoStream(src, base, first, lba)` was written to base a
file's user data at sector `first + lba`, and `user()` used `first + sec` — the `lba` argument was never
read. `findResInImage` then "fixed" the problem by opening a second stream at the payload file's extent,
which did nothing at all: both streams read the volume's first sector. Two independent readers, the same
class of mistake, one line apart.

**A CRC that is not the zip's CRC.** The first hash of a byte-for-byte-correct `NIGHT.RES` came out
`1a99f4b1`. CRC-32 as the zip tools define it starts at `0xffffffff` and inverts the result; a routine that
starts at 0 and does not invert computes a different function over identical bytes. `f578a854` is what the
zip's central directory states for this file, and both the C fixture and the JS test now assert against the
extracted copy as well, so a wrong hash cannot agree with a wrong reference.

## Commands

```sh
make test                                     # cd_res: the installer's payload off the disc
./build/night --res-dir build/disc-only --frames 400 --no-audio   # the rip alone
./build/night --res-dir "build/renamed" --frames 250 --no-audio   # and under other names
./build/night --cue TG_NIGHT.CUE --res-dir build/renamed --frames 120 --no-audio
node wasm/scripts/byod-test.js TG_NIGHT.CUE TG_NIGHT.IMG
make -C wasm check-byod Z=/tmp/nkzip/orig.zip
```
