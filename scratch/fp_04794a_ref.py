"""Ground-truth reference for FUN_0004794a: a mini x86 emulator that executes
the 149-instruction Ghidra disasm. Register-ABI: param_1=EAX, param_2=EDX,
param_3=EBX, param_4=ECX, ESI=exponent. Returns (EAX, EDX).

This is the differential-verification reference: the C port must agree over
random inputs. Only CF/ZF/SF flags are tracked (the jumps used: JNZ,JZ,JNC,JNS,
JBE,JC never test OF)."""

INSNS = [
(293194, "OR EAX,EAX"), (293196, "JNZ 0x0004796d"), (293198, "OR EDX,EDX"),
(293200, "JNZ 0x0004796d"), (293202, "ADD SI,SI"), (293205, "JNZ 0x0004796a"),
(293207, "SHR ESI,0x10"), (293210, "MOV EAX,EBX"), (293212, "MOV EDX,ECX"),
(293214, "ADD ESI,ESI"), (293216, "OR BX,SI"), (293219, "OR EBX,EDX"),
(293221, "JZ 0x00047969"), (293223, "SHR ESI,0x1"), (293225, "RET"),
(293226, "RCR SI,0x1"), (293229, "OR ECX,ECX"), (293231, "JNZ 0x0004797e"),
(293233, "OR EBX,EBX"), (293235, "JNZ 0x0004797e"), (293237, "TEST ESI,0x7fff0000"),
(293243, "JNZ 0x0004797e"), (293245, "RET"), (293246, "PUSH EBP"),
(293247, "PUSH EDI"), (293248, "XCHG ESI,ECX"), (293250, "MOV EDI,ECX"),
(293252, "ROL EDI,0x10"), (293255, "SAR EDI,0x10"), (293258, "SAR ECX,0x10"),
(293261, "AND EDI,0x80007fff"), (293267, "AND ECX,0x80007fff"), (293273, "MOV EBP,ECX"),
(293275, "ROL EDI,0x10"), (293278, "ROL ECX,0x10"), (293281, "ADD CX,DI"),
(293284, "ROL EDI,0x10"), (293287, "ROL ECX,0x10"), (293290, "SUB CX,DI"),
(293293, "JZ 0x000479cd"), (293295, "JNC 0x000479b9"), (293297, "MOV EBP,EDI"),
(293299, "NEG CX"), (293302, "XCHG EAX,EBX"), (293303, "XCHG ESI,EDX"),
(293305, "CMP CX,0x40"), (293309, "JBE 0x000479cd"), (293311, "ADD EBP,EBP"),
(293313, "RCR BP,0x1"), (293316, "MOV EAX,EBX"), (293318, "MOV EDX,ESI"),
(293320, "MOV ESI,EBP"), (293322, "POP EDI"), (293323, "POP EBP"), (293324, "RET"),
(293325, "MOV CH,0x0"), (293327, "OR ECX,ECX"), (293329, "JNS 0x000479e2"),
(293331, "MOV CH,0xff"), (293333, "NEG ESI"), (293335, "NEG EBX"), (293337, "SBB ESI,0x0"),
(293340, "XOR EBP,0x80000000"), (293346, "SUB EDI,EDI"), (293348, "CMP CL,0x0"),
(293351, "JZ 0x00047a15"), (293353, "PUSH EBX"), (293354, "SUB EBX,EBX"),
(293356, "CMP CL,0x20"), (293359, "JC 0x00047a07"), (293361, "OR EAX,EAX"),
(293363, "SETNZ BL"), (293366, "MOV EDI,EBX"), (293368, "SUB EBX,EBX"),
(293370, "CMP CL,0x40"), (293373, "JNZ 0x00047a03"), (293375, "OR EDI,EDX"),
(293377, "SUB EDX,EDX"), (293379, "MOV EAX,EDX"), (293381, "SUB EDX,EDX"),
(293383, "SHRD EBX,EAX,CL"), (293386, "OR EDI,EBX"), (293388, "SUB EBX,EBX"),
(293390, "SHRD EAX,EDX,CL"), (293393, "SHRD EDX,EBX,CL"), (293396, "POP EBX"),
(293397, "ADD EAX,EBX"), (293399, "ADC EDX,ESI"), (293401, "ADC CH,0x0"),
(293404, "JNS 0x00047a46"), (293406, "CMP CL,0x40"), (293409, "JNZ 0x00047a37"),
(293411, "TEST EDI,0x7fffffff"), (293417, "SETNZ BL"), (293420, "SHR EBX,0x1"),
(293422, "ADC EAX,0x0"), (293425, "ADC EDX,0x0"), (293428, "ADC CH,0x0"),
(293431, "NEG EDX"), (293433, "NEG EAX"), (293435, "SBB EDX,0x0"), (293438, "MOV CH,0x0"),
(293440, "XOR EBP,0x80000000"), (293446, "MOV EBX,EAX"), (293448, "OR BL,CH"),
(293450, "OR EBX,EDX"), (293452, "JZ 0x00047a9d"), (293454, "OR BP,BP"),
(293457, "JZ 0x00047a9f"), (293459, "CMP CH,0x0"), (293462, "JNZ 0x00047a66"),
(293464, "ROL EDI,0x1"), (293466, "ROR EDI,0x1"), (293468, "DEC BP"),
(293470, "JZ 0x00047a9f"), (293472, "ADC EAX,EAX"), (293474, "ADC EDX,EDX"),
(293476, "JNC 0x00047a5c"), (293478, "INC BP"), (293480, "CMP BP,0x7fff"),
(293485, "JZ 0x00047aa9"), (293491, "STC"), (293492, "RCR EDX,0x1"),
(293494, "RCR EAX,0x1"), (293496, "JNC 0x00047a9b"), (293498, "ADD EDI,EDI"),
(293500, "JNZ 0x00047a82"), (293502, "ROR EAX,0x1"), (293504, "ROL EAX,0x1"),
(293506, "ADC EAX,0x0"), (293509, "ADC EDX,0x0"), (293512, "JNC 0x00047a9b"),
(293514, "RCR EDX,0x1"), (293516, "RCR EAX,0x1"), (293518, "INC BP"),
(293520, "CMP BP,0x7fff"), (293525, "JZ 0x00047aa9"), (293531, "JMP 0x00047a9f"),
(293533, "MOV EBP,EBX"), (293535, "ADD EBP,EBP"), (293537, "RCR BP,0x1"),
(293540, "MOV ESI,EBP"), (293542, "POP EDI"), (293543, "POP EBP"), (293544, "RET"),
(293545, "MOV BP,0x7fff"), (293549, "SUB EAX,EAX"), (293551, "MOV EDX,0x80000000"),
(293556, "JMP 0x00047a9f"),
]

