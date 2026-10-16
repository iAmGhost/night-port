// func #278  FUN_0004297a  0004297a  size=45


void FUN_0004297a(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  undefined4 *puVar2;
  undefined8 uVar3;
  int local_2c [9];
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_8 = param_2;
  uStack_4 = param_3;
  iVar1 = FUN_00045687(local_2c,param_1);
  if (499 < iVar1) {
    local_2c[0] = local_2c[0] + 1;
  }
  uVar3 = FUN_0004574a(local_2c);
  puVar2 = (undefined4 *)((ulonglong)uVar3 >> 0x20);
  if (puVar2 != (undefined4 *)0x0) {
    *puVar2 = (int)uVar3;
  }
  return;
}


