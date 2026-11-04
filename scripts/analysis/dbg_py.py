def to_i16(x):
    x &= 0xffff
    return x - 0x10000 if x >= 0x8000 else x
p0=0x12345678;p1=0x80000001;p2=0x00000000
u2=p2&0xffff;u5=p0;u3=u4=u7=u1=b8=0;i6=0
u3=p1;u7=0;u2=((u2&0x7fff)+0xc001)&0xffff;u4=u5
print(f"after-init u2={u2:04x} u3={u3:08x} u4={u4:08x} u7={u7:08x}")
if (u2&1)==0:
    print("DIV2 taken");u7=u3&1;u3=u3>>1;u4=(((u7&1)<<32)|u5)>>1&0xffffffff;u7=((1 if u5&1 else 0)<<0x1f)&0xffffffff
else:
    print("no div2")
u5=((u3&0xffffffff)|0x100000000)>>1&0xffffffff
print(f"u5(recip)={u5:08x}")
iter=0
while True:
    iter+=1;u1=(((u3<<32)|u4)//u5)&0xffffffff
    print(f"  iter{iter} u1={u1:08x} u5={u5:08x} break? {'Y' if (u5-1)<=u1 else 'N'}")
    if (u5-1)<=u1:break
    ssum=(u5+u1)&0xffffffff;c=1 if (u5+u1)>=0x100000000 else 0;u5=(((c<<32)|ssum)>>1)&0xffffffff
r=((u3<<32)|u4)%u5
print(f"r={r:08x}")
u3=(((r<<0x20)|u7)//u5)&0xffffffff
u5=(u5+u1)&0xffffffff
i6=((u5&0xffffffff)|0x100000000)>>1&0x7fffffff
u4=((( (1 if u5&1 else 0)<<32)|u3)>>1)&0xffffffff
u5=1 if u3&1 else 0
b8=1 if (u4+u5)>=0x100000000 else 0
u4=(u4+u5)&0xffffffff
print(f"FINAL u4(p0)={u4:08x} i6={i6:08x} b8={b8} -> p1={(i6+b8)&0xffffffff:08x}")
