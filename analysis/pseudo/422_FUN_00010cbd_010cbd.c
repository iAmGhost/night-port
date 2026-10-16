// func #422  FUN_00010cbd  00010cbd  size=137

undefined8 FUN_00010cbd(void)

{
  int iVar1;
  undefined4 extraout_EDX;
  undefined4 *puVar2;
  undefined4 *puVar3;
  byte bVar4;
  undefined1 local_3c [14];
  ushort local_2e;
  undefined2 local_2c;
  uint local_24;
  undefined1 local_20 [4];
  
  bVar4 = 0;
  FUN_000418ee();
  puVar2 = &DAT_00051b38;
  puVar3 = (undefined4 *)local_3c;
  for (iVar1 = 6; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + (uint)bVar4 * -2 + 1;
    puVar3 = puVar3 + (uint)bVar4 * -2 + 1;
  }
  local_2e = (ushort)DAT_00053c5c & 0xf;
  local_2c = (undefined2)(DAT_00053c5c >> 4);
  FUN_00041e6c(DAT_00053c5c,local_20,1);
  FUN_00010570(local_3c,1);
  local_24 = (uint)((local_3c._3_4_ & 0x200) == 0);
  return CONCAT44(extraout_EDX,local_24);
}
