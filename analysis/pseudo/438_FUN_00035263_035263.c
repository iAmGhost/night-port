// func #438  FUN_00035263  00035263  size=197

void FUN_00035263(void)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int iVar5;
  undefined8 uVar6;
  
  uVar6 = FUN_000418ee();
  iVar5 = (int)((ulonglong)uVar6 >> 0x20);
  iVar3 = (int)uVar6;
  iVar2 = *(int *)(iVar5 + 0x14);
  bVar1 = **(byte **)(iVar2 * 0xe + iVar3);
  if ((bVar1 & 0x80) == 0) {
    FUN_000353d0(*(undefined1 *)(iVar2 * 0xe + iVar3 + 4),iVar2 * 0xe + iVar3);
  }
  else {
    piVar4 = (int *)(iVar2 * 0xe + iVar3);
    *piVar4 = *piVar4 + 1;
    FUN_000353d0(bVar1,iVar2 * 0xe + iVar3);
  }
  if (*(char *)(iVar2 * 0xe + iVar3 + 5) == '/') {
    *(undefined4 *)(iVar2 * 0xe + iVar3 + 10) = 0x7fffffff;
  }
  else {
    uVar6 = FUN_000351dc(iVar2 * 0xe + iVar3,iVar2 * 0xe + iVar3);
    piVar4 = (int *)((int)((ulonglong)uVar6 >> 0x20) + 10);
    *piVar4 = *piVar4 + (int)uVar6;
  }
  FUN_00035329(iVar3,iVar5);
  return;
}
