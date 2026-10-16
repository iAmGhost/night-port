// func #275  FUN_00042928  00042928  size=48


undefined8 FUN_00042928(int param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined8 uVar4;
  
  uVar4 = FUN_00044848(*(undefined4 *)(param_1 + 0x10),param_1);
  iVar3 = (int)((ulonglong)uVar4 >> 0x20);
  iVar2 = (int)uVar4;
  if ((iVar2 != -1) && (iVar1 = *(int *)(iVar3 + 4), iVar1 != 0)) {
    if ((*(byte *)(iVar3 + 0xd) & 0x10) != 0) {
      return CONCAT44(param_2,iVar1 + iVar2);
    }
    iVar2 = iVar2 - iVar1;
  }
  return CONCAT44(param_2,iVar2);
}


