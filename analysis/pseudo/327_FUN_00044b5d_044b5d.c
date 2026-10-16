// func #327  FUN_00044b5d  00044b5d  size=57


void FUN_00044b5d(int param_1)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  
  puVar1 = &DAT_00063ffc;
  do {
    puVar2 = puVar1;
    puVar1 = (undefined4 *)*puVar2;
    if (puVar1 == (undefined4 *)0x0) {
      return;
    }
  } while (param_1 != puVar1[1]);
  *(byte *)(param_1 + 0xc) = *(byte *)(puVar1[1] + 0xc) | 3;
  *puVar2 = *puVar1;
  *puVar1 = DAT_00063ff8;
  DAT_00063ff8 = puVar1;
  return;
}


