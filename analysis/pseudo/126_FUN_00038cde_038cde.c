// func #126  FUN_00038cde  00038cde  size=607


void FUN_00038cde(void)

{
  int iVar1;
  int iVar2;
  undefined1 local_3c [16];
  int local_2c;
  int local_28;
  int local_24;
  int local_20;
  
  FUN_000418ee();
  local_20 = 0;
  local_2c = 0;
  local_24 = 0;
  local_28 = 10;
  FUN_00035f1b("help10.pcx",0);
  iVar1 = FUN_00040660(&DAT_000636b8,0x32);
  while ((((DAT_00063eb2 == '\0' && (DAT_00063ebe == '\0')) && (DAT_00063e89 == '\0')) &&
         (((DAT_00063ea4 == '\0' && (DAT_00063ec1 == '\0')) &&
          ((DAT_00063ea5 == '\0' && (DAT_00063ec0 == '\0'))))))) {
    if ((DAT_00063ed3 != '\0') && (local_20 == 0)) {
      if (local_28 == 10) {
        local_28 = 0x23;
      }
      else {
        local_28 = local_28 + -1;
      }
      FUN_000428f8();
      iVar1 = FUN_00035f1b(local_3c,1);
      local_20 = 1;
    }
    iVar2 = local_20;
    if (DAT_00063ed3 == '\0') {
      local_20 = 0x13;
      iVar2 = iVar1;
    }
    else {
      local_20 = local_20 + 1;
      if (local_20 == 0x14) {
        local_20 = 0;
      }
    }
    if ((DAT_00063ed5 != '\0') && (local_2c == 0)) {
      if (local_28 == 0x23) {
        local_28 = 10;
      }
      else {
        local_28 = local_28 + 1;
      }
      FUN_000428f8();
      iVar2 = FUN_00035f1b(local_3c,1);
      local_2c = 1;
    }
    iVar1 = local_2c;
    if (DAT_00063ed5 == '\0') {
      local_2c = 0x13;
      iVar1 = iVar2;
    }
    else {
      local_2c = local_2c + 1;
      if (local_2c == 0x14) {
        local_2c = 0;
      }
    }
    FUN_00035ec2(iVar1);
    if (local_24 % 3 == 0) {
      FUN_0003d67c(&DAT_000636b8,0x40,0x4f);
      FUN_0003d67c(&DAT_000636b8,0x50,0x5f);
      FUN_0003d67c(&DAT_000636b8,0x60,0x6f);
      FUN_00040042();
    }
    iVar1 = local_24;
    local_24 = local_24 + 1;
  }
  do {
    do {
    } while (DAT_00063ec1 != '\0');
  } while (((((DAT_00063ea4 != '\0') || (DAT_00063ea5 != '\0')) || (DAT_00063ec0 != '\0')) ||
           ((DAT_00063eb2 != '\0' || (DAT_00063ebe != '\0')))) || (DAT_00063e89 != '\0'));
  FUN_000406f5(&DAT_000636b8,0x32);
  return;
}


