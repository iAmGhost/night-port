// func #407  FUN_00049506  00049506  size=229


int FUN_00049506(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined8 uVar5;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  int local_20;
  int local_1c;
  undefined4 local_18;
  
  if (*(int *)(param_1 + 0x20) == 0) {
    uVar5 = FUN_000494cb(param_2 + 0x76c);
    local_24 = (undefined4)((ulonglong)uVar5 >> 0x20);
    if ((int)uVar5 == 0) {
      iVar3 = *(int *)(&DAT_00051960 + *(int *)(param_1 + 0x10) * 2);
      iVar1 = *(int *)(&DAT_0005195e + *(int *)(param_1 + 0x10) * 2);
    }
    else {
      iVar3 = *(int *)(&DAT_0005197a + *(int *)(param_1 + 0x10) * 2);
      iVar1 = *(int *)(&DAT_00051978 + *(int *)(param_1 + 0x10) * 2);
    }
    local_38 = 0;
    local_34 = 0;
    local_30 = 0;
    local_2c = 1;
    local_28 = *(undefined4 *)(param_1 + 0x10);
    local_18 = 0;
    FUN_0004574a(&local_38);
    iVar4 = ((*(int *)(param_1 + 0x18) - local_20) + 7) % 7;
    if (*(int *)(param_1 + 0xc) == 5) {
      iVar2 = 4;
      if ((iVar3 >> 0x10) - (iVar1 >> 0x10) < iVar4 + 0x1d) {
        iVar2 = *(int *)(param_1 + 0xc) + -2;
      }
    }
    else {
      iVar2 = *(int *)(param_1 + 0xc) + -1;
    }
    iVar3 = iVar2 * 7 + local_1c + iVar4;
  }
  else if (*(int *)(param_1 + 0x20) == 1) {
    iVar3 = *(int *)(param_1 + 0x1c) + -1;
  }
  else {
    iVar3 = *(int *)(param_1 + 0x1c);
  }
  return iVar3;
}


