#!/usr/bin/env python3
"""Emit a disasm-faithful C translation of FUN_00047b79.

Extends the fp_04794a pipeline with: a stack-memory model (PUSH/POP +
[EBP+disp] locals + ADD ESP + MOV EBP,ESP), DIV/MUL, the OF flag (for
JG/JGE/JL/JA/JS), and a 3-value return (EAX,EDX,SI). The two CALL 0x463dc
dispatches are net no-ops and are omitted from INSNS (see fp_047b79_ref.py).
The generated body is verified by differential testing against the emulator.
"""
import re
import os as _os, sys as _sys; _d=_os.path.dirname(_os.path.abspath(__file__)); _sys.path[:0]=[_d]+[_os.path.join(_d,"..",x) for x in ("ref","analysis","gen")]  # sibling analysis modules
from fp_047b79_ref import INSNS

R32 = {'EAX', 'EBX', 'ECX', 'EDX', 'ESI', 'EBP', 'EDI'}
R16 = {'AX', 'BX', 'CX', 'DX', 'SI', 'DI', 'BP'}
R8LO = {'AL', 'BL', 'CL', 'DL'}
R8HI = {'AH', 'BH', 'CH', 'DH'}
REG2C = {'EAX': 'eax', 'EBX': 'ebx', 'ECX': 'ecx', 'EDX': 'edx', 'ESI': 'esi',
         'EBP': 'ebp', 'EDI': 'edi',
         'AX': 'eax', 'BX': 'ebx', 'CX': 'ecx', 'DX': 'edx', 'SI': 'esi',
         'DI': 'edi', 'BP': 'ebp',
         'AL': 'eax', 'BL': 'ebx', 'CL': 'ecx', 'DL': 'edx',
         'AH': 'eax', 'BH': 'ebx', 'CH': 'ecx', 'DH': 'edx'}


def size(reg):
    return 32 if reg in R32 else (16 if reg in R16 else 8)


def memdisp(t):
    inner = t[1:-1].replace('EBP', '')
    if inner.startswith('+'): return int(inner[1:], 0)
    if inner.startswith('-'): return -int(inner[1:], 0)
    return int(inner, 0)


