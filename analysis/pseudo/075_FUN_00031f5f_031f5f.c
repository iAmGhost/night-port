// func #75  FUN_00031f5f  00031f5f  size=222


void FUN_00031f5f(void)

{
  int iVar1;
  int iVar2;
  undefined4 local_28;
  
  iVar2 = FUN_000418ee();
  if (*(char *)(iVar2 + 0x31) != '\0') {
    *(char *)(iVar2 + 0x31) = *(char *)(iVar2 + 0x31) + -1;
    *(undefined4 *)(iVar2 + 0x17) = *(undefined4 *)(iVar2 + 0x32);
    iVar1 = *(int *)(*(int *)(*(int *)(iVar2 + 0x123) + 4) + *(int *)(iVar2 + 0x17) * 4);
    if (*(char *)(iVar1 + 0x69) != '\0') {
      DAT_00063364 = (uint)*(byte *)(iVar1 + 0x69);
      DAT_00063368 = (uint)*(ushort *)(iVar1 + 0x6a);
    }
    *(undefined4 *)(iVar2 + 0x11) = 0;
    for (local_28 = 0; local_28 < (int)(uint)*(byte *)(iVar2 + 0x31); local_28 = local_28 + 1) {
      *(undefined4 *)(local_28 * 4 + iVar2 + 0x32) = *(undefined4 *)(local_28 * 4 + iVar2 + 0x36);
    }
  }
  return;
}


