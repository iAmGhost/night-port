// === pipe_22f5 @ 0322f5 ===


void FUN_000322f5(void)

{
  int local_20;
  
  FUN_000418ee();
  FUN_00032158();
  DAT_00053c78 = DAT_00053c70;
  for (local_20 = 0; local_20 < DAT_00053c68; local_20 = local_20 + 1) {
    if ((DAT_00053c78 != DAT_00053c7c) && (*DAT_00053c78 != '\x02')) {
      FUN_00031a8e(DAT_00053c78);
    }
    DAT_00053c78 = *(char **)(DAT_00053c78 + 0x13f);
  }
  return;
}




// === pipe_209c @ 03209c ===


void FUN_0003209c(void)

{
  undefined4 local_24;
  undefined4 local_20;
  
  FUN_000418ee();
  local_20 = DAT_00053c70;
  for (local_24 = 0; local_24 < DAT_00053c68; local_24 = local_24 + 1) {
    FUN_00012616(local_20);
    local_20 = *(int *)(local_20 + 0x13f);
  }
  return;
}




// === keypoll_47e3 @ 0347e3 ===


uint FUN_000347e3(void)

{
  uint uVar1;
  uint local_20;
  
  uVar1 = FUN_000418ee();
  if (DAT_00052ff8 != '\0') {
    for (local_20 = 0; (int)local_20 < 0x80; local_20 = local_20 + 1) {
      if ((&DAT_00062c58)[local_20] == '\0') goto LAB_00034811;
      if (local_20 < 0x38) {
        if (local_20 < 0x2a) {
          if (local_20 == 0x1d) goto LAB_00034851;
        }
        else if ((local_20 < 0x2b) ||
                ((0x2b < local_20 && ((local_20 < 0x2d || (local_20 == 0x36)))))) {
LAB_00034851:
          FUN_00034735(local_20 & 0xff);
        }
      }
      else {
        if (local_20 < 0x39) goto LAB_00034851;
        if (local_20 < 0x4b) {
          if (local_20 == 0x48) goto LAB_00034842;
        }
        else if ((local_20 < 0x4c) ||
                ((0x4c < local_20 && ((local_20 < 0x4e || (local_20 == 0x50)))))) {
LAB_00034842:
          FUN_00034635(local_20 & 0xff);
        }
      }
LAB_00034811:
      uVar1 = local_20;
    }
    if (DAT_00062d5c != 0) {
      DAT_00062d5c = DAT_00062d5c + -1;
    }
  }
  return uVar1;
}




// === pipe_20f5 @ 0320f5 ===


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000320f5(void)

{
  undefined4 local_24;
  undefined4 local_20;
  
  FUN_000418ee();
  local_20 = DAT_00053c70;
  _DAT_00063388 = 0;
  for (local_24 = 0; local_24 < DAT_00053c68; local_24 = local_24 + 1) {
    FUN_0001441e(local_20);
    local_20 = *(int *)(local_20 + 0x13f);
  }
  return;
}




// === stgev_3d4e4 @ 03d4e4 ===


void FUN_0003d4e4(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if ((((*(char *)(iVar1 + 0x3b) == '\0') && (0xbe < *(int *)(DAT_00053c7c + 1) - DAT_000639dc)) &&
      ((*(int *)(iVar1 + 0x2e) == 0 || (DAT_000639dc + 0x140 <= *(int *)(iVar1 + 0x2e))))) &&
     (((*(int *)(iVar1 + 0x36) == 0 || (DAT_000639dc + 0x140 <= *(int *)(iVar1 + 0x36))) ||
      (DAT_00053c68 == 1)))) {
    FUN_0003cb38(DAT_00063354);
  }
  if ((((*(char *)(iVar1 + 0x3a) == '\0') && (*(int *)(DAT_00053c7c + 1) - DAT_000639dc < 0x82)) &&
      ((*(int *)(iVar1 + 0x2a) == 0 || (*(int *)(iVar1 + 0x2a) <= DAT_000639dc)))) &&
     (((*(int *)(iVar1 + 0x32) == 0 || (*(int *)(iVar1 + 0x32) <= DAT_000639dc)) ||
      (DAT_00053c68 == 1)))) {
    FUN_0003cbd9(DAT_00063354);
  }
  if (DAT_000639e0 + 0xc2 <= *(int *)(DAT_00053c7c + 9)) {
    FUN_0003cc6b(DAT_00063354);
  }
  return;
}




// === render_8098 @ 038098 ===


void FUN_00038098(void)

