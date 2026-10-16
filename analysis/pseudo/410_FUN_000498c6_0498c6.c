// func #410  FUN_000498c6  000498c6  size=48


undefined4 FUN_000498c6(int *param_1,int *param_2)

{
  undefined4 uVar1;
  
  uVar1 = 0;
  if ((param_1[2] < param_2[2]) ||
     ((param_1[2] == param_2[2] &&
      ((param_1[1] < param_2[1] || ((param_1[1] == param_2[1] && (*param_1 < *param_2)))))))) {
    uVar1 = 1;
  }
  return uVar1;
}


