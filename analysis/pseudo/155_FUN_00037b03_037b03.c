// func #116  FUN_00037b03  00037b03  size=1304


ulonglong FUN_00037b03(void)

{
  int iVar1;
  undefined4 uVar2;
  ulonglong uVar3;
  undefined4 local_80;
  undefined4 local_7c;
  char *local_6c;
  undefined1 local_68 [20];
  undefined4 local_54;
  int local_50;
  int local_4c;
  undefined4 local_48;
  int local_44;
  int local_40;
  undefined4 local_3c;
  undefined4 local_38;
  int local_30;
  int local_2c;
  undefined4 local_28;
  char *local_24;
  int local_20;
  
  uVar3 = FUN_000418ee();
  local_20 = (int)uVar3;
  DAT_00062a8c = 0;
  DAT_00062a90 = 0;
  FUN_00041e20(&DAT_0005d08c,0,0x14);
  DAT_00053c80 = 0;
  if (local_20 == 0) {
    DAT_00063358 = 0;
    FUN_000428f8();
    local_50 = FUN_00041bdc(0x354);
    local_4c = local_50;
    if (local_50 == 0) {
      local_3c = 0;
    }
    else {
      local_54 = FUN_0003c6ec(local_50);
      local_3c = local_54;
    }
    DAT_0006334c = local_3c;
    FUN_000367c9(local_68);
    DAT_0006335c = 0;
    FUN_0004192e(&DAT_000639d8,1,4,DAT_000639b8);
    DAT_000639e8 = 0;
    FUN_0003c853(DAT_0006334c,DAT_000639b8);
    FUN_0003cad3(DAT_0006334c);
    FUN_000428f8();
    local_44 = FUN_00041bdc(0x3c);
    local_40 = local_44;
    if (local_44 == 0) {
      local_38 = 0;
    }
    else {
      local_48 = FUN_0003d8d4(local_44,local_68);
      local_38 = local_48;
    }
    DAT_00063dfc = local_38;
  }
  FUN_000428f8();
  FUN_000367c9(local_68);
  FUN_0004192e(&DAT_00063e14,1,4,DAT_000639b8);
  DAT_00062a90 = 0;
  for (local_30 = 0; local_30 < DAT_00063e14; local_30 = local_30 + 1) {
    iVar1 = FUN_00041bdc(0x59);
    if (iVar1 == 0) {
      local_6c = (char *)0x0;
    }
    else {
      local_6c = (char *)FUN_0003e0a0(iVar1,DAT_000639b8);
    }
    local_24 = local_6c;
    if (*local_6c == '\x01') {
      local_2c = *(int *)(local_6c + 1);
      if (*(int *)(&DAT_000639ec + local_2c * 4) == 0) {
        local_28 = FUN_00042928(DAT_000639b8);
        iVar1 = FUN_00041bdc(0x3c);
        if (iVar1 == 0) {
          local_80 = 0;
        }
        else {
          local_80 = FUN_0003d8d4(iVar1,(&PTR_s_gun_spr_00051d88)[local_2c]);
        }
        *(undefined4 *)(&DAT_000639ec + local_2c * 4) = local_80;
        iVar1 = FUN_00041bdc(8);
        if (iVar1 == 0) {
          local_7c = 0;
        }
        else {
          local_7c = FUN_00010010(iVar1,(&PTR_s_gun_act_00052198)[local_2c]);
        }
        (&DAT_00053818)[local_2c] = local_7c;
        FUN_0004247a(DAT_000639b8,local_28);
      }
    }
    else if (*local_6c == '\x05') {
      (&DAT_0005d0b4)[DAT_00062a90 * 5] = *(undefined4 *)(local_6c + 1);
      (&DAT_0005d0b8)[DAT_00062a90 * 5] = *(undefined4 *)(local_6c + 9);
      (&DAT_0005d0bc)[DAT_00062a90 * 5] = *(undefined4 *)(local_6c + 0xd);
      (&DAT_0005d0c0)[DAT_00062a90 * 5] = *(undefined4 *)(local_6c + 5);
      (&DAT_0005d0c4)[DAT_00062a90 * 5] = 0;
      DAT_00062a90 = DAT_00062a90 + 1;
      *local_6c = '\0';
    }
  }
  FUN_000428f8();
  FUN_000367c9(local_68);
  FUN_00036853(local_68,2,0);
  for (local_30 = 0; local_30 < 0x65; local_30 = local_30 + 1) {
    FUN_0004192e(&DAT_00053e9c + local_30 * 0x22,1,2,DAT_000639b8);
    uVar2 = FUN_00041c8f(DAT_000639b8);
    *(undefined4 *)(&DAT_00053ea0 + local_30 * 0x88) = uVar2;
    FUN_0004192e(&DAT_00053ea4 + local_30 * 0x44,1,2,DAT_000639b8);
    FUN_0004192e(&DAT_00053ea8 + local_30 * 0x88,1,2,DAT_000639b8);
    uVar2 = FUN_00041c8f(DAT_000639b8);
    *(undefined4 *)(&DAT_00053eac + local_30 * 0x88) = uVar2;
    FUN_0004192e(&DAT_00053eb0 + local_30 * 0x88,1,2,DAT_000639b8);
    FUN_0004192e(&DAT_00053eb4 + local_30 * 0x88,1,2,DAT_000639b8);
    FUN_0004192e(&DAT_00053eb8 + local_30 * 0x88,10,10,DAT_000639b8);
    FUN_0004192e(&DAT_00053f1c + local_30 * 0x88,1,2,DAT_000639b8);
    FUN_0004192e(&DAT_00053f20 + local_30 * 0x88,1,2,DAT_000639b8);
  }
  return uVar3 & 0xffffffff00000000;
}


