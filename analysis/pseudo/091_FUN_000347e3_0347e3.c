// func #91  FUN_000347e3  000347e3  size=255


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


