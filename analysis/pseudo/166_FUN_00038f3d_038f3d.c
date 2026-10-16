// func #127  FUN_00038f3d  00038f3d  size=3270


void FUN_00038f3d(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  int extraout_EDX;
  int extraout_EDX_00;
  int extraout_EDX_01;
  int extraout_EDX_02;
  int extraout_EDX_03;
  int extraout_EDX_04;
  int extraout_EDX_05;
  undefined4 *puVar6;
  undefined1 *puVar7;
  undefined1 *puVar8;
  int *piVar9;
  undefined4 *puVar10;
  int *piVar11;
  byte bVar12;
  undefined4 local_15c;
  undefined4 local_158;
  undefined4 local_154;
  undefined1 local_150 [16];
  undefined4 local_140;
  int local_13c;
  uint local_138;
  uint local_134;
  uint local_130;
  uint local_12c;
  undefined4 local_128;
  int local_120 [6];
  int local_108 [6];
  undefined4 local_f0 [6];
  undefined4 local_d8 [6];
  undefined4 local_c0 [6];
  undefined4 local_a8 [6];
  int local_90;
  int local_8c;
  int local_88;
  int local_84;
  int local_80;
  int local_7c;
  int local_78;
  int local_74;
  int local_70;
  uint local_6c;
  uint local_68;
  int local_64;
  int local_60;
  int local_5c;
  int local_58;
  int local_54;
  int local_50;
  int local_4c;
  int local_48;
  int local_44;
  int local_40;
  int local_3c;
  int local_38;
  int local_34;
  int local_30;
  int local_2c;
  int local_28;
  int local_24;
  int local_20;
  
  bVar12 = 0;
  local_20 = FUN_000418ee();
  local_7c = 0;
  local_78 = 0;
  local_74 = 0;
  local_8c = 0;
  local_90 = 0;
  local_64 = 0;
  local_84 = 0;
  puVar6 = &DAT_00053150;
  puVar10 = local_a8;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar10 = *puVar6;
    puVar6 = puVar6 + (uint)bVar12 * -2 + 1;
    puVar10 = puVar10 + (uint)bVar12 * -2 + 1;
  }
  puVar6 = &DAT_00053168;
  puVar10 = local_c0;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar10 = *puVar6;
    puVar6 = puVar6 + (uint)bVar12 * -2 + 1;
    puVar10 = puVar10 + (uint)bVar12 * -2 + 1;
  }
  puVar6 = &DAT_00053180;
  puVar10 = local_d8;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar10 = *puVar6;
    puVar6 = puVar6 + (uint)bVar12 * -2 + 1;
    puVar10 = puVar10 + (uint)bVar12 * -2 + 1;
  }
  puVar6 = &DAT_00053198;
  puVar10 = local_f0;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar10 = *puVar6;
    puVar6 = puVar6 + (uint)bVar12 * -2 + 1;
    puVar10 = puVar10 + (uint)bVar12 * -2 + 1;
  }
  piVar9 = &DAT_000531b0;
  piVar11 = local_108;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *piVar11 = *piVar9;
    piVar9 = piVar9 + (uint)bVar12 * -2 + 1;
    piVar11 = piVar11 + (uint)bVar12 * -2 + 1;
  }
  piVar9 = &DAT_000531c8;
  piVar11 = local_120;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *piVar11 = *piVar9;
    piVar9 = piVar9 + (uint)bVar12 * -2 + 1;
    piVar11 = piVar11 + (uint)bVar12 * -2 + 1;
  }
  *(int *)(DAT_00053c7c + 0x2b) = *(int *)(DAT_00053c7c + 0x2b) + *(int *)(DAT_00053c7c + 0x52);
  *(undefined4 *)(DAT_00053c7c + 0x52) = 0;
  FUN_00041e20(&local_138,0,0x18);
  local_138 = (uint)*(byte *)(DAT_00053c7c + 0x27);
  local_134 = (uint)*(byte *)(DAT_00053c7c + 0x28);
  local_130 = (uint)*(byte *)(DAT_00053c7c + 0x26);
  local_12c = (uint)*(byte *)(DAT_00053c7c + 0x25);
  local_58 = FUN_00041bdc(0x3c);
  local_54 = local_58;
  if (local_58 == 0) {
    local_2c = 0;
  }
  else {
    local_5c = FUN_0003d8d4(local_58,"levelup.spr");
    local_2c = local_5c;
  }
  local_60 = local_2c;
  local_4c = FUN_00041bdc(0x3c);
  local_48 = local_4c;
  if (local_4c == 0) {
    local_28 = 0;
  }
  else {
    local_50 = FUN_0003d8d4(local_4c,"number1.spr");
    local_28 = local_50;
  }
  local_13c = local_28;
  local_40 = FUN_00041bdc(0x3c);
  local_3c = local_40;
  if (local_40 == 0) {
    local_24 = 0;
  }
  else {
    local_44 = FUN_0003d8d4(local_40,"number2.spr");
    local_24 = local_44;
  }
  local_80 = local_24;
  local_140 = FUN_00041af8(64000);
  if (local_20 == 0) {
    FUN_00036190("levelup.pcx",local_140);
  }
  else {
    FUN_00036190("levelup2.pcx",local_140);
  }
  FUN_00041e20(&DAT_000636b8,0,0x300);
  FUN_00040042();
  if (DAT_000633a4 == 1) {
    FUN_00041e6c(&DAT_000a0000,local_140,64000);
    FUN_00040660(&DAT_000633b8,0x32);
  }
  else {
    FUN_00035f1b("help1.pcx",0);
    FUN_00040660(&DAT_000633b8,0x32);
    DAT_000633a4 = 1;
    FUN_00035d5d();
    FUN_00041e6c(&DAT_000a0000,local_140,64000);
  }
  if (local_20 == 1) {
    FUN_00010b1d(0xb);
  }
  FUN_0004185e(DAT_00053de4);
  iVar5 = extraout_EDX;
  puVar6 = &local_15c;
  while (((DAT_00063e89 == '\0' && (DAT_00063eb2 == '\0')) && (DAT_00063ebe == '\0'))) {
    if (DAT_00063ec3 != '\0') {
      *(undefined4 *)((int)puVar6 + -4) = 0x39241;
      FUN_0004185e(DAT_00053dd8,iVar5);
      if (local_7c == 0) {
        local_88 = 10;
        *(undefined4 *)((int)puVar6 + -4) = 0x39261;
        iVar5 = FUN_00035f1b("help10.pcx",1);
        while (((DAT_00063eb2 == '\0' && (DAT_00063ebe == '\0')) &&
               ((DAT_00063e89 == '\0' &&
                ((((DAT_00063ea4 == '\0' && (DAT_00063ec1 == '\0')) && (DAT_00063ea5 == '\0')) &&
                 (DAT_00063ec0 == '\0'))))))) {
          if ((DAT_00063ed3 != '\0') && (local_90 == 0)) {
            if (local_88 == 10) {
              local_88 = 0x23;
            }
            else {
              local_88 = local_88 + -1;
            }
            *(int *)((int)puVar6 + -4) = local_88;
            *(char **)((int)puVar6 + -8) = "help%d.pcx";
            *(undefined1 **)((int)puVar6 + -0xc) = local_150;
            *(undefined4 *)((int)puVar6 + -0x10) = 0x392eb;
            FUN_000428f8();
            *(undefined4 *)((int)puVar6 + -4) = 0x392fe;
            iVar5 = FUN_00035f1b(local_150,1);
            local_90 = 1;
          }
          iVar4 = local_90;
          if (DAT_00063ed3 == '\0') {
            local_90 = 0x13;
            iVar4 = iVar5;
          }
          else {
            local_90 = local_90 + 1;
            if (local_90 == 0x14) {
              local_90 = 0;
            }
          }
          if ((DAT_00063ed5 != '\0') && (local_64 == 0)) {
            if (local_88 == 0x23) {
              local_88 = 10;
            }
            else {
              local_88 = local_88 + 1;
            }
            *(int *)((int)puVar6 + -4) = local_88;
            *(char **)((int)puVar6 + -8) = "help%d.pcx";
            *(undefined1 **)((int)puVar6 + -0xc) = local_150;
            *(undefined4 *)((int)puVar6 + -0x10) = 0x39366;
            FUN_000428f8();
            *(undefined4 *)((int)puVar6 + -4) = 0x39379;
            iVar4 = FUN_00035f1b(local_150,1);
            local_64 = 1;
          }
          iVar5 = local_64;
          if (DAT_00063ed5 == '\0') {
            local_64 = 0x13;
            iVar5 = iVar4;
          }
          else {
            local_64 = local_64 + 1;
            if (local_64 == 0x14) {
              local_64 = 0;
            }
          }
          *(undefined4 *)((int)puVar6 + -4) = 0x393aa;
          FUN_00035ec2(iVar5);
          if (local_84 % 3 == 0) {
            *(undefined4 *)((int)puVar6 + -4) = 0x393d2;
            FUN_0003d67c(&DAT_000636b8,0x40,0x4f);
            *(undefined4 *)((int)puVar6 + -4) = 0x393e6;
            FUN_0003d67c(&DAT_000636b8,0x50,0x5f);
            *(undefined4 *)((int)puVar6 + -4) = 0x393fa;
            FUN_0003d67c(&DAT_000636b8,0x60,0x6f);
            *(undefined **)((int)puVar6 + -4) = &DAT_000636b8;
            *(undefined4 *)((int)puVar6 + -8) = 0xff;
            *(undefined4 *)((int)puVar6 + -0xc) = 0;
            *(undefined4 *)((int)puVar6 + -0x10) = 0x3940e;
            FUN_00040042();
          }
          iVar5 = local_84;
          local_84 = local_84 + 1;
        }
        do {
          do {
          } while (DAT_00063ec1 != '\0');
        } while ((((DAT_00063ea4 != '\0') || (DAT_00063ea5 != '\0')) || (DAT_00063ec0 != '\0')) ||
                (((DAT_00063eb2 != '\0' || (DAT_00063ebe != '\0')) || (DAT_00063e89 != '\0'))));
      }
      else {
        *(int *)((int)puVar6 + -4) = local_7c + 1;
        *(char **)((int)puVar6 + -8) = "help%d.pcx";
        *(undefined1 **)((int)puVar6 + -0xc) = local_150;
        *(undefined4 *)((int)puVar6 + -0x10) = 0x3947e;
        FUN_000428f8();
        *(undefined4 *)((int)puVar6 + -4) = 0x39491;
        FUN_00035f1b(local_150,1);
        *(undefined4 *)((int)puVar6 + -4) = 0x39496;
        FUN_00035d5d();
      }
      *(undefined4 *)((int)puVar6 + -4) = 0x394a0;
      FUN_0004185e(DAT_00053dd8);
      iVar5 = extraout_EDX_00;
    }
    if (((DAT_00063ea4 != '\0') || (DAT_00063ec1 != '\0')) ||
       ((DAT_00063ea5 != '\0' || (DAT_00063ec0 != '\0')))) {
      if ((*(int *)(DAT_00053c7c + 0x2b) <
           *(int *)(&DAT_00053c94 + local_7c * 0x14 + (&local_138)[local_7c] * 4)) ||
         (local_108[local_7c] <= (int)(&local_138)[local_7c])) {
        if ((int)(&local_138)[local_7c] < local_108[local_7c]) {
          *(undefined4 *)((int)puVar6 + -4) = 0x39640;
          FUN_00035f1b("help8.pcx",1);
        }
        else {
          *(undefined4 *)((int)puVar6 + -4) = 0x3962f;
          FUN_00035f1b("help9.pcx",1);
        }
        *(undefined4 *)((int)puVar6 + -4) = 0x3964a;
        FUN_0004185e(DAT_00053de0);
        *(undefined4 *)((int)puVar6 + -4) = 0x3964f;
        FUN_00035d5d();
        iVar5 = extraout_EDX_03;
      }
      else {
        *(int *)(DAT_00053c7c + 0x2b) =
             *(int *)(DAT_00053c7c + 0x2b) -
             *(int *)(&DAT_00053c94 + local_7c * 0x14 + (&local_138)[local_7c] * 4);
        if (local_7c == 5) {
          DAT_00053100 = DAT_00053100 + 1;
          *(undefined4 *)((int)puVar6 + -4) = 0x39602;
          FUN_0004185e(DAT_00053ddc);
          iVar5 = extraout_EDX_02;
        }
        else {
          (&local_138)[local_7c] = (&local_138)[local_7c] + 1;
          if ((local_7c == 4) || (local_7c == 1)) {
            if (local_7c == 4) {
              local_128 = 0;
            }
            if ((int)*(short *)(DAT_00053c7c + 0x23) == *(int *)(&DAT_00052738 + local_134 * 4)) {
              *(int *)(DAT_00053c7c + 0x2b) =
                   *(int *)(DAT_00053c7c + 0x2b) +
                   *(int *)(&DAT_00053c94 + (&local_138)[local_7c] * 4 + local_7c * 0x14);
              *(undefined4 *)((int)puVar6 + -4) = 0x395b6;
              FUN_0004185e(DAT_00053de0);
            }
            else {
              *(undefined4 *)((int)puVar6 + -4) = 0x395c2;
              FUN_0004185e(DAT_00053ddc);
            }
            iVar5 = CONCAT22((short)(local_134 * 4 >> 0x10),
                             *(undefined2 *)(&DAT_00052738 + local_134 * 4));
            *(undefined2 *)(DAT_00053c7c + 0x23) = *(undefined2 *)(&DAT_00052738 + local_134 * 4);
            *(undefined1 *)(DAT_00053c7c + 0x30) = 0;
          }
          else {
            *(undefined4 *)((int)puVar6 + -4) = 0x395f0;
            FUN_0004185e(DAT_00053ddc);
            iVar5 = extraout_EDX_01;
          }
        }
      }
      do {
        do {
        } while (DAT_00063ea4 != '\0');
      } while (((DAT_00063ec1 != '\0') || (DAT_00063ea5 != '\0')) || (DAT_00063ec0 != '\0'));
    }
    if ((DAT_00063ed0 != '\0') && (local_78 == 0)) {
      if (local_7c == 0) {
        local_7c = 5;
      }
      else {
        local_7c = local_7c + -1;
      }
      if ((local_20 == 0) && (local_7c == 5)) {
        local_7c = 3;
      }
      local_78 = 1;
      *(undefined4 *)((int)puVar6 + -4) = 0x396c3;
      FUN_0004185e(DAT_00053dd8,iVar5);
    }
    if (DAT_00063ed0 == '\0') {
      local_78 = 0x13;
    }
    else {
      local_78 = local_78 + 1;
      if (local_78 == 0x14) {
        local_78 = 0;
      }
    }
    if ((DAT_00063ed8 != '\0') && (local_74 == 0)) {
      if (local_7c == 5) {
        local_7c = 0;
      }
      else {
        local_7c = local_7c + 1;
      }
      if ((local_20 == 0) && (local_7c == 4)) {
        local_7c = 0;
      }
      local_74 = 1;
      *(undefined4 *)((int)puVar6 + -4) = 0x39732;
      FUN_0004185e(DAT_00053dd8);
    }
    uVar1 = local_140;
    if (DAT_00063ed8 == '\0') {
      local_74 = 0x13;
    }
    else {
      local_74 = local_74 + 1;
      if (local_74 == 0x14) {
        local_74 = 0;
      }
    }
    *(undefined4 *)((int)puVar6 + -4) = 0x3976c;
    FUN_00041e6c(DAT_000633b0,uVar1,64000);
    *(undefined4 *)((int)puVar6 + -4) = *(undefined4 *)(DAT_00053c7c + 0x2b);
    iVar5 = local_13c;
    *(undefined4 *)((int)puVar6 + -8) = 0x3978e;
    FUN_00038b30(DAT_000633b0,iVar5,0x49,0x19);
    *(undefined4 *)((int)puVar6 + -8) =
         *(undefined4 *)(&DAT_00053c94 + (&local_138)[local_7c] * 4 + local_7c * 0x14);
    iVar5 = local_80;
    *(undefined4 *)((int)puVar6 + -0xc) = 0x397c1;
    FUN_00038b30(DAT_000633b0,iVar5,0x5e,0x26);
    *(undefined4 *)((int)puVar6 + -0xc) = 0;
    *(int *)((int)puVar6 + -0x10) =
         *(int *)(local_60 + 0x38) + *(int *)(*(int *)(local_60 + 0x34) + local_7c * 4);
    *(undefined4 *)((int)puVar6 + -0x14) = *(undefined4 *)(local_7c * 4 + *(int *)(local_60 + 8));
    uVar1 = *(undefined4 *)(local_7c * 4 + *(int *)(local_60 + 4));
    uVar2 = local_a8[local_7c];
    uVar3 = local_c0[local_7c];
    *(undefined4 *)((int)puVar6 + -0x18) = 0x3981f;
    FUN_0003bb42(DAT_000633b0,uVar3,uVar2,uVar1);
    puVar8 = (undefined1 *)((int)puVar6 + -0x18);
    *(int *)((int)puVar6 + -0x18) = DAT_00053100;
    iVar5 = local_13c;
    *(undefined4 *)((int)puVar6 + -0x1c) = 0x3983f;
    FUN_00038c23(DAT_000633b0,iVar5,0x68,0xac);
    for (local_70 = 0; local_70 < 6; local_70 = local_70 + 1) {
      local_6c = (&local_138)[local_70];
      local_68 = (&local_138)[local_70];
      if (((0x19 < local_8c % 0x32) && (local_70 == local_7c)) &&
         ((int)local_6c < local_108[local_70])) {
        local_6c = local_6c + 1;
      }
      puVar7 = puVar8;
      if (local_6c != 0) {
        *(undefined4 *)(puVar8 + -4) = 0;
        *(int *)(puVar8 + -8) =
             *(int *)(local_60 + 0x38) +
             *(int *)((local_120[local_70] + local_6c) * 4 + *(int *)(local_60 + 0x34) + -4);
        puVar7 = puVar8 + -0xc;
        *(undefined4 *)(puVar8 + -0xc) =
             *(undefined4 *)(*(int *)(local_60 + 8) + (local_120[local_70] + local_6c) * 4 + -4);
        uVar1 = *(undefined4 *)((local_120[local_70] + local_6c) * 4 + *(int *)(local_60 + 4) + -4);
        uVar2 = local_d8[local_70];
        uVar3 = local_f0[local_70];
        *(undefined4 *)(puVar8 + -0x10) = 0x39943;
        FUN_0003bb42(DAT_000633b0,uVar3,uVar2,uVar1);
      }
      puVar8 = puVar7;
      if ((local_70 < 2) && (local_68 != 0)) {
        *(undefined4 *)(puVar7 + -4) = 0;
        *(int *)(puVar7 + -8) =
             *(int *)(local_60 + 0x38) + *(int *)(local_68 * 4 + *(int *)(local_60 + 0x34) + 0x30);
        puVar8 = puVar7 + -0xc;
        *(undefined4 *)(puVar7 + -0xc) =
             *(undefined4 *)(*(int *)(local_60 + 8) + local_68 * 4 + 0x30);
        uVar1 = *(undefined4 *)(*(int *)(local_60 + 4) + local_68 * 4 + 0x30);
        *(undefined4 *)(puVar7 + -0x10) = 0x399a2;
        FUN_0003bb42(DAT_000633b0,0x8a,local_70 * 0x19 + 0x38,uVar1);
      }
    }
    iVar5 = local_8c / 4;
    if (local_8c % 4 == 0) {
      *(undefined4 *)(puVar8 + -4) = 0x399cf;
      FUN_0003d67c(&DAT_000633b8,0x40,0x4f);
      *(undefined4 *)(puVar8 + -4) = 0x399e3;
      iVar5 = FUN_0003d67c(&DAT_000633b8,0x50,0x5f);
    }
    *(undefined4 *)(puVar8 + -4) = 0x399e8;
    FUN_00035ec2(iVar5);
    *(undefined **)(puVar8 + -4) = &DAT_000633b8;
    *(undefined4 *)(puVar8 + -8) = 0x100;
    *(undefined4 *)(puVar8 + -0xc) = 0;
    *(undefined4 *)(puVar8 + -0x10) = 0x399fc;
    FUN_00040042();
    *(undefined4 *)(puVar8 + -4) = 0x39a14;
    FUN_00041e6c(&DAT_000a0000,DAT_000633b0,64000);
    local_8c = local_8c + 1;
    iVar5 = extraout_EDX_04;
    puVar6 = (undefined4 *)puVar8;
    if ((local_20 == 1) && (iVar5 = DAT_00063e38 % 0x1ea, DAT_00053c48 < DAT_00063e38 / 0x1ea)) {
      DAT_00063e38 = 0;
      *(undefined4 *)(puVar8 + -4) = 0x39a51;
      FUN_00010b1d(0xb);
      iVar5 = extraout_EDX_05;
    }
  }
  if (*(byte *)(DAT_00053c7c + 0x27) != local_138) {
    DAT_00062a98 = (&DAT_00062aa0)[local_138];
    DAT_00062a9c = (&DAT_00062ab4)[local_138];
    *(undefined1 *)(DAT_00053c7c + 0x27) = (undefined1)local_138;
  }
  *(undefined1 *)(DAT_00053c7c + 0x28) = (undefined1)local_134;
  if (*(byte *)(DAT_00053c7c + 0x26) != local_130) {
    *(undefined1 *)(DAT_00053c7c + 0x26) = (undefined1)local_130;
    DAT_00053818 = (&DAT_00053c28)[local_130];
    *(undefined4 *)(DAT_00053c7c + 0x123) = DAT_00053818;
  }
  *(undefined1 *)(DAT_00053c7c + 0x25) = (undefined1)local_12c;
  *(undefined4 *)((int)puVar6 + -4) = 0x39b16;
  FUN_0004185e(DAT_00053ddc);
  *(undefined4 *)((int)puVar6 + -4) = 0x39b25;
  FUN_000406f5(&DAT_000633b8,0x32);
  uVar1 = local_140;
  do {
    do {
    } while (DAT_00063e89 != '\0');
  } while ((DAT_00063eb2 != '\0') || (DAT_00063ebe != '\0'));
  *(undefined4 *)((int)puVar6 + -4) = 0x39b4d;
  FUN_00041c31(uVar1);
  iVar5 = local_80;
  local_38 = local_80;
  if (local_80 == 0) {
    local_154 = 0;
  }
  else {
    *(undefined4 *)((int)puVar6 + -4) = 0x39b63;
    FUN_0003d999(iVar5,0);
    *(undefined4 *)((int)puVar6 + -4) = 0x39b68;
    local_154 = FUN_00041c28();
  }
  iVar5 = local_13c;
  local_34 = local_13c;
  if (local_13c == 0) {
    local_158 = 0;
  }
  else {
    *(undefined4 *)((int)puVar6 + -4) = 0x39b93;
    FUN_0003d999(iVar5,0);
    *(undefined4 *)((int)puVar6 + -4) = 0x39b98;
    local_158 = FUN_00041c28();
  }
  iVar5 = local_60;
  local_30 = local_60;
  if (local_60 == 0) {
    local_15c = 0;
  }
  else {
    *(undefined4 *)((int)puVar6 + -4) = 0x39bc0;
    FUN_0003d999(iVar5,0);
    *(undefined4 *)((int)puVar6 + -4) = 0x39bc5;
    local_15c = FUN_00041c28();
  }
  local_80 = 0;
  local_13c = 0;
  local_60 = 0;
  if (local_20 == 1) {
    *(undefined4 *)((int)puVar6 + -4) = 0x39bfa;
    FUN_00010dde();
  }
  return;
}


