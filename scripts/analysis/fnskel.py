"""Condensed skeleton of a function: calls (+the register/imm loads right before each), DS globals
touched, entity-field offsets touched, stack locals, jump table. Usage: fnskel.py <va-hex> [size]"""
import sys, re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
D=0x2600
va=int(sys.argv[1],16); size=int(sys.argv[2],16) if len(sys.argv)>2 else 0
d=open('NIGHT/NIGHT.EXE','rb').read()
if not size:
    import json
    size=None
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
if size is None:
    # fall back: scan the STATUS table for the size
    st=open('docs/12-functions/STATUS.md').read()
    m=re.search(r'\| 0x%06x \| (\d+) \|' % va, st); size=int(m.group(1))
code=d[va-D:va-D+size]
ins=list(md.disasm(code,va))
print("=== 0x%06x  size %d  insns %d ===" % (va,size,len(ins)))
globals_, calls, stacks, fields = {}, [], set(), {}
pend=[]
for i in ins:
    for op in i.operands:
        if op.type==3 and 0x1000<=op.mem.base==0 if False else False: pass
    t=i.mnemonic
    if t=='call' and re.fullmatch(r'0x[0-9a-f]+', i.op_str or ''):
        calls.append((i.address,int(i.op_str,16),list(pend)[-6:])); pend=[]
    elif t in ('mov','cmp','add','sub','and','or','lea','movzx','movsx','test','mov','not','neg','imul','inc','dec','xor','shl','shr','sar','sbb','adc','push','jmp','jcc','setne','sete','fst','fld'):
        if re.search(r'dword ptr \[0x[0-9a-f]+\]|byte ptr \[0x[0-9a-f]+\]|word ptr \[0x[0-9a-f]+\]', i.op_str or ''):
            globals_[i.address]=i.mnemonic+' '+i.op_str
        elif 'ebp' in (i.op_str or ''):
            m=re.findall(r'\[ebp (- 0x[0-9a-f]+|\+ 0x[0-9a-f]+)\]', i.op_str or '')
            for x in m: stacks.add(x.replace(' ',''))
        else:
            m=re.findall(r'\[(e[a-z]|edi|esi|ebx|ecx|edx|eax) ([-+] 0x[0-9a-f]+)\]', i.op_str or '')
            for r,o in m: fields.setdefault(r.strip()+o.replace(' ',''),[]).append(i.address)
    if t in ('mov','lea','and','or','xor','movzx','movsx','imul','add','sub','cmp'):
        pend.append('%x %s %s'%(i.address,i.mnemonic,i.op_str))
    else:
        if t not in ('call',): pend=[]
print("\n-- calls (%d) --" % len(calls))
for a,tgt,pre in calls:
    print(" %x -> 0x%06x   prep: %s" % (a,tgt,'; '.join(p.split(' ',2)[-1] if False else p.split(' ',1)[1][:26] for p in pre)))
print("\n-- DS globals (%d) --" % len(globals_))
for a,o in sorted(globals_.items()): print(" %x  %s" % (a,o))
print("\n-- register-relative offsets (top 40) --")
for k,v in sorted(fields.items(), key=lambda kv:-len(kv[1]))[:40]: print("  %-14s x%d" % (k,len(v)))
print("\n-- stack locals --", ' '.join(sorted(stacks)))
print("\n-- jump tables / indirect --")
for i in ins:
    if i.mnemonic in ('jmp','call') and 'ptr' in (i.op_str or ''): print("  %x  %s %s"%(i.address,i.mnemonic,i.op_str))
