// func #128  FUN_00039c03  00039c03  size=1456


/* WARNING: Heritage AFTER dead removal. Example location: s0xffffffc2 : 0x00039e3b */
/* WARNING: Restarted to delay deadcode elimination for space: stack */

undefined8 FUN_00039c03(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 extraout_EDX;
  undefined4 *puVar4;
  undefined1 *puVar5;
  undefined4 *puVar6;
  byte bVar7;
  undefined4 local_54 [5];
  byte local_40 [4];
  byte local_3c;
  undefined4 local_38;
  int local_34;
  int local_30;
  int local_2c;
  undefined4 local_28;
  undefined4 local_24;
  int local_20;
  
  bVar7 = 0;
  FUN_000418ee();
  local_20 = 0;
  local_34 = 0;
  local_2c = 0;
  local_30 = 0;
  puVar4 = &DAT_000531e0;
  puVar6 = local_54;
  for (iVar3 = 5; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar6 = *puVar4;
    puVar4 = puVar4 + (uint)bVar7 * -2 + 1;
    puVar6 = puVar6 + (uint)bVar7 * -2 + 1;
  }
  local_28 = DAT_00063e24;
  local_24 = DAT_00063e28;
  DAT_00052ff8 = 0;
  FUN_00041e20(local_40,0,4);
  if (DAT_00053338 == '\0') {
    local_40[3] = 1;
  }
  if (DAT_00051a10 == 0) {
    local_40[2] = '\x01';
  }
  local_3c = 1;
  FUN_0004185e(DAT_00053dd4);
  FUN_00041e6c(DAT_000633b4,DAT_000633b0,64000);
  puVar4 = local_54;
LAB_00039cb9:
  do {
    if ((((DAT_00063ea4 == '\0') && (DAT_00063ec1 == '\0')) && (DAT_00063ea5 == '\0')) &&
       (DAT_00063ec0 == '\0')) {
      if (((DAT_00063e89 == '\0') && (DAT_00063eb2 == '\0')) && (DAT_00063ebe == '\0')) {
        if ((DAT_00063ed0 != '\0') && (local_34 == 0)) {
          if (local_20 == 0) {
            local_20 = 4;
          }
          else {
            local_20 = local_20 + -1;
          }
          local_34 = 1;
          *(undefined4 *)((int)puVar4 + -4) = 0x39d61;
          FUN_0004185e(DAT_00053dd8);
        }
        if (DAT_00063ed0 == '\0') {
          local_34 = 0x13;
        }
        else {
          local_34 = local_34 + 1;
          if (local_34 == 0x14) {
            local_34 = 0;
          }
        }
        if ((DAT_00063ed8 != '\0') && (local_30 == 0)) {
          if (local_20 == 4) {
            local_20 = 0;
          }
          else {
            local_20 = local_20 + 1;
          }
          local_30 = 1;
          *(undefined4 *)((int)puVar4 + -4) = 0x39dbd;
          FUN_0004185e(DAT_00053dd8);
        }
        if (DAT_00063ed8 == '\0') {
          local_30 = 0x13;
        }
        else {
          local_30 = local_30 + 1;
          if (local_30 == 0x14) {
            local_30 = 0;
          }
        }
        if (((DAT_00063ed3 != '\0') || (DAT_00063ed5 != '\0')) && (local_2c == 0)) {
          if (local_20 == 3) {
            *(undefined4 *)((int)puVar4 + -4) = 0x39e1f;
            FUN_0004185e(DAT_00053dd8);
            DAT_00053338 = '\x01' - local_40[local_20];
          }
          if (local_20 == 2) {
            DAT_00051a10 = 1 - (uint)(byte)(1 - local_40[2]);
            if ((byte)(1 - local_40[2]) == 0) {
              uVar1 = *(undefined4 *)
                       (&DAT_00052da4 +
                       *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28);
              *(undefined4 *)((int)puVar4 + -4) = 0x39e91;
              FUN_00010b1d(uVar1);
            }
            else {
              *(undefined4 *)((int)puVar4 + -4) = 0x39e6e;
              FUN_00010dde();
            }
            *(undefined4 *)((int)puVar4 + -4) = 0x39e9b;
            FUN_0004185e(DAT_00053dd8);
          }
          if (local_20 == 4) {
            *(undefined4 *)((int)puVar4 + -4) = 0x39ebb;
            FUN_0004185e(DAT_00053ddc);
          }
        }
        if ((DAT_00063ed3 == '\0') && (DAT_00063ed5 == '\0')) {
          local_2c = 0x13;
        }
        else {
          local_2c = local_2c + 1;
          if (local_2c == 0x14) {
            local_2c = 0;
          }
        }
        *(undefined4 *)((int)puVar4 + -4) = 0x39efe;
        FUN_00041e6c(DAT_000633b0,DAT_000633b4,64000);
        *(undefined4 *)((int)puVar4 + -4) = 0;
        *(int *)((int)puVar4 + -8) = *(int *)(DAT_00063e0c + 0x38) + **(int **)(DAT_00063e0c + 0x34)
        ;
        *(undefined4 *)((int)puVar4 + -0xc) = **(undefined4 **)(DAT_00063e0c + 8);
        uVar1 = **(undefined4 **)(DAT_00063e0c + 4);
        *(undefined4 *)((int)puVar4 + -0x10) = 0x39f3d;
        FUN_0003bb42(DAT_000633b0,0xbe,99,uVar1);
        *(undefined4 *)((int)puVar4 + -0x10) = 0;
        *(int *)((int)puVar4 + -0x14) =
             *(int *)(DAT_00063e0c + 0x38) +
             *(int *)(*(int *)(DAT_00063e0c + 0x34) + local_20 * 4 + 4);
        puVar5 = (undefined1 *)((int)puVar4 + -0x18);
        *(undefined4 *)((int)puVar4 + -0x18) =
             *(undefined4 *)(*(int *)(DAT_00063e0c + 8) + local_20 * 4 + 4);
        uVar1 = *(undefined4 *)(*(int *)(DAT_00063e0c + 4) + local_20 * 4 + 4);
        uVar2 = local_54[local_20];
        *(undefined4 *)((int)puVar4 + -0x1c) = 0x39f9b;
        FUN_0003bb42(DAT_000633b0,200,uVar2,uVar1);
        if ((local_20 == 2) || (local_20 == 3)) {
          *(undefined4 *)((int)puVar4 + -0x1c) = 0;
          *(int *)((int)puVar4 + -0x20) =
               *(int *)(DAT_00063e0c + 0x38) +
               *(int *)(*(int *)(DAT_00063e0c + 0x34) + (uint)local_40[local_20] * 4 + 0x18);
          puVar5 = (undefined1 *)((int)puVar4 + -0x24);
          *(undefined4 *)((int)puVar4 + -0x24) =
               *(undefined4 *)(*(int *)(DAT_00063e0c + 8) + (uint)local_40[local_20] * 4 + 0x18);
          uVar1 = *(undefined4 *)(*(int *)(DAT_00063e0c + 4) + (uint)local_40[local_20] * 4 + 0x18);
          *(undefined4 *)((int)puVar4 + -0x28) = 0x3a017;
          FUN_0003bb42(DAT_000633b0,0x100,0xad,uVar1);
        }
        else if (local_20 == 4) {
          *(undefined4 *)((int)puVar4 + -0x1c) = 0;
          *(int *)((int)puVar4 + -0x20) =
               *(int *)(DAT_00063e0c + 0x38) +
               *(int *)(*(int *)(DAT_00063e0c + 0x34) + (uint)local_3c * 4 + 0x20);
          puVar5 = (undefined1 *)((int)puVar4 + -0x24);
          *(undefined4 *)((int)puVar4 + -0x24) =
               *(undefined4 *)(*(int *)(DAT_00063e0c + 8) + (uint)local_3c * 4 + 0x20);
          uVar1 = *(undefined4 *)(*(int *)(DAT_00063e0c + 4) + (uint)local_3c * 4 + 0x20);
          *(undefined4 *)((int)puVar4 + -0x28) = 0x3a08f;
          FUN_0003bb42(DAT_000633b0,0x100,0xad,uVar1);
        }
        *(undefined4 *)(puVar5 + -4) = 0x3a094;
        FUN_00035ec2();
        *(undefined4 *)(puVar5 + -4) = 0x3a0a9;
        FUN_00041e6c(&DAT_000a0000,DAT_000633b0,64000);
        puVar4 = (undefined4 *)puVar5;
        goto LAB_00039cb9;
      }
      do {
        do {
        } while (DAT_00063e89 != '\0');
      } while ((DAT_00063eb2 != '\0') || (DAT_00063ebe != '\0'));
      local_20 = 100;
    }
    *(undefined4 *)((int)puVar4 + -4) = 0x3a0b8;
    FUN_0004185e(DAT_00053ddc);
    if (local_20 == 0) {
      *(undefined4 *)((int)puVar4 + -4) = 0x3a0d0;
      FUN_000406f5(DAT_00063354 + 0x48,0x32);
      *(undefined4 *)((int)puVar4 + -4) = 0x3a0d7;
      FUN_00038f3d(0);
      *(undefined4 *)((int)puVar4 + -4) = 0x3a0ec;
      FUN_00041e6c(&DAT_000a0000,DAT_000633b4,64000);
      *(undefined4 *)((int)puVar4 + -4) = 0x3a0fe;
      FUN_00040660(DAT_00063354 + 0x48,0x32);
    }
    else {
      if (local_20 != 1) {
        *(undefined4 *)((int)puVar4 + -4) = 0x3a15d;
        FUN_00041e20(&DAT_00062c58,0,0x80);
        DAT_00063e24 = local_28;
        DAT_00063e28 = local_24;
        DAT_00052ff8 = 1;
        if ((local_20 == 4) && (local_3c == 0)) {
          *(undefined4 *)((int)puVar4 + -4) = 0x3a197;
          FUN_000406f5(DAT_00063354 + 0x48,0x32);
          local_38 = 0;
        }
        else {
          local_38 = 1;
        }
        return CONCAT44(extraout_EDX,local_38);
      }
      *(undefined4 *)((int)puVar4 + -4) = 0x3a11b;
      FUN_000406f5(DAT_00063354 + 0x48,0x32);
      *(undefined4 *)((int)puVar4 + -4) = 0x3a120;
      FUN_00038cde();
      *(undefined4 *)((int)puVar4 + -4) = 0x3a135;
      FUN_00041e6c(&DAT_000a0000,DAT_000633b4,64000);
      *(undefined4 *)((int)puVar4 + -4) = 0x3a147;
      FUN_00040660(DAT_00063354 + 0x48,0x32);
    }
  } while( true );
}


