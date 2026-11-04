#!/usr/bin/env python3
"""Ground-truth mini x86 emulator for FUN_00047b79 (L1, 177 insns).

A pure-integer fixed-point binary op (multiply/divide with normalization).
Register-ABI (from wrapper 0x047b4f): in EAX=a0, EDX=a1, ESI=(b2<<16|a2),
ECX=b1, EBX=b0 ; out EAX=f0, EDX=f1, SI(=ESI&0xffff)=f2.

This is the differential-verification reference: the auto-generated C port
(gen_047b79_c.py) must agree over random inputs.  It extends the fp_04794a
emulator with: a stack-memory model (PUSH/POP + [EBP+disp] locals), DIV/MUL,
the OF flag (for JG/JGE/JS/JA/JL), and a 3-value return.  The two CALL 0x463dc
dispatches are net no-ops (handler 0x042a07 is a no-op primitive; EAX is reset
immediately after each) and are omitted from INSNS.
"""
import random

INSNS = [
(0x47b79,"OR ECX,ECX"), (0x47b7b,"JNZ 0x47bc4"), (0x47b7d,"OR EBX,EBX"),
(0x47b7f,"JNZ 0x47bc4"), (0x47b81,"TEST ESI,0x7fff0000"), (0x47b87,"JNZ 0x47bc4"),
(0x47b89,"OR EAX,EAX"), (0x47b8b,"JNZ 0x47bae"), (0x47b8d,"OR EDX,EDX"),
(0x47b8f,"JNZ 0x47bae"), (0x47b91,"MOV EAX,ESI"), (0x47b93,"ADD AX,AX"),
(0x47b96,"JNZ 0x47bae"), (0x47b98,"MOV AL,0x1"), (0x47b9a,"MOV AH,0x81"),
# CALL 0x463dc (0x47b9c) -- net no-op, omitted
(0x47ba1,"MOV EDX,0xc0000000"), (0x47ba6,"SUB EAX,EAX"), (0x47ba8,"MOV SI,0xffff"),
(0x47bac,"JMP 0x47bc3"),
(0x47bae,"MOV AL,0x4"), (0x47bb0,"MOV AH,0x83"),
# CALL 0x463dc (0x47bb2) -- net no-op, omitted
(0x47bb7,"MOV EDX,0x80000000"), (0x47bbc,"SUB EAX,EAX"), (0x47bbe,"OR SI,0x7fff"),
(0x47bc3,"RET"),
(0x47bc4,"OR EAX,EAX"), (0x47bc6,"JNZ 0x47bd5"), (0x47bc8,"OR EDX,EDX"),
(0x47bca,"JNZ 0x47bd5"), (0x47bcc,"ADD SI,SI"), (0x47bcf,"JNZ 0x47bd2"),
(0x47bd1,"RET"), (0x47bd2,"RCR SI,0x1"),
(0x47bd5,"PUSH EBP"), (0x47bd6,"MOV EBP,ESP"), (0x47bd8,"PUSH EDI"),
(0x47bd9,"XCHG ESI,ECX"), (0x47bdb,"MOV EDI,ECX"), (0x47bdd,"ROL EDI,0x10"),
(0x47be0,"SAR EDI,0x10"), (0x47be3,"SAR ECX,0x10"), (0x47be6,"AND EDI,0x80007fff"),
(0x47bec,"AND ECX,0x80007fff"), (0x47bf2,"ROL EDI,0x10"), (0x47bf5,"ROL ECX,0x10"),
(0x47bf8,"ADD DI,CX"), (0x47bfb,"ROL EDI,0x10"), (0x47bfe,"ROL ECX,0x10"),
(0x47c01,"OR DI,DI"), (0x47c04,"JNZ 0x47c10"),
(0x47c06,"ADD EAX,EAX"), (0x47c08,"ADC EDX,EDX"), (0x47c0a,"DEC DI"),
(0x47c0c,"OR EDX,EDX"), (0x47c0e,"JNS 0x47c06"),
(0x47c10,"OR CX,CX"), (0x47c13,"JNZ 0x47c1f"),
(0x47c15,"ADD EBX,EBX"), (0x47c17,"ADC ESI,ESI"), (0x47c19,"DEC CX"),
(0x47c1b,"OR ESI,ESI"), (0x47c1d,"JNS 0x47c15"),
(0x47c1f,"SUB DI,CX"), (0x47c22,"ADD DI,0x3fff"), (0x47c27,"JS 0x47c42"),
(0x47c29,"CMP DI,0x7fff"), (0x47c2e,"JC 0x47c42"),
(0x47c30,"MOV EDI,ECX"), (0x47c32,"MOV DI,0x7fff"), (0x47c36,"MOV EDX,0x80000000"),
(0x47c3b,"SUB EAX,EAX"), (0x47c3d,"JMP 0x47d1a"),
(0x47c42,"CMP DI,-0x40"), (0x47c46,"JGE 0x47c53"),
(0x47c48,"SUB EAX,EAX"), (0x47c4a,"SUB EDX,EDX"), (0x47c4c,"SUB EDI,EDI"),
(0x47c4e,"JMP 0x47d1a"),
(0x47c53,"PUSH EDI"), (0x47c54,"PUSH ESI"), (0x47c55,"PUSH EBX"),
(0x47c56,"MOV ECX,ESI"), (0x47c58,"MOV EDI,EDX"), (0x47c5a,"MOV ESI,EAX"),
(0x47c5c,"SUB EAX,EAX"), (0x47c5e,"CMP ECX,EDI"), (0x47c60,"JA 0x47c65"),
(0x47c62,"SUB EDX,ECX"), (0x47c64,"INC EAX"), (0x47c65,"PUSH EAX"),
(0x47c66,"MOV EAX,ESI"), (0x47c68,"DIV ECX"), (0x47c6a,"PUSH EAX"),
(0x47c6b,"XCHG EAX,EBX"), (0x47c6c,"MUL EBX"), (0x47c6e,"XCHG EAX,ECX"),
(0x47c6f,"XCHG EDX,EBX"), (0x47c71,"MUL EDX"), (0x47c73,"ADD EAX,EBX"),
(0x47c75,"ADC EDX,0x0"), (0x47c78,"MOV EBX,[EBP-0x10]"),
(0x47c7b,"TEST [EBP-0x14],0x1"), (0x47c7f,"JZ 0x47c86"),
(0x47c81,"ADD EAX,EBX"), (0x47c83,"ADC EDX,[EBP-0xc]"),
(0x47c86,"NEG ECX"), (0x47c88,"SBB ESI,EAX"), (0x47c8a,"SBB EDI,EDX"),
(0x47c8c,"JZ 0x47ca0"), (0x47c8e,"SUB [EBP-0x18],0x1"), (0x47c92,"SBB [EBP-0x14],0x0"),
(0x47c96,"ADD ECX,EBX"), (0x47c98,"ADC ESI,[EBP-0xc]"), (0x47c9b,"ADC EDI,0x0"),
(0x47c9e,"JNZ 0x47c8e"),
(0x47ca0,"MOV EDI,ESI"), (0x47ca2,"MOV ESI,ECX"), (0x47ca4,"MOV ECX,[EBP-0xc]"),
(0x47ca7,"CMP ECX,EDI"), (0x47ca9,"JA 0x47cb5"),
(0x47cab,"SUB EDI,ECX"), (0x47cad,"ADD [EBP-0x18],0x1"), (0x47cb1,"ADC [EBP-0x14],0x0"),
(0x47cb5,"MOV EDX,EDI"), (0x47cb7,"MOV EAX,ESI"), (0x47cb9,"DIV ECX"),
(0x47cbb,"PUSH EAX"), (0x47cbc,"OR EAX,EAX"), (0x47cbe,"JZ 0x47cec"),
(0x47cc0,"XCHG EAX,EBX"), (0x47cc1,"MUL EBX"), (0x47cc3,"XCHG EAX,ECX"),
(0x47cc4,"XCHG EDX,EBX"), (0x47cc6,"MUL EDX"), (0x47cc8,"ADD EAX,EBX"),
(0x47cca,"ADC EDX,0x0"), (0x47ccd,"NEG ECX"), (0x47ccf,"SBB ESI,EAX"),
(0x47cd1,"SBB EDI,EDX"), (0x47cd3,"JZ 0x47cec"), (0x47cd5,"SUB [EBP-0x1c],0x1"),
(0x47cd9,"SBB [EBP-0x18],0x0"), (0x47cdd,"SBB [EBP-0x14],0x0"),
(0x47ce1,"ADD ECX,[EBP-0x10]"), (0x47ce4,"ADC ESI,[EBP-0xc]"), (0x47ce7,"ADC EDI,0x0"),
(0x47cea,"JNZ 0x47cd5"),
(0x47cec,"POP EAX"), (0x47ced,"POP EDX"), (0x47cee,"POP EBX"),
(0x47cef,"ADD ESP,0x8"), (0x47cf2,"POP EDI"), (0x47cf3,"DEC DI"),
(0x47cf5,"SHR EBX,0x1"), (0x47cf7,"JNC 0x47cfe"), (0x47cf9,"RCR EDX,0x1"),
(0x47cfb,"RCR EAX,0x1"), (0x47cfd,"INC EDI"), (0x47cfe,"OR DI,DI"),
(0x47d01,"JG 0x47d1a"), (0x47d03,"JNZ 0x47d09"), (0x47d05,"MOV CL,0x1"),
(0x47d07,"JMP 0x47d0f"), (0x47d09,"NEG DI"), (0x47d0c,"MOV CX,DI"),
(0x47d0f,"SUB EBX,EBX"), (0x47d11,"SHRD EAX,EDX,CL"), (0x47d14,"SHRD EDX,EBX,CL"),
(0x47d17,"SUB DI,DI"), (0x47d1a,"ADD EDI,EDI"), (0x47d1c,"RCR DI,0x1"),
(0x47d1f,"MOV ESI,EDI"), (0x47d21,"POP EDI"), (0x47d22,"POP EBP"),
(0x47d23,"RET"),
]

