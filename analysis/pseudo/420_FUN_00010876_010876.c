// func #420  FUN_00010876  00010876  size=176

undefined8 FUN_00010876(void)

{
  int iVar1;
  undefined4 extraout_EDX;
  undefined4 *puVar2;
  undefined4 *puVar3;
  byte bVar4;
  undefined4 local_44 [3];
  ushort local_36;
  undefined2 local_34;
  uint local_28;
  undefined2 auStack_24 [4];
  
  bVar4 = 0;
  FUN_000418ee();
  local_28 = 1;
  auStack_24[(uint)bVar4 * -4] = (&DAT_00051aac)[(uint)bVar4 * -4];
  puVar2 = &DAT_00051ab0;
  puVar3 = local_44;
  for (iVar1 = 6; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + (uint)bVar4 * -2 + 1;
    puVar3 = puVar3 + (uint)bVar4 * -2 + 1;
  }
  *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
  local_36 = (ushort)DAT_00053c5c & 0xf;
  local_34 = (undefined2)(DAT_00053c5c >> 4);
  FUN_00041e6c(DAT_00053c5c,&local_28,6);
  FUN_00010570(local_44,6);
  FUN_00041e6c(&local_28,DAT_00053c5c,6);
  return CONCAT44(extraout_EDX,
                  (local_28 >> 8 & 0xff) +
                  (local_28 >> 0x18) * 0x1194 + (local_28 >> 0x10 & 0xff) * 0x4b + -0x96);
}
