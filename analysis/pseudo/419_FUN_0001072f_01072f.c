// func #419  FUN_0001072f  0001072f  size=137

void FUN_0001072f(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  byte bVar4;
  undefined4 local_38 [3];
  ushort local_2a;
  undefined2 local_28;
  
  bVar4 = 0;
  FUN_000418ee();
  puVar2 = &DAT_00051a58;
  puVar3 = local_38;
  for (iVar1 = 6; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + (uint)bVar4 * -2 + 1;
    puVar3 = puVar3 + (uint)bVar4 * -2 + 1;
  }
  *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
  local_2a = (ushort)DAT_00053c5c & 0xf;
  local_28 = (undefined2)(DAT_00053c5c >> 4);
  DAT_00053c4c = 4;
  FUN_00041e6c(DAT_00053c5c,&DAT_00053c4c,9);
  FUN_00010570(local_38,9);
  FUN_00041e6c(&DAT_00053c4c,DAT_00053c5c,9);
  return;
}
