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
