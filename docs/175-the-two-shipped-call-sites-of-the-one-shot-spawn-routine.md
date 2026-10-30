# 175 — The one-shot spawn routine has exactly two shipped call sites, and the port guards both

docs/174 §4 pointed at `fn_011827` (allocate a fresh 0x14f-block entity, `+0x30 = 1`, `+0x17 = <stack arg>`) as one
of the two writers of the dying flag and as the producer of the extra `type=0x02` records.  Before changing
anything in it, the shipped call graph was enumerated.

## 1. Shipped: two call sites, both inside the entity dispatcher `fn_01194c`

```
callers of 0x11827: 2   (call at 0x11998 and call at 0x119ca, both inside fn_01194c, 0x1194c..0x12615)

0x11975  mov  eax, [ebp-4]
0x11978  cmp  dword [eax + 0x11], 0x12        ; action counter == 0x12
0x1197c  je   0x11987
0x1197e  mov  eax, [ebp-4]
0x11981  cmp  dword [eax + 0x11], 0xb         ; ... or == 0x0b
0x11985  jne  0x1199d
0x11987  push 1 ; mov ecx,1 ; xor ebx,ebx ; mov edx,9 ; mov eax,[ebp-4]
0x11998  call 0x11827

0x119a2  mov  eax, [ebp-4]
0x119a5  cmp  eax, [0x53c7c]                  ; entity == the player pointer
0x119ab  jne  0x119e7
```

## 2. The port reproduces both guards

```c
if (ac == 0x12 || ac == 0x0b) fn_011827(e, 9, 0, 1, 1);                      /* f.c:25904  <- 0x11978/0x11981 */
...
if (st == 0x8c && is_player && ac == 0x3f) fn_011827(e, 0x20, -4, 1, 4);     /* f.c:26004 */
else if (st == 0x8d && is_player && ac == 0x38) fn_011827(e, 0x20, -4, 1, 4);/* f.c:26006 */
```

So the extra one-shot records are **not** explained by an unguarded call: both shipped gates exist in the port with
the same conditions (`+0x11` counter values, and the player-identity test against `DS 0x53c7c`).

## 3. Where the extra spawns must therefore come from

