// func #229  FUN_00041d18  00041d18  size=47


undefined8 FUN_00041d18(undefined4 param_1,undefined4 param_2)

{
  byte *pbVar1;
  undefined4 *puVar2;
  undefined8 uVar3;
  
  uVar3 = FUN_00041d47(param_1,param_1);
  puVar2 = (undefined4 *)((ulonglong)uVar3 >> 0x20);
  if ((int)uVar3 == 0) {
    return CONCAT44(param_2,0xffffffff);
  }
  pbVar1 = (byte *)*puVar2;
  puVar2[1] = puVar2[1] + -1;
  *puVar2 = pbVar1 + 1;
  return CONCAT44(param_2,(uint)*pbVar1);
}


