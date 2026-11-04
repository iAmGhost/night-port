"""
fp_0487b0_ref.py -- Python differential reference for fn_0487b0
(0x0487b0, 464B, 158 insns, L2). In-place: fn_0487b0(a), a={lo,hi,exp(16b)}.

Trace-faithful model of the exact disassembly (scratch/dis_0487b0.txt).
16 callee calls: r94a x6, rb79 x2, rd4e x5, f4c x2, f0e x1. ESI tracked as full
32-bit (callee juggling + [ESP+n] reads depend on it).

Callee register ABIs (verified against clean disasm):
  r94a = fn_04794a(eax,edx,ebx,ecx,esi) -> (lo,hi);      ESI PRESERVED
  rb79 = fn_047b79(eax,edx,esi,ecx,ebx) -> (o_lo,o_hi,ESI_full32); clobbers ESI (MOV ESI,EDI)
  rd4e = fn_047d4e(a_lo,a_hi,b_lo,b_hi,esi) -> (lo,hi,exp);  clobbers ESI (returns normalized exp)
  f4c  = fn_048f4c(mant_lo,mant_hi,count,arr,exp_base) -> (lo,hi,ESI_final); clobbers ESI
  f0e  = fn_047f0e = fx_from_s32: EAX=v, EDX=out -> fx_normalize(out, v/-v, 0x3fff/0xbfff)

Branch (0x0487ea-8801): if-branch IFF (a.hi==0xb504f333 AND a.lo<=0xf9de6484)
  [a.exp check always true: a.exp is forced to 0x3ffe at 0x0487d0].
flagword = a.orig_exp - 0x3ffe (else) / a.orig_exp - 0x3fff (if, DEC [ESP] at 0x048826).
  It is PUSH 0x0487cf / POP 0x0488f7; JZ 0x048972 taken iff flagword==0 (skip f0e block).
"""
import fp_04794a_ref as r94a_mod
import fp_047b79_ref as rb79_mod
import fp_047d4e_ref as rd4e_mod

M32 = 0xffffffff
def m32(x): return x & M32
def rol16(x): return ((x << 16) | (x >> 16)) & M32

def _r94a(eax, edx, ebx, ecx, esi):
    lo, hi = r94a_mod.emulate(m32(eax), m32(ebx), m32(ecx), m32(edx), m32(esi))
    return m32(lo), m32(hi)                       # ESI preserved

def _rb79(eax, edx, esi, ecx, ebx):
    o_lo, o_hi, o_esi = rb79_mod.emulate(m32(eax), m32(ebx), m32(ecx), m32(edx), m32(esi))
    return m32(o_lo), m32(o_hi), m32(o_esi)       # full 32-bit post-call ESI

def _rd4e(a_lo, a_hi, b_lo, b_hi, esi):
    r = rd4e_mod.ref(m32(a_lo), m32(a_hi), m32(b_lo), m32(b_hi), m32(esi))
    return m32(r[0]), m32(r[1]), m32(r[2])        # ESI = returned exp (32-bit)

def _fx_normalize(v, expbase):
    v = m32(v)
    if v == 0:
        return (0, 0, 0)
    p = v.bit_length() - 1                        # BSR: MSB bit position (== fx_normalize 31-__builtin_clz)
    norm = (v << (31 - p)) & M32
    return (norm, norm, (p + expbase) & 0xffff)   # lo=hi=norm, exp=p+base

def _f0e(v):
    v = m32(v)
    if (v & 0x80000000) == 0:                     # JNS: non-negative
        return _fx_normalize(v, 0x3fff)
    return _fx_normalize(m32(-v), 0xbfff)         # NEG EAX; base|0x8000

# Coefficient tables -- 10-byte fx128 entries [lo,hi,exp] from NIGHT.EXE
# (file offset = RVA - 0x2600). 0x4872c: 4 entries (count=3 -> arr[0..3]);
# 0x4870e: 3 entries (count=2 -> arr[0..2]).
TBL_4872c = [
    (0x00000000, 0x80000000, 0x3fff),
    (0x3e7076bb, 0x8eac025b, 0xc004),
    (0xa933ef60, 0x9c041fd0, 0x4007),
    (0x6c83bb96, 0xc05ff4e0, 0xc008),
]
TBL_4870e = [
    (0xb5e946e9, 0xca20ad9a, 0xbffe),
    (0xb57f6509, 0x83125100, 0x4003),
    (0x9dacd228, 0x803ff895, 0xc005),
]

