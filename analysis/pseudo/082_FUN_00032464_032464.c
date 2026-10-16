// func #82  FUN_00032464  00032464  size=605


int * FUN_00032464(void)

{
  int *piVar1;
  int iVar2;
  undefined8 uVar3;
  int local_30;
  int local_2c;
  byte local_24;
  
  uVar3 = FUN_000418ee();
  piVar1 = (int *)uVar3;
  FUN_00041e20(&DAT_00062c58,0,0x80);
  FUN_00041e20(&DAT_00062cd8,0,0x80);
  FUN_000367c9((int)((ulonglong)uVar3 >> 0x20));
  FUN_00042023();
  for (local_30 = 0; local_30 < *piVar1; local_30 = local_30 + 1) {
    FUN_00042023();
    FUN_00042023();
    iVar2 = FUN_00041af8(*(undefined1 *)((int)piVar1 + local_30 + 4));
    piVar1[local_30 + 0x1a] = iVar2;
    iVar2 = FUN_00041af8(*(undefined1 *)((int)piVar1 + local_30 + 4));
    piVar1[local_30 + 0x7e] = iVar2;
    for (local_2c = 0; local_2c < (int)(uint)*(byte *)((int)piVar1 + local_30 + 4);
        local_2c = local_2c + 1) {
      FUN_00042023();
      *(undefined *)(piVar1[local_30 + 0x1a] + local_2c) = (&DAT_0005302c)[local_24];
    }
    FUN_00042023();
    for (local_2c = 0; local_2c < (int)(uint)*(byte *)((int)piVar1 + local_30 + 4);
        local_2c = local_2c + 1) {
      FUN_00042023();
    }
    FUN_00042023();
    (&DAT_00062ac8)[*(ushort *)((int)piVar1 + local_30 * 2 + 0x388)] = (undefined1)local_2c;
    FUN_00042023();
    (&DAT_00062b90)[*(ushort *)((int)piVar1 + local_30 * 2 + 0x388)] = (undefined1)local_2c;
    FUN_00042023();
  }
  return piVar1;
}


