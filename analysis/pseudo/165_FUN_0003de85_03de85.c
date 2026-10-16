// func #165  FUN_0003de85  0003de85  size=273


undefined4 FUN_0003de85(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  undefined8 uVar4;
  
  uVar4 = FUN_000418ee();
  iVar1 = (int)uVar4;
  iVar2 = (int)((ulonglong)uVar4 >> 0x20) << 2;
  FUN_0003dcbd(iVar1);
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 4) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 8) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0xc) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0x10) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0x14) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0x18) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0x1c) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0x20) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0x24) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0x28) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0x2c) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0x30) = uVar3;
  uVar3 = FUN_00041af8(iVar2);
  *(undefined4 *)(iVar1 + 0x34) = uVar3;
  uVar3 = FUN_00041af8(param_3);
  *(undefined4 *)(iVar1 + 0x38) = uVar3;
  return 1;
}


