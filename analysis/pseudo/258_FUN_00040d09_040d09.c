// func #206  FUN_00040d09  00040d09  size=388


undefined8 FUN_00040d09(void)

{
  uint uVar1;
  int iVar2;
  undefined4 extraout_EDX;
  undefined2 in_CS;
  ushort in_DS;
  bool bVar3;
  undefined6 uVar4;
  undefined4 local_30;
  int local_20;
  
  FUN_000418ee();
  DAT_00053334 = FUN_00040f4f();
  if (DAT_00053334 == 0x210) {
    local_30 = 0;
  }
  else {
    DAT_00063f84 = FUN_00040fb3();
    if (DAT_00063f84 == 0) {
      local_30 = 0;
    }
    else {
      DAT_00063f88 = FUN_000411d9();
      if (DAT_00063f88 == 0) {
        local_30 = 0;
      }
      else {
        DAT_00063fb4 = FUN_00040788(&DAT_00063fb0,0xffff);
        if (DAT_00063fb4 == 0) {
          local_30 = 0;
        }
        else {
          DAT_00063fac._2_1_ = (char)((uint)DAT_00063fb4 >> 0x10);
          bVar3 = DAT_00063fac._2_1_ == '\0';
          DAT_00063fac = DAT_00063fb4;
          if (bVar3) {
            uVar1 = DAT_00063fb4 + (uint)in_DS * 0x10;
            iVar2 = 0x10000 - (uVar1 & 0xffff);
            DAT_00063fac = uVar1 + iVar2;
            DAT_00063fb4 = DAT_00063fb4 + (uint)in_DS * 0x10;
            DAT_00063fb4 = DAT_00063fb4 + iVar2;
          }
          if (DAT_00063f84 == 10) {
            local_20 = 0x72;
          }
          else {
            local_20 = DAT_00063f84 + 8;
          }
          uVar4 = FUN_0004232d(local_20);
          DAT_00063f58 = (undefined2)((uint6)uVar4 >> 0x20);
          DAT_00063f54 = (undefined4)uVar4;
          FUN_00042361(local_20,local_20,&LAB_000415c9,in_CS);
          FUN_000413e7();
          DAT_00063f90 = 1;
          local_30 = 1;
        }
      }
    }
  }
  return CONCAT44(extraout_EDX,local_30);
}


