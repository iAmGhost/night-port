# 83. DAT_53200 / DAT_532f4 as one contiguous DS block (the overrun closed)

Closes the item docs/78 filed but deliberately did not fix: *"`fn_03f6bc_impl` walks the timed-event
table with `cursor` incremented on every match and never bounded, and reads `DAT_53200[cursor]` past
the table's 61 dwords ... Fix shape (not applied): lay `DAT_53200` and `DAT_532f4` out in one
contiguous block covering `0x53200..0x53338` and index through it."*

## The shipped layout, from the image

| DS range | content | size |
|---|---|---|
| `0x53200 .. 0x532f3` | the timed-event tick table, 61 dwords (`229 .. 1086`) | 244 B |
| `0x532f4 .. 0x53337` | byte table of `DAT_53d0c` sound-slot indices | 68 B |
| `0x53338` | the sound/music enable byte (separate global, value 1 in the image) | 1 B |

`0x53200 + 61*4 == 0x532f4` and `0x532f4 + 68 == 0x53338`, so in the original there is **nothing
between the two tables**: once `fn_03f6bc`'s cursor passes the 61st tick entry, the code at
`0x3f795-a1` compares against the sound-slot *bytes* read as a dword, and (on a match) at
`0x3f7a9-c0` indexes the same bytes as the slot index. That aliasing is the program's actual
behaviour, not undefined behaviour.

The port modelled the two tables as two separate C globals, so the same read landed in
compiler-chosen padding. Measured before (docs/78, ASan):

```
ERROR: AddressSanitizer: global-buffer-overflow ... READ of size 4
  #0 fn_03f6bc_impl  f.c:13625     if ((uint32_t)(uint16_t)idx == g_tab_53200[cursor])
0x...ef4 is located 0 bytes after global variable 'g_tab_53200' (0x154 bytes)
   and 44 bytes before 'g_dat_53338'
```

## The change (`port/src/f/f.h`, `port/src/f/f.c`)

One global, laid out like the data segment:

```c
struct ds_53200_block {
    uint32_t tab_53200[0x3d];   /* 0x53200: timed-event tick table (image constant) */
    uint8_t  tab_532f4[0x44];   /* 0x532f4: DAT_53d0c sound-slot indices */
};                              /* 0x138 bytes: the block ends at DAT_00053338 exactly */
extern struct ds_53200_block g_ds_53200;
#define g_tab_53200 g_ds_53200.tab_53200
#define g_tab_532f4 g_ds_53200.tab_532f4
```

The struct needs no padding (member 2 has alignment 1 and member 1 ends at offset `0xf4`), so
`sizeof(struct ds_53200_block) == 0x138` and the byte table starts exactly `0xf4` in -- the shipped
offsets. All existing `g_tab_53200[]` / `g_tab_532f4[]` write and read sites keep working through the
member macros; the tick table's 61 image constants are unchanged.

The walk in `fn_03f6bc_impl` is now bounded **at the modelled span**, not at the tick table:

```c
if (cursor >= TAB532_WALK_LIMIT) {          /* 0x44: DAT_532f4+cursor would pass DAT_00053337 */
    g_tab532_walk_clamped++;                /* host witness, one stderr note */
} else if ((uint32_t)(uint16_t)idx == g_tab_53200[cursor]) { ... cursor++; }
```

`TAB532_WALK_LIMIT = 0x44` is the *byte* read's limit, which is reached before the dword read's
(`0x138/4 = 78`), so it is the binding one. This is the single deliberate deviation from the
original: past `0x53337` the original keeps reading DS bytes that the port models as *other* globals
(starting with `DAT_53338` itself), so the port refuses to invent them and counts the refusal instead.
It is a no-op for all shipped data, as measured below.

## Measurement

Host witness (`NIGHT_TAB532=1`, printed at exit): `max_cursor`, `clamped`, `limit`.

```
cd port
for s in route_attract_wait.txt route_walk_fight.txt; do for w in "" "--warp-stage 1"; do
  NIGHT_TAB532=1 SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    ./test/night $w --script ../scripts/playtest/$s --no-audio --frames 6000 --shot-limit 0
done; done
```

ASan build (`make -C port app CFLAGS="-std=c11 -O1 -g -w -fsanitize=address"`), logs in
`analysis/legacy/tab532/`:

| run | AddressSanitizer reports | `tab53200 walk:` |
|---|---|---|
| `route_attract_wait` (title/attract) | **0** | `max_cursor=61 clamped=0 limit=68` |
| `route_attract_wait --warp-stage 1` | **0** | `max_cursor=61 clamped=0 limit=68` |
| `route_walk_fight` | **0** | `max_cursor=61 clamped=0 limit=68` |
| `route_walk_fight --warp-stage 1` | **0** | `max_cursor=61 clamped=0 limit=68` |

Two things follow, both measured rather than assumed:

* **The aliasing read really happens.** `max_cursor = 61` in every run means all 61 tick entries were
  matched, so the loop's next comparison read the dword at `DAT_53200 + 61*4` -- i.e.
  `DAT_532f4[0..3]`. That is exactly the access ASan used to flag; it now reads the shipped bytes
  instead of padding, and reports nothing.
* **The clamp never engages** (`clamped = 0` in all four runs). The byte table has no producer inside
  the EXE, so those four bytes are 0, `idx` (a frame counter, never 0 there) never matches again, and
  `cursor` freezes at 61 long before the limit of 68. So bounding the walk changes no observable
  behaviour on shipped data; it only makes the modelled span explicit.

## Tests

`make -C port test` (4 new checks, all PASS):

```
[PASS]   0x53200 block: tick table ends at DAT_532f4, block ends at DAT_53338 exactly
[PASS]   the past-the-table dword IS DAT_532f4[0] (0: the byte table has no shipped producer)
[PASS]   seeding DAT_532f4 changes that dword: the aliasing is real, not padding
[PASS]   the walk stops at the byte table's 68 entries (the read that leaves the block first)
```

The third check is the one that distinguishes the fix from a layout accident: seeding
`tab_532f4[0..1] = 0x11,0x22` (through the byte member, then restoring) changes the dword read at
`+61*4` to `0x00002211`. With two separate globals that would have been padding.

`make -C port selftest` -> `ALL PASS (0 failures)`.

## Left open, with the reason

`fn_03f6bc` also feeds `g_tab_53d0c[g_tab_532f4[cursor]]`, a 100-entry pointer array. If the byte
table were ever seeded with an index >= 100, that read would leave its array (the original would
read past `0x53d0c + 400 = 0x53e9c` into further DS). It is **not** clamped here, because nothing in
the shipped image seeds the byte table (every byte is 0, so the index is always 0) and inventing a
guard for unreachable data would hide a real seeding bug if the CD backend ever fills the table.
Re-check if the byte table ever gains a producer.