`fn_011827` is also reached through the action-record macros in the same dispatcher -- `MV` is literally
`fn_011827(e, -10, -4, 1, 4)` (`f.c:25895`) and `RYMV` is `fn_01545e(e, prng_next() % 0x15 - 10)` followed by
`fn_011827(e, -10, -4, 1, 4)`.  Those arms are driven by the **per-frame action record**, not by a fixed call
graph: how many times a spawn happens is a property of the record stream, exactly the open question docs/171
left ("the `d == 0` rate is expected behaviour of action records; the open question is whether the port's action
records *are* the shipped records").

Named test, and it is the same test docs/171 §3 asked for: compare the loaded action-record stream byte-level for
the record that produced the extra records (`type = 0x02`, docs/156's `NIGHT_ACTCENSUS` containers plus the
`actdump` drive verb), rather than counting spawn calls.  Spawn counts are an *output* of the record stream; the
rig has never compared the record streams themselves, and both docs/171 and docs/175 now converge on that gap.

## 4. First run of the record-stream capture, and why it came back empty

`scripts/rig/drives/ep1_actall.txt` is the drive built for exactly this comparison: 278 lines that `actdump`
types 0..N at `t = 72 s` each.  Running it (`--frames 8000`, calibrated cadence) captured nothing:

```
script: actdump type 0 -> 0 bytes at t=45450ms
script: actdump type 1 -> 0 bytes at t=45450ms
script: actdump type 2 -> 0 bytes at t=45450ms
```

`actdump` captures containers that are *currently loaded*, and docs/156 §2 established that the loader fills the
per-type slots at guest `0x37da9` and clears them at `0x373d3` -- so a container is only dumpable inside the
window where its episode has loaded it, and this run reached the dump verbs at 45.45 s, before the 72 s anchors the
drive schedules, with nothing loaded.

Consequence for the named test: the record-stream comparison has to ride the **census recorder**, not the drive
verb.  `NIGHT_ACTCENSUS` records each container at the load site (docs/156's `g_act_load[]` recorder, which is how
the 5-35 containers-per-episode coverage numbers were produced), so the byte-level capture should hang off that
site -- dump on load, per episode, instead of asking for a type at a wall-clock moment.  That is the concrete fix
to the capture path before any record-stream equivalence claim can be made.

## 5. The capture path fixed: dump on load, and it captures the attacker type

`act_census_dump()` now hangs off the census recorder (`NIGHT_ACTCENSUS_DUMP=<dir>`), and on `ep1_kill3`
12 000 frames it wrote 9 containers while the same run's census reported
`blocks=21 records=3159 reaction-band=1881 FALLEN(slots 0-5)=1`:

```
type001_0018f788.bin  type011_00356b80.bin  type031_0034b744.bin  type032_00241d68.bin
type036_0033bbc4.bin  type037_0038fb68.bin  type041_00350fc8.bin  type042_0037fd50.bin
type046_003a26e4.bin                        (8192 B each)
```

`type036` is the attacker family docs/174 §3 named (`type=36 st=0x29 slot=0 atype=11`), so the record that produced
the code-11 hit on the port's player is now captured as bytes.

One negative result that shapes the comparison: the first 64 bytes of every dumped container are **not found** in
`NIGHT/NIGHT.RES`.  The assets in `NIGHT.RES` are packed and the loader expands them into the arena, so a raw
byte-for-byte compare against the resource file is the wrong test -- the comparison has to go through the decoder
(`scripts/extract_res.py` / `scripts/decode_act.py`, which is byte-exact against the C `act_decode` per the
selftest) rather than against the container file.  Recorded before anyone spends a session proving an equality
that the packing makes meaningless.

## 6. The hit code 11 *is* in the shipped data -- for exactly that attacker

`NIGHT_ACTCENSUS_SLOTS="type:state"` prints the hit-slot codes of one record through the same lookup
`fn_01441e` uses (`atype = hb[rec + slot + 0x38]` via `lvl2 = *(block+4)`, `rec = *(lvl2 + 4*state)`).  On
`ep1_kill3` (12 000 frames, 21 loaded blocks):

```
ACTSLOTS type=36 state=0x29 rec=0x33f9c0 codes: 11 0 0 0 0 0 7 0      <- slot 0 carries code 11
ACTSLOTS type=36 state=0x19 rec=0x33e490 codes: 0 0 0 0 0 0 0 0
ACTSLOTS type=36 state=0x00 rec=0x33bca0 codes: 0 0 0 0 0 0 0 0
```

docs/174 §3 asked whether the shipped record for the attacker that produced the code-11 hit on the port's player
(`type=36 st=0x29 slot=0 atype=11 dmg=8`, docs/162 §12) carries code 11 at all.  It does: **slot 0 of that record
is 11**, and the neighbouring states of the same type carry no hit codes.  The containers are the shipped assets
loaded by the port's loader, so this is shipped data, not a port transcription.

Consequence, and it narrows the cluster further: the divergence is **not** in the record decode and **not** in the
hit table.  Code 11 exists in the data on both sides; what differs is that the port *owns a type-36 attacker in
state 0x29 adjacent to the player* while docs/155 showed shipped never applies 11 to the player.  So the open
question is now a single one: **why does the port field type-36 attackers that the shipped run does not field (or
never brings to the player)?**  That is the spawn/population item, narrowed from "extra entities" to one named
family, and it is the same family whose one-shot spawn records docs/174 §4 counted.

Tooling added: `NIGHT_ACTCENSUS_SLOTS="type:state"` witness printing a record's eight hit codes, which turns any
"this hit should not exist" claim into a byte-level lookup against the loaded shipped record.
