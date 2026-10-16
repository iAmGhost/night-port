// func #263  FUN_00042438  00042438  size=66


undefined4 FUN_00042438(int param_1,int *param_2)

{
  *(byte *)(param_2 + 3) = *(byte *)(param_2 + 3) & 0xef;
  if ((param_1 <= param_2[1]) && (param_2[2] - *param_2 <= param_1)) {
    *param_2 = *param_2 + param_1;
    param_2[1] = param_2[1] - param_1;
    return 0;
  }
  param_2[1] = 0;
  *param_2 = param_2[2];
  return 1;
}


