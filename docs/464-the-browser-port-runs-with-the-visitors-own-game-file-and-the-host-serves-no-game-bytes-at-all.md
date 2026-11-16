# 464 — The browser port runs with the visitor's own game file, and the host serves no game bytes at all

Asked of the browser port: **let people play it from a game-data file they downloaded themselves**, so the
host carries code and nothing else. No proxy, no server-side copy of the install set, no redirect that makes
our server the thing that handed out somebody's rip.

The port already knew how to read a disc over Range requests (docs/457) and how to answer the same reads out
of MP3 (docs/463). Neither is a distribution mechanism. What was missing was a second byte source — one that
begins on the visitor's disk — and a way to tell the disc layer that the sectors live there now.

## What archive.org will and will not let a page do

The obvious version needs no file picker at all: put the URL of a zip on archive.org in the page, fetch it
from the browser, and index it there. Measured on `archive.org` (2026-11-16):

| request | answer |
| --- | --- |
| `GET /metadata/<id>` | `200`, `Access-Control-Allow-Origin: *` — the item's own metadata is CORS-enabled |
| `GET /download/<id>/<file>` | `302` to a node, then `206 Partial Content` — and **no `Access-Control-Allow-Origin`** on either hop |
| `OPTIONS /download/...` | `405` |

So the metadata is readable and the bytes are not: a page cannot pull a file out of an item, not with Range
headers, not with any combination of them. What a visitor can do is download the file, which is exactly what
they were going to do anyway, and then hand it to the page. That is the whole design.

The preferred item is `eojjeonji-definitive-for-dosbox` (533 MB zip). Its `NIGHT.RES` is byte-identical to
the copy this port was verified against — CRC-32 `f578a854`, md5 `09383a0c6e6b144ca93de2cd14c4e38b`, 50,964,894
bytes out of 20,171,981 compressed. The other item that turns up first in searches,
`msdos_Eojjeonji_Joheun_Il-i_Saenggil_Geot_Gateun_Jeonyeok_1997`, is a different build: its `NIGHT.RES`
differs from byte 1657 and is 162 bytes shorter, and it carries no CD-DA at all. A port that read either one
blind would load one of them wrongly and never say so, which is why the reader checks the CRC the zip states
against the member it unpacked, and says out loud when a `NIGHT.RES` is not the one this port was verified
against.

## What a file may contain

The gate says this to the visitor, so it is also the specification:

```
NIGHT.RES — the game, silent
NIGHT.RES + TG_NIGHT.CUE / TG_NIGHT.IMG — with the music
NIGHT.RES + MUSIC.RES / SOUND.RES — the fan repack, with the music
```

The third line is the interesting one. In the fan repack `SOUND.RES` is not a resource file at all: it is 645
bytes of plain CUE sheet whose one `FILE` names `MUSIC.RES`, and `MUSIC.RES` is a raw 546,522,480-byte
audio image — the repack's author used `imgmount`, and named the sheet with a `.RES` suffix because that is
what the game's other files were called. A reader that looks for `.cue` finds nothing; a reader that reads
the first 645 bytes of `SOUND.RES` finds `FILE "MUSIC.RES" BINARY`.

`runtime/byod.js` is that reader. It scans backwards for the EOCD, walks the 173-member central directory,
inflates with `DecompressionStream('deflate-raw')`, and verifies each member against the CRC-32 the zip
states — for both paths it has, because the second one is not a memory operation:

```js
  const chain = m.method === 0 ? src : src.pipeThrough(new DecompressionStream('deflate-raw'));
```

