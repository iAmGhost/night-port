// func #121  FUN_00038445  00038445  size=1499


undefined8 FUN_00038445(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 extraout_EDX;
  int extraout_EDX_00;
  int extraout_EDX_01;
  int *piVar4;
  undefined1 *puVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  int *piVar8;
  byte bVar9;
  int local_b0;
  int local_ac [6];
  int local_94 [4];
  int local_84;
  undefined4 local_7c [6];
  undefined1 local_64 [16];
  int local_54 [8];
  int local_34;
  int local_30;
  int local_2c;
  int local_28;
  int local_24;
  char local_20;
  
  bVar9 = 0;
  FUN_000418ee();
  local_54[3] = 0;
  local_54[4] = 0;
  piVar4 = &DAT_00053108;
  piVar8 = local_94;
  for (iVar3 = 6; iVar3 != 0; iVar3 = iVar3 + -1) {
    *piVar8 = *piVar4;
    piVar4 = piVar4 + (uint)bVar9 * -2 + 1;
    piVar8 = piVar8 + (uint)bVar9 * -2 + 1;
  }
  puVar6 = &DAT_00053120;
  puVar7 = local_7c;
  for (iVar3 = 6; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar7 = *puVar6;
    puVar6 = puVar6 + (uint)bVar9 * -2 + 1;
    puVar7 = puVar7 + (uint)bVar9 * -2 + 1;
  }
  piVar4 = &DAT_00053138;
  piVar8 = local_ac;
  for (iVar3 = 6; iVar3 != 0; iVar3 = iVar3 + -1) {
    *piVar8 = *piVar4;
    piVar4 = piVar4 + (uint)bVar9 * -2 + 1;
    piVar8 = piVar8 + (uint)bVar9 * -2 + 1;
  }
  local_20 = '\0';
  FUN_000406f5(DAT_00063354 + 0x48,0x32);
  local_2c = FUN_00041bdc(0x3c);
  local_28 = local_2c;
  if (local_2c == 0) {
    local_54[7] = 0;
  }
  else {
    local_54[7] = FUN_0003d8d4(local_2c,"number3.spr");
    local_30 = local_54[7];
  }
  local_34 = local_54[7];
  for (local_54[6] = 0; local_54[6] < 3; local_54[6] = local_54[6] + 1) {
    iVar3 = FUN_00041af8(64000);
    local_54[local_54[6]] = iVar3;
    FUN_000428f8();
    FUN_00036190(local_64,local_54[local_54[6]]);
  }
  FUN_00041e6c(&DAT_000a0000,local_54[0],64000);
  piVar4 = &local_b0;
  for (local_54[6] = 0; local_54[6] < 6; local_54[6] = local_54[6] + 1) {
    *(undefined4 *)((int)piVar4 + -4) = 0;
    *(int *)((int)piVar4 + -8) =
         *(int *)(DAT_00063e08 + 0x38) +
         *(int *)(local_54[6] * 4 + *(int *)(DAT_00063e08 + 0x34) + 4);
    *(undefined4 *)((int)piVar4 + -0xc) =
         *(undefined4 *)(*(int *)(DAT_00063e08 + 8) + local_54[6] * 4 + 4);
    uVar1 = *(undefined4 *)(*(int *)(DAT_00063e08 + 4) + local_54[6] * 4 + 4);
    uVar2 = local_7c[local_54[6]];
    iVar3 = local_94[local_54[6]];
    *(undefined4 *)((int)piVar4 + -0x10) = 0x385cc;
    FUN_0003bb42(&DAT_000a0000,iVar3,uVar2,uVar1);
    piVar4 = (int *)((int)piVar4 + -0xc);
  }
  *(undefined4 *)((int)piVar4 + -4) = 0;
  *(undefined4 *)((int)piVar4 + -8) = *(undefined4 *)(DAT_00063e08 + 0x38);
  *(undefined4 *)((int)piVar4 + -0xc) = **(undefined4 **)(DAT_00063e08 + 8);
  uVar1 = **(undefined4 **)(DAT_00063e08 + 4);
  *(undefined4 *)((int)piVar4 + -0x10) = 0x38606;
  FUN_0003bb42(&DAT_000a0000,200,local_54[3] * 0x12 + 0x5a,uVar1);
  *(int *)((int)piVar4 + -0x10) = DAT_00053100;
  *(undefined4 *)((int)piVar4 + -0x14) = 0x38623;
  FUN_000383a4(DAT_000633b0,local_34,0x118,0xae);
  *(undefined4 *)((int)piVar4 + -0x14) = 0x38632;
  FUN_00040660(&DAT_000633b8,0x32);
  *(undefined4 *)((int)piVar4 + -0x14) = 0x3863c;
  FUN_00010b1d(0xc);
  iVar3 = extraout_EDX_00;
  while( true ) {
    puVar5 = (undefined1 *)((int)piVar4 + -0x10);
    if ((((DAT_00063ea4 != '\0') || (DAT_00063ec1 != '\0')) || (DAT_00063ea5 != '\0')) ||
       (DAT_00063ec0 != '\0')) break;
    if (((DAT_00063ed0 != '\0') || (DAT_00063ed8 != '\0')) && (local_20 == '\0')) {
      local_54[3] = 1 - local_54[3];
      local_20 = '\x01';
      *(undefined4 *)((int)piVar4 + -0x14) = 0x3869f;
      FUN_0004185e(DAT_00053dd8,iVar3);
    }
    if ((DAT_00063ed0 == '\0') && (DAT_00063ed8 == '\0')) {
      local_20 = '\x13';
    }
    else {
      local_20 = local_20 + '\x01';
      if (local_20 == '\x14') {
        local_20 = '\0';
      }
    }
    iVar3 = local_54[local_ac[(int)((longlong)
                                    ((ulonglong)(uint)((int)((longlong)local_54[4] % 0x4e) >> 0x1f)
                                     << 0x20 | (longlong)local_54[4] % 0x4e & 0xffffffffU) / 0xd)]];
    *(undefined4 *)((int)piVar4 + -0x14) = 0x38709;
    FUN_00041e6c(DAT_000633b0,iVar3,64000);
    piVar4 = (int *)((int)piVar4 + -0x10);
    for (local_54[6] = 0; local_54[6] < 6; local_54[6] = local_54[6] + 1) {
      *(undefined4 *)((int)piVar4 + -4) = 0;
      *(int *)((int)piVar4 + -8) =
           *(int *)(DAT_00063e08 + 0x38) +
           *(int *)(local_54[6] * 4 + *(int *)(DAT_00063e08 + 0x34) + 4);
      *(undefined4 *)((int)piVar4 + -0xc) =
           *(undefined4 *)(*(int *)(DAT_00063e08 + 8) + local_54[6] * 4 + 4);
      uVar1 = *(undefined4 *)(*(int *)(DAT_00063e08 + 4) + local_54[6] * 4 + 4);
      uVar2 = local_7c[local_54[6]];
      iVar3 = local_94[local_54[6]];
      *(undefined4 *)((int)piVar4 + -0x10) = 0x38781;
      FUN_0003bb42(DAT_000633b0,iVar3,uVar2,uVar1);
      piVar4 = (int *)((int)piVar4 + -0xc);
    }
    *(undefined4 *)((int)piVar4 + -4) = 0;
    *(undefined4 *)((int)piVar4 + -8) = *(undefined4 *)(DAT_00063e08 + 0x38);
    *(undefined4 *)((int)piVar4 + -0xc) = **(undefined4 **)(DAT_00063e08 + 8);
    uVar1 = **(undefined4 **)(DAT_00063e08 + 4);
    *(undefined4 *)((int)piVar4 + -0x10) = 0x387bb;
    FUN_0003bb42(DAT_000633b0,200,local_54[3] * 0x12 + 0x5a,uVar1);
    *(int *)((int)piVar4 + -0x10) = DAT_00053100;
    *(undefined4 *)((int)piVar4 + -0x14) = 0x387d8;
    FUN_000383a4(DAT_000633b0,local_34,0x118,0xae);
    *(undefined4 *)((int)piVar4 + -0x14) = 0x387dd;
    FUN_00035ec2();
    if (local_54[4] % 3 == 0) {
      *(undefined4 *)((int)piVar4 + -0x14) = 0x38805;
      FUN_0003d67c(&DAT_000633b8,0x70,0x7f);
      *(undefined4 *)((int)piVar4 + -0x14) = 0x38819;
      FUN_0003d67c(&DAT_000633b8,0x40,0x4f);
      *(undefined **)((int)piVar4 + -0x14) = &DAT_000633b8;
      *(undefined4 *)((int)piVar4 + -0x18) = 0xff;
      *(undefined4 *)((int)piVar4 + -0x1c) = 0;
      *(undefined4 *)((int)piVar4 + -0x20) = 0x3882d;
      FUN_00040042();
    }
    *(undefined4 *)((int)piVar4 + -0x14) = 0x38845;
    FUN_00041e6c(&DAT_000a0000,DAT_000633b0,64000);
    local_54[4] = local_54[4] + 1;
    iVar3 = DAT_00063e38 % 0x1ea;
    if (DAT_00053c48 < DAT_00063e38 / 0x1ea) {
      DAT_00063e38 = 0;
      *(undefined4 *)((int)piVar4 + -0x14) = 0x3887c;
      FUN_00010b1d(0xc);
      iVar3 = extraout_EDX_01;
    }
  }
  *(undefined4 *)((int)piVar4 + -0x14) = 0x3888b;
  FUN_0004185e(DAT_00053ddc,iVar3);
  if (local_54[3] == 0) {
    *(undefined4 *)((int)piVar4 + -0x14) = 0x388a1;
    FUN_00036190("continue.pcx",DAT_000633b0);
    puVar5 = (undefined1 *)((int)piVar4 + -0x10);
  }
  else {
    *(undefined4 *)((int)piVar4 + -0x14) = 0x388b3;
    FUN_00036190("gameove1.pcx",DAT_000633b0);
  }
  do {
    iVar3 = local_54[4];
    local_54[4] = local_54[4] + 1;
    *(undefined4 *)(puVar5 + -4) = 0x388be;
    FUN_00035ec2(iVar3);
    *(undefined4 *)(puVar5 + -4) = 0x388d3;
    FUN_00041e6c(&DAT_000a0000,DAT_000633b0,64000);
    for (local_54[6] = 0; local_54[6] < 6; local_54[6] = local_54[6] + 1) {
      *(undefined4 *)(puVar5 + -4) = 0;
      *(int *)(puVar5 + -8) =
           *(int *)(DAT_00063e08 + 0x38) +
           *(int *)(*(int *)(DAT_00063e08 + 0x34) + local_54[6] * 4 + 4);
      *(undefined4 *)(puVar5 + -0xc) =
           *(undefined4 *)(*(int *)(DAT_00063e08 + 8) + local_54[6] * 4 + 4);
      uVar1 = *(undefined4 *)(*(int *)(DAT_00063e08 + 4) + local_54[6] * 4 + 4);
      uVar2 = local_7c[local_54[6]];
      iVar3 = local_94[local_54[6]];
      *(undefined4 *)(puVar5 + -0x10) = 0x3894f;
      FUN_0003bb42(&DAT_000a0000,iVar3,uVar2,uVar1);
      if (local_54[6] % 2 == 0) {
        local_94[local_54[6]] = local_94[local_54[6]] + -9;
      }
      else {
        local_94[local_54[6]] = local_94[local_54[6]] + 9;
      }
      puVar5 = puVar5 + -0xc;
    }
  } while (-0x3c < local_84);
  *(undefined4 *)(puVar5 + -4) = 0x38999;
  FUN_000406f5(&DAT_000633b8,0x32);
  local_24 = local_34;
  if (local_34 == 0) {
    local_b0 = 0;
  }
  else {
    *(undefined4 *)(puVar5 + -4) = 0x389af;
    FUN_0003d999(local_34,0);
    *(undefined4 *)(puVar5 + -4) = 0x389b4;
    local_b0 = FUN_00041c28();
  }
  local_34 = 0;
  iVar3 = local_b0;
  for (local_54[6] = 0; local_54[6] < 3; local_54[6] = local_54[6] + 1) {
    iVar3 = local_54[local_54[6]];
    *(undefined4 *)(puVar5 + -4) = 0x389f1;
    FUN_00041c31(iVar3);
    iVar3 = local_54[6];
  }
  DAT_00053100 = DAT_00053100 + -1;
  *(undefined4 *)(puVar5 + -4) = 0x389fe;
  FUN_00010dde(iVar3);
  if (local_54[3] == 0) {
    local_54[5] = 2;
  }
  else {
    local_54[5] = 0;
  }
  return CONCAT44(extraout_EDX,local_54[5]);
}