def _f4c(mant_lo, mant_hi, count, arr, exp_base):
    EAX, EDX = arr[0][0], arr[0][1]
    ESI = m32(arr[0][2])
    E0 = exp_base & 0xffff
    for i in range(count):
        ESI = rol16(ESI)
        ESI = (ESI & 0xffff0000) | E0             # MOV SI, E0
        ESI = rol16(ESI)
        r = _rd4e(EAX, EDX, mant_lo, mant_hi, ESI)
        EAX, EDX = r[0], r[1]
        ESI = rol16(r[2])
        ESI = (ESI & 0xffff0000) | (arr[i + 1][2] & 0xffff)   # MOV SI, [ESI+8]
        ESI = rol16(ESI)
        r = _r94a(EAX, EDX, arr[i + 1][0], arr[i + 1][1], ESI)
        EAX, EDX = r[0], r[1]
    return m32(EAX), m32(EDX), m32(ESI)           # ESI = final (clobbered)

# juggle(esi, src): ROL ESI,16; MOV SI,src_low16; ROL ESI,16
def _juggle(esi, src):
    esi = rol16(esi)
    esi = m32((esi & 0xffff0000) | (src & 0xffff))
    return rol16(esi)

def ref_0487b0(lo, hi, exp):
    orig_lo, orig_hi, orig_exp = m32(lo), m32(hi), exp & 0xffff
    a = [orig_lo, orig_hi, orig_exp]
    # stack (st[0] == top == [ESP+0]); 5 entry-saves are placeholders
    st = []
    def push(x): st.insert(0, m32(x))
    def pop():   return st.pop(0)
    for _ in range(5): push(0)                    # 0x0487b0-74 PUSH EDI,ESI,EDX,ECX,EBX

    # ---- 0x0487b5-0x0487cf: flagword = orig_exp - 0x3ffe; a.exp <- 0x3ffe ----
    flagword = m32(orig_exp - 0x3ffe)
    push(flagword)                                 # 0x0487cf PUSH ECX
    a[2] = 0x3ffe                                  # 0x0487d0 MOV [EDI+8], DX

    # ---- 0x0487d4-0x0487e5: R = r94a(a.lo,a.hi,0,0x80000000,0xbffe3ffe) ----
    ESI = 0xbffe3ffe
    Rlo, Rhi = _r94a(a[0], a[1], 0, 0x80000000, ESI)

    # ---- 0x0487ea-0x048826: branch ----
    branch = "if" if (orig_hi == 0xb504f333 and orig_lo <= 0xf9de6484) else "else"
    if branch == "if":
        a = [Rlo, Rhi, 0x3ffe]                     # 0x04881d-8822 write a = R
        flagword = m32(flagword - 1)               # 0x048826 DEC [ESP]
        st[0] = flagword
    else:
        ESI = m32((ESI & 0xffff) | 0xbffe0000)     # 0x048803-09 = 0xbffe3ffe
        Rlo, Rhi = _r94a(a[0], a[1], 0, 0x80000000, ESI)  # 0x048816 (same R)
        ESI = 0xbffe3ffe                           # (a unchanged)

    # ---- 0x048829-0x04882b: PUSH Rlo, Rhi, ESI ----
    push(Rlo); push(Rhi); push(ESI)
    # ---- 0x048831-0x04884a: t2 = r94a(a.lo,a.hi,0,0x80000000,0x3ffe3ffd) ----
    t2lo, t2hi = _r94a(a[0], a[1], 0, 0x80000000, 0x3ffe3ffd)
    # ---- 0x04884f-0x048857: ESI = 0x3ffd3ffe (SHL,16; POP; MOV SI,AX) ----
    ESI = m32(0x3ffe3ffd << 16)                    # = 0x3ffd0000
    ECX, EBX = t2hi, t2lo
    EAX = pop()                                    # 0xbffe3ffe
    ESI = m32((ESI & 0xffff0000) | (EAX & 0xffff)) # = 0x3ffd3ffe
    EDX, EAX = pop(), pop()                        # Rhi, Rlo
    # ---- 0x04885c: o = rb79(Rlo,Rhi,0x3ffd3ffe,t2hi,t2lo) ----
    olo, ohi, oesi = _rb79(EAX, EDX, ESI, ECX, EBX)

    # ---- 0x048861-0x048870: square o (rd4e#1) ----
    push(olo); push(ohi); push(oesi)               # 0x048861-63
    ESI = m32(((oesi & 0xffff) << 16) | (oesi & 0xffff))   # 0x048864-69
    sqlo, sqhi, sqexp = _rd4e(olo, ohi, olo, ohi, ESI)     # 0x048870
    a = [sqlo, sqhi, sqexp & 0xffff]               # 0x048875-7a

    # ---- 0x04887e-0x048889: f4c#1 = f4c(a, 3, tbl_4872c, a.exp) ----
    f4c1lo, f4c1hi, f4c1esi = _f4c(a[0], a[1], 3, TBL_4872c, a[2])
    push(f4c1lo); push(f4c1hi); push(f4c1esi)      # 0x04888e-90
    EAX, EDX = a[0], a[1]                          # 0x048891-93
    ESI = m32((f4c1esi & 0xffff0000) | (a[2] & 0xffff))    # 0x048896
    # ---- 0x04889a-0x0488a5: f4c#2 = f4c(a, 2, tbl_4870e, a.exp) ----
    f4c2lo, f4c2hi, f4c2esi = _f4c(EAX, EDX, 2, TBL_4870e, a[2])
    ECX = pop()                                    # 0x0488aa f4c1esi
    ESI = _juggle(f4c2esi, ECX)                    # 0x0488ab-ae-b1
    ECX, EBX = pop(), pop()                        # 0x0488b4-b5 f4c1hi, f4c1lo
    # ---- 0x0488b6: o2 = rb79(f4c2, ESI, f4c1hi, f4c1lo) ----
    o2lo, o2hi, o2esi = _rb79(f4c2lo, f4c2hi, ESI, ECX, EBX)

    # ---- 0x0488bb-0x0488ca: m2 = rd4e(o2, a, juggle(o2esi,a.exp)) ----
    EBX, ECX = a[0], a[1]
    ESI = _juggle(o2esi, a[2])
    m2lo, m2hi, m2exp = _rd4e(o2lo, o2hi, EBX, ECX, ESI)
    # ---- 0x0488cf-0x0488e1: m3 = rd4e(m2, o, juggle(m2exp,oesi)) ----
    ESI = _juggle(m2exp, st[0])                    # 0x0488cf-d6 (MOV SI,[ESP])
    ECX, EBX = st[1], st[2]                        # 0x0488d9/dd: ohi, olo
    m3lo, m3hi, m3exp = _rd4e(m2lo, m2hi, EBX, ECX, ESI)
    # ---- 0x0488e6-0x0488f2: r3 = r94a(m3, o, juggle(m3exp,oesi)) ----
    ESI = rol16(m3exp)                             # 0x0488e6
    ECX = pop()                                    # 0x0488e9 oesi (P7)
    ESI = m32((ESI & 0xffff0000) | (ECX & 0xffff)) # 0x0488ea
    ESI = rol16(ESI)                               # 0x0488ed
    ECX, EBX = pop(), pop()                        # 0x0488f0-f1 ohi, olo
    r3lo, r3hi = _r94a(m3lo, m3hi, EBX, ECX, ESI)  # 0x0488f2

    # ---- 0x0488f7-0x0488fa: POP flagword; JZ ----
    fw = pop()
    if fw == 0:
        a = [r3lo, r3hi, ESI & 0xffff]             # 0x048972-77
        return {"lo": a[0], "hi": a[1], "exp": a[2], "branch": branch, "fw": 0, "path": "jz"}
    # ---- 0x0488fc-0x04896d: f0e block ----
    push(r3lo); push(r3hi); push(ESI)              # 0x0488fc-fe
    a = list(_f0e(fw))                             # 0x048903 f0e -> a
    ESI = m32(m32(a[2] & 0xffff) | 0xbff20000)     # 0x04890d-17
    c1lo, c1hi, c1exp = _rd4e(a[0], a[1], 0x0865435c, 0xde8082e3, ESI)  # 0x048927
    ECX = pop()                                    # 0x04892c ESI_p3
    ESI = _juggle(c1exp, ECX)                      # 0x04892d-33
    ECX, EBX = pop(), pop()                        # 0x048936-37 r3hi, r3lo
    r4lo, r4hi = _r94a(c1lo, c1hi, EBX, ECX, ESI)  # 0x048938
    push(r4lo); push(r4hi); push(ESI)              # 0x04893d-3f
    ESI = m32(m32(a[2] & 0xffff) | 0x3ffe0000)     # 0x048945-4f
    c2lo, c2hi, c2exp = _rd4e(a[0], a[1], 0, 0xb1800000, ESI)  # 0x04895c
    ECX = pop()                                    # 0x048961 ESI_p4
    ESI = _juggle(c2exp, ECX)                      # 0x048962-68
    ECX, EBX = pop(), pop()                        # 0x04896b-6c r4hi, r4lo
    r5lo, r5hi = _r94a(c2lo, c2hi, EBX, ECX, ESI)  # 0x04896d
    a = [r5lo, r5hi, ESI & 0xffff]                 # 0x048972-77
    return {"lo": a[0], "hi": a[1], "exp": a[2], "branch": branch, "fw": fw, "path": "f0e"}

if __name__ == "__main__":
    import random, sys
    random.seed(int(sys.argv[1], 0) if len(sys.argv) > 1 else 1)
    for i in range(30):
        lo = random.getrandbits(32); hi = random.getrandbits(32)
        exp = random.randrange(0x0000, 0x4000)
        r = ref_0487b0(lo, hi, exp)
        print("in lo=%08x hi=%08x exp=%04x -> lo=%08x hi=%08x exp=%04x [%s/%s]" %
              (lo, hi, exp, r["lo"], r["hi"], r["exp"], r["branch"], r["path"]))
