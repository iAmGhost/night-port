// func #9  FUN_000107b9  000107b9  size=189


void FUN_000107b9(void)

{
  int iVar1;
  undefined4 *puVar2;
  byte *pbVar3;
  byte bVar4;
  byte abStack_c16 [3040];
  ushort local_36;
  undefined2 local_34;
  undefined4 local_28;
  byte local_24 [4];
  undefined8 local_20;
  
  bVar4 = 0;
  local_20 = FUN_000418ee();
  local_28 = DAT_00051a84;
  *(undefined2 *)(local_24 + (uint)bVar4 * -8) = (&DAT_00051a88)[(uint)bVar4 * -4];
  local_24[(uint)bVar4 * -4 + (uint)bVar4 * -8 + 2] =
       (&DAT_00051a8a)[(uint)bVar4 * -4 + (uint)bVar4 * -8];
  local_28._0_2_ = CONCAT11((undefined1)local_20,(undefined1)local_28);
  puVar2 = &DAT_00051a8c;
  pbVar3 = abStack_c16 + 0xbd2;
  for (iVar1 = 6; iVar1 != 0; iVar1 = iVar1 + -1) {
    *(undefined4 *)pbVar3 = *puVar2;
    puVar2 = puVar2 + (uint)bVar4 * -2 + 1;
    pbVar3 = pbVar3 + ((uint)bVar4 * -2 + 1) * 4;
  }
  *(undefined2 *)pbVar3 = *(undefined2 *)puVar2;
  local_36 = (ushort)DAT_00053c5c & 0xf;
  local_34 = (undefined2)(DAT_00053c5c >> 4);
  FUN_00041e6c(DAT_00053c5c,&local_28,7);
  FUN_00010570(abStack_c16 + 0xbd2,7);
  FUN_00041e6c(&local_28,DAT_00053c5c,7);
  *(uint *)(local_20._4_4_ + 1) =
       (uint)local_24[0] * 0x1194 + (local_28 >> 0x18) * 0x4b + (local_28 >> 0x10 & 0xff) + -0x96;
  return;
}


