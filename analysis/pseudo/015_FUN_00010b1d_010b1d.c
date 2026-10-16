// func #11  FUN_00010b1d  00010b1d  size=289


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00010b1d(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  byte bVar4;
  undefined4 local_38 [3];
  int local_2a;
  int local_26;
  int local_20;
  
  bVar4 = 0;
  local_20 = FUN_000418ee();
  if ((DAT_00051a10 != 0) && (local_20 < DAT_00053c60)) {
    puVar2 = &DAT_00051b20;
    puVar3 = local_38;
    for (iVar1 = 5; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + (uint)bVar4 * -2 + 1;
      puVar3 = puVar3 + (uint)bVar4 * -2 + 1;
    }
    *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
    _DAT_00053c44 = *(int *)(local_20 * 5 + DAT_00053c64 + 1);
    if (DAT_00053c60 + -1 == local_20) {
      local_26 = *(int *)(DAT_00053c60 * 5 + DAT_00053c64 + 1) -
                 *(int *)(DAT_00053c64 + local_20 * 5 + 1);
    }
    else {
      local_26 = *(int *)(local_20 * 5 + DAT_00053c64 + 6) -
                 *(int *)(DAT_00053c64 + local_20 * 5 + 1);
    }
    _DAT_00053c40 = _DAT_00053c44 + local_26;
    DAT_00063e38 = 0;
    DAT_00053c48 = local_26 / 0x4b;
    local_2a = _DAT_00053c44;
    FUN_00010570(local_38,0);
  }
  return;
}


