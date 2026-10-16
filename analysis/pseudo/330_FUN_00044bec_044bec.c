// func #330  FUN_00044bec  00044bec  size=50


longlong FUN_00044bec(undefined4 param_1,undefined4 param_2,uint param_3)

{
  code *pcVar1;
  undefined4 extraout_EDX;
  byte in_CF;
  
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)(CONCAT11(0x3e,(char)param_1),param_1,param_1);
  if ((in_CF & 1) == 0) {
    FUN_00049325(extraout_EDX,0);
    return (ulonglong)param_3 << 0x20;
  }
  FUN_00042a0d(4);
  return CONCAT44(param_3,0xffffffff);
}


