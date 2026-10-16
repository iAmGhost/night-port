// func #129  FUN_0003a224  0003a224  size=880


undefined8 FUN_0003a224(void)

{
  int iVar1;
  int iVar2;
  undefined4 extraout_EDX;
  undefined1 *puVar3;
  undefined1 *puVar7;
  undefined1 *puVar8;
  int aiStack_6c [7];
  undefined1 local_50 [16];
  int local_40 [4];
  int local_30;
  int local_2c;
  int local_28;
  int local_24;
  int local_20;
  undefined1 *puVar4;
  undefined1 *puVar5;
  undefined1 *puVar6;
  
  FUN_000418ee();
  local_24 = 0;
  local_20 = 0;
  FUN_0003f645(0);
  local_40[3] = FUN_00041af8(0x4b000);
  local_30 = FUN_00041af8(0x4b000);
  for (local_2c = 0; local_2c < 2; local_2c = local_2c + 1) {
    FUN_000428f8();
    iVar1 = FUN_00041af8(0x4b000);
    local_40[local_2c] = iVar1;
    FUN_00036417(local_50,local_40[local_2c],0);
  }
  FUN_00036417("title1.pcx",local_40[3],1);
  puVar7 = local_50;
  for (local_2c = 0; local_2c < 0x1e0; local_2c = local_2c + 2) {
    for (local_28 = 0; local_28 < 0x280; local_28 = local_28 + 1) {
      *(uint *)(puVar7 + -4) = (uint)*(byte *)((0x1df - local_2c) * 0x280 + local_28 + local_40[3]);
      *(int *)(puVar7 + -8) = 0x1e0 - local_2c;
      *(undefined4 *)(puVar7 + -0xc) = 0;
      *(int *)(puVar7 + -0x10) = local_28;
      *(undefined4 *)(puVar7 + -0x14) = 0x280;
      *(int *)(puVar7 + -0x18) = local_30;
      *(undefined4 *)(puVar7 + -0x1c) = 0x3a342;
      FUN_00040506();
    }
    *(undefined4 *)(puVar7 + -4) = 0x3a375;
    FUN_00041e6c((0x1df - local_2c) * 0x280 + local_30,(0x1df - local_2c) * 0x280 + local_40[3],
                 local_2c * 0x280);
    puVar4 = puVar7 + -4;
    puVar5 = puVar7 + -4;
    puVar6 = puVar7 + -4;
    puVar3 = puVar7 + -4;
    *(int *)(puVar7 + -4) = local_30;
    *(undefined4 *)(puVar7 + -8) = 0x3a38c;
    FUN_0003f462(0,0,0x280,0x1e0);
    puVar7 = puVar7 + -4;
    if ((((DAT_00063ec1 != '\0') || (puVar7 = puVar6, DAT_00063ea4 != '\0')) ||
        (puVar7 = puVar5, DAT_00063ea5 != '\0')) || (puVar7 = puVar4, DAT_00063ec0 != '\0'))
    goto LAB_0003a409;
    puVar7 = puVar3;
  }
  for (local_2c = 0; local_2c < 0x400; local_2c = local_2c + 1) {
    *(undefined4 *)(puVar7 + -4) = 0x3a3dd;
    thunk_FUN_0004564f(1);
    if (((DAT_00063ec1 != '\0') || (DAT_00063ea4 != '\0')) ||
       ((DAT_00063ea5 != '\0' || (DAT_00063ec0 != '\0')))) break;
  }
LAB_0003a409:
  do {
    do {
    } while (DAT_00063ec1 != '\0');
  } while (((DAT_00063ea4 != '\0') || (DAT_00063ea5 != '\0')) || (DAT_00063ec0 != '\0'));
  DAT_00063f08 = 0;
  *(undefined4 *)(puVar7 + -4) = 0x3a447;
  FUN_00010b1d(9);
  *(undefined4 *)(puVar7 + -4) = 0x3a451;
  iVar1 = FUN_0004185e(DAT_00053dd4);
  do {
    if ((((DAT_00063ea4 != '\0') || (DAT_00063ec1 != '\0')) || (DAT_00063ea5 != '\0')) ||
       ((DAT_00063ec0 != '\0' || (DAT_00063e89 != '\0')))) goto LAB_0003a529;
    if (((DAT_00063ed0 != '\0') || (DAT_00063ed8 != '\0')) && (local_20 == 0)) {
      local_24 = 1 - local_24;
      local_20 = 1;
      *(undefined4 *)(puVar7 + -4) = 0x3a4bf;
      iVar1 = FUN_0004185e(DAT_00053dd8);
    }
    iVar2 = local_20;
    if ((DAT_00063ed0 == '\0') && (DAT_00063ed8 == '\0')) {
      local_20 = 0x13;
      iVar2 = iVar1;
    }
    else {
      local_20 = local_20 + 1;
      if (local_20 == 0x14) {
        local_20 = 0;
      }
    }
    *(undefined4 *)(puVar7 + -4) = 0x3a4f2;
    FUN_00035ec2(iVar2);
    puVar8 = puVar7 + -4;
    *(int *)(puVar7 + -4) = local_40[local_24];
    *(undefined4 *)(puVar7 + -8) = 0x3a50f;
    iVar1 = FUN_0003f462(0,0,0x280,0x1e0);
    puVar7 = puVar7 + -4;
  } while (DAT_00063f08 < 0xc4f);
  local_24 = 3;
  puVar7 = puVar8;
LAB_0003a529:
  if (DAT_00063e89 != '\0') {
    local_24 = 3;
  }
  if (local_24 != 3) {
    *(undefined4 *)(puVar7 + -4) = 0x3a549;
    FUN_0004185e(DAT_00053ddc);
  }
  *(undefined4 *)(puVar7 + -4) = 0x3a558;
  FUN_000406f5(&DAT_000636b8,0x32);
  iVar1 = local_40[3];
  *(undefined4 *)(puVar7 + -4) = 0x3a560;
  FUN_00041c31(iVar1);
  *(undefined4 *)(puVar7 + -4) = 0x3a568;
  FUN_00041c31(local_40[0]);
  *(undefined4 *)(puVar7 + -4) = 0x3a570;
  FUN_00041c31(local_40[1]);
  iVar1 = local_30;
  *(undefined4 *)(puVar7 + -4) = 0x3a578;
  FUN_00041c31(iVar1);
  *(undefined4 *)(puVar7 + -4) = 0x3a57d;
  FUN_00010dde();
  *(undefined4 *)(puVar7 + -4) = 0x3a582;
  FUN_000405a7();
  return CONCAT44(extraout_EDX,local_24);
}


