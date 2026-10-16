// func #370  FUN_00047f0e  00047f0e  size=17


void FUN_00047f0e(uint param_1,undefined4 *param_2)

{
  int iVar1;
  int iVar2;
  short sVar3;
  
  if ((int)param_1 < 0) {
    param_1 = -param_1;
    sVar3 = -0x4001;
  }
  else {
    sVar3 = 0x3fff;
  }
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
    sVar3 = ((ushort)iVar1 & 0xff) + sVar3;
  }
  *param_2 = 0;
  param_2[1] = iVar2;
  *(short *)(param_2 + 2) = sVar3;
  return;
}


