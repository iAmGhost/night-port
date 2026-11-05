# 339 — audio tracks were sitting in the OS save window, which is why intro and stage music never played

Core 0.1.22.  One line of the card log explains every music symptom since docs/337:

```
cdda req idx=1 lba=37262 sectors=3153 (42.0 s) -> disc track 2  slot 18
disc: slot 18 sector 111 read 1072 of 2352 bytes
cdda: partial prefill (111 of 300 sectors) -- starting with what arrived, pump will top up
cdda req idx=9  lba=185120 sectors=27697 (369.3 s) -> disc track 10 slot 26
cdda req idx=11 lba=223146 sectors=17856 (238.1 s) -> disc track 12 slot 28
cdda req idx=2  lba=40415  sectors=19639 (261.9 s) -> disc track 3  slot 19
disc: slot 19 sector 111 read 1072 of 2352 bytes
```

`111 * 2352 + 1072 = 262 144 = 0x40000 = 256 KiB`, on both failures, at the same byte.  Slots 26 and 28 streamed
700 KB+ without complaint.  The difference between the two groups is the slot **id**, not the file:

> | 10-19 | Save 0-9 | Nonvolatile CRAM0 save slots (256 KB each) |  — openfpgaSDK README:644
> "10 persistent save slots (256 KB each), mapped to APF file IDs 10-19 … Save slots use bridge address
> `0x20100000` (CRAM0 save window) with 256 KB stride"  — README:462

Our 14 audio tracks were on ids 18…31.  So **track02.bin sat on Save 8 and track03.bin on Save 9** and the OS
answered reads for them from the 256 KB save window: every read past byte 262 144 comes back short.  track02 is
the intro theme and track03 the stage theme — precisely "intro video music and stage music do not play", while
the title (track10, slot 26) and the episode slides (track12, slot 28) played.  The buzz that came back in
0.1.20 was the same wall from the other side: the prefill stopped at 111 sectors, voices started anyway, and the
loop ran through the rest of a 4 s ring of nothing.

Nothing about the files was wrong: `md5` and sizes on the card match `scratch/pocket/cd` for all 14.

## The mapping now

`dist/night/slot_map.tsv` (generated) is the single source; `of_disc.c: of_disc_track_slot()` is its runtime
copy and `deploy.sh` labels its copies from the TSV rather than doing arithmetic:

| id | bound to |
|----|----------|
| 4 | NIGHT.RES (Data 1) |
| 5, 6 | track02.bin, track03.bin (Data 2, Data 3) |
| 10-16 | night_0..6.sav (Save 0-6) |
| 17 | nightlog.sav (Save 7) |
| 20-31 | track04..track15.bin |

28 data slots, 26 instance bindings, no id in 8 or 10-19 carrying anything but a save, max id 31.  Ids 18 and 19
are now simply not declared — they are Save 8 and Save 9, and if a later build wants two more saves it may.

## What openfpgaOS-Quake does, since it is the other shipping CDDA core

- Its CD tracks are `CD Track 03..11` on ids **23-31**, with `Data 5-7` on 20-22: nothing audio near the save range.
- It opens a track by **directory entry first**: find the `readdir` entry whose slot id matches, `fopen(e->d_name)`,
  and only falls back to `fopen("slot:N")` "in case the slot mapping differs" (`src/quake/engine/cd_of.c:151-235`).
- It refills with **`of_file_read_async()` — data-slot DMA** into a CRAM0 staging buffer, 16 384 frames per
  read, completion via IRQ callback, because the synchronous path costs "~13 % of all CPU just to stream
  176 KB/s" (`cd_of.c:351-357`), and it sizes the chunk by `of_file_async_max_read()` at runtime.

Both are open for this port: the filename-first open is a one-function change, and async slot reads are the
right answer to what our own bench measures (a 4 s prefill is 705 KB of polled reads, and the read wall above
shows the synchronous path is also the fragile one).
