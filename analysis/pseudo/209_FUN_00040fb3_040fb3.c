// func #209  FUN_00040fb3  00040fb3  size=550


undefined8 FUN_00040fb3(void)

{
  uint uVar1;
  uint uVar2;
  undefined4 extraout_EDX;
  undefined2 uVar3;
  undefined2 in_CS;
  undefined6 uVar4;
  int local_20;
  
  FUN_000418ee();
  local_20 = 0x7fff;
  uVar4 = FUN_0004232d(10);
  DAT_00063f60 = (undefined2)((uint6)uVar4 >> 0x20);
  DAT_00063f5c = (undefined4)uVar4;
  FUN_00042361(10,10,&LAB_00041439,in_CS);
  uVar4 = FUN_0004232d(0xb);
  DAT_00063f68 = (undefined2)((uint6)uVar4 >> 0x20);
  DAT_00063f64 = (undefined4)uVar4;
  FUN_00042361(0xb,0xb,&LAB_00041486,in_CS);
  uVar4 = FUN_0004232d(0xd);
  DAT_00063f70 = (undefined2)((uint6)uVar4 >> 0x20);
  DAT_00063f6c = (undefined4)uVar4;
  FUN_00042361(0xd,0xd,&LAB_000414d3,in_CS);
  uVar4 = FUN_0004232d(0xf);
  DAT_00063f78 = (undefined2)((uint6)uVar4 >> 0x20);
  DAT_00063f74 = (undefined4)uVar4;
  FUN_00042361(0xf,0xf,&LAB_00041520,in_CS);
  uVar4 = FUN_0004232d(0x72);
  DAT_00063f80 = (undefined2)((uint6)uVar4 >> 0x20);
  DAT_00063f7c = (undefined4)uVar4;
  uVar3 = 4;
  FUN_00042361(0x72,0x72,&LAB_0004156d,in_CS);
  uVar1 = FUN_0004238c(0x21);
  uVar2 = FUN_0004238c(0xa1);
  FUN_0004231d(0x21,uVar1 & 0x53);
  FUN_0004231d(0xa1,uVar2 & 0xfb);
  FUN_00041340(0xf2);
  do {
    local_20 = local_20 + -1;
    if (local_20 == -1) break;
  } while (DAT_00063f84 == 0);
  FUN_00042361(0x72,0x72,DAT_00063f7c,CONCAT22(uVar3,DAT_00063f80));
  FUN_00042361(0xf,0xf,DAT_00063f74,DAT_00063f78);
  FUN_00042361(0xd,0xd,DAT_00063f6c,DAT_00063f70);
  FUN_00042361(0xb,0xb,DAT_00063f64,DAT_00063f68);
  FUN_00042361(10,10,DAT_00063f5c,DAT_00063f60);
  return CONCAT44(extraout_EDX,DAT_00063f84);
}