def memc(t):
    return 'stk[ebsp + %d]' % (memdisp(t) // 4)


def ismem(t):
    return t.strip().startswith('[')


def cval(tok):
    t = tok.strip()
    if t.startswith('['):
        return memc(t)
    if t.startswith('0x') or t.startswith('-0x'):
        return '((uint32_t)(%s))' % t
    if re.fullmatch(r'-?\d+', t):
        return '((uint32_t)(%s))' % t
    return REG2C[t]


def setc(tok, valc):
    t = tok.strip()
    if t.startswith('['):
        return '%s=((uint32_t)(%s))&0xffffffffu;' % (memc(t), valc)
    if t in R32:
        return '%s=((uint32_t)(%s))&0xffffffffu;' % (REG2C[t], valc)
    if t in R16:
        return '%s=(%s&0xffff0000u)|((((uint32_t)(%s))&0xffffu));' % (REG2C[t], REG2C[t], valc)
    if t in R8LO:
        return '%s=(%s&0xffffff00u)|((((uint32_t)(%s))&0xffu));' % (REG2C[t], REG2C[t], valc)
    return '%s=(%s&0xffff00ffu)|((((uint32_t)(%s))&0xffu)<<8);' % (REG2C[t], REG2C[t], valc)


def tgt(t):
    return 'l_%x' % int(t, 16)


def arith(mn, dreg, stok, n, m):
    d = cval(dreg); s = cval(stok)
    L = ['{ uint32_t _d=(%s)&%s; uint32_t _s=(%s)&%s; uint64_t _t64; uint32_t _r;' % (d, m, s, m)]
    if mn == 'ADD':
        L.append('_t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)%s)?1:0; _r=(uint32_t)(_t64&%s);'
                 ' of=(((~(_d^_s))&(_d^_r))>>%d)&1u;' % (m, m, n - 1))
    elif mn == 'SUB':
        L.append('cf=(_d<_s)?1:0; _r=(_d-_s)&%s; of=(((_d^_s)&~(_d^_r))>>%d)&1u;' % (m, n - 1))
    elif mn == 'OR':
        L.append('cf=0; of=0; _r=(_d|_s)&%s;' % m)
    elif mn == 'AND':
        L.append('cf=0; of=0; _r=(_d&_s)&%s;' % m)
    elif mn == 'XOR':
        L.append('cf=0; of=0; _r=(_d^_s)&%s;' % m)
    elif mn == 'ADC':
        L.append('_t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)%s)?1:0; '
                 '_r=(uint32_t)(_t64&%s); of=(((~(_s^_r))&(_d^_r))>>%d)&1u;' % (m, m, n - 1))
    elif mn == 'SBB':
        L.append('_t64=(uint64_t)_d-((uint64_t)_s+(uint64_t)cf); cf=(uint32_t)_d<((uint32_t)(_s+cf))?1:0; _r=(uint32_t)_t64; of=0;')
    L.append(setc(dreg, '_r'))
    L.append('zf=(_r==0)?1:0; sf=(_r>>%d)&1; }' % (n - 1))
    return '\n        '.join(L)


def emit(mn, args):
    if mn == 'JMP':
        return 'goto %s;' % tgt(args[0])
    if mn in ('JNZ', 'JZ', 'JNC', 'JNS', 'JBE', 'JC', 'JS', 'JG', 'JGE', 'JL', 'JA'):
        cond = {'JNZ': 'zf==0', 'JZ': 'zf==1', 'JNC': 'cf==0', 'JNS': 'sf==0',
                'JC': 'cf==1', 'JBE': '(cf==1||zf==1)', 'JS': 'sf==1',
                'JG': '(sf==of && zf==0)', 'JGE': '(sf==of)', 'JL': '(sf!=of)',
                'JA': '(cf==0 && zf==0)'}[mn]
        return 'if(%s) goto %s;' % (cond, tgt(args[0]))
    if mn == 'RET':
        return '*o_eax=eax; *o_edx=edx; *o_si=(uint16_t)esi; return;'
    if mn == 'PUSH':
        return 'esp--; stk[esp]=(uint32_t)(%s);' % cval(args[0])
    if mn == 'POP':
        return setc(args[0], 'stk[esp++]')
    if mn == 'MOV':
        a, b = args[0].strip(), args[1].strip()
        if a == 'EBP' and b == 'ESP':
            return 'ebsp=esp; ebp=esp;'
        if a == 'ESP':
            return 'esp=%d;' % int(b, 0)
        return setc(a, cval(b))
    if mn == 'XCHG':
        a, b = args[0].strip(), args[1].strip()
        assert a in R32 and b in R32, mn
        return '{ uint32_t _t=%s; %s %s=_t; }' % (REG2C[a], setc(a, REG2C[b]), REG2C[b])
    if mn == 'NEG':
        r = args[0].strip(); n = size(r); m = (1 << n) - 1
        return ('{ uint32_t _d=(%s)&%s; cf=(_d!=0)?1:0; of=0; uint32_t _r=(0u-_d)&%s; '
                '%s zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (cval(r), m, m, setc(r, '_r'), n - 1)
    if mn in ('INC', 'DEC'):
        r = args[0].strip(); n = size(r); m = (1 << n) - 1
        op = '+' if mn == 'INC' else '-'
        return ('{ uint32_t _d=(%s)&%s; of=0; uint32_t _r=(_d %s 1u)&%s; '
                '%s zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (cval(r), m, op, m, setc(r, '_r'), n - 1)
    if mn == 'CMP':
        a, b = args[0].strip(), args[1].strip(); n = 16 if ismem(a) else size(a); m = (1 << n) - 1
        return ('{ uint32_t _d=(%s)&%s; uint32_t _s=(%s)&%s; uint32_t _r=(_d-_s)&%s; '
                'cf=(_d<_s)?1:0; of=(((_d^_s)&~(_d^_r))>>%d)&1u; zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (
            cval(a), m, cval(b), m, m, n - 1, n - 1)
    if mn == 'TEST':
        a, b = args[0].strip(), args[1].strip(); n = 16 if ismem(a) else size(a); m = (1 << n) - 1
        return ('{ uint32_t _r=(((%s)&(%s))&%s); cf=0; of=0; zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (
            cval(a), cval(b), m, n - 1)
    if mn == 'ADD' and args[0].strip() == 'ESP':
        return 'esp += %d;' % (int(args[1], 0) // 4)
    if mn in ('ADD', 'SUB', 'OR', 'AND', 'XOR', 'ADC', 'SBB'):
        if ismem(args[0]):
            a = args[0].strip(); d = memc(a); s = cval(args[1])
            if mn == 'ADD':
                body = '_t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>0x100000000u)?1:0; _r=(uint32_t)_t64;'
            elif mn == 'SUB':
                body = 'cf=(_d<_s)?1:0; _r=(_d-_s);'
            elif mn == 'ADC':
                body = '_t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>0x100000000u)?1:0; _r=(uint32_t)_t64;'
            else:  # SBB
                body = '_t64=(uint64_t)_d-((uint64_t)_s+(uint64_t)cf); cf=(uint32_t)_d<((uint32_t)(_s+cf))?1:0; _r=(uint32_t)_t64;'
            return ('{ uint64_t _t64; uint32_t _d=(%s); uint32_t _s=(%s); uint32_t _r; %s of=0; '
                    '%s zf=(_r==0)?1:0; sf=(_r>>31)&1; }') % (d, s, body, setc(a, '_r'))
        a = args[0].strip(); n = size(a); m = (1 << n) - 1
        return arith(mn, a, args[1], n, m)
    if mn in ('SHL', 'SHR', 'SAR'):
        r = args[0].strip(); n = size(r); m = (1 << n) - 1
        d = cval(r); c = cval(args[1]); cmask = '31u' if n != 8 else '7u'
        sar = '(uint32_t)((int32_t)_d >> _c)' if n == 32 else '(uint16_t)((int16_t)_d >> _c)'
        if mn == 'SHL': body = 'cf=_c?((_d>>(%d-1u-_c))&1u):0u; _r=(_d<<_c)&%s;' % (n, m)
        elif mn == 'SHR': body = 'cf=_c?((_d>>(_c-1u))&1u):0u; _r=(_d>>_c)&%s;' % m
        else: body = 'cf=_c?((_d>>(_c-1u))&1u):0u; _r=%s;' % sar
        return ('{ uint32_t _d=(%s)&%s; uint32_t _c=(%s)&%s; of=0; uint32_t _r; %s '
                '%s zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (d, m, c, cmask, body, setc(r, '_r'), n - 1)
    if mn == 'SHRD':
        dreg, sreg = args[0].strip(), args[1].strip(); n = size(dreg); m = (1 << n) - 1
        c = cval(args[2])
        return ('{ uint32_t _d=%s; uint32_t _s=%s; uint32_t _c=(%s)&31u; uint64_t _v; of=0; '
                'cf=_c?((_d>>(_c-1u))&1u):0u; _v=((uint64_t)_d<<32)|(uint32_t)_s; _v>>=_c; '
                '%s %s }') % (cval(dreg), cval(sreg), c, setc(dreg, '((uint32_t)(_v>>32))'), setc(sreg, '((uint32_t)_v)'))
    if mn in ('ROL', 'ROR'):
        r = args[0].strip(); n = size(r); m = (1 << n) - 1
        d = cval(r); c = cval(args[1]); cmask = '31u' if n != 8 else '7u'
        if mn == 'ROL': body = 'cf=_c?((_d>>(%d-1u-_c))&1u):0u; _r=((_d<<_c)|(_d>>(%d-_c)))&%s;' % (n, n, m)
        else: body = 'cf=_c?((_d>>(_c-1u))&1u):0u; _r=((_d>>_c)|(_d<<(%d-_c)))&%s;' % (n, m)
        return ('{ uint32_t _d=(%s)&%s; uint32_t _c=(%s)&%s; of=0; uint32_t _r; %s '
                '%s zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (d, m, c, cmask, body, setc(r, '_r'), n - 1)
    if mn == 'RCR':
        r = args[0].strip(); n = size(r); m = (1 << n) - 1
        d = cval(r); c = cval(args[1]); cmask = '31u' if n != 8 else '7u'
        return ('{ uint32_t _d=(%s)&%s; uint32_t _c=(%s)&%s; int _i; uint32_t _b0; '
                'for(_i=0;_i<_c;_i++){ _b0=_d&1u; _d=((_d>>1u)|(((uint32_t)cf<<%d))&%s); cf=_b0?1:0; } '
                '%s }') % (d, m, c, cmask, n - 1, m, setc(r, '_d'))
    if mn == 'DIV':
        r = cval(args[0].strip())
        return ('{ uint32_t _dv=(%s)&0xffffffffu; uint64_t _div=((uint64_t)edx<<32)|(uint64_t)eax; '
                'eax=(uint32_t)(_div/_dv); edx=(uint32_t)(_div%%_dv); }') % r
    if mn == 'MUL':
        r = cval(args[0].strip())
        return ('{ uint64_t _p=((uint64_t)eax*((uint32_t)(%s)&0xffffffffu)); edx=(uint32_t)(_p>>32); '
                'eax=(uint32_t)_p; cf=of=(edx!=0)?1:0; }') % r
    raise RuntimeError('unhandled ' + mn)


def main():
    out = []
    out.append('#include <stdint.h>')
    out.append('/* Disasm-faithful C translation of FUN_00047b79 (pure-integer fixed-point')
    out.append(' * binary op), generated by gen_047b79_c.py from the Ghidra 177-instruction disasm,')
    out.append(' * mirroring the flag/register/stack semantics of fp_047b79_ref.py. Verified by')
    out.append(' * differential testing. Two CALL 0x463dc dispatches are net no-ops (handler')
    out.append(' * 0x042a07) and are omitted. In: EAX,EDX,ESI,ECX,EBX ; out: EAX,EDX,SI. */')
    out.append('void fn_047b79(uint32_t eax, uint32_t edx, uint32_t esi, uint32_t ecx, uint32_t ebx,')
    out.append('               uint32_t *o_eax, uint32_t *o_edx, uint16_t *o_si)')
    out.append('{')
    out.append('    uint32_t ebp=0, edi=0;')
    out.append('    int cf=0, zf=0, sf=0, of=0;')
    out.append('    uint32_t stk[128]; unsigned esp=96, ebsp=0;')
    out.append('    (void)of;')
    out.append('    goto l_%x;' % INSNS[0][0])
    for addr, text in INSNS:
        parts = text.split(None, 1)
        mn = parts[0]
        a = [x for x in (parts[1].split(',') if len(parts) > 1 else [])]
        out.append('    l_%x: { /* %s */' % (addr, text))
        out.append('        ' + emit(mn, a))
        out.append('    }')
    out.append('    return;')
    out.append('}')
    print('\n'.join(out))


if __name__ == '__main__':
    main()
