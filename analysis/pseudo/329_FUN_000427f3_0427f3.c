// func #272  FUN_000427f3  000427f3  size=105


void FUN_000427f3(undefined4 param_1,uint param_2)

{
  undefined1 uVar1;
  uint uVar2;
  undefined1 *extraout_ECX;
  int extraout_ECX_00;
  int extraout_ECX_01;
  int extraout_ECX_02;
  undefined1 *puVar3;
  undefined1 *extraout_EDX;
  
  uVar2 = FUN_00044be6();
  *extraout_ECX = 0x74;
  puVar3 = extraout_ECX + 4;
  do {
    uVar1 = FUN_000427e7(uVar2 & 0xf,puVar3);
    *extraout_EDX = uVar1;
    puVar3 = extraout_EDX + -1;
    uVar2 = uVar2 >> 4;
  } while (puVar3 != extraout_ECX);
  *(undefined1 *)(extraout_ECX_00 + 5) = 0x5f;
  uVar1 = FUN_000427e7((int)param_2 >> 4 & 0xf);
  *(undefined1 *)(extraout_ECX_01 + 6) = uVar1;
  uVar1 = FUN_000427e7(param_2 & 0xf);
  *(undefined1 *)(extraout_ECX_02 + 8) = 0x2e;
  *(undefined1 *)(extraout_ECX_02 + 9) = 0x74;
  *(undefined1 *)(extraout_ECX_02 + 10) = 0x6d;
  *(undefined1 *)(extraout_ECX_02 + 0xb) = 0x70;
  *(undefined1 *)(extraout_ECX_02 + 0xc) = 0;
  *(undefined1 *)(extraout_ECX_02 + 7) = uVar1;
  return;
}


