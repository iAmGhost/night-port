// func #399  FUN_000492b2  000492b2  size=30


undefined8 FUN_000492b2(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  code *pcVar1;
  uint extraout_EDX;
  
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)(0x4400,param_2,param_1,param_4,param_4,param_3);
  return CONCAT44(param_4,(uint)((extraout_EDX & 0x80) != 0));
}


