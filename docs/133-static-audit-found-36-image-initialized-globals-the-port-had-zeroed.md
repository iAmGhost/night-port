# 133 — Two new static checks found 36 globals the port had zeroed, and a real buffer overflow ASan then caught

## 1. c9: shipped access width vs the port's declared element width

`scripts/audit/ghidra_db.py::ds_access_widths()` reads operand sizes straight out of the Ghidra access
records (`MOV AL, byte ptr [0x63384]` ⇒ a byte lives at 0x63384). Ghidra only *defines* a data symbol where
it can, so 66 of the rig's 283 cells had no shipped span and `diff_state.py` compared them at the port's own
width — a comparison basis nobody had checked (docs/132 §3).

The first version counted base-relative operands too, which is wrong: `MOV EAX, dword ptr [EAX+0x53d08]`
touches **EAX+0x53d08**, not DS 0x53d08 (docs/113's sound-slot table is read exactly that way). Ghidra
records the displacement as the reference, so mixing the two produced 18 "width mismatch" findings of which
most were false. The widths are now split `direct` / `based` and only `direct` counts.

Coverage of the rig's comparison basis (`c9_width_coverage`):

```
292 port DS cells -> 217 shipped span from defined_data
                   ->  11 width only, from the access operand
                   ->  64 with no shipped width at all
```

c9 findings, classified by actionability (11 rows, 6 unique cells):

| kind | cells | verdict |
|---|---|---|
| `scalar-width-mismatch` | 0x537a7, 0x63338, 0x63380, 0x63384, 0x633a4 | **all fixed below** |
| `byte-array-with-dword-fields` | 0x53e9c, 0x5d08c, 0x5d0a0 | informational: byte arrays whose fields the guest moves as dwords; the port reaches them through dword views |
| `host-pointer-vs-guest-offset` | 0x53394, 0x53c5c, 0x53c84 | already filed by c4 (docs/94 family) |

## 2. c3b extended from brace initializers to scalars: 36 cells transcribed from the image

`c3b_full_span_initializers` (docs/122) only looked at globals with a `{…}` initializer. A DS-marked
**scalar** initialised to `0` — explicitly, or by BSS default — is the same claim ("this cell starts zero"),
and that is exactly where docs/132 §3's 28 "port holds 0 where the shipped side holds a value" cells live.
Extending c3b to scalars found 38 mismatching cells immediately; two parser blind spots had to be fixed
before it could be trusted:

- a header `extern uint32_t g_x;` was being read as the cell's initializer (it has none);
- some definitions in dense comment blocks are missed by `port_src._statements()`, so a cell looked
  uninitialised when a definition with an initializer exists in the text.

Both fixed: a name is judged by its *initialised* declaration, with a text scan for a non-`extern`
definition as a fallback.

Transcribed from `NIGHT.EXE` at `DS-0x7600` (36 rows, 21 distinct globals):

```
g_dat_51a10 0x1     g_dat_51c98 0x1     g_dat_52fb4 0x3     g_dat_52ff8 0x1
g_dat_530ac 0x320   g_dat_530b4 0x64    g_dat_530b8 0x64    g_dat_53100 0x3
g_dat_531f4 0x10000 g_dat_531f8 0xffff  g_dat_53338 0x1     g_dat_53368 0x20
g_dat_53619 0x100   g_dat_5363c 0x1     g_dat_53640 0x10    g_dat_536dc 0x14
g_dat_5377c 0x4650  g_dat_53780 0xe10   g_dat_53784 0x1     g_dat_537ca_target 0xa7
g_rng_state 0x1
```

After the transcriptions: **c3b real byte mismatches 0**, and the independent check `c3_image_init`
(defined-symbol cells only) dropped from 27 findings to 12 — two checks agreeing on the same family.

## 3. The TZ cluster, end to end

The shipped image at `DS 0x5377c..0x537c5` is *not* zeros:

```
5377c  50 46 00 00 10 0e 00 00 01 00 00 00 45 53 54 00   ..P.......E S T .
5378c  00 ... 00 45   537a8  44 54 00 ...   ->  "EST" at 0x53788, "EDT" at 0x537a7
537b8  00 ... 88 37 00 00  a7 37 00 00  11 00 00 00
```

and the shipped runtime dumps (docs/132's g6 run) show precisely those values live:

| DS | shipped (measured) | port, before this change |
|---|---|---|
| 0x5377c TZ seconds | `0x4650` (18000 = 5 h) | `0` |
| 0x53780 TZ range diff | `0xe10` (3600 = 1 h) | `0` |
| 0x53784 has-second-part | `1` | `0` |
| 0x53788 / 0x537a7 prefix buffers | `"EST"` / `"EDT"` | all zero |

So the port had no timezone configuration at all — a shipped image-initialized block modelled as zeros.

Adjacent open item, same family: `g_dat_537d4` (the `key=value` config table read by `config_value_ptr`)
is `0x19d07c` — a live heap pointer — in the shipped dumps and `NULL` in the port, with **no write site at
all** in `f.c`. The guest builds it once at guest `0x4aab9` (`mov dword ptr [0x537d4], eax` after allocating
at 0x41af8 and copying a NUL-delimited string block through `es:[eax]` with `es = fs`, i.e. the DOS
environment). Until the port models that, every config lookup in the port is dead.

## 4. What the width finding turned out to be: a global buffer overflow

`parse_time_str` (guest 0x49a6e) copies `min(len, 0x1e)` characters and a NUL into the buffer address it is
handed — up to **31 bytes**. The port declared the two TZ buffers as `uint32_t` and passed `&g_dat_53788`:

```
==86099==ERROR: AddressSanitizer: global-buffer-overflow on address 0x0001006a23c4
WRITE of size 5 ...
    #1 parse_time_str f.c:11103
    #2 fn_049c76 f.c:6808
0x...23c4 is located 0 bytes after global variable 'g_dat_53788' ... of size 4
0x...23c4 is located 28 bytes before global variable 'g_dat_537a7' ... of size 4
SUMMARY: AddressSanitizer: global-buffer-overflow: parse_time_str
```

The shipped layout makes those writes legal: `0x53788 + 0x1f = 0x537a7`, and the second buffer ends at
0x537c5, four bytes before `DAT_000537ca`. Fix: both buffers declared `uint8_t [0x1f]`, which reproduces the
shipped adjacency exactly, plus `f.h` updated and new `test_f1` cases asserting that a 26-char prefix
(`"NIGHT-TIMEZONE-CONFIG-XX:12:34:56"` → `buf1="NIGHT"`, `buf2="TIMEZONE"`) and a 30/31-char prefix
(the 0x49aae clamp fills 30 chars + NUL exactly) behave. Same driver, fixed tree: no ASan report.
`make -C port test` and `make -C port selftest`: ALL PASS.

The guest side, disassembled from the EXE (`fn_049c76`, docs/133 §5), confirms the branch semantics the port
models — `mov ah, byte ptr [eax]; test ah, ah; jne …; mov byte ptr [0x37a7], ah` (the byte store only in the
"nothing left to parse" arm), `mov edx, 0x3788` / `mov edx, 0x37a7` as the two buffer addresses.

Width fixes from c9 also applied: `g_dat_63338` → `uint16_t` (guest word write at 0x34de9, word read at
0x34edd), `g_dat_63380`/`g_dat_63384` → `uint8_t` (the only direct access is `MOV byte ptr …`),
`g_dat_633a4` → `uint32_t` (guest stores a dword 1 at 0x000391cc).

## 5. New tool: `scripts/audit/disasm.py` — disassemble shipped code without Ghidra

Two file-offset bases are in play, and only one had been recorded:

```
data: file offset = DS address - 0x7600        (already used by c3/c3b)
code: file offset = flat address - 0x2600      (measured here)
```

The code base was pinned by locating a known instruction: Ghidra reports `MOV byte ptr [0x000537a7], AH` at
guest 0x49c9b, and `88 25 a7 37 00 00` sits at file offset 0x4769b → 0x49c9b − 0x4769b = 0x2600.
`--verify` re-checks that and reports that 47 of the first 80 exported function bodies end with
`ret`/`int3` at that base (the rest end mid-instruction, so treat body_end as approximate). Operand
displacements in the code are 16-bit (`[0x3784]` means DS 0x53784); the script annotates them.

## 6. State-oracle vocabulary (docs/132 §4(b))

`diff_state.py` no longer calls everything DIFF: `DIFF` now means same shipped width, same compared width,
different value; weak-basis divergences are `DIFF-WIDTH` (spans disagree) or `DIFF-SIZE` (no shipped span),
and the run prints `divergent cells: N of which same-width value divergences: M`.

## 7. Open items filed here

1. Model the config-table build (guest 0x4aa49-0x4ab14) so `config_value_ptr` is live; that is the reason
   the whole TZ cluster was dead in the port.
2. Re-measure the state-oracle DIFF set on a fresh route now that 36 cells have their shipped initial
   values (`/tmp/rig/g8` run started against the fixed build; docs/132's g2/g6 baseline was 37 DIFF cells).
3. Adjudicate the 12 `c3_image_init` rows that remain after the transcriptions.
4. The 64 port cells with no shipped width at all still need Ghidra spans or access-derived widths before
   their verdicts carry weight.
