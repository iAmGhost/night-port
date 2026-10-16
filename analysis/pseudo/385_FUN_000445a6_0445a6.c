// func #316  FUN_000445a6  000445a6  size=35


undefined8 FUN_000445a6(undefined4 *param_1,undefined4 param_2)

{
  int iVar1;
  undefined4 uVar2;
  undefined8 uVar3;
  
  iVar1 = param_1[3];
  param_1[3] = iVar1 + -1;
  if (iVar1 != 0) {
    uVar3 = (*(code *)*param_1)(param_1);
    uVar2 = (undefined4)uVar3;
    if ((*(byte *)((int)((ulonglong)uVar3 >> 0x20) + 0x10) & 2) == 0) goto LAB_000445c6;
  }
  uVar2 = 0xffffffff;
LAB_000445c6:
  return CONCAT44(param_2,uVar2);
}


