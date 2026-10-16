// func #439  FUN_00035329  00035329  size=166

void FUN_00035329(void)

{
  int iVar1;
  int iVar2;
  undefined8 uVar3;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  
  uVar3 = FUN_000418ee();
  iVar2 = (int)((ulonglong)uVar3 >> 0x20);
  iVar1 = (int)uVar3;
  local_28 = 0;
  local_24 = *(int *)(iVar1 + 10);
  *(undefined1 *)(iVar2 + 0x20) = 1;
  for (local_2c = 0; local_2c < *(uint *)(iVar2 + 0x1c); local_2c = local_2c + 1) {
    if (*(char *)(local_2c * 0xe + iVar1 + 5) != '/') {
      if (*(int *)(local_2c * 0xe + iVar1 + 10) < local_24) {
        local_28 = local_2c;
        local_24 = *(int *)(local_2c * 0xe + iVar1 + 10);
      }
      *(undefined1 *)(iVar2 + 0x20) = 0;
    }
  }
  *(uint *)(iVar2 + 0x14) = local_28;
  *(int *)(iVar2 + 0x10) = local_24;
  return;
}