REGS_32 = {'EAX','EBX','ECX','EDX','ESI','EBP','EDI'}
REGS_16 = {'AX','BX','CX','DX','SI','DI','BP'}
LO8 = {'AL':'EAX','BL':'EBX','CL':'ECX','DL':'EDX'}
HI8 = {'AH':'EAX','BH':'EBX','CH':'ECX','DH':'EDX'}
PARENT = dict({**{r: 'E' + r for r in REGS_16}, **LO8, **HI8})


def _sz(reg):
    return 32 if reg in REGS_32 else (16 if reg in REGS_16 else 8)


def _get(R, reg):
    if reg in REGS_32:
        return R[reg]
    if reg in REGS_16:
        return R[PARENT[reg]] & 0xffff
    if reg in LO8:
        return R[PARENT[reg]] & 0xff
    return (R[PARENT[reg]] >> 8) & 0xff


def _set(R, reg, val):
    if reg in REGS_32:
        R[reg] = val & 0xffffffff
    elif reg in REGS_16:
        R[PARENT[reg]] = (R[PARENT[reg]] & 0xffff0000) | (val & 0xffff)
    elif reg in LO8:
        R[PARENT[reg]] = (R[PARENT[reg]] & 0xffffff00) | (val & 0xff)
    else:
        R[PARENT[reg]] = (R[PARENT[reg]] & 0xffff00ff) | ((val & 0xff) << 8)


