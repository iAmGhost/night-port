import sys, collections
from capstone import *
from capstone.x86_const import *
EXE='NIGHT/NIGHT.EXE'; D=0x2600
va=int(sys.argv[1],16); n=int(sys.argv[2],0)
code=open(EXE,'rb').read()[va-D:va-D+n]
md=Cs(CS_ARCH_X86,CS_MODE_32); md.detail=True
base=va; calls=[]; globs=collections.OrderedDict(); branches=[]
for ins in md.disasm(code, base):
    for op in ins.operands:
        if op.type==X86_OP_MEM and op.mem.base==0 and op.mem.disp>0x1000:
            d=op.mem.disp
            if d>=0x10000: globs.setdefault('DAT_%06X'%(d+0x50000), []).append(ins.address)
    if ins.mnemonic=='call':
        calls.append((ins.address, ins.op_str))
    if ins.mnemonic.startswith('j'):
        try: t=int(ins.op_str,16)
        except: continue
        branches.append((ins.address, ins.mnemonic, t, 'BACK' if t<=ins.address else 'fwd'))
print('== calls ==')
for a,o in calls: print(' %06X -> %s'%(a,o))
print('== globals ==')
for g,addrs in globs.items(): print(' %-14s refs@ %s'%(g, ' '.join('%x'%a for a in addrs[:8])))
print('== backward jumps (loops) ==')
for a,m,t,d in branches:
    if d=='BACK': print(' %06X %s -> %06X'%(a,m,t))
print('== all jumps ==')
print(' '.join('%s@%x'%(m,a) for a,m,t,d in branches))
