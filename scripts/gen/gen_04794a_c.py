#!/usr/bin/env python3
"""Emit a disasm-faithful C translation of FUN_0004794a.

The generated C mirrors, instruction-by-instruction, the exact register/flag
semantics executed by fp_04794a_ref.py (the ground-truth mini x86 emulator of
the 149-instruction Ghidra disasm). Jumps become gotos to per-address labels;
each instruction is one C block. The result is verified by differential
testing against the emulator over random inputs.

Register-ABI (matches the wrappers 0x0478e2/0x0478f0/0x04791a):
    param_1=EAX, param_2=EDX, param_3=EBX, param_4=ECX, ESI (exponent).
Returns (EDX<<32)|EAX as uint64_t.
"""
import re
import os as _os, sys as _sys; _d=_os.path.dirname(_os.path.abspath(__file__)); _sys.path[:0]=[_d]+[_os.path.join(_d,"..",x) for x in ("ref","analysis","gen")]  # sibling analysis modules
from fp_04794a_ref import INSNS

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


def cvar(reg):
    return REG2C[reg]


def size(reg):
    return 32 if reg in R32 else (16 if reg in R16 else 8)


def get_c(tok):
    t = tok.strip()
    if t.startswith('0x'):
        return '0x%su' % t[2:]
    if re.fullmatch(r'-?\d+', t):
        return '((uint32_t)(%s))' % t
    reg = t
    c = cvar(reg)
    if reg in R32:
        return c
    if reg in R16:
        return '((uint16_t)(' + c + '))'
    if reg in R8LO:
        return '((uint8_t)(' + c + '))'
    if reg in R8HI:
        return '((uint8_t)(((' + c + '))>>8))'
    raise RuntimeError('bad operand ' + t)


def set_c(tok, valc):
    t = tok.strip()
    c = cvar(t)
    if t in R32:
        return c + '=((uint32_t)(' + valc + '))&0xffffffffu;'
    if t in R16:
        return c + '=(' + c + '&0xffff0000u)|(((uint32_t)(' + valc + '))&0xffffu);'
    if t in R8LO:
        return c + '=(' + c + '&0xffffff00u)|(((uint32_t)(' + valc + '))&0xffu);'
    if t in R8HI:
        return c + '=(' + c + '&0xffff00ffu)|((((uint32_t)(' + valc + '))&0xffu)<<8);'
    raise RuntimeError('bad reg ' + t)


def tgt(t):
    return 'l_%x' % int(t, 16)


def arith(mn, dreg, stok, n, m):
    d = get_c(dreg)
    s = get_c(stok)
    L = ['{ uint32_t _d=(%s)&%s; uint32_t _s=(%s)&%s; uint64_t _t64; uint32_t _r;' % (d, m, s, m)]
    if mn == 'ADD':
        L.append('_t64=(uint64_t)_d+(uint64_t)_s; cf=(_t64>(uint64_t)%s)?1:0; _r=(uint32_t)(_t64&%s);' % (m, m))
    elif mn == 'SUB':
        L.append('cf=(_d<_s)?1:0; _r=(_d-_s)&%s;' % m)
    elif mn == 'OR':
        L.append('cf=0; _r=(_d|_s)&%s;' % m)
    elif mn == 'AND':
        L.append('cf=0; _r=(_d&_s)&%s;' % m)
    elif mn == 'XOR':
        L.append('cf=0; _r=(_d^_s)&%s;' % m)
    elif mn == 'ADC':
        L.append('_t64=(uint64_t)_d+(uint64_t)_s+(uint64_t)cf; cf=(_t64>(uint64_t)%s)?1:0; _r=(uint32_t)(_t64&%s);' % (m, m))
    elif mn == 'SBB':
        L.append('cf=(_d<(_s+cf))?1:0; _r=(_d-_s-cf)&%s;' % m)
    L.append(set_c(dreg, '_r'))
    L.append('zf=(_r==0)?1:0; sf=(_r>>%d)&1; }' % (n - 1))
    return '\n'.join(L)


