# 187 — Item 2b's 54 is a queue value, and the cell the port's model reads is zero on both sides

Continuing docs/186.  The census (docs/186 §8-§9 style, run `g33`, `ep1_kill3` 150 s, port 10 Hz / 1 500 samples,
shipped 4 Hz / 413 states) grouped admissible entity records by `(type, rec[+0x0d], rec[+0x32])`:

```
shipped: (0, 0, 54) x325   (0, 0, 5) x246   (0, 0, 0) x138   (0, 0, 53) x2
port   : (0, 0, 0) x927    (0, 0, 5) x174
```

So the divergence is a *value population*: shipped produces 54 (and once 53) and the port never does.

## 1. The path the port models cannot produce 54 — its source cell is zero on both sides

The port's model of the `fn_01441e` death path (`f.c:25246-25261`) reads

```c
((struct ua32 *)((cur + 0x32)))->v =
    ((struct ua32 *)((g_dat_53ea8 + (size_t)((struct sa32 *)((cur + 0x0d)))->v * 0x88u)))->v;
```

with `g_dat_53ea8 = g_dat_53e9c + 0xc` (f.c:8115, the 0x88-stride entity-type table seen from another field,
docs/91/docs/108).  Read straight out of the shipped DS snapshot (`state_NNNN.ds.bin` is one byte per DS address from
`DS_LO = 0x50000`, `scripts/rig/det_report.py:16`):

```
shipped DS 0x53e9c..0x53ebf = 64 00 00 00  00 00 00 00  e8 03 00 00  00 00 00 00
   -> dword[0x53e9c] = 100, dword[0x53ea0] = 0, dword[0x53ea4] = 1000, dword[0x53ea8] = 0
type-table rows, field +0x0c, idx 0..12: 0, 250, 0, 0, 0, 0, 0, 0, 0, 0, 0, 85, 0
type-table rows, field +0x00, idx 0..12: 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 330, 0
```

`DS 0x53ea8` is **0 in every shipped state sampled**, and a scan of `DS 0x53e9c..0x54400` finds **no** dword equal to
53 or 54.  With `rec[+0x0d] = 0` on every admissible record (the census above), the modelled expression evaluates to
`DS 0x53ea8` = 0.  So the modelled path is not where 54 comes from — on either side.

## 2. Where it does come from: `fn_031f5f` shifts a dword queue inside the record

`fn_031f5f` (called from `fn_012616` at `0x12837` and `0x129a7`) ends in a loop over a slot counter (`0x32004-0x3202f`):

```
0x32004  mov  eax, [ebp-0xc]            ; k
0x32007  inc  dword [ebp-0xc]
0x3200a  mov  eax, [ebp-4]              ; record
0x3200d  mov  al,  [eax + 0x31]         ; slot count
0x32010  and  eax, 0xff
0x32015  cmp  eax, [ebp-0xc]
0x32018  jle  0x32034                   ; while (count > k)
0x3201a  mov  edx, [ebp-0xc]
0x3201d  shl  edx, 2                    ; k*4
0x32020  add  edx, [ebp-4]              ; rec + k*4
0x32023  mov  eax, [ebp-0xc]
0x32026  shl  eax, 2
0x32029  add  eax, [ebp-4]
0x3202c  mov  edx, [edx + 0x36]         ; read slot k+1  (rec + k*4 + 0x36)
0x3202f  mov  [eax + 0x32], edx         ; write slot k   (rec + k*4 + 0x32)
```

That is `slot[k] = slot[k+1]` over a 4-byte-slot array whose head is `rec+0x32` and whose tail is `rec+0x36`, driven by
the byte counter `rec[+0x31]`.  It matches the byte windows in docs/186 §5 exactly: shipped showed `+0x32 = 0x35`
becoming `0x36` while `+0x36` held `0x36` — the shift copies the `0x36` neighbour up.  The port models this function
(`f.c:2117`, `fn_031f5f(uint32_t a0)`, with the same `slot[i] = slot[i+1]` loop and the same documented latent bug
where `a[0x31]--` wraps 0 → 255 and shifts 255 slots).

## 3. What is actually missing, then

The mechanism exists in the port; the **content** does not.  Shipped's records hold `54` in the slot at `rec+0x36`
(the queue's tail), the port's hold `0` there (docs/186 §5 windows).  So the thing to find is the *producer* that puts
54 into the queue: searching for stores into `+0x36` finds only two immediate stores, both of zero
(`mov dword [eax+0x36], 0` at `0x3accc` in `fn_03a829` and `0x3c813` in `fn_03c7d7`), and no
`mov [reg+0x36], reg` in the plain `[reg+disp]` forms — so the writer uses an indexed address (the same
`rec + k*4 + 0x36` shape) and the pattern census has to be widened to indexed forms (`89 94 ...`, SIB with scale 4)
before it can be named.

Two candidate consequences worth separating later, both about item 2's visible half:
* `fn_015295`'s award `*(player + 0x52) += victim[+0x32]` reads the queue head, so a queue that never holds 54 awards
  5 or 0 instead of 54 — which is the "EXP does not accumulate" symptom.
* `fn_031f5f` also latches `g_dat_63364 = base[+0x69]` / `g_dat_63368 = base[+0x6a]` from the *saved* slot value, so a
  wrong queue head changes those cells too (they are in the state oracle's cell list).

Next measurement: widen the store census to SIB-indexed forms writing `+0x32/+0x36` in a `k*4` pattern, and read the
shipped queue contents (`+0x32..+0x4a`) across consecutive states to see what gets pushed and when.
