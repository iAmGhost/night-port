// func #476  FUN_000429c6  000429c6  size=1

void FUN_000429c6(char *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 extraout_ECX;
  undefined2 in_DS;
  
  *param_1 = *param_1 + (char)param_1;
  iVar1 = FUN_00045a44(param_1,in_DS,param_1,param_2,param_4,param_3);
  if (iVar1 == 0) {
    thunk_FUN_000422d5(param_1,extraout_ECX);
  }
  return;
}
