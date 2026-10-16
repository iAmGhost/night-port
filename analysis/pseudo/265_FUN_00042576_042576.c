// func #265  FUN_00042576  00042576  size=180


undefined8 FUN_00042576(undefined1 *param_1,undefined4 param_2)

{
  char cVar1;
  uint uVar2;
  undefined3 uVar4;
  uint uVar3;
  int extraout_EDX;
  uint uVar5;
  bool bVar6;
  
  uVar5 = 0;
  cVar1 = FUN_00044859(*param_1,param_1,0);
  if (((cVar1 != 'r') && (cVar1 != 'w')) && (cVar1 != 'a')) {
    FUN_00042a0d(9);
    uVar5 = 0;
    goto LAB_00042623;
  }
  uVar2 = uVar5 | 3;
  uVar4 = (undefined3)(uVar5 >> 8);
  if (*(char *)(extraout_EDX + 1) == '+') {
    uVar3 = CONCAT31(uVar4,(char)uVar2) | 0x40;
    if (*(char *)(extraout_EDX + 2) == 'b') {
LAB_000425d2:
      uVar5 = uVar3;
    }
    else {
      uVar5 = uVar2;
      if (*(char *)(extraout_EDX + 2) != 't') {
        bVar6 = DAT_00053619 == 0x200;
LAB_000425d0:
        if (bVar6) goto LAB_000425d2;
      }
    }
  }
  else {
    uVar3 = uVar5 | 0x40;
    if (*(char *)(extraout_EDX + 1) == 'b') {
      uVar5 = uVar3;
      if (*(char *)(extraout_EDX + 2) == '+') {
        uVar3 = CONCAT31(uVar4,(char)uVar3) | 3;
LAB_00042607:
        uVar5 = uVar3;
      }
    }
    else {
      if (*(char *)(extraout_EDX + 1) == 't') {
        bVar6 = *(char *)(extraout_EDX + 2) == '+';
        uVar3 = uVar2;
        goto LAB_000425d0;
      }
      if (DAT_00053619 == 0x200) goto LAB_00042607;
    }
  }
  if (cVar1 == 'w') {
    uVar5 = uVar5 | 2;
  }
  else if (cVar1 == 'a') {
    uVar5 = uVar5 | 0x82;
  }
  else {
    uVar5 = uVar5 | 1;
  }
LAB_00042623:
  return CONCAT44(param_2,uVar5);
}


