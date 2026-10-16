// func #421  FUN_00010a56  00010a56  size=131

void FUN_00010a56(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  byte bVar4;
  undefined4 local_38 [3];
  undefined4 local_2a;
  int local_26;
  int local_20;
  
  bVar4 = 0;
  local_20 = FUN_000418ee();
  if (local_20 < DAT_00053c60) {
    puVar2 = &DAT_00051af0;
    puVar3 = local_38;
    for (iVar1 = 5; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + (uint)bVar4 * -2 + 1;
      puVar3 = puVar3 + (uint)bVar4 * -2 + 1;
    }
    *(undefined2 *)puVar3 = *(undefined2 *)puVar2;
    local_2a = *(undefined4 *)(DAT_00053c64 + local_20 * 5 + 1);
    local_26 = *(int *)(DAT_00053c60 * 5 + DAT_00053c64 + 1) -
               *(int *)(DAT_00053c64 + local_20 * 5 + 1);
    FUN_00010570(local_38,0);
  }
  return;
}
