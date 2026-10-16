// func #134  FUN_0003ace9  0003ace9  size=421


void FUN_0003ace9(void)

{
  undefined4 uVar1;
  int *piVar2;
  int local_20;
  
  FUN_000418ee();
  piVar2 = &local_20;
  for (local_20 = 0; local_20 < 0x14; local_20 = local_20 + 1) {
    *(undefined4 *)((int)piVar2 + -4) = 0x3ad2f;
    FUN_00041e6c(DAT_000633b4,DAT_000633b0,64000);
    *(undefined4 *)((int)piVar2 + -4) = 0;
    *(int *)((int)piVar2 + -8) =
         *(int *)(DAT_000639c4 + 0x38) + *(int *)(*(int *)(DAT_000639c4 + 0x34) + 0x2c);
    *(undefined4 *)((int)piVar2 + -0xc) = *(undefined4 *)(*(int *)(DAT_000639c4 + 8) + 0x2c);
    uVar1 = *(undefined4 *)(*(int *)(DAT_000639c4 + 4) + 0x2c);
    *(undefined4 *)((int)piVar2 + -0x10) = 0x3ad75;
    FUN_0003bb42(DAT_000633b4,local_20 * 5 + -0x34,0x35,uVar1);
    *(undefined4 *)((int)piVar2 + -0x10) = 0;
    *(int *)((int)piVar2 + -0x14) =
         *(int *)(DAT_000639c4 + 0x38) + *(int *)(*(int *)(DAT_000639c4 + 0x34) + 0x30);
    *(undefined4 *)((int)piVar2 + -0x18) = *(undefined4 *)(*(int *)(DAT_000639c4 + 8) + 0x30);
    uVar1 = *(undefined4 *)(*(int *)(DAT_000639c4 + 4) + 0x30);
    *(undefined4 *)((int)piVar2 + -0x1c) = 0x3adc0;
    FUN_0003bb42(DAT_000633b4,local_20 * -5 + 0x115,0x35,uVar1);
    *(undefined4 *)((int)piVar2 + -0x1c) = 0x3adc5;
    FUN_00035ec2();
    *(undefined4 *)((int)piVar2 + -0x1c) = 0x3adda;
    FUN_00041e6c(&DAT_000a0000,DAT_000633b4,64000);
    piVar2 = (int *)((int)piVar2 + -0x18);
  }
  for (local_20 = 0; local_20 < 0x400; local_20 = local_20 + 1) {
    *(undefined4 *)((int)piVar2 + -4) = 0x3ae01;
    thunk_FUN_0004564f(1);
  }
  *(undefined4 *)((int)piVar2 + -4) = 0;
  *(int *)((int)piVar2 + -8) =
       *(int *)(DAT_000639c4 + 0x38) + *(int *)(*(int *)(DAT_000639c4 + 0x34) + 0x2c);
  *(undefined4 *)((int)piVar2 + -0xc) = *(undefined4 *)(*(int *)(DAT_000639c4 + 8) + 0x2c);
  uVar1 = *(undefined4 *)(*(int *)(DAT_000639c4 + 4) + 0x2c);
  *(undefined4 *)((int)piVar2 + -0x10) = 0x3ae44;
  FUN_0003bb42(DAT_000633b0,0x30,0x35,uVar1);
  *(undefined4 *)((int)piVar2 + -0x10) = 0;
  *(int *)((int)piVar2 + -0x14) =
       *(int *)(DAT_000639c4 + 0x38) + *(int *)(*(int *)(DAT_000639c4 + 0x34) + 0x30);
  *(undefined4 *)((int)piVar2 + -0x18) = *(undefined4 *)(*(int *)(DAT_000639c4 + 8) + 0x30);
  uVar1 = *(undefined4 *)(*(int *)(DAT_000639c4 + 4) + 0x30);
  *(undefined4 *)((int)piVar2 + -0x1c) = 0x3ae85;
  FUN_0003bb42(DAT_000633b0,0xb1,0x35,uVar1);
  return;
}