REGS_32 = {'EAX','EBX','ECX','EDX','ESI','EBP','EDI'}
REGS_16 = {'AX','BX','CX','DX','SI','DI','BP'}
LO8 = {'AL':'EAX','BL':'EBX','CL':'ECX','DL':'EDX'}
HI8 = {'AH':'EAX','BH':'EBX','CH':'ECX','DH':'EDX'}
PARENT = dict({**{r:'E'+r for r in REGS_16}, **LO8, **HI8})


def _sz(reg):
    return 32 if reg in REGS_32 else (16 if reg in REGS_16 else 8)


def _get(R, reg):
    if reg in REGS_32: return R[reg]
    if reg in REGS_16: return R[PARENT[reg]] & 0xffff
    if reg in LO8: return R[PARENT[reg]] & 0xff
    return (R[PARENT[reg]] >> 8) & 0xff


def _set(R, reg, val):
    if reg in REGS_32: R[reg] = val & 0xffffffff
    elif reg in REGS_16: R[PARENT[reg]] = (R[PARENT[reg]] & 0xffff0000) | (val & 0xffff)
    elif reg in LO8: R[PARENT[reg]] = (R[PARENT[reg]] & 0xffffff00) | (val & 0xff)
    else: R[PARENT[reg]] = (R[PARENT[reg]] & 0xffff00ff) | ((val & 0xff) << 8)


