// func #44  FUN_00016fec  00016fec  size=556


undefined4 FUN_00016fec(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int extraout_ECX;
  int iVar6;
  undefined4 local_40;
  int local_3c;
  int local_24;
  
  iVar1 = FUN_000418ee();
  if (DAT_00051c98 == 1) {
    DAT_00051c98 = 2;
    DAT_00053c90 = 0;
    local_40 = 0;
  }
  else {
    *(undefined4 *)(iVar1 + 0x17) = 1;
    iVar2 = FUN_0001519f(iVar1);
    iVar3 = FUN_000151d5(iVar1);
    iVar6 = iVar3 % 0xc;
    local_24 = DAT_00053c70;
    for (local_3c = 0; local_3c < DAT_00053c68; local_3c = local_3c + 1) {
      if (((local_24 != DAT_00053c7c) && (local_24 != iVar1)) && (*(int *)(local_24 + 0x1f) == 4)) {
        iVar4 = FUN_0001519f(local_24);
        iVar5 = FUN_000151d5(local_24);
        iVar6 = iVar5 % 0xc;
        if ((iVar4 / 0x14 == iVar2 / 0x14) && (iVar5 / 0xc == iVar3 / 0xc)) {
          iVar2 = FUN_00041e9f();
          *(int *)(iVar1 + 0xbb) = iVar2 % 4;
          DAT_00051c98 = 1;
          return 2;
        }
      }
      local_24 = *(int *)(local_24 + 0x13f);
    }
    if (((DAT_00053c90 == 0) && (0x82 < *(uint *)(DAT_00053c7c + 0x17))) &&
       (*(uint *)(DAT_00053c7c + 0x17) < 0x86)) {
      iVar2 = FUN_000151d5(iVar1,iVar6);
      iVar2 = FUN_00041f76(iVar2 - extraout_ECX);
      if (iVar2 < 6) {
        iVar2 = FUN_00041e9f();
        if (iVar2 % 100 < 0x32) {
          DAT_00051c98 = 1;
          DAT_00053c90 = 1;
          return 3;
        }
        DAT_00053c90 = 1;
      }
    }
    if (*(int *)(iVar1 + 0xab) < 0) {
      DAT_00051c98 = 1;
      local_40 = 1;
    }
    else {
      *(int *)(iVar1 + 0xab) = *(int *)(iVar1 + 0xab) + -1;
      local_40 = 0;
    }
  }
  return local_40;
}


