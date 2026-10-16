// func #279  FUN_000429a8  000429a8  size=30


int FUN_000429a8(byte *param_1,byte *param_2,int param_3)

{
  int iVar1;
  bool bVar2;
  bool bVar3;
  
  bVar2 = false;
  iVar1 = 0;
  bVar3 = true;
  do {
    if (param_3 == 0) break;
    param_3 = param_3 + -1;
    bVar2 = *param_1 < *param_2;
    bVar3 = *param_1 == *param_2;
    param_1 = param_1 + 1;
    param_2 = param_2 + 1;
  } while (bVar3);
  if (!bVar3) {
    iVar1 = (1 - (uint)bVar2) - (uint)(bVar2 != 0);
  }
  return iVar1;
}


