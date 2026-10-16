// func #418  FUN_000105f7  000105f7  size=108

void FUN_000105f7(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  byte bVar4;
  undefined4 local_38 [3];
  ushort local_2a;
  undefined2 local_28;
  undefined1 local_20 [4];
  
  bVar4 = 0;
  local_20[0] = FUN_000418ee();
  puVar2 = &DAT_00051a14;
  puVar3 = local_38;
  for (iVar1 = 6; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + (uint)bVar4 * -2 + 1;
    puVar3 = puVar3 + (uint)bVar4 * -2 + 1;
  }
  local_2a = (ushort)DAT_00053c5c & 0xf;
  local_28 = (undefined2)(DAT_00053c5c >> 4);
  FUN_00041e6c(DAT_00053c5c,local_20,1);
  FUN_00010570(local_38,1);
  return;
}
