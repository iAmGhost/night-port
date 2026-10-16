// func #326  FUN_00044ab5  00044ab5  size=168


undefined8 FUN_00044ab5(undefined4 param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  int iVar2;
  undefined *puVar3;
  undefined4 *extraout_ECX;
  undefined4 *puVar4;
  int extraout_ECX_00;
  uint uVar5;
  
  if (DAT_00063ff8 == (undefined4 *)0x0) {
    for (puVar3 = &DAT_00053410; puVar3 < &DAT_00053618; puVar3 = puVar3 + 0x1a) {
      if ((puVar3[0xc] & 3) == 0) {
        puVar1 = (undefined4 *)FUN_00041af8(8);
        if (puVar1 == (undefined4 *)0x0) goto LAB_00044b4b;
        uVar5 = 3;
        puVar4 = extraout_ECX;
        goto LAB_00044b26;
      }
    }
    uVar5 = 0x4003;
    puVar1 = (undefined4 *)FUN_00041af8(0x22);
    if (puVar1 == (undefined4 *)0x0) {
LAB_00044b4b:
      FUN_00042a0d(5);
      iVar2 = 0;
      goto LAB_00044b57;
    }
    puVar4 = puVar1 + 2;
  }
  else {
    puVar4 = (undefined4 *)DAT_00063ff8[1];
    uVar5 = (uint)((ushort)puVar4[3] & 0x4003 | 3);
    puVar1 = DAT_00063ff8;
    DAT_00063ff8 = (undefined4 *)*DAT_00063ff8;
  }
LAB_00044b26:
  FUN_00041e20(puVar4,0,0x1a);
  *(uint *)(extraout_ECX_00 + 0xc) = uVar5;
  puVar1[1] = extraout_ECX_00;
  *puVar1 = DAT_00063ffc;
  DAT_00063ffc = puVar1;
  iVar2 = extraout_ECX_00;
LAB_00044b57:
  return CONCAT44(param_2,iVar2);
}


