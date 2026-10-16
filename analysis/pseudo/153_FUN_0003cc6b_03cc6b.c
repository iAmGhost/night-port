// func #153  FUN_0003cc6b  0003cc6b  size=416


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0003cc6b(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_000639e4 == '\0') {
    if (DAT_000639dc == **(int **)(iVar1 + 0x34c)) {
      DAT_00063354 = *(undefined4 *)(iVar1 + 0x34c);
      DAT_000639e4 = '\x01';
      if (DAT_00063370 != 1) {
        DAT_00063374 = *(undefined4 *)(DAT_00053c7c + 0x2b);
        _DAT_00063384 = (uint)*(byte *)(DAT_00053c7c + 0x28);
        DAT_00063378 = (uint)*(byte *)(DAT_00053c7c + 0x27);
        DAT_0006337c = (uint)*(byte *)(DAT_00053c7c + 0x26);
        _DAT_00063380 = (uint)*(byte *)(DAT_00053c7c + 0x25);
        DAT_00063358 = DAT_00063354;
      }
      if (DAT_000639e0 < *(int *)(*(int *)(iVar1 + 0x34c) + 0xc) + -200) {
        DAT_000639e0 = DAT_000639e0 + 1;
      }
    }
  }
  else {
    if (*(int *)(*(int *)(iVar1 + 0x34c) + 4) + -1 == DAT_000639e0) {
      DAT_00063354 = *(undefined4 *)(iVar1 + 0x34c);
      DAT_000639e4 = '\0';
      if (DAT_00063370 != 1) {
        DAT_00063374 = *(undefined4 *)(DAT_00053c7c + 0x2b);
        _DAT_00063384 = (uint)*(byte *)(DAT_00053c7c + 0x28);
        DAT_00063378 = (uint)*(byte *)(DAT_00053c7c + 0x27);
        DAT_0006337c = (uint)*(byte *)(DAT_00053c7c + 0x26);
        _DAT_00063380 = (uint)*(byte *)(DAT_00053c7c + 0x25);
        DAT_00063358 = DAT_00063354;
      }
    }
    if (DAT_000639e0 < *(int *)(*(int *)(iVar1 + 0x34c) + 0xc) + -200) {
      DAT_000639e0 = DAT_000639e0 + 1;
    }
  }
  return;
}


