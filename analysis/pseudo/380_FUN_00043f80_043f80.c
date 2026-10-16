// func #311  FUN_00043f80  00043f80  size=67


byte * FUN_00043f80(undefined4 param_1,int param_2)

{
  byte *pbVar1;
  uint uVar2;
  byte *extraout_ECX;
  byte *pbVar3;
  
  FUN_00041e20(param_2,0,0x20,param_1);
  uVar2 = (uint)*extraout_ECX;
  pbVar3 = extraout_ECX + 1;
  if (uVar2 != 0) {
    do {
      pbVar1 = (byte *)(((int)uVar2 >> 3) + param_2);
      *pbVar1 = *pbVar1 | (&DAT_00051854)[uVar2 & 7];
      uVar2 = (uint)*pbVar3;
      if (uVar2 == 0) {
        return pbVar3;
      }
      pbVar3 = pbVar3 + 1;
    } while (uVar2 != 0x5d);
  }
  return pbVar3;
}


