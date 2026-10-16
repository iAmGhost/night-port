// func #371  FUN_00047f1f  00047f1f  size=53


void FUN_00047f1f(uint param_1,undefined4 *param_2)

{
  int iVar1;
  int iVar2;
  short sVar3;
  
  if (param_1 == 0) {
    iVar2 = 0;
    sVar3 = 0;
  }
  else {
    iVar1 = 0x1f;
    if (param_1 != 0) {
      for (; param_1 >> iVar1 == 0; iVar1 = iVar1 + -1) {
      }
    }
    iVar2 = param_1 << (0x1fU - (char)iVar1 & 0x1f);
    sVar3 = ((ushort)iVar1 & 0xff) + 0x3fff;
  }
  *param_2 = 0;
  param_2[1] = iVar2;
  *(short *)(param_2 + 2) = sVar3;
  return;
}