def emit(mn, args):
    if mn in ('JNZ', 'JZ', 'JNC', 'JNS', 'JBE', 'JC'):
        cond = {'JNZ': 'zf==0', 'JZ': 'zf==1', 'JNC': 'cf==0', 'JNS': 'sf==0',
                'JC': 'cf==1', 'JBE': '(cf==1||zf==1)'}[mn]
        return 'if(%s) goto %s;' % (cond, tgt(args[0]))
    if mn == 'JMP':
        return 'goto %s;' % tgt(args[0])
    if mn == 'RET':
        return 'return ((uint64_t)edx<<32)|(uint64_t)eax;'
    if mn == 'STC':
        return 'cf=1;'
    if mn == 'PUSH':
        return 'stk[sp++]=%s;' % get_c(args[0])
    if mn == 'POP':
        return set_c(args[0], 'stk[--sp]')
    if mn == 'MOV':
        return set_c(args[0], get_c(args[1]))
    if mn == 'XCHG':
        a = args[0].strip(); b = args[1].strip()
        assert size(a) == 32 and size(b) == 32, mn
        return '{ uint32_t _t=%s; %s=%s; %s=_t; }' % (cvar(a), cvar(a), get_c(b), cvar(b))
    if mn in ('NEG',):
        r = args[0].strip(); n = size(r); m = (1 << n) - 1
        d = get_c(r)
        return ('{ uint32_t _d=(%s)&%s; cf=(_d!=0)?1:0; uint32_t _r=(0u-_d)&%s; '
                '%s zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (
                    d, m, m, set_c(r, '_r'), n - 1)
    if mn in ('INC', 'DEC'):
        r = args[0].strip(); n = size(r); m = (1 << n) - 1
        d = get_c(r)
        op = '+' if mn == 'INC' else '-'
        return ('{ uint32_t _d=(%s)&%s; uint32_t _r=(_d %s 1u)&%s; '
                '%s zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (d, m, op, m, set_c(r, '_r'), n - 1)
    if mn == 'SETNZ':
        return set_c(args[0].strip(), '(zf?0u:1u)')
    if mn == 'CMP':
        a = args[0].strip(); b = args[1].strip(); n = size(a); m = (1 << n) - 1
        return ('{ uint32_t _d=(%s)&%s; uint32_t _s=(%s)&%s; uint32_t _r=(_d-_s)&%s; '
                'cf=(_d<_s)?1:0; zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (
                    get_c(a), m, get_c(b), m, m, n - 1)
    if mn == 'TEST':
        a = args[0].strip(); b = args[1].strip(); n = size(a); m = (1 << n) - 1
        return ('{ uint32_t _r=(((%s)&(%s))&%s); cf=0; zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (
            get_c(a), get_c(b), m, n - 1)
    if mn in ('ADD', 'SUB', 'OR', 'AND', 'XOR', 'ADC', 'SBB'):
        a = args[0].strip(); n = size(a); m = (1 << n) - 1
        return arith(mn, a, args[1], n, m)
    if mn in ('SHL', 'SHR', 'SAR'):
        r = args[0].strip(); n = size(r); m = (1 << n) - 1
        d = get_c(r); c = get_c(args[1])
        cmask = '31u' if n != 8 else '7u'
        if n == 32:
            sar = '(uint32_t)((int32_t)_d >> _c)'
        else:
            sar = '(uint16_t)((int16_t)_d >> _c)'
        if mn == 'SHL':
            body = 'cf=_c?((_d>>(%d-1u-_c))&1u):0u; _r=(_d<<_c)&%s;' % (n, m)
        elif mn == 'SHR':
            body = 'cf=_c?((_d>>(_c-1u))&1u):0u; _r=(_d>>_c)&%s;' % m
        else:
            body = 'cf=_c?((_d>>(_c-1u))&1u):0u; _r=%s;' % sar
        return ('{ uint32_t _d=(%s)&%s; uint32_t _c=(%s)&%s; uint32_t _r; %s '
                '%s zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (d, m, c, cmask, body, set_c(r, '_r'), n - 1)
    if mn == 'SHRD':
        dreg = args[0].strip(); sreg = args[1].strip(); n = size(dreg); m = (1 << n) - 1
        c = get_c(args[2])
        assert n == 32, 'only 32-bit SHRD in this function'
        return ('{ uint32_t _d=%s; uint32_t _s=%s; uint32_t _c=(%s)&31u; uint64_t _v; '
                'cf=_c?((_d>>(_c-1u))&1u):0u; _v=((uint64_t)_d<<32)|(uint32_t)_s; _v>>=_c; '
                '%s %s }') % (get_c(dreg), get_c(sreg), c, set_c(dreg, '((uint32_t)(_v>>32))'),
                              set_c(sreg, '((uint32_t)_v)'))
    if mn in ('ROL', 'ROR'):
        r = args[0].strip(); n = size(r); m = (1 << n) - 1
        d = get_c(r); c = get_c(args[1])
        cmask = '31u' if n != 8 else '7u'
        if mn == 'ROL':
            body = 'cf=_c?((_d>>(%d-1u-_c))&1u):0u; _r=((_d<<_c)|(_d>>(%d-_c)))&%s;' % (n, n, m)
        else:
            body = 'cf=_c?((_d>>(_c-1u))&1u):0u; _r=((_d>>_c)|(_d<<(%d-_c)))&%s;' % (n, m)
        return ('{ uint32_t _d=(%s)&%s; uint32_t _c=(%s)&%s; uint32_t _r; %s '
                '%s zf=(_r==0)?1:0; sf=(_r>>%d)&1; }') % (d, m, c, cmask, body, set_c(r, '_r'), n - 1)
    if mn == 'RCR':
        r = args[0].strip(); n = size(r); m = (1 << n) - 1
        d = get_c(r); c = get_c(args[1])
        cmask = '31u' if n != 8 else '7u'
        return ('{ uint32_t _d=(%s)&%s; uint32_t _c=(%s)&%s; int _i; uint32_t _b0; '
                'for(_i=0;_i<_c;_i++){ _b0=_d&1u; _d=((_d>>1u)|(((uint32_t)cf<<%d))&%s); cf=_b0?1:0; } '
                '%s }') % (d, m, c, cmask, n - 1, m, set_c(r, '_d'))
    raise RuntimeError('unhandled ' + mn)


def main():
    out = []
    out.append('#include <stdint.h>')
    out.append('/* Disasm-faithful C translation of FUN_0004794a, generated by')
    out.append(' * scratch/gen_04794a_c.py from the Ghidra 149-instruction disasm, mirroring')
    out.append(' * the flag/register semantics of the ground-truth emulator')
    out.append(' * (scratch/fp_04794a_ref.py). Verified by differential testing against it.')
    out.append(' * Register-ABI: EAX,EDX in (low:high 64-bit), EBX,ECX,ESI (exponent).')
    out.append(' * Returns (EDX<<32)|EAX. */')
    out.append('uint64_t fn_04794a(uint32_t eax, uint32_t edx, uint32_t ebx, uint32_t ecx, uint32_t esi)')
    out.append('{')
    out.append('    uint32_t ebp=0, edi=0;')
    out.append('    int cf=0, zf=0, sf=0;')
    out.append('    uint32_t stk[24]; int sp=0;')
    out.append('    (void)stk; (void)sp;')
    out.append('    goto l_%x;  /* fall through into first instruction */' % INSNS[0][0])
    for addr, text in INSNS:
        parts = text.split(None, 1)
        mn = parts[0]
        args = [a for a in (parts[1].split(',') if len(parts) > 1 else [])]
        out.append('l_%x: { /* %s */' % (addr, text))
        out.append('    ' + emit(mn, args))
        out.append('}')
    out.append('    return 0;')
    out.append('}')
    print('\n'.join(out))


if __name__ == '__main__':
    main()
