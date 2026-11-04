# MIDI sequencer family — cursor-model fix + 0x035263 / 0x034e3c port plan

Discovered 2026-07-13 while porting 0x035263. The 0x035xxx "device" family is actually the
**MIDI track sequencer** (fn_0353d0's `cmd & 0xf0` dispatch = MIDI status bytes: 0x80–0xEF
channel voice, 0xC0 program change, 0xD0 pitch bend, 0xF0 sysex, 0xFF meta).

## The record array (shared by fn_034f31 / fn_035329 / fn_035263 / fn_0353d0)
Stride **0xe (14 bytes)**, built by fn_034f31 (MThd/MTrk parser):
- offset 0   (4 B): **cursor = g_heap_base-relative OFFSET** (uint32_t), NOT a pointer
- offset 4   (1 B): cmd byte
- offset 5   (1 B): tag byte (0x2f = "has tempo / stop" marker)
- offset 6-9 (4 B): (len etc.)
- offset 0xa (4 B): acc / next-event-time value (int32_t)

Cursor is resolved against g_heap_base whenever dereferenced. Established pattern (fn_034f31 f.c:1571-1579):
```c
uint8_t *lc = hb + *(uint32_t *)(rec + 0x0);   // offset -> real ptr
uint32_t v = fn_0351dc(&lc);                    // LEB128, advances lc
*(uint32_t *)(rec + 0x0) = (uint32_t)(lc - hb); // store back offset
```

## BUG / INCONSISTENCY: committed fn_0353d0 models head as a direct 8-byte pointer
Current `struct dev_cmd_ctx { uint8_t *head; uint8_t f4; uint8_t f5; }` (f.h) treats the
cursor as a direct host pointer (8 B) with f4/f5 at offsets 8/9 and implicit stride 0x12.
This is self-consistent with fn_0353d0's OWN isolated test (which builds its own ctx), but
**disagrees with the MIDI record array** (stride 0xe, 4-byte offset cursor at rec[0], cmd at
rec[4], tag at rec[5]). fn_035329 (f.c, committed) uses `p0[i*0xe+0x5]` / `p0[i*0xe+0xa]`.

Because fn_035263 passes a real MIDI record to fn_0353d0, the two MUST agree. => **rework
fn_0353d0 to the 4-byte-offset cursor model before porting fn_035263.**

## Fix 1: rework fn_0353d0 (committed, f.c ~9488 + f.h ~613)
Change the ctx to the exact original 14-byte layout (cursor = 4-byte offset):
```c
struct dev_cmd_ctx {
    uint32_t head_off;  /* +0: 4-byte g_heap_base-relative cursor (rec[0]) */
    uint8_t f4;         /* +4: cmd byte  (rec[4]) */
    uint8_t f5;         /* +5: tag byte  (rec[5]) */
};
```
Replace every cursor use with offset resolution (hb = g_heap_base):
- `*arg->head++`            -> `*(hb + arg->head_off); arg->head_off++;`
- `arg->head++` (INC *rec)  -> `arg->head_off++;`
- `fn_0351dc(&arg->head)`   -> `uint8_t *lc = hb + arg->head_off; uint32_t r = fn_0351dc(&lc); arg->head_off = (uint32_t)(lc - hb);`
- `arg->head += r`          -> `arg->head_off += r;`

The logic/branches of fn_0353d0 are otherwise unchanged (cmd<0xf0: f4=cmd; cmd==0xff: c,
r, c==0x2f->f4=f5=0x2f / c==0x51->g_63308,fn_03565a, head+=r; else op=cmd&0xf0 dispatch
read2/read1/readN/no-op).

