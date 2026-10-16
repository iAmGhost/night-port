// func #413  FUN_00049a48  00049a48  size=38


void FUN_00049a48(byte *param_1,int *param_2)

{
  int iVar1;
  
  iVar1 = 0;
  for (; (0x2f < *param_1 && (*param_1 < 0x3a)); param_1 = param_1 + 1) {
    iVar1 = iVar1 * 10 + (uint)*param_1 + -0x30;
  }
  *param_2 = iVar1;
  return;
}


