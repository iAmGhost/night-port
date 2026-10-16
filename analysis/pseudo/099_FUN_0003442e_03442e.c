// func #88  FUN_0003442e  0003442e  size=519


void FUN_0003442e(void)

{
  bool bVar1;
  byte bVar2;
  byte local_2c;
  
  bVar2 = FUN_000418ee();
  bVar1 = false;
  if (bVar2 < 0x2a) {
    if (bVar2 == 0x1d) {
      if (DAT_00062c90 == '\0') {
        if ((DAT_00062c82 != '\0') || (DAT_00062c8e != '\0')) {
          (&DAT_00063f14)[DAT_00063f50] = 0x68;
          local_2c = 0x2a;
          bVar1 = true;
        }
      }
      else {
        (&DAT_00063f14)[DAT_00063f50] = 0x67;
        local_2c = 0x38;
        bVar1 = true;
      }
    }
    goto LAB_00034580;
  }
  if (0x2a < bVar2) {
    if (bVar2 < 0x36) goto LAB_00034580;
    if (0x36 < bVar2) {
      if (bVar2 == 0x38) {
        if (DAT_00062c75 == '\0') {
          if ((DAT_00062c82 != '\0') || (DAT_00062c8e != '\0')) {
            (&DAT_00063f14)[DAT_00063f50] = 0x69;
            local_2c = 0x2a;
            bVar1 = true;
          }
        }
        else {
          (&DAT_00063f14)[DAT_00063f50] = 0x67;
          local_2c = 0x1d;
          bVar1 = true;
        }
      }
      goto LAB_00034580;
    }
  }
  if (DAT_00062c75 == '\0') {
    if (DAT_00062c90 != '\0') {
      (&DAT_00063f14)[DAT_00063f50] = 0x69;
      local_2c = 0x38;
      bVar1 = true;
    }
  }
  else {
    (&DAT_00063f14)[DAT_00063f50] = 0x68;
    local_2c = 0x1d;
    bVar1 = true;
  }
LAB_00034580:
  if (!bVar1) {
    return;
  }
  (&DAT_00063f1e)[DAT_00063f50] = 0;
  *(int *)(&DAT_00063f28 + (uint)DAT_00063f50 * 4) = DAT_00063f08 - DAT_00063f0c;
  if (DAT_00063f50 == 9) {
    DAT_00063f50 = '\0';
  }
  else {
    DAT_00063f50 = DAT_00063f50 + 1;
  }
  FUN_000343d1(bVar2);
  if (local_2c < 0x2a) {
    if (local_2c != 0x1d) {
      return;
    }
  }
  else {
    if (local_2c < 0x2b) {
      FUN_000343d1(0x2a);
      FUN_000343d1(0x36);
      return;
    }
    if (local_2c != 0x38) {
      return;
    }
  }
  FUN_000343d1(local_2c);
  return;
}