def _opval(R, tok, CF):
    """resolve an operand token to (is_reg, value)."""
    t = tok.strip()
    if t.startswith('0x') or t.startswith('-0x'):
        return False, int(t, 16)
    if t.lstrip('-').isdigit():
        return False, int(t)
    return True, _get(R, t)


def emulate_full(EAX, EBX, ECX, EDX, ESI):
    R = {'EAX': EAX & 0xffffffff, 'EBX': EBX & 0xffffffff, 'ECX': ECX & 0xffffffff,
         'EDX': EDX & 0xffffffff, 'ESI': ESI & 0xffffffff, 'EBP': 0, 'EDI': 0}
    CF = ZF = SF = 0
    stack = []
    addr2pc = {a: i for i, (a, _) in enumerate(INSNS)}
    pc = 0
    guard = 0
    while True:
        guard += 1
        if guard > 10000:
            raise RuntimeError("loop overrun")
        a, text = INSNS[pc]
        parts = [p for p in text.split(' ')]
        mn = parts[0]
        args = text[len(mn):].strip().split(',')
        if mn in ('JNZ', 'JZ', 'JNC', 'JNS', 'JBE', 'JC', 'JMP'):
            tgt = int(args[0], 16)
            if mn == 'JMP' or _jump(mn, CF, ZF, SF):
                pc = addr2pc[tgt]
            else:
                pc += 1
            continue
        if mn == 'RET':
            return R['EAX'], R['EDX'], R['ESI']
        if mn == 'STC':
            CF = 1
        elif mn == 'PUSH':
            stack.append(_get(R, args[0].strip()))
        elif mn == 'POP':
            _set(R, args[0].strip(), stack.pop())
        elif mn == 'MOV':
            isr, v = _opval(R, args[1], CF)
            _set(R, args[0].strip(), v)
        elif mn == 'XCHG':
            ra, rb = args[0].strip(), args[1].strip()
            va, vb = _get(R, ra), _get(R, rb)
            _set(R, ra, vb); _set(R, rb, va)
        elif mn == 'NEG':
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            d = _get(R, reg); CF = 1 if d != 0 else 0
            r = (0 - d) & m; _set(R, reg, r); ZF = r == 0; SF = (r >> (n-1)) & 1
        elif mn == 'INC':
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            d = _get(R, reg); r = (d + 1) & m; _set(R, reg, r); ZF = r == 0; SF = (r >> (n-1)) & 1
        elif mn == 'DEC':
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            d = _get(R, reg); r = (d - 1) & m; _set(R, reg, r); ZF = r == 0; SF = (r >> (n-1)) & 1
        elif mn == 'SETNZ':
            _set(R, args[0].strip(), 0 if ZF else 1)
        elif mn == 'CMP':
            _, dv = _opval(R, args[0], CF); _, sv = _opval(R, args[1], CF)
            n = _sz(args[0].strip()); m = (1 << n) - 1
            dd = dv & m; ss = sv & m
            CF = 1 if dd < ss else 0; r = (dd - ss) & m; ZF = r == 0; SF = (r >> (n-1)) & 1
        elif mn == 'TEST':
            _, dv = _opval(R, args[0], CF); _, sv = _opval(R, args[1], CF)
            n = _sz(args[0].strip()); m = (1 << n) - 1
            r = (dv & sv) & m; CF = 0; ZF = r == 0; SF = (r >> (n-1)) & 1
        elif mn in ('ADD', 'SUB', 'OR', 'AND', 'XOR', 'ADC', 'SBB'):
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            d = _get(R, reg) & m
            isr, sv = _opval(R, args[1], CF); s = (sv & m)
            if mn == 'ADD':
                t = d + s; CF = 1 if t > m else 0; r = t & m
            elif mn == 'SUB':
                CF = 1 if d < s else 0; r = (d - s) & m
            elif mn == 'OR':
                CF = 0; r = (d | s) & m
            elif mn == 'AND':
                CF = 0; r = (d & s) & m
            elif mn == 'XOR':
                CF = 0; r = (d ^ s) & m
            elif mn == 'ADC':
                t = d + s + CF; CF = 1 if t > m else 0; r = t & m
            elif mn == 'SBB':
                CF = 1 if d < (s + CF) else 0; r = (d - s - CF) & m
            _set(R, reg, r); ZF = r == 0; SF = (r >> (n-1)) & 1
        elif mn in ('SHL', 'SHR', 'SAR'):
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            isr, cnt = _opval(R, args[1], CF); cnt = cnt & ((n-1) if n == 8 else 31)
            d = _get(R, reg) & m
            if mn == 'SHL':
                CF = (d >> (n-1-cnt)) & 1 if cnt else 0; r = (d << cnt) & m
            elif mn == 'SHR':
                CF = (d >> (cnt-1)) & 1 if cnt else 0; r = d >> cnt
            else:
                sign = d >> (n-1)
                CF = (d >> (cnt-1)) & 1 if cnt else 0
                r = (d >> cnt) | (sign * ((1 << cnt) - 1) << (n-cnt)) if cnt else d
            _set(R, reg, r)
        elif mn == 'SHRD':
            # SHRD dest, src, count : (dest:src) >> count
            dreg, sreg = args[0].strip(), args[1].strip()
            n = _sz(dreg)
            isr, cnt = _opval(R, args[2], CF); cnt = cnt & 31
            d = _get(R, dreg) & ((1 << n) - 1); s = _get(R, sreg) & ((1 << n) - 1)
            CF = (d >> (cnt-1)) & 1 if cnt else 0
            val = ((d << n) | s) >> cnt
            _set(R, dreg, (val >> n) & ((1 << n) - 1))
            _set(R, sreg, val & ((1 << n) - 1))
        elif mn in ('ROL', 'ROR'):
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            isr, cnt = _opval(R, args[1], CF); cnt = cnt & ((n-1) if n == 8 else 31)
            d = _get(R, reg) & m
            if mn == 'ROL':
                CF = (d >> (n-1-cnt)) & 1 if cnt else 0; r = ((d << cnt) | (d >> (n-cnt))) & m
            else:
                CF = (d >> (cnt-1)) & 1 if cnt else 0; r = ((d >> cnt) | (d << (n-cnt))) & m
            _set(R, reg, r)
        elif mn == 'RCR':
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            isr, cnt = _opval(R, args[1], CF); cnt = cnt & ((n-1) if n == 8 else 31)
            d = _get(R, reg) & m
            for _ in range(cnt):
                b0 = d & 1
                d = ((d >> 1) | (CF << (n-1))) & m
                CF = b0
            _set(R, reg, d)
        else:
            raise RuntimeError(f"unhandled: {text} at {a:#x}")
        pc += 1


def emulate(EAX, EBX, ECX, EDX, ESI):
    """Backward-compat: returns (EAX, EDX) only. Use emulate_full for (EAX,EDX,ESI)."""
    lo, hi, _esi = emulate_full(EAX, EBX, ECX, EDX, ESI)
    return lo, hi


def _jump(mn, CF, ZF, SF):
    if mn == 'JNZ': return ZF == 0
    if mn == 'JZ': return ZF == 1
    if mn == 'JNC': return CF == 0
    if mn == 'JNS': return SF == 0
    if mn == 'JC': return CF == 1
    if mn == 'JBE': return (CF == 1) or (ZF == 1)
    return False


if __name__ == '__main__':
    # sanity: all-zero, and a few simple cases
    print("zero:", emulate(0, 0, 0, 0, 0))
    print("eax=1:", emulate(1, 0, 0, 0, 0))
    print("eax=1,esi=0x3fff0000:", emulate(1, 0, 0, 0, 0x3fff0000))