def _memdisp(t):
    inner = t[1:-1].replace('EBP', '')
    if inner.startswith('+'): return int(inner[1:], 0)
    if inner.startswith('-'): return -int(inner[1:], 0)
    return int(inner, 0)


def _opval(R, tok, mem, ebp):
    t = tok.strip()
    if t.startswith('['):
        return False, mem.get(ebp + _memdisp(t), 0) & 0xffffffff
    if t.startswith('0x') or t.startswith('-0x'): return False, int(t, 16)
    if t.lstrip('-').isdigit(): return False, int(t)
    return True, _get(R, t)


def _memwrite(mem, ebp, tok, val):
    d = _memdisp(tok)
    mem[ebp + d] = val & 0xffffffff


class DE(Exception):
    pass


def emulate(EAX, EBX, ECX, EDX, ESI):
    R = {'EAX': EAX & 0xffffffff, 'EBX': EBX & 0xffffffff, 'ECX': ECX & 0xffffffff,
         'EDX': EDX & 0xffffffff, 'ESI': ESI & 0xffffffff, 'EBP': 0, 'EDI': 0}
    CF = ZF = SF = OF = 0
    mem = {}
    esp = 0x1000
    ebp = 0
    addr2pc = {a: i for i, (a, _) in enumerate(INSNS)}
    pc = 0
    guard = 0
    while True:
        guard += 1
        if guard > 200000: raise RuntimeError("loop overrun")
        a, text = INSNS[pc]
        mn = text.split(' ', 1)[0]
        args = text[len(mn):].strip().split(',')
        # memory write? (reg form is args[0]=reg; mem form is args[0]='[..]')
        def mw(tok, v): _memwrite(mem, ebp, tok, v)
        if mn in ('JNZ','JZ','JNC','JNS','JBE','JC','JS','JG','JGE','JL','JA','JMP'):
            t = int(args[0], 16)
            if mn == 'JMP' or _jump(mn, CF, ZF, SF, OF):
                pc = addr2pc[t]
            else: pc += 1
            continue
        if mn == 'RET':
            # full 32-bit post-call ESI (asm `MOV ESI,EDI` on the main return path);
            # 16-bit consumers (fp_048422/04836c) mask with & 0xffff as before.
            return R['EAX'], R['EDX'], R['ESI'] & 0xffffffff
        if mn == 'PUSH':
            esp -= 4; mem[esp] = _get(R, args[0].strip()) & 0xffffffff
        elif mn == 'POP':
            _set(R, args[0].strip(), mem[esp]); esp += 4
        elif mn == 'ADD' and args[0].strip() == 'ESP':
            esp += int(args[1], 0)
        # memory-dest arith (SUB/ADC/SBB/ADD with [..] dest)
        elif args and args[0].strip().startswith('[') and mn in ('ADD','SUB','ADC','SBB'):
            m = mem.get(ebp + _memdisp(args[0].strip()), 0) & 0xffffffff
            d = m
            isr, sv = _opval(R, args[1], mem, ebp); s = sv & 0xffffffff
            if mn == 'ADD': tt = d + s; CF = 1 if tt > 0xffffffff else 0; r = tt & 0xffffffff
            elif mn == 'SUB': CF = 1 if d < s else 0; r = (d - s) & 0xffffffff
            elif mn == 'ADC': tt = d + s + CF; CF = 1 if tt > 0xffffffff else 0; r = tt & 0xffffffff
            elif mn == 'SBB': CF = 1 if d < (s + CF) else 0; r = (d - s - CF) & 0xffffffff
            mem[ebp + _memdisp(args[0].strip())] = r & 0xffffffff
            ZF = r == 0; SF = (r >> 31) & 1
        elif mn == 'MOV':
            da = args[0].strip()
            if da == 'EBP' and args[1].strip() == 'ESP':
                R['EBP'] = esp; ebp = esp
            elif da.startswith('['):
                isr, v = _opval(R, args[1], mem, ebp); mw(da, v)
            elif da == 'ESP':
                esp = int(args[1], 0)
            else:
                isr, v = _opval(R, args[1], mem, ebp); _set(R, da, v)
        elif mn == 'XCHG':
            ra, rb = args[0].strip(), args[1].strip()
            va, vb = _get(R, ra), _get(R, rb); _set(R, ra, vb); _set(R, rb, va)
        elif mn in ('NEG','INC','DEC'):
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            d = _get(R, reg) & m
            if mn == 'NEG': r = (0 - d) & m; CF = 1 if d != 0 else 0
            elif mn == 'INC': r = (d + 1) & m
            else: r = (d - 1) & m
            _set(R, reg, r); ZF = r == 0; SF = (r >> (n - 1)) & 1
        elif mn in ('CMP','TEST'):
            dreg, stok = args[0].strip(), args[1].strip()
            if dreg.startswith('['): dd = mem.get(ebp + _memdisp(dreg), 0) & 0xffffffff
            else: dd = _get(R, dreg)
            isr, sv = _opval(R, stok, mem, ebp)
            n = 8 if (dreg.startswith('[') and stok.lstrip('0x-').isdigit() and int(stok, 0) < 0x100) else _sz(dreg) if not dreg.startswith('[') else 32
            # size: use dest reg size; for byte TEST [mem],1 the dest is a byte but we mask to 16 below via n
            if dreg.startswith('['): n = 16
            m = (1 << n) - 1
            dd &= m; ss = sv & m
            if mn == 'TEST':
                r = (dd & ss) & m; CF = 0; OF = 0; ZF = r == 0; SF = (r >> (n - 1)) & 1
            else:
                CF = 1 if dd < ss else 0; r = (dd - ss) & m
                OF = 1 if (((dd ^ ss) & ~(dd ^ r)) >> (n - 1)) & 1 else 0
                ZF = r == 0; SF = (r >> (n - 1)) & 1
        elif mn in ('ADD','SUB','OR','AND','XOR','ADC','SBB'):
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            d = _get(R, reg) & m
            isr, sv = _opval(R, args[1], mem, ebp); s = sv & m
            if mn == 'ADD':
                t = d + s; CF = 1 if t > m else 0; r = t & m
                OF = 1 if (((d ^ s) & ~(d ^ r)) >> (n - 1)) & 1 else 0
            elif mn == 'SUB':
                CF = 1 if d < s else 0; r = (d - s) & m
                OF = 1 if (((d ^ s) & ~(d ^ r)) >> (n - 1)) & 1 else 0
            elif mn == 'OR': CF = 0; OF = 0; r = (d | s) & m
            elif mn == 'AND': CF = 0; OF = 0; r = (d & s) & m
            elif mn == 'XOR': CF = 0; OF = 0; r = (d ^ s) & m
            elif mn == 'ADC':
                t = d + s + CF; CF = 1 if t > m else 0; r = t & m
                OF = 1 if (((d ^ r) & ~(s ^ r)) >> (n - 1)) & 1 else 0
            elif mn == 'SBB':
                s2 = s + CF; CF = 1 if d < s2 else 0; r = (d - s2) & m
                OF = 1 if (((d ^ s2) & ~(d ^ r)) >> (n - 1)) & 1 else 0
            _set(R, reg, r); ZF = r == 0; SF = (r >> (n - 1)) & 1
        elif mn in ('SHL','SHR','SAR'):
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            isr, cnt = _opval(R, args[1], mem, ebp); cnt = cnt & ((n - 1) if n == 8 else 31)
            d = _get(R, reg) & m
            if mn == 'SHL': CF = (d >> (n - 1 - cnt)) & 1 if cnt else 0; r = (d << cnt) & m
            elif mn == 'SHR': CF = (d >> (cnt - 1)) & 1 if cnt else 0; r = d >> cnt
            else:
                sign = d >> (n - 1); CF = (d >> (cnt - 1)) & 1 if cnt else 0
                r = (d >> cnt) | (sign * ((1 << cnt) - 1) << (n - cnt)) if cnt else d
            _set(R, reg, r); ZF = r == 0; SF = (r >> (n - 1)) & 1; OF = 0
        elif mn == 'SHRD':
            dreg, sreg = args[0].strip(), args[1].strip()
            n = _sz(dreg); isr, cnt = _opval(R, args[2], mem, ebp); cnt = cnt & 31
            d = _get(R, dreg) & ((1 << n) - 1); s = _get(R, sreg) & ((1 << n) - 1)
            CF = (d >> (cnt - 1)) & 1 if cnt else 0
            val = ((d << n) | s) >> cnt
            _set(R, dreg, (val >> n) & ((1 << n) - 1)); _set(R, sreg, val & ((1 << n) - 1))
        elif mn in ('ROL','ROR'):
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            isr, cnt = _opval(R, args[1], mem, ebp); cnt = cnt & ((n - 1) if n == 8 else 31)
            d = _get(R, reg) & m
            if mn == 'ROL': CF = (d >> (n - 1 - cnt)) & 1 if cnt else 0; r = ((d << cnt) | (d >> (n - cnt))) & m
            else: CF = (d >> (cnt - 1)) & 1 if cnt else 0; r = ((d >> cnt) | (d << (n - cnt))) & m
            _set(R, reg, r); OF = 0
        elif mn == 'RCR':
            reg = args[0].strip(); n = _sz(reg); m = (1 << n) - 1
            isr, cnt = _opval(R, args[1], mem, ebp); cnt = cnt & ((n - 1) if n == 8 else 31)
            d = _get(R, reg) & m
            for _ in range(cnt):
                b0 = d & 1; d = ((d >> 1) | (CF << (n - 1))) & m; CF = b0
            _set(R, reg, d)
        elif mn == 'DIV':
            dreg = args[0].strip(); isr, dv = _opval(R, dreg, mem, ebp)
            dv = dv & 0xffffffff
            div = ((R['EDX'] & 0xffffffff) << 32) | (R['EAX'] & 0xffffffff)
            if dv == 0: raise DE("div by zero")
            q = div // dv
            if q > 0xffffffff: raise DE("div overflow")
            R['EAX'] = q & 0xffffffff; R['EDX'] = (div % dv) & 0xffffffff
        elif mn == 'MUL':
            dreg = args[0].strip(); isr, mv = _opval(R, dreg, mem, ebp)
            mv = mv & 0xffffffff
            prod = (R['EAX'] & 0xffffffff) * mv
            R['EDX'] = (prod >> 32) & 0xffffffff; R['EAX'] = prod & 0xffffffff
            CF = OF = 1 if R['EDX'] else 0
        else:
            raise RuntimeError("unhandled: %s at %#x" % (text, a))
        pc += 1


