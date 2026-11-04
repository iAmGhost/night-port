# 0x044ab5 (L6, 57 insns, 0 FPU) — node-pool record allocator — PORT SPEC

Clean single win. 3 unique callees, ALL ported:
- `heap_malloc(uint32_t size)`           /* 0x041af8 */  (f.c:2314)
- `fn_041e20(uint8_t *dest, uint8_t b, uint32_t n)`      /* 0x041e20 */ = memset  (f.c:4413)
- `fn_042a0d(uint32_t x)`                  /* 0x042a0d */ -> g_dat_64008 = x     (f.c:2999)

Single caller: **FUN_000426f2** (call @ 0x0426f8). Caller:
  ESI=EAX(arg); XOR EAX,EAX; CALL 0x044ab5; ECX=EAX(ret); TEST EAX,EAX; JZ skip;
  EAX=ESI(arg); CALL 0x04262a          ;  => fn_04262a(EAX=arg, ECX=record)
So the return is consumed as a **record POINTER** (reg-ABI ECX). => signature:

    uint8_t *fn_044ab5(void);           /* returns the freshly-allocated RECORD, or NULL */

## Data model (reuse existing globals — do NOT add new ones)
- `g_ndpool` (uint8_t*)   base of the RECORD pool. Records are 0x1a bytes, flags at +0xc.
- `g_heap_base` (uintptr) node arena. heap_malloc returns an OFFSET from it.
- `g_node_list_done` (0x63ff8)  = OFFSET of the head node of the DONE list (0=empty).
- `g_node_list_active` (0x63ffc) = OFFSET of the head node of the ACTIVE list (0=empty).
- `struct nd_node { uint32_t next; uint32_t rec; };`  (next@0 = node offset; rec@4 = g_ndpool OFFSET of the record)

IMPORTANT modeling note (the one subtlety): for the WARM and POOL paths the record
lives in g_ndpool (node->rec = g_ndpool offset). For the OVERFLOW path the record is
allocated in-heap at (node_off+8) — it is NOT in g_ndpool. Since the ONLY consumer of
these records is fn_04262a (which receives the record as a POINTER, never via
node->rec), we return the actual record POINTER in all three paths and store node->rec
as a g_ndpool offset in the warm/pool paths. In the overflow path set node->rec to the
heap offset of the in-heap record (n_off+8) — it is never re-consumed via g_ndpool, so
this is faithful to the flat-model "store the address" behavior.

## Full disasm trace (0x044ab5-0x044b5c)
```
PUSH EBX/ECX/EDX/ESI/EDI
ESI = [0x63ff8]                ; node offset = done head
if (ESI == 0) -> COLD else WARM

WARM (done head != 0):
  node = g_heap_base + ESI
  ecx_off = node->rec                 ; record offset (g_ndpool)
  edi     = *(g_ndpool + ecx_off + 0xc)    ; record->flags
  next    = node->next
  g_node_list_done = next             ; [0x63ff8] = head->next
  edi    = (edi & 0x4003) | 0x3
  FINALIZE(esi=Esi, ecx_ptr=g_ndpool+ecx_off, edi)

COLD (done head == 0):
  ecx_off = 0x53410
  for ( ; ecx_off < 0x53618 ; ecx_off += 0x1a) {
      if ((*(g_ndpool + ecx_off + 0xc) & 0x3) == 0) {   // free record
          esi = heap_malloc(8);
          if (esi == 0) { fn_042a0d(5); return 0; }
          edi = 0x3;
          FINALIZE(esi, g_ndpool+ecx_off, edi);
      }
  }
  // pool exhausted (ecx_off >= 0x53618):
  n_off = heap_malloc(0x22);
  if (n_off == 0) { fn_042a0d(5); return 0; }
  base = g_heap_base + n_off;
  FINALIZE(esi=n_off, ecx_ptr=base+8, edi=0x4003)   ; record in-heap at +8

FINALIZE(node_off ESI, record_ptr ECX, flags EDI):
  fn_041e20(ECX, 0, 0x1a)            ; memset(record, 0, 0x1a)
  *(ECX + 0xc) = EDI                 ; record->flags = EDI
  old_active = g_node_list_active
  *(g_heap_base + ESI + 4) = <record offset>   ; node->rec  (g_ndpool off for warm/pool; heap off for overflow)
  g_node_list_active = ESI           ; [0x63ffc] = node
  *(g_heap_base + ESI + 0) = old_active ; node->next = old active head
  return ECX                          ; record pointer
```
Node "next" is a g_heap_base OFFSET (matches node_deactivate/fn_0427a3 convention).

