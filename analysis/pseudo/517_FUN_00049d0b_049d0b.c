// func #517  FUN_00049d0b  00049d0b  size=166

int FUN_00049d0b(char *param_1,int param_2,int param_3,int param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  char local_10;
  
  if (param_2 < 0) {
    param_2 = -param_2;
    local_10 = '-';
  }
  else {
    local_10 = '+';
  }
  iVar2 = 100;
  iVar1 = 3;
  if (param_2 < 100) {
    iVar2 = 10;
    iVar1 = 2;
    if (param_2 < 10) {
      iVar1 = 1;
      iVar2 = 1;
    }
  }
  if ((param_3 == 0) && (param_3 = 2, iVar1 == 3)) {
    param_3 = 3;
  }
  iVar3 = param_3 + 1;
  if (iVar3 <= param_4) {
    if (param_3 < iVar1) {
      iVar3 = iVar1 + 1;
    }
    else {
      *param_1 = local_10;
      for (; param_1 = param_1 + 1, iVar1 < param_3; param_3 = param_3 + -1) {
        *param_1 = '0';
      }
      do {
        *param_1 = (char)(param_2 / iVar2) + '0';
        param_2 = param_2 % iVar2;
        iVar2 = iVar2 / 10;
        param_1 = param_1 + 1;
      } while (iVar2 != 0);
    }
  }
  return iVar3;
}