def _jump(mn, CF, ZF, SF, OF):
    if mn == 'JNZ': return ZF == 0
    if mn == 'JZ': return ZF == 1
    if mn == 'JNC': return CF == 0
    if mn == 'JNS': return SF == 0
    if mn == 'JC': return CF == 1
    if mn == 'JBE': return CF == 1 or ZF == 1
    if mn == 'JS': return SF == 1
    if mn == 'JG': return SF == OF and ZF == 0
    if mn == 'JGE': return SF == OF
    if mn == 'JL': return SF != OF
    if mn == 'JA': return CF == 0 and ZF == 0
    return False


def pack(a0, a1, a2, b0, b1, b2):
    """wrapper 0x047b4f register packing: EAX=a0, EDX=a1, ESI=(b2<<16|a2), ECX=b1, EBX=b0"""
    esi = ((b2 & 0xffff) << 16) | (a2 & 0xffff)
    return emulate(a0, b0, b1, a1, esi)


if __name__ == '__main__':
    print("zero:", emulate(0, 0, 0, 0, 0))
    random.seed(7)
    de = 0; ok = 0
    for _ in range(200000):
        a0 = random.getrandbits(32); a1 = random.getrandbits(32); a2 = random.getrandbits(16)
        b0 = random.getrandbits(32); b1 = random.getrandbits(32); b2 = random.getrandbits(16)
        try:
            emulate(a0, b0, b1, a1, ((b2 & 0xffff) << 16) | (a2 & 0xffff)); ok += 1
        except DE as e:
            de += 1
            if de <= 3:
                print("DE:", a0, a1, a2, b0, b1, b2, e)
    print("ran", ok, "DE:", de)
