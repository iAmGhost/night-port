# Image entries that are not game functions (Watcom DOS-extender startup)

While clearing the last clean wins, two remaining "pending functions" turned out not to be
game code at all. They are listed as functions by Ghidra because the loader handed the range
to the disassembler, but the bytes are the **Watcom C/C++32 DOS-extender runtime startup**,
partly a banner string decoded as instructions.

Detection method (`scratch/data_entries.py`): for every `[ ]` row in
`docs/12-functions/STATUS.md`, read the 16 bytes at `file offset = VA - 0x2600` and count
printable characters. Exactly **2 of 59** pending rows are ascii-dominant, and both sit in the
same 0x42044..0x42310 region. The third entry recorded below (`0x0420bc`) is not ascii-dominant -- it
starts with real instructions -- but it is the startup body the entry thunk jumps to, so it belongs to
the same extender block; it was found by following the `jmp 0x420bc` target.

## 0x00042044 `_entry` (1 byte)

```
00042044  eb 76            jmp 0x420bc
```

A one-instruction thunk: the image entry point jumps into the extender startup at `0x420bc`.
There is no game behaviour to port. In the SDL port the process entry is `main()` in the sys
layer; the DOS entry thunk is replaced wholesale.

## 0x00042046 `FUN_00042046` (116 bytes)

First bytes at `0x42046` (as disassembled by capstone):

```
00042046  57                 push edi
00042047  41                 inc  ecx
00042048  54                 push esp
00042049  43                 inc  ebx
0004204a  4f                 dec  edi
0004204b  4d                 dec  ebp
0004204c  20 43 2f           and  byte ptr [ebx+0x2f], al
...
```

which is the ASCII text **`WATCOM C/C++32 Runtime ... /C++ by Tom...`** -- i.e. the banner
string that the runtime keeps next to its code, decoded as instructions (Ghidra reports the
same thing as overlapping-instruction and unreachable-block warnings for this function).
After the string the range continues with the runtime's own initialisation, which Ghidra splices
into the same pseudo:

* `INT 0x21, AH=0x30` (get DOS version) -> `DAT_000533bb` / `DAT_000533bc`;
* DPMI host detection (`INT 0x31`, and the `AH=0x4A` DPMI free-block / `0x4458`/`0x4243`
  host-signature paths) -> `DAT_00053364`, `DAT_00053384`, `DAT_00053390`, `DAT_000533b2/b3`;
* command-line scan for the literal `no78=` (`(*p | 0x20202020) == 0x6e6f3738` and `p[1] == '='`)
  counting occurrences into `DAT_000533b0`, and the trimmed program name copied to `DAT_000640c0`
  (`DAT_00053388` points at it);
* zeroing of a block starting at `DAT_00053818` (0x1000 or 0x108a4 bytes depending on
  `DAT_000533b2`), then `FUN_00044618(0xff)`, `FUN_000445c9()`, `FUN_00044663(0,0xff)`;
* finally `INT 0x21, AH=0x4C` (terminate program).

None of that is NIGHT's own logic: the extender sets it up before `main`, and the port already
substitutes the whole DOS-extender layer (SDL host process, its own arena instead of DPMI,
`sys_*` for file/console/timing). Marking it "ported" would mean emulating a DOS extender that
the port deliberately replaces.

## Ruling

All three rows are recorded as `[-]` (skipped, with this file as the reason) in
`docs/12-functions/STATUS.md`:

| entry | why not ported |
|---|---|
| `0x042044` `_entry` | 1-byte `jmp` thunk = image entry into the extender startup; replaced by the port's `main` |
| `0x042046` | banner string + Watcom extender startup (DOS version, DPMI host probe, `no78=` command-line flag, terminate); no game behaviour |
| `0x0420bc` | the startup body the `_entry` thunk jumps to: `sti; and esp,~3; mov [0x53398]/[0x53384],esp; mov ax,0x24; mov [0x53390],ax; mov ah,0x30; int 0x21` ... then the same DPMI host probe and terminate (536 B) |

Everything else that is still pending is genuine game code. The only entry in the whole image
that mixes game and non-game code in one Ghidra function is this startup block; its only
*observable* game-visible side effect is the `no78=` command-line counter `DAT_000533b0`, which
the port models as a plain global (nothing in the game reads it other than the startup).
