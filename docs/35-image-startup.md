# 0x042044 / 0x042046 / 0x0420bc — the image entry and the DOS4GW startup, ported

These were the three rows docs/16 had ruled `[-]` ("the extender layer the port replaces"). They are now
ported, so the whole chain `image entry -> startup -> dispatch_a -> entry thunk -> game main ->
dispatch_b -> terminate` exists in the port and is driven by a test.

## What the three rows actually are

| address | what it is |
|---|---|
| `0x042044` | 2 bytes: `jmp 0x420bc`. The image entry point. |
| `0x042046` | `0x42046..0x420bb` is the runtime banner string (`Watcom C++ ... Time Stamp ... by Watson C Information ...`) decoded as code — capstone reads `push edi; inc ecx; push esp; ...`, Ghidra threw out twelve unreachable blocks. Behind it sits the same startup body. |
| `0x0420bc` | the startup body itself (536 B). |

So there is one real body; `fn_042044` and `fn_042046` are thin aliases into it (`startup_body()` in
`port/src/f/f.c`).

## The body, step by step (addresses from the capstone listing)

1. `0x420bc` `sti; and esp,~3` — latch the 4-aligned stack into `DAT_53398` / `DAT_53384`, and `DAT_53390 = 0x24`.
2. `0x420dd` `xor eax,eax; mov ah,0x30; int 0x21` — the DOS version call. `AL`/`AH` land in `DAT_533bb`/`DAT_533bc`
   and **CX selects the host branch**:
   * `CX = 0x4458` ('DX', DR DOS): the buffer pointer comes from `DS:0x5c` rounded up to a page, marker `0x22`, `CX = 0x2c`.
   * `CX = 0x4243` ('CB', DOS4GW): `EDX` is the extenders info block → `DAT_53364`; the DTA address at `+0x1c` is
     handed to `INT 21h AH=2Ah`; the PSP base at `+0x10` is added to the `0x81` command-tail pointer; the DS
     paragraph at `+0x2c` shifted left 4 becomes `DAT_533b5`; marker **9**.
   * otherwise: `INT 21h AX=FF00 DX=78` DPMI probe. No host → marker `0/0`, `CX = 0x2c`. A host → latch `GS`
     into `DAT_53368` if set, `INT 31h AX=0006` (disable interrupts), marker `1` with `AH` set when the returned
     disable count is non-zero.
3. `0x421d1` tail: marker pair → `DAT_533b2`/`DAT_533b3`, `DAT_533b5`, `DAT_533b9`; the command buffer address is
   `0x140bc + 0xf` with **only DL masked to 0xf0** → `0x140c0` (`DAT_53388`).
4. `0x42209` `mov cl,[es:edi-1]` reads the PSP tail length, `repe scasb` against `' '` skips the leading spaces,
   `rep movsb` copies the rest into `0x140c0`, then two `stosb` NULs. Empty tail → `sub ecx,ecx` left ZF set, so
   nothing is copied — that quirk is reproduced.
5. `0x42236` walks the environment that follows, counting entries that open with **`no78=`** — the compare ORs
   four bytes with `0x20202020`, so `NO78=` matches too — into `DAT_533b0`, then copies the string three bytes past
   the block terminator (the `lodsb` + two `inc esi` at `0x42258`) with `movsb` up to and including its NUL.
6. `0x42278` zero `0x3818..0x140bc` (`0x108a4` bytes) with `rep stosd` + the `and cl,3` remainder — **capped at
   `0x1000` when the marker byte is 1**, which is why the command buffer survives under a DPMI host.
7. `0x422c0` writes the return slot at `[ESP-4]` and calls `dispatch_a(0xff)`; then slot → `0x422d3` and the entry
   thunk `fn_0445c9`; then slot → `0x42308`, `dispatch_b(0,0xff)`, and finally `INT 21h AH=4Ch` with AL from that
   slot (exit code `0x08`).

## How the flat port models the host side

`port/src/sys/sys.c` gained the probes as explicit, testable state instead of real interrupts:
`sys_dos_version_set()` (major/minor/CX marker, defaulting to DOS 3.31 with the `CB` marker),
`sys_dos4gw_info()` (the info block: `+0x10` PSP base, `+0x1c` DTA, `+0x2c` DS paragraph),
`sys_dpmi_host_probe_set()`, `sys_dpmi_disable_ints_set()`, `sys_psp_tail_set()` (length byte + 127 chars),
`sys_dos_env_set(bytes, len)` and `sys_dos_terminate()` / `sys_dos_exit_code()`.

Two deliberate deviations, both written into the source:

* **The BSS zeroing is recorded, not applied.** That range is a set of C globals in this port, all zero before
  anything runs; re-applying it after startup would wipe live tables. The computed extent is kept in
  `g_startup_zero_len`, and the test pins both values (`0x108a4` normally, `0x1000` with marker 1).
* **The environment scan walks the modelled env block** rather than flat memory at `0x140c0 + copied + 2`, and
  copies it into the same buffer — the state the original leaves behind.

## Test

`test_fn_0420bc` (17 checks) runs the real chain four times, once per host branch, escaping the game main through
the terminate callbacks (`DAT_5336c/70/74` → `longjmp`) exactly like `test_fn_0445c9`. It pins the DOS-version
latch, the marker pair per branch, the info-block pointer, `DAT_533b5`, the trimmed command tail in `DAT_640c0`,
the case-insensitive `no78=` count, the terminator+3 copy quirk, `DAT_53388 = 0x140c0`, both zeroing extents, that
`dispatch_a(0xff)` fired a pending event-table entry *before* the entry thunk, and the `[ESP-4]` slot value at the
moment the game main is entered.

Two things the test caught about the port itself:

* the event table left over from earlier tests is walked by `dispatch_a` (the scan keeps the **last** entry whose
  key is under the rolling threshold), so the test zeroes `g_evtab` and gives entry 0 a key the rest cannot beat;
* the first chain run consumes the pending-key latch `DAT_533a4` inside `fn_03ae8e`, so every later run has to
  re-arm it or the key wait spins — visible in the backtrace as `fn_03ae8e → fn_04295d_keywait → sys_key_spin`.

Suite after this: **2763 PASS / 0 FAIL**, ASan clean. Inventory: **526 [x] + 2 [-] = 528, zero pending** — the two
`[-]` are the bundled 8087 emulator pair (`0x0461cc`, `0x0473bf`).
