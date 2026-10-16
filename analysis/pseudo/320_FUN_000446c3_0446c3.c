// func #320  FUN_000446c3  000446c3  size=133


undefined4 FUN_000446c3(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  byte bVar3;
  undefined4 extraout_EDX;
  bool bVar4;
  
  uVar1 = *(uint *)(param_1 + 0xc);
  *(byte *)(param_1 + 0xc) = *(byte *)(param_1 + 0xc) & 0xcf;
  if (*(int *)(param_1 + 8) == 0) {
    FUN_00042a43(param_1);
  }
  bVar4 = (*(byte *)(param_1 + 0xd) & 4) != 0;
  if (bVar4) {
    bVar3 = *(byte *)(param_1 + 0xd) & 0xfa;
    *(byte *)(param_1 + 0xd) = bVar3;
    *(byte *)(param_1 + 0xd) = bVar3 | 1;
  }
  uVar2 = FUN_00044c23(param_1);
  if (bVar4) {
    bVar3 = *(byte *)(param_1 + 0xd) & 0xfa;
    *(byte *)(param_1 + 0xd) = bVar3;
    *(byte *)(param_1 + 0xd) = bVar3 | 4;
    FUN_00044748(param_1);
    uVar2 = extraout_EDX;
  }
  if ((*(byte *)(param_1 + 0xc) & 0x20) != 0) {
    uVar2 = 0xffffffff;
  }
  *(uint *)(param_1 + 0xc) = *(uint *)(param_1 + 0xc) | uVar1 & 0x30;
  return uVar2;
}


