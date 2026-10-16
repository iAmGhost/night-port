// func #224  FUN_00041af8  00041af8  size=227


undefined8 FUN_00041af8(uint param_1,undefined4 param_2)

{
  bool bVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int extraout_ECX;
  uint uVar5;
  undefined2 in_DS;
  
  if ((param_1 == 0) || (0xffffffd4 < param_1)) {
    iVar2 = 0;
  }
  else {
    iVar2 = 0;
    bVar1 = false;
    uVar5 = param_1 + 3 & 0xfffffffc;
    while( true ) {
      while( true ) {
        uVar3 = uVar5;
        if (uVar5 < 0xc) {
          uVar3 = 0xc;
        }
        if ((uVar3 <= DAT_00053358) || (iVar4 = DAT_00053354, DAT_00053354 == 0)) {
          DAT_00053358 = 0;
          iVar4 = DAT_00053350;
        }
        while (iVar4 != 0) {
          DAT_00053354 = iVar4;
          iVar2 = FUN_00042ada(param_1,in_DS,iVar4);
          if (iVar2 != 0) goto LAB_00041bc2;
          if (DAT_00053358 < *(uint *)(extraout_ECX + 0x14)) {
            DAT_00053358 = *(uint *)(extraout_ECX + 0x14);
          }
          iVar4 = *(int *)(extraout_ECX + 8);
        }
        if ((bVar1) || (iVar4 = FUN_00043071(param_1), iVar4 == 0)) break;
        bVar1 = true;
      }
      iVar4 = FUN_0004322c(param_1);
      if (iVar4 == 0) break;
      bVar1 = false;
    }
LAB_00041bc2:
    DAT_00063fbd = 0;
  }
  return CONCAT44(param_2,iVar2);
}


