// func #?  FUN_0003ee88  03ee88  (recovered)

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0003ee88(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  
  FUN_00042310();
  FUN_0004231b();
  iVar2 = DAT_00063e3c;
  iVar1 = DAT_00063e28;
  DAT_00063e28 = DAT_00063e28 + 1;
  if (7 < DAT_00063e28) {
    DAT_00063f08 = DAT_00063f08 + 1;
    DAT_00063e28 = iVar1 + -6;
  }
  DAT_00063e24 = DAT_00063e24 + 1;
  DAT_00063e3c = DAT_00063e3c + 1;
  DAT_00063e38 = DAT_00063e38 + 1;
  if (0x11 < DAT_00063e3c) {
    DAT_00063e40 = DAT_00063e40 + 1;
    DAT_00063e3c = iVar2 + -0x10;
  }
  DAT_00063e30 = DAT_00063e30 + DAT_00063e2c;
  if (0xffff < DAT_00063e30) {
    DAT_00063e30 = DAT_00063e30 + -0xffff;
    _DAT_00063e34 = _DAT_00063e34 + 1;
  }
  FUN_0004232b();
  FUN_0004231d(0x20,0x20);
  return CONCAT44(param_2,param_1);
}