Update test_fn_0353d0: the byte buffer must sit at a known g_heap_base offset (follow
fn_034f31's test arena pattern). Set `ctx.head_off = (uint32_t)(buf - g_heap_base)` instead
of `ctx.head = buf`, and advance expectations in OFFSET units (same byte counts: T1 +2, T3 +2,
T4 +1, T5 +2). g_530bc side effect (T1: [7,0x42]->0x42) is unchanged.

## Fix 2: port fn_035263 (L15, 197 B, 66 insns) — ALL callees ported
ABI: `void fn_035263(uint8_t *base, uint8_t *ctx)` (a0=EAX=records, a1=EDX=sched ctx).
`idx = *(uint32_t*)(ctx + 0x14)` (set by a prior fn_035329); `rec = (struct dev_cmd_ctx*)base + idx`
(stride 0xe). Body:
```c
(void)canary_id(0, 0x28);
uint32_t idx = *(uint32_t *)(ctx + 0x14);
struct dev_cmd_ctx *rec = (struct dev_cmd_ctx *)base + idx;
uint8_t b = *(uint8_t *)(g_heap_base + rec->head_off);   // *cursor (byte)
if ((b & 0x80) == 0) {                                    // 0x3529a-0x352a3
    fn_0353d0(rec->f4, rec);                              // cmd = rec[4]
} else {
    rec->head_off++;                                      // INC *rec
    fn_0353d0(b, rec);                                    // cmd = b
}
if (rec->f5 == 0x2f) {                                    // 0x352eb-0x352ee
    *(int32_t *)((uint8_t*)rec + 0xa) = 0x7fffffff;       // rec[0xa] = INT32_MAX
} else {
    uint8_t *lc = (uint8_t *)g_heap_base + rec->head_off;
    uint32_t r = fn_0351dc(&lc);
    *(int32_t *)((uint8_t*)rec + 0xa) += (int32_t)r;      // rec[0xa] += LEB128
    rec->head_off = (uint32_t)(lc - (uint8_t*)g_heap_base);
}
fn_035329(base, ctx);                                     // re-find min track
```
Note: acc at offset 0xa is a 4-byte field BEYOND dev_cmd_ctx (which is 6 B); access it via
`(uint8_t*)rec + 0xa`. (dev_cmd_ctx = 6 B: head_off[4] + f4 + f5; the record is 14 B, so a
local pointer-cast to offset 0xa is correct and matches the 0xe stride.)

Test (arena buffers at g_heap_base offsets):
- T1 bit7-clear: buf=[0x07,0x42,0x00,..], rec->f4=0xb0, rec->f5=0x00, ctx[0x1c]=1(count).
  => b=0x07<0x80 -> fn_0353d0(0xb0,rec): f4=0xb0, op=0xb0 read2 -> g_dat_530bc[0]=0x42,
  head_off+=2; f5!=0x2f -> acc += LEB128(0x00)=0; head_off+=1. Verify g_530bc[0]==0x42,
  head_off advanced 3.
- T2 bit7-set + tag 0x2f: buf=[0x80,x,y,..], rec->f5=0x2f, acc=0. => b=0x80: head_off++
  (+1); fn_0353d0(0x80,rec): read2 consumes 2 (head_off+=2); f5==0x2f -> acc=0x7fffffff.
  Verify acc==0x7fffffff, head_off advanced 3.
- ctx[0x14] stays 0 (single record, fn_035329 count=1).

## Fix 3 (next): port fn_034e3c (L16, 115 B, 36 insns) — unblocked once 0x035263 done
Callees (all ported after 0x035263): 0x0418ee, 0x035263, 0x034f31, 0x034eb0. Grab disasm,
ABI = (EAX,EDX,EBX,ECX or stack) — verify against disasm. Likely the per-tick scheduler:
fn_034f31 (parse) -> loop fn_035263 (advance min track) gated by a timer.

## After this family
- 0x035263 (L15), 0x034e3c (L16) -> marks L15 0->1, L16 0->1.
- Remaining L13-L23 small fns stay blocked on their (unported) higher-tier callees; 0x042046
  is a spurious misdisassembled data/string region (not a real fn).
- Critical path unchanged: 0x0461cc (5909B FPU engine) + 0x04192e (I/O reader).