## C port (write to f.c near node_deactivate)
```c
/* 0x044ab5 (L6, 57 B): allocate one file-structure RECORD from the 26-entry pool
 * at g_ndpool+0x53410 (stride 0x1a), else from the heap; wire a fresh node onto the
 * front of the ACTIVE list. Returns the record pointer, or NULL (g_dat_64008=5). */
uint8_t *fn_044ab5(void) {
    uint8_t *rec;      // record pointer (return)
    uint32_t node_off, edi;
    uint32_t done = g_node_list_done;          // 0x63ff8
    if (done != 0) {                            // WARM
        struct nd_node *n = (struct nd_node *)(g_heap_base + done);
        rec = g_ndpool + n->rec;
        edi = *(uint32_t *)(rec + 0xc);
        g_node_list_done = n->next;
        edi = (edi & 0x4003) | 0x3;
        node_off = done;
    } else {                                    // COLD
        for (uint32_t off = 0x53410; off < 0x53618; off += 0x1a) {
            if ((g_ndpool[off + 0xc] & 0x3) == 0) {
                node_off = heap_malloc(8);
                if (node_off == 0) { fn_042a0d(5); return NULL; }
                rec = g_ndpool + off; edi = 0x3;
                goto finalize;
            }
        }
        node_off = heap_malloc(0x22);
        if (node_off == 0) { fn_042a0d(5); return NULL; }
        rec = (uint8_t *)g_heap_base + node_off + 8; edi = 0x4003;
    }
finalize:
    fn_041e20(rec, 0, 0x1a);                    // memset 0x1a
    *(uint32_t *)(rec + 0xc) = edi;
    uint32_t old_active = g_node_list_active;
    struct nd_node *n = (struct nd_node *)(g_heap_base + node_off);
    n->rec = (uint32_t)(rec - g_ndpool);        // g_ndpool offset (overflow: heap offset)
    g_node_list_active = node_off;
    n->next = old_active;
    return rec;
}
```

## Test plan (test_f1.c, test_fn_044ab5)
Reuse the existing heap + a static g_ndpool buffer. Set g_ndpool to a >=0x1c8-byte
arena; g_node_list_done/active to 0 initially.
- Cold pool path: clear g_ndpool[0x53410-ish]; call fn_044ab5; assert return==g_ndpool+
  first-free, record->flags==0x3, node wired to active list, g_node_list_active set.
- Warm path: call again (done now non-empty after a node_deactivate-style setup) or
  pre-seed g_node_list_done with a node pointing at a record; assert (flags&0x4003)|3,
  done head advances to node->next, active head = node.
- Overflow path: set all 26 pool records' flags |= 1 (busy); call; assert record is a
  heap pointer (>= g_heap_base), flags==0x4003.
- Keep g_ndpool/heap save/restore around the test (the suite already does this pattern).

## STATUS.md
- L6 row 0x044ab5 [x], fn name fn_044ab5. L6 count 4->5. TOTALS 375->376/528.
- Add a dated log entry.

## Notes / risks
- Verify the pool base: fn_04933a uses g_ndpool + 0x53410 for the SAME 26-record table,
  so 0x53410/0x53618/0x1a here match exactly (26*0x1a=0x1c8).
- n->rec for overflow = rec - g_ndpool is a negative/large value; it is never consumed
  via g_ndpool (only fn_04262a gets the pointer). Document as a flat-model artifact.
- fn_04262a is the only consumer; confirm it is ported before relying on end-to-end
  behavior (unit test of 0x044ab5 itself needs no fn_04262a).
