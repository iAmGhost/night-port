// func #308  FUN_00043dae  00043dae  size=134


int FUN_00043dae(undefined4 *param_1,int *param_2)

{
  undefined2 uVar1;
  undefined2 *extraout_ECX;
  int iVar2;
  int extraout_EDX;
  int iVar3;
  
  if ((*(byte *)(param_1 + 4) & 1) != 0) {
    if ((*(byte *)(param_1 + 4) & 4) == 0) {
      if ((*(byte *)(param_1 + 4) & 8) == 0) {
        *param_2 = *param_2 + 4;
      }
      else {
        *param_2 = *param_2 + 4;
      }
    }
    else {
      *param_2 = *param_2 + 8;
    }
  }
  iVar2 = param_1[3];
  iVar3 = 0;
  if (iVar2 == -1) {
    iVar2 = 1;
  }
  while( true ) {
    if (iVar2 < 1) {
      return iVar3;
    }
    uVar1 = (*(code *)*param_1)(param_1);
    if ((*(byte *)(param_1 + 4) & 2) != 0) break;
    iVar3 = iVar3 + 1;
    iVar2 = extraout_EDX + -1;
    if ((*(byte *)(param_1 + 4) & 1) != 0) {
      if ((*(byte *)(param_1 + 4) & 0x20) == 0) {
        *(char *)extraout_ECX = (char)uVar1;
      }
      else {
        *extraout_ECX = uVar1;
      }
    }
  }
  return iVar3;
}