A stored member is the zip file itself, sliced: no copy, and slices are random access, which is what the disc
reader needs. A deflated 546 MB member (the fan's `MUSIC.RES` is 500,612,187 compressed) cannot be seeked at
all, so it is unpacked **once**, in a stream, into a file in the Origin Private File System, with the CRC
accumulated chunk by chunk and checked at the end. The staging file is named
`night-music.res-36c386de.img` — the CRC of the member it came out of is in the name, so two different zips
can never share a staging file, and a stale one can never answer for a new one.

## The one new byte source: `nklocal:`

The disc layer in `src/cd/cd_iso.c` answers sector reads through two function pointers installed by the host:

```c
g_remote_pread(track, byte_off, out, 2352)   /* how many bytes this track is */
g_remote_size(track, *bytes)
```

`wasm/src/wasm_disc.c` builds those by turning a track number into a URL — `<base>/trackNN.bin` — and reading
the URL with Range windows. So the local source was not written in the disc layer at all. It is a URL scheme:

```c
if (!strncmp(url, "nklocal:", 8))   /* -> globalThis.__nkLocal.map.get(basename) */
```

`nkjs_start` and `nkjs_head_size` both recognise it and answer out of a `File`/`File System File Handle`
registered by `byod.js`, with the same token, poll and supersede discipline the network path uses. Everything
above that line is unchanged: the window service, the read-ahead, the game's LBA arithmetic, `cd_iso`'s
sector bounds. `--cd-url nklocal:cd` is the only thing the page changes.

## Where the fan image's audio actually is

The fan's cue sheet numbers its own image, and its numbers are not the disc's. Measured by correlating the
two images (both are Red Book sector streams; the comparison is `bytes` at a sliding offset):

```
audio begins 560 bytes (140 samples) before cue_sector * 2352          -- every track
tracks 08 and 09 are a further 4,455,768 bytes (25.26 s) later         -- their cue's own INDEX is wrong
track 02 at 408,688 ; track 03 at 7,824,544 ; track 15 at 544,565,056  -- anchors, after both corrections
```

So the fan cue is used for **one thing only**: finding the audio for each track inside `MUSIC.RES`. The game
itself is never told about it. It asks music in by the disc's LBAs — 37,262 for track 02, 268,621 for track
15 — which is what `fn_010c3e` compares and what the compiled-in table in `src/gen/disc_toc.h` (docs/455)
carries. Both halves of the arithmetic live in one function, `planTracks()`, and it is asserted against a
real repack outside the browser (`wasm/scripts/byod-test.js`):

```
ok    track 8 starts at 237820648  planned 237820648
ok    track 15 is 1956864 bytes    planned 1956864
ok    the corrections are applied only to the bytes they were measured against  crc=36c386de
```

The corrections are applied **only** to a `MUSIC.RES` whose CRC is `36c386de`, the copy they were measured
against. Any other `MUSIC.RES` is read as its own cue describes it, and the page says so.

Two honest caveats, which the requirement of this feature allows ("we can treat music as track based"): this
image is somebody else's rip, and on tracks 07, 09, 11, 13 and 15 the audio does not match this repo's rip of
the disc at all — different content in the same place, not an offset. And its track 09 runs 25 s into the
bytes its own layout gives to track 10, so a `planTracks()` result on this image reports an overlap; the
slices are still each within the file and each track still plays its own length.

## The sheet the port writes for itself

A fan cue describes the fan image. `cd_iso` with no cue parses the compiled-in geometry and — deliberately —
**refuses every read**, because a table with no byte source is the empty-drive case and `fn_010c3e` is
supposed to show `cdinsert.pcx` (docs/455). A browser host that has the sectors but not a sheet on disk is
not that case, and it does not want a second copy of the geometry written in JavaScript.

So the geometry renders itself back out. `cd_iso_toc_cue()` (in `src/cd/cd_iso.c`) turns `src/gen/disc_toc.h`
into the 669-byte CUE sheet it was generated from — image-relative LBAs, one `PREGAP 00:02:00` before the
first audio track — and `--cue-toc` (in `wasm/src/main_wasm.c`) writes it to the path the host named before
opening it. The fixture suite holds the claim:

```
cd_iso: the compiled-in geometry renders back the cue it was generated from
  [PASS] byte for byte the shipped cue
  [note] rendered 669 bytes, extracted/TG_NIGHT.CUE holds 669
  [PASS] and the port can open what it rendered
  [PASS] with the disc LBAs the game asks music in
```

Byte-for-byte is the right assertion, because a rendering of the geometry that drifts from the geometry would
be a music mismatch nobody can see.

## An empty `--cd-url`, which is not the same as no argument

The zip's shape decides whether there is any audio to answer. `NIGHT.RES` only, means no byte source at all.
The first version of this left `--cd-url data/cd` alone and sent the page asking the host for sectors the
visitor's file had replaced; worse, `NIGHT_CD_DIR` being set and answering nothing makes `cd_iso_open`
**fail**, on purpose — a rig that named a track directory must not be silently measured against the image or
against silence — so the run ended on the insert-CD screen instead of playing the game silent.

The contract is now spelled out: an empty `--cd-url` means "there is no byte source for the audio". The host
installs no remote source and names no directory, `cd_iso` opens the TOC with nothing behind it and refuses
the reads, and the log says which of the two happened:

```
disc: no audio byte source (--cd-url empty) -- the TOC answers, every read is refused
disc: /data/TG_NIGHT.CUE written from the compiled-in geometry (--cue-toc)
disc: /data/TG_NIGHT.CUE wrote 669, reads back 669 byte(s) [FILE "TG_NIGHT.IMG" BINARY
disc: no track bytes are answerable -- the game runs silent
```

## A host with no `data/` says so before it can be asked

Whether the press has anything to spend used to be a question the gate answered from a `HEAD` it had already
made. With the file path in the page, the same question decides what the gate *is*, so three bugs in it came
out of the woodwork — all three real, all three silent:

* A `HEAD` of a missing file answers `404` **with a body**, and the body has a `Content-Length`. Read as a
  resource size, it painted `0.0 MB of resources -- then no network while you play` on a host that has none,
  which is the difference between offering a press and offering a file.
* A `fetch` of a missing `tracks.tsv` answers `404` with an HTML page, and the line
  `Error code explanation: 404 - Nothing matches the given URI.` parses as **a track 404 of two bytes** — enough
  for the page to believe the host has music. Both are fixed by reading the status, and the track number is
  now required to be one or two digits as well.
* The probe was awaited too late. `?sink=1` starts a run without a press, and a start that asks before the
  answer arrives decides from `hostHasData`'s initial `true` and goes to fetch a resource pack that is not
  there. The probe now starts when the page's script runs and the run *awaits it* before deciding (`sizesReady`).

A code-only host then looks like this — no download sizes, no music box for music it cannot play, the press
waiting, and the file's shape written where a visitor will read it:

```
   this copy ships no game data -- pick your file below
   [ press to play ]        (disabled)
   [ use a game-data file I downloaded ]
   NIGHT.RES — the game, silent
   NIGHT.RES + TG_NIGHT.CUE / TG_NIGHT.IMG — with the music
   NIGHT.RES + MUSIC.RES / SOUND.RES — the fan repack, with the music
```

Choosing a file replaces the empty note with what the file turned out to supply
(`game data from dosbox.zip: resources and music -- nothing else is downloaded`) and arms the press. The
file input is a static `#file-byod` rather than one created on the click, for two reasons: a handle can be
kept for a later visit without inventing a place to put one, and a test can hand a file to it.

## What runs in the test loop

`make -C wasm check-byod` serves a dist with **no `data/` directory**, hands `build/byod.zip` (the real
`NIGHT.RES`, deflated, 20 MB) to the browser's file input over the DevTools protocol — `DOM.setFileInputFiles`
against a `Runtime` object id, because a `nodeId` is invalidated by the repaint the gate does while it reads
the zip — and asserts the ordinary machine checks plus four claims of its own: the file was read, its
`NIGHT.RES` is what got written to the in-memory filesystem, the host's log says so in words a reader sees
(`(from your file)`), and a file with no image asked for silence.

```
byod: dosbox.zip -- 1 members, NIGHT.RES 50964894, image -, cue -, host data none
byod: staged 50964894 resource bytes, 0 tracks (none, undefined B of sectors)
CHECK: PASS (1026 frames, 46.6 fps, 70 keys, 0 CD tracks played)
```

`node wasm/scripts/byod-test.js <zip>` runs `byod.js` under node against a real file — the whole 533 MB
archive.org zip in about a second for the index — and is what checks the fan arithmetic, since node has no
Origin Private File System and the 546 MB pass is the browser's half. It asserts `NIGHT.RES` twice: against
the CRC the zip states, and against the md5 of the extracted copy this port was verified with.

The browser half of the fan path was run by hand, because staging 546 MB is not a test you want in a loop:

```
byod: dosbox.zip -- 173 members, NIGHT.RES 50964894, image MUSIC.RES, cue SOUND.RES, host data none
byod: staged 50964894 resource bytes, 14 tracks (fan, 546113232 B of sectors)
pump: t=150213ms ... cdda=25060560B tracks=3 ... disc reads=10655 fetched=85/25479216B fails=0
      ... silence=0B ... holes=0
```

Three tracks of the fan image heard in 150 seconds of play, 25 MB of CD-DA consumed, zero bytes of it
requested from the server, and no hole in the ring. The one line the automated check objected to was the
host's own `cdda: fed nothing ... want=0`, which is the host correctly reporting that the game's plan asked
for nothing at a track's end — the harness's comment says that condition is exempt and the code now agrees.

## Left on purpose, and left open

* Only zip is read. The item that matters is a zip; a RAR or a 7z is the same work with a decoder nobody
  should trust from a browser without a reason.
* `NIGHT.RES` is checked against the CRC this port was verified against and *warned* about when it differs —
  loading it anyway is the only way a visitor with a legitimate different build finds out which one they have.
* The visitor's file is asked for again every visit. The `FileSystemFileHandle` for it can be stored in
  IndexedDB and re-granted with one click; nothing here does that yet, so the honest statement is that the
  staging file under OPFS is the cache and the file picker is not warm.
* The fan repack's audio differs from this repo's rip on five tracks (docs above). Nothing here pretends
  otherwise, and nothing here tries to repair it from the disc: the requirement was track-based music.
