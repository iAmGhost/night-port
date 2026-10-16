// func #226  FUN_00041c28  00041c28  size=9


void FUN_00041c28(uint param_1)

{
  uint uVar1;
  int extraout_ECX;
  undefined2 in_DS;
  
  uVar1 = DAT_00053350;
  if (param_1 != 0) {
    for (; (*(uint *)(uVar1 + 8) != 0 && ((param_1 < uVar1 || (*(uint *)(uVar1 + 8) <= param_1))));
        uVar1 = *(uint *)(uVar1 + 8)) {
    }
    FUN_00042b82(param_1,in_DS,uVar1);
    if ((extraout_ECX != DAT_00053354) && (DAT_00053358 < *(uint *)(extraout_ECX + 0x14))) {
      DAT_00053358 = *(uint *)(extraout_ECX + 0x14);
    }
    DAT_00063fbd = 0;
    return;
  }
  return;
}


