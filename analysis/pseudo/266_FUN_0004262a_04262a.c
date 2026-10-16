// func #266  FUN_0004262a  0004262a  size=200


undefined4 FUN_0004262a(undefined4 param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  undefined4 uVar1;
  int extraout_ECX;
  int extraout_ECX_00;
  int extraout_ECX_01;
  int iVar2;
  undefined4 extraout_ECX_02;
  char extraout_DL;
  undefined8 uVar3;
  
  *(byte *)(param_4 + 0xc) = *(byte *)(param_4 + 0xc) & 0xfc;
  uVar3 = FUN_00042576(param_2);
  *(uint *)(extraout_ECX + 0xc) = *(uint *)(extraout_ECX + 0xc) | (uint)uVar3;
  FUN_00044859(*(undefined1 *)((ulonglong)uVar3 >> 0x20));
  uVar1 = FUN_0004488b();
  *(undefined4 *)(extraout_ECX_00 + 0x10) = uVar1;
  if (*(int *)(extraout_ECX_00 + 0x10) == -1) {
    FUN_00044b5d(extraout_ECX_00);
    return 0;
  }
  *(undefined4 *)(extraout_ECX_00 + 4) = 0;
  *(undefined4 *)(extraout_ECX_00 + 8) = 0;
  *(undefined4 *)(extraout_ECX_00 + 0x14) = 0;
  iVar2 = extraout_ECX_00;
  if (extraout_DL == 'a') {
    FUN_0004247a(extraout_ECX_00,0,2);
    iVar2 = extraout_ECX_01;
  }
  FUN_00044bb4(iVar2);
  return extraout_ECX_02;
}


