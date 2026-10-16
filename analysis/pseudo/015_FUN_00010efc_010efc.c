// func #15  FUN_00010efc  00010efc  size=1025


undefined1 *
FUN_00010efc(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined1 param_5)

{
  undefined1 *puVar1;
  int iVar2;
  undefined4 extraout_ECX;
  undefined8 uVar3;
  int in_stack_00000000;
  
  uVar3 = FUN_000418ee();
  puVar1 = (undefined1 *)uVar3;
  if (in_stack_00000000 == DAT_00063370) {
    DAT_00053c80 = puVar1;
  }
  *puVar1 = param_5;
  *(int *)(puVar1 + 1) = (int)((ulonglong)uVar3 >> 0x20);
  *(undefined4 *)(puVar1 + 5) = param_3;
  *(undefined4 *)(puVar1 + 9) = extraout_ECX;
  *(int *)(puVar1 + 0xd) = in_stack_00000000;
  *(undefined4 *)(puVar1 + 0x11) = 0;
  puVar1[0x15] = 0;
  puVar1[0x16] = 0;
  *(undefined4 *)(puVar1 + 0x17) = 0;
  *(undefined4 *)(puVar1 + 0x1b) = 0;
  *(undefined4 *)(puVar1 + 0x1f) = 0;
  puVar1[0x25] = (&DAT_00053ea0)[*(int *)(puVar1 + 0xd) * 0x88];
  puVar1[0x26] = 0;
  puVar1[0x27] = 0;
  puVar1[0x28] = 0;
  *(undefined2 *)(puVar1 + 0x29) = (&DAT_00053ea4)[*(int *)(puVar1 + 0xd) * 0x44];
  *(undefined4 *)(puVar1 + 0x2b) = 0;
  puVar1[0x30] = 0;
  puVar1[0x31] = 0;
  FUN_00041e20(puVar1 + 0x32,0,0x14);
  *(undefined4 *)(puVar1 + 0x46) = 0;
  *(undefined4 *)(puVar1 + 0x4a) = 0;
  puVar1[0x4e] = 0;
  puVar1[0x4f] = 0;
  *(undefined2 *)(puVar1 + 0x50) = 0;
  *(undefined4 *)(puVar1 + 0x52) = 0;
  *(undefined4 *)(puVar1 + 0x56) = 0;
  *(undefined2 *)(puVar1 + 0x23) = 0;
  puVar1[0x5a] = 0;
  if (in_stack_00000000 == 0) {
    *(undefined4 *)(puVar1 + 0x56) = (&DAT_00053e9c)[*(int *)(puVar1 + 0xd) * 0x22];
  }
  else {
    *(undefined2 *)(puVar1 + 0x23) = *(undefined2 *)(&DAT_00053e9c + *(int *)(puVar1 + 0xd) * 0x22);
    if ((0 < in_stack_00000000) && (in_stack_00000000 < 10)) {
      *(undefined4 *)(puVar1 + 0x56) = 0x30c;
    }
  }
  *(undefined4 *)(puVar1 + 0x123) = (&DAT_00053818)[*(int *)(puVar1 + 0xd)];
  *(undefined4 *)(puVar1 + 0x127) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 0x38);
  *(undefined4 *)(puVar1 + 299) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 4);
  *(undefined4 *)(puVar1 + 0x12f) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 8);
  *(undefined4 *)(puVar1 + 0x133) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 0x34);
  *(undefined4 *)(puVar1 + 0x137) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 0xc);
  *(undefined4 *)(puVar1 + 0x13b) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 0x10);
  if (DAT_00053c70 == (undefined1 *)0x0) {
    DAT_00053c70 = puVar1;
    DAT_00053c74 = puVar1;
    *(undefined4 *)(puVar1 + 0x143) = 0;
    *(undefined4 *)(puVar1 + 0x13f) = 0;
  }
  else {
    *(undefined1 **)((int)DAT_00053c74 + 0x13f) = puVar1;
    *(undefined1 **)(puVar1 + 0x143) = DAT_00053c74;
    *(undefined4 *)(puVar1 + 0x13f) = 0;
    DAT_00053c74 = puVar1;
  }
  DAT_00053c68 = DAT_00053c68 + 1;
  *(undefined4 *)(puVar1 + 0x8b) = 0;
  *(undefined4 *)(puVar1 + 0x8f) = 0;
  *(undefined4 *)(puVar1 + 0xab) = 0;
  *(undefined4 *)(puVar1 + 0xaf) = 4;
  *(undefined4 *)(puVar1 + 0xbb) = 0;
  *(undefined4 *)(puVar1 + 199) = 1;
  *(undefined4 *)(puVar1 + 99) = 0;
  *(undefined4 *)(puVar1 + 0xff) = 0xffffffff;
  *(undefined4 *)(puVar1 + 0x5f) = 0;
  *(undefined4 *)(puVar1 + 0x6b) = 0;
  *(undefined4 *)(puVar1 + 0x6f) = 0;
  *(undefined4 *)(puVar1 + 0x113) = 0;
  *(undefined4 *)(puVar1 + 0xd3) = 0;
  *(int *)(puVar1 + 0x73) =
       *(int *)(puVar1 + 0x56) + (int)*(short *)(puVar1 + 0x23) + (int)*(short *)(puVar1 + 0x29);
  *(undefined4 *)(puVar1 + 0x77) = *(undefined4 *)(puVar1 + 0x73);
  *(undefined4 *)(puVar1 + 0x7b) = 0;
  *(undefined4 *)(puVar1 + 0x7f) = 0;
  *(undefined4 *)(puVar1 + 0x10b) = 0;
  *(undefined4 *)(puVar1 + 0x107) = *(undefined4 *)(puVar1 + 0x10b);
  *(undefined4 *)(puVar1 + 0x103) = *(undefined4 *)(puVar1 + 0x107);
  iVar2 = FUN_00041e9f();
  *(int *)(puVar1 + 0x11f) = iVar2 % 0x14;
  return puVar1;
}


