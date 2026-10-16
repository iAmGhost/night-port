// func #8  FUN_00010664  00010664  size=203


void FUN_00010664(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  byte bVar4;
  undefined4 local_3c [3];
  ushort local_2e;
  undefined2 local_2c;
  int local_20;
  
  bVar4 = 0;
  local_20 = FUN_000418ee();
  puVar2 = &DAT_00051a3c;
  puVar3 = local_3c;
  for (iVar1 = 6; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + (uint)bVar4 * -2 + 1;
    puVar3 = puVar3 + (uint)bVar4 * -2 + 1;
  }
  *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
  local_2e = (ushort)DAT_00053c5c & 0xf;
  local_2c = (undefined2)(DAT_00053c5c >> 4);
  if ((-1 < local_20) && (local_20 < 0x100)) {
    DAT_00053c4d = 0;
    DAT_00053c4f = 1;
    DAT_00053c51 = 2;
    DAT_00053c53 = 3;
    DAT_00053c54 = (undefined1)local_20;
    DAT_00053c52 = (undefined1)local_20;
    DAT_00053c50 = (undefined1)local_20;
    DAT_00053c4e = (undefined1)local_20;
    DAT_00053c4c = 3;
    FUN_00041e6c(DAT_00053c5c,&DAT_00053c4c,9);
    FUN_00010570(local_3c,9);
  }
  return;
}


