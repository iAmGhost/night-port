# fn_036537 (0x036537, L11, 86 B) — boot/console init sequence — READY, untested

**Status:** all 8 callees are [x]; it is a clean win. Ported (verified faithful) but NOT
committed because an end-to-end test SIGSEGVs on this 64-bit host (see "Test blocker").
Next session: apply the fix below, then it is a 2-check wiring test.

## Exact disasm (0x036537, 86 B)

```
0x36537 push 0x28              ; canary_id(EAX=arg1, 0x28)
0x3653c call 0x418ee
0x36541 push ebx/ecx/edx/esi/edi/ebp; mov ebp,esp; sub esp,4
0x3654f mov [ebp-4],eax        ; save arg1
0x36552 call 0x40570           ; fn_00040570  (DPMI rm call {3} -> no-op)
0x36557 call 0x3eff2           ; fn_03eff2    (install IRQ 0x1c)
0x3655c call 0x1053f           ; fn_1053f     (DPMI INT 31 AX=101 -> no-op)
0x36561 call 0x40e8d           ; fn_040e8d
0x36566 call 0x3ffab           ; fn_03ffab    (install IRQ 9)
0x3656b mov eax,[ebp-4]; push eax            ; push arg1 (varargs base)
0x3656f mov eax,0x5147c; push eax            ; push fmt = DAT_0005147c
0x36575 call 0x42394           ; fn_042394(fmt, varargs)
0x3657a add esp,8
0x3657d xor eax,eax
0x3657f call 0x423b7           ; fn_0423b7(EAX=0)
0x36584 mov esp,ebp; pop...; ret
```

DAT_0005147c = "\n%s\n" (5 bytes: 0a 25 73 0a 00).

## C port (reg-ABI EAX = arg1 = varargs-array base; heap-offset convention)

```c
static const uint8_t g_dat_5147c[] = { '\n', '%', 's', '\n', 0 };  /* DAT_0005147c */
void fn_036537(uint32_t arg1) {
    (void)canary_id((uintptr_t)arg1, 0x28);
    fn_00040570(); fn_03eff2(); fn_1053f(); fn_040e8d(); fn_03ffab();
    fn_042394(g_dat_5147c, (uint8_t *)g_heap_base + arg1);   /* varargs = heap offset */
    fn_0423b7(0, 0, 0, 0);
}
```

Callers (for ABI sanity): FUN_0003f15f, FUN_0003af18, FUN_00036537's own caller passes
EAX = a resource/string pointer used as the "%s" varargs.

## Test blocker (why it SIGSEGVs, not a port bug)

`fn_042394` -> `fn_0446c3(&g_dat_5342a, fmt, &src)`. Because the node is the FIXED GLOBAL
`g_dat_5342a` (a static C global), `fn_044c23` stores `st[0] = (uint32)(node - g_heap_base)`.
On macOS all data (globals + the static heap arena) live just above 4 GB, so that delta wraps
in uint32 and `fn_0446b2`'s `g_heap_base + *(uint32_t*)st` lands on a **bogus address** -> SEGV
inside `fn_049192`'s putc. (`%s` with varargs dword == 0 is itself fine — it selects `alt`.)

This is the same 32-bit-flat-pointer limitation already documented for fn_044c23's near/far
%n stores ("verified by code inspection"). The 5 preceding init calls are all no-ops, so a
test that skips the banner printf proves nothing about this function's real effect.

## Fix to make it testable (the real unlock — pick one)

Option A (small, recommended): make the console node heap-resident so its pointer round-trips
through the uint32 convention. At heap_arena_init (or in the test), point `g_dat_5342a` into
the arena. Cleanest: store the node itself in the arena, i.e. make `g_dat_5342a` an OFFSET set
during init, or heap_malloc a node and copy the 20-byte io_state there so `node - g_heap_base`
is a valid small offset. Then the 2-check wiring test:
  - heap_arena_init(); pre-seed; heap_malloc node(0x20)+buf(32) INSIDE the arena;
  - node f0=buf_off, f8=buf_off, f4=0, f12=0x02 (no flush), f20(cap)=16;
  - varargs dword = 0  (so %s -> alt empty);
  - expect emitted = 2 ("\n" + "\n" written into the node buffer), ret = 2.

Option B: add a proper flat-address translation table (map low 32-bit data addrs -> host ptrs).
That is the general mid-subsystems task; too invasive for a drive-by.

## Also unlocked (still pending, now clean)

- 0x036537 itself once fix A lands.
- Its fmt-descendant chain is otherwise done (0x0446c3 [x], 0x042394 [x], fn_044c23 [x],
  fn_0446b2 [x], fmt_state pos=uint32 heap-offset convention fixed).
