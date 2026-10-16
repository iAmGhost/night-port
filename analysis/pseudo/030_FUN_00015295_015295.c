// func #30  FUN_00015295  00015295  size=457


void FUN_00015295(void)

{
  char *pcVar1;
  int iVar2;
  undefined8 uVar3;
  
  uVar3 = FUN_000418ee();
  pcVar1 = (char *)uVar3;
  *(int *)(pcVar1 + 5) = *(int *)(pcVar1 + 5) + (int)((ulonglong)uVar3 >> 0x20);
  if (0 < *(int *)(pcVar1 + 5)) {
    pcVar1[5] = '\0';
    pcVar1[6] = '\0';
    pcVar1[7] = '\0';
    pcVar1[8] = '\0';
  }
  if ((((pcVar1[0x30] == '\x01') && (pcVar1[0x25] != -1)) && (*(int *)(pcVar1 + 5) == 0)) &&
     ((0x15 < *(int *)(pcVar1 + 0x11) && (pcVar1[0x25] = -1, *pcVar1 == '\x01')))) {
    if (pcVar1 == DAT_00053c80) {
      iVar2 = FUN_00041bdc(0x19);
      if (iVar2 != 0) {
        FUN_0003b5c0(iVar2,*(undefined4 *)(pcVar1 + 1),*(int *)(pcVar1 + 9) + -0x1e,
                     *(undefined4 *)(pcVar1 + 0x32),1);
      }
    }
    else {
      iVar2 = FUN_00041bdc(0x19);
      if (iVar2 != 0) {
        FUN_0003b5c0(iVar2,*(undefined4 *)(pcVar1 + 1),*(int *)(pcVar1 + 9) + -0x1e,
                     *(undefined4 *)(pcVar1 + 0x32),0);
      }
    }
    *(int *)(DAT_00053c7c + 0x52) = *(int *)(DAT_00053c7c + 0x52) + *(int *)(pcVar1 + 0x32);
  }
  return;
}


