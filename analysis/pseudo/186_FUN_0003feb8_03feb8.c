// func #186  FUN_0003feb8  0003feb8  size=243


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0003feb8(void)

{
  undefined2 in_CS;
  undefined6 uVar1;
  
  FUN_000418ee();
  if (DAT_00063e86 == '\0') {
    uVar1 = FUN_0004232d(9);
    DAT_00063e84 = (undefined2)((uint6)uVar1 >> 0x20);
    DAT_00063e80 = (undefined4)uVar1;
    FUN_00042361(9,9,&LAB_0003fd40,in_CS);
    FUN_00041e20(&DAT_00063e88,0,0x80);
    FUN_00041e20(&DAT_00063f14,0,10);
    FUN_00041e20(&DAT_00063f1e,1,10);
    FUN_00041e20(&DAT_00063f28,0,0x28);
    DAT_00063f0c = 0;
    DAT_00063f08 = 0;
    DAT_00063f50 = 0;
    _DAT_00063f4c = 0xffffffff;
    DAT_00063e86 = '\x01';
    DAT_00063f51 = DAT_00000417;
    DAT_00000417 = DAT_00000417 & 0xdf;
  }
  return;
}


