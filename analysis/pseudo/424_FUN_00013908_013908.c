// func #424  FUN_00013908  00013908  size=420

void FUN_00013908(void)

{
  int *piVar1;
  int iVar2;
  uint uVar3;
  
  iVar2 = FUN_000418ee();
  piVar1 = *(int **)(*(int *)(*(int *)(iVar2 + 0x123) + 4) + *(int *)(iVar2 + 0x17) * 4);
  uVar3 = (uint)*(ushort *)((*(int *)(iVar2 + 0x11) % *piVar1) * 2 + *(int *)((int)piVar1 + 0x6d));
  if (*(int *)(iVar2 + 0x1b) == 0) {
    FUN_0003ba86((*(int *)(iVar2 + 1) - *(int *)(uVar3 * 4 + *(int *)(iVar2 + 0x137))) -
                 DAT_000639dc,
                 ((*(int *)(iVar2 + 5) + *(int *)(iVar2 + 9)) -
                 *(int *)(*(int *)(iVar2 + 0x13b) + uVar3 * 4)) - DAT_000639e0,
                 *(undefined4 *)(uVar3 * 4 + *(int *)(iVar2 + 0x12f)),
                 *(int *)(iVar2 + 0x127) + *(int *)(uVar3 * 4 + *(int *)(iVar2 + 0x133)),
                 *(undefined1 *)(iVar2 + 0x1b));
  }
  else {
    FUN_0003ba86((*(int *)(iVar2 + 1) -
                 (*(int *)(uVar3 * 4 + *(int *)(iVar2 + 299)) -
                 *(int *)(*(int *)(iVar2 + 0x137) + uVar3 * 4))) - DAT_000639dc,
                 ((*(int *)(iVar2 + 5) + *(int *)(iVar2 + 9)) -
                 *(int *)(*(int *)(iVar2 + 0x13b) + uVar3 * 4)) - DAT_000639e0,
                 *(undefined4 *)(*(int *)(iVar2 + 0x12f) + uVar3 * 4),
                 *(int *)(iVar2 + 0x127) + *(int *)(*(int *)(iVar2 + 0x133) + uVar3 * 4),
                 *(undefined1 *)(iVar2 + 0x1b));
  }
  return;
}
