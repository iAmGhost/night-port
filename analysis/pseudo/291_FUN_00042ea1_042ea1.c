// func #291  FUN_00042ea1  00042ea1  size=78


longlong FUN_00042ea1(undefined4 param_1,uint param_2)

{
  int *piVar1;
  undefined3 uVar2;
  ushort in_DS;
  
  if (DAT_00053350 != 0) {
    piVar1 = *(int **)(DAT_00053350 + 0x24);
    if (((1 < DAT_000533b2) && (DAT_000533b2 < 9)) && (DAT_000533b4 == '\0')) {
      uVar2 = SegmentLimit((uint)in_DS);
      DAT_00053384 = (ushort)uVar2 + 1;
    }
    if ((int)piVar1 + *piVar1 + 4 == DAT_00053384) {
      return CONCAT44(param_2,*piVar1);
    }
  }
  return (ulonglong)param_2 << 0x20;
}