{
  undefined4 uVar1;
  
  uVar1 = FUN_000418ee();
  FUN_00013aad(DAT_00053c70);
  if (DAT_00063e24 < (int)(0x1ea / (longlong)DAT_00053104)) {
    FUN_0003ce0b(DAT_00063354,uVar1);
    FUN_0003203d(uVar1);
    FUN_0003df96(uVar1);
    FUN_0003b953(uVar1);
    if (DAT_00053c84 != 0) {
      FUN_0001348f(DAT_00053c84,uVar1,0);
      FUN_000137c1(DAT_00053c84,uVar1);
    }
    if (DAT_00053c88 != 0) {
      FUN_0001348f(DAT_00053c88,uVar1,1);
    }
    FUN_000404c6();
  }
  do {
  } while (DAT_00063e24 < (int)(0x1ea / (longlong)DAT_00053104));
  DAT_00063e24 = DAT_00063e24 - (int)(0x1ea / (longlong)DAT_00053104);
  return;
}




// === entloop_203d @ 03203d ===


void FUN_0003203d(void)

{
  undefined4 uVar1;
  undefined4 local_28;
  undefined4 local_24;
  
  uVar1 = FUN_000418ee();
  local_24 = DAT_00053c70;
  for (local_28 = 0; local_28 < DAT_00053c68; local_28 = local_28 + 1) {
    FUN_00012efa(local_24,uVar1);
    local_24 = *(int *)(local_24 + 0x13f);
  }
  return;
}




// === triga_asm @ 03cb38 ===


void FUN_0003cb38(void)

{
  int iVar1;
  int iVar2;
  
  iVar2 = FUN_000418ee();
  iVar1 = DAT_000639dc;
  if (DAT_000639e4 == '\0') {
    if (*(int *)(DAT_00053c7c + 0x17) == 3) {
      if (DAT_000639dc < *(int *)(iVar2 + 8) + -0x142) {
        DAT_000639dc = DAT_000639dc + 3;
      }
      else {
        DAT_000639dc = *(int *)(iVar2 + 8) + -0x140;
      }
    }
    else if (DAT_000639dc < *(int *)(iVar2 + 8) + -0x140) {
      DAT_000639dc = DAT_000639dc + 1;
    }
  }
  if (iVar1 != DAT_000639dc) {
    FUN_0003d75c(DAT_000639dc - iVar1);
  }
  return;
}




// === trigb_asm @ 03cbd9 ===


void FUN_0003cbd9(void)

{
  int iVar1;
  int *piVar2;
  
  piVar2 = (int *)FUN_000418ee();
  iVar1 = DAT_000639dc;
  if (DAT_000639e4 == '\0') {
    if (*(int *)(DAT_00053c7c + 0x17) == 3) {
      if (*piVar2 + 2 < DAT_000639dc) {
        DAT_000639dc = DAT_000639dc + -3;
      }
      else {
        DAT_000639dc = *piVar2;
      }
    }
    else if (*piVar2 < DAT_000639dc) {
      DAT_000639dc = DAT_000639dc + -1;
    }
  }
  if (iVar1 != DAT_000639dc) {
    FUN_0003d75c(DAT_000639dc - iVar1);
  }
  return;
}




// === trigc_asm @ 03cc6b ===


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0003cc6b(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_000639e4 == '\0') {
    if (DAT_000639dc == **(int **)(iVar1 + 0x34c)) {
      DAT_00063354 = *(undefined4 *)(iVar1 + 0x34c);
      DAT_000639e4 = '\x01';
      if (DAT_00063370 != 1) {
        DAT_00063374 = *(undefined4 *)(DAT_00053c7c + 0x2b);
        _DAT_00063384 = (uint)*(byte *)(DAT_00053c7c + 0x28);
        DAT_00063378 = (uint)*(byte *)(DAT_00053c7c + 0x27);
        DAT_0006337c = (uint)*(byte *)(DAT_00053c7c + 0x26);
        _DAT_00063380 = (uint)*(byte *)(DAT_00053c7c + 0x25);
        DAT_00063358 = DAT_00063354;
      }
      if (DAT_000639e0 < *(int *)(*(int *)(iVar1 + 0x34c) + 0xc) + -200) {
        DAT_000639e0 = DAT_000639e0 + 1;
      }
    }
  }
  else {
    if (*(int *)(*(int *)(iVar1 + 0x34c) + 4) + -1 == DAT_000639e0) {
      DAT_00063354 = *(undefined4 *)(iVar1 + 0x34c);
      DAT_000639e4 = '\0';
      if (DAT_00063370 != 1) {
        DAT_00063374 = *(undefined4 *)(DAT_00053c7c + 0x2b);
        _DAT_00063384 = (uint)*(byte *)(DAT_00053c7c + 0x28);
        DAT_00063378 = (uint)*(byte *)(DAT_00053c7c + 0x27);
        DAT_0006337c = (uint)*(byte *)(DAT_00053c7c + 0x26);
        _DAT_00063380 = (uint)*(byte *)(DAT_00053c7c + 0x25);
        DAT_00063358 = DAT_00063354;
      }
    }
    if (DAT_000639e0 < *(int *)(*(int *)(iVar1 + 0x34c) + 0xc) + -200) {
      DAT_000639e0 = DAT_000639e0 + 1;
    }
  }
  return;
}




