// func #115  FUN_000376c8  000376c8  size=602


void FUN_000376c8(void)

{
  undefined4 *puVar1;
  undefined4 uStack_44;
  undefined4 uStack_40;
  undefined1 auStack_3c [16];
  int local_2c;
  int local_28;
  int local_24;
  int local_20;
  
  FUN_000418ee();
  puVar1 = (undefined4 *)auStack_3c;
  uStack_40 = 0x376e5;
  FUN_0003d88f();
  uStack_40 = 0x376ea;
  FUN_0003237b();
  uStack_40 = 0x376ef;
  FUN_0003ee0a();
  uStack_40 = 0x376f9;
  local_28 = FUN_00041bdc(0x147);
  local_24 = local_28;
  if (local_28 == 0) {
    local_20 = 0;
  }
  else {
    uStack_40 = 0;
    puVar1 = &uStack_44;
    uStack_44 = 0;
    local_2c = FUN_00010efc(local_28,0xffffffe2,0,
                            *(undefined4 *)
                             (&DAT_00052c3c +
                             *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28));
    local_20 = local_2c;
  }
  DAT_00053c7c = local_20;
  *(undefined2 *)(local_20 + 0x23) = 0;
  *(undefined1 *)(DAT_00053c7c + 0x28) = DAT_00063384;
  if (*(char *)(DAT_00053c7c + 0x28) == '\0') {
    *(uint *)(DAT_00053c7c + 0x56) = DAT_00053e9c & 0xffff;
  }
  else {
    *(uint *)(DAT_00053c7c + 0x56) =
         *(uint *)(&DAT_00052738 + (uint)*(byte *)(DAT_00053c7c + 0x28) * 4) & 0xffff;
  }
  *(undefined2 *)(DAT_00053c7c + 0x29) = DAT_00053ea4;
  DAT_00053c88 = 0;
  DAT_00053c84 = DAT_00053c7c;
  *(undefined4 *)((int)puVar1 + -4) = 0x377e3;
  FUN_00037b03(1);
  if (DAT_00063358 == 0) {
    DAT_00063354 = DAT_0006334c;
  }
  else {
    DAT_00063354 = DAT_00063358;
  }
  DAT_00062a98 = (&DAT_00062aa0)[DAT_00063378];
  DAT_00062a9c = (&DAT_00062ab4)[DAT_00063378];
  DAT_00053818 = (&DAT_00053c28)[DAT_0006337c];
  *(undefined4 *)(DAT_00053c7c + 0x123) = DAT_00053818;
  *(undefined4 *)(DAT_00053c7c + 0x2b) = DAT_00063374;
  *(undefined1 *)(DAT_00053c7c + 0x27) = (undefined1)DAT_00063378;
  *(undefined1 *)(DAT_00053c7c + 0x26) = (undefined1)DAT_0006337c;
  *(undefined1 *)(DAT_00053c7c + 0x25) = DAT_00063380;
  *(undefined4 *)((int)puVar1 + -4) = 0x3788e;
  FUN_0003cad3(DAT_00063354);
  if (DAT_00063398 == 0) {
    *(int *)(DAT_00053c7c + 1) = DAT_000639dc + -0x1e;
    *(undefined4 *)(DAT_00053c7c + 0x1b) = 0;
  }
  else {
    *(int *)(DAT_00053c7c + 1) = DAT_000639dc + 0x15e;
    *(undefined4 *)(DAT_00053c7c + 0x1b) = 1;
  }
  *(int *)(DAT_00053c7c + 9) =
       DAT_000639e0 +
       *(int *)(&DAT_00052c3c + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28);
  *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
  *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
  return;
}


