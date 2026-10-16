// func #406  FUN_000494cb  000494cb  size=59


longlong FUN_000494cb(uint param_1,uint param_2)

{
  if (((param_1 & 3) == 0) && (((int)param_1 % 100 != 0 || ((int)param_1 % 400 == 0)))) {
    return CONCAT44(param_2,1);
  }
  return (ulonglong)param_2 << 0x20;
}


