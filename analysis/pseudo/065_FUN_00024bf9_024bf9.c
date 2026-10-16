// func #65  FUN_00024bf9  00024bf9  size=6565


/* WARNING: Removing unreachable block (ram,0x0002654e) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00024bf9(void)

{
  int iVar1;
  undefined4 uVar2;
  int extraout_ECX;
  int extraout_ECX_00;
  int iVar3;
  int **ppiVar4;
  float10 fVar5;
  undefined8 uVar6;
  int *piStack_98;
  int *piStack_94;
  undefined1 local_90 [4];
  int local_8c;
  undefined4 local_88;
  undefined4 local_84;
  int local_80;
  int local_7c;
  int local_78;
  int local_74;
  int local_70;
  int local_6c;
  int local_68;
  uint local_64;
  int local_60;
  undefined4 uStack_5c;
  float local_58;
  int local_54;
  int local_50;
  int local_4c;
  int local_48;
  int local_44;
  int local_40;
  int local_3c;
  int local_38;
  int local_34;
  undefined4 local_30;
  undefined4 local_2c;
  int local_28;
  int local_24;
  char *local_20;
  
  local_20 = (char *)FUN_000418ee();
  piStack_94 = (int *)0x24c1e;
  local_50 = FUN_0001519f(DAT_00053c7c);
  piStack_94 = (int *)0x24c2b;
  local_4c = FUN_000151d5(DAT_00053c7c);
  piStack_94 = (int *)0x24c36;
  local_30 = FUN_0001519f(local_20);
  piStack_94 = (int *)0x24c41;
  local_2c = FUN_000151d5(local_20);
  if (local_20 == DAT_00053c7c) {
    return;
  }
  if ((*(int *)(local_20 + 0x1f) < 10) || (0x13 < *(int *)(local_20 + 0x1f))) {
    local_34 = *(int *)(&DAT_00053ea0 + *(int *)(local_20 + 0xd) * 0x88);
  }
  else {
    local_34 = *(int *)(&DAT_00053f1c + *(int *)(local_20 + 0xd) * 0x88);
  }
  if (*(int *)(local_20 + 0x6f) == 1) {
    local_34 = *(int *)(&DAT_00053f1c + *(int *)(local_20 + 0xd) * 0x88);
  }
  fVar5 = (float10)*(float *)(&DAT_00051d5c + local_34 * 4);
  piStack_94 = (int *)0x24cba;
  FUN_00041f7e();
  local_48 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)(local_20 + 99) + (float10)*(float *)(&DAT_00051d5c + local_34 * 4);
  piStack_94 = (int *)0x24cd4;
  FUN_00041f7e();
  local_54 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)(local_20 + 99);
  piStack_94 = (int *)0x24ce2;
  FUN_00041f7e();
  local_58 = (float)(int)ROUND(fVar5);
  if ((int)local_58 + local_48 < local_54) {
    local_48 = local_48 + 1;
  }
  *(float *)(local_20 + 99) = *(float *)(&DAT_00051d5c + local_34 * 4) + *(float *)(local_20 + 99);
  local_44 = 0;
  local_40 = 0;
  if (((((*(int *)(DAT_00053c7c + 0x17) != 0x15) && (*(int *)(DAT_00053c7c + 0x17) != 0x16)) &&
       (*(int *)(DAT_00053c7c + 0x17) != 0x17)) &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x18 && (*(int *)(DAT_00053c7c + 0x17) != 0x1a)))) &&
     ((*(int *)(DAT_00053c7c + 0x17) != 0x1d &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x1e && (*(int *)(DAT_00053c7c + 0x17) != 0x20)))))) {
    piStack_94 = (int *)0x24d87;
    local_3c = FUN_00015b5b(local_20);
    if (local_3c != 0) {
      *(int *)(local_20 + 0x10f) = local_3c;
      local_20[0x1f] = '\n';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
    }
  }
  if ((int)*(short *)(local_20 + 0x23) + *(int *)(local_20 + 0x56) +
      (int)*(short *)(local_20 + 0x29) != *(int *)(local_20 + 0x77)) {
    *(int *)(local_20 + 0x7b) =
         *(int *)(local_20 + 0x77) -
         ((int)*(short *)(local_20 + 0x23) + *(int *)(local_20 + 0x56) +
         (int)*(short *)(local_20 + 0x29));
    *(int *)(local_20 + 0x77) =
         *(int *)(local_20 + 0x56) + (int)*(short *)(local_20 + 0x23) +
         (int)*(short *)(local_20 + 0x29);
  }
  if ((((*(uint *)(local_20 + 0x17) < 0x15) || (0x18 < *(uint *)(local_20 + 0x17))) &&
      (*(int *)(local_20 + 0x17) != 0x1a)) &&
     (((*(int *)(local_20 + 0x17) != 0x1d && (*(int *)(local_20 + 0x17) != 0x1e)) &&
      (*(int *)(local_20 + 0x17) != 0x20)))) {
    if (200 < *(int *)(local_20 + 0x73) -
              ((int)*(short *)(local_20 + 0x23) + *(int *)(local_20 + 0x56) +
              (int)*(short *)(local_20 + 0x29))) {
      *(int *)(local_20 + 0x73) =
           *(int *)(local_20 + 0x56) + (int)*(short *)(local_20 + 0x23) +
           (int)*(short *)(local_20 + 0x29);
      if (10 < *(int *)(local_20 + 0x7b)) {
        return;
      }
      local_20[0x17] = '/';
      local_20[0x18] = '\0';
      local_20[0x19] = '\0';
      local_20[0x1a] = '\0';
      local_20[0x11] = '\0';
      local_20[0x12] = '\0';
      local_20[0x13] = '\0';
      local_20[0x14] = '\0';
      return;
    }
  }
  else {
    *(int *)(local_20 + 0x73) =
         *(int *)(local_20 + 0x56) + (int)*(short *)(local_20 + 0x23) +
         (int)*(short *)(local_20 + 0x29);
  }
  if ((*(int *)(local_20 + 0x17) != 2) && (*(int *)(local_20 + 0x17) != 1)) {
    if (*(int *)(local_20 + 0x17) != 0x2e) {
      if (*(int *)(local_20 + 0x17) != 5) {
        return;
      }
      local_20[0x7f] = '\x01';
      local_20[0x80] = '\0';
      local_20[0x81] = '\0';
      local_20[0x82] = '\0';
      return;
    }
    if (*(int *)(local_20 + 0x11) == 0) {
      piStack_94 = (int *)0x24f04;
      uVar2 = FUN_0001519f(local_20);
      *(undefined4 *)(local_20 + 0xd3) = uVar2;
      piStack_94 = (int *)0x24f17;
      uVar2 = FUN_000151d5(local_20);
      *(undefined4 *)(local_20 + 0xd7) = uVar2;
      *(int *)(local_20 + 0xcb) = local_50;
      *(int *)(local_20 + 0xcf) = local_4c;
      piStack_94 = (int *)0x24f42;
      iVar1 = FUN_0001519f(local_20);
      if (*(int *)(local_20 + 0xcb) == iVar1) {
        local_20[0xdb] = '\0';
        local_20[0xdc] = 'P';
        local_20[0xdd] = -0x3d;
        local_20[0xde] = 'G';
      }
      else {
        piStack_94 = (int *)0x24f6a;
        iVar1 = FUN_000151d5(local_20);
        local_58 = (float)(*(int *)(local_20 + 0xcf) - iVar1);
        piStack_94 = (int *)0x24f88;
        local_54 = FUN_0001519f(local_20);
        local_54 = *(int *)(local_20 + 0xcb) - local_54;
        *(float *)(local_20 + 0xdb) = local_58 / (float)local_54;
      }
      piStack_94 = (int *)0x24faf;
      iVar1 = FUN_0001519f(local_20);
      piStack_94 = (int *)0x24fc1;
      iVar1 = FUN_00041f76(*(int *)(local_20 + 0xcb) - iVar1);
      piStack_94 = (int *)0x24fcb;
      iVar3 = FUN_000151d5(local_20);
      piStack_94 = (int *)0x24fdd;
      iVar3 = FUN_00041f76(*(int *)(local_20 + 0xcf) - iVar3);
      if (iVar3 < iVar1) {
        piStack_94 = (int *)0x24fed;
        iVar1 = FUN_0001519f(local_20);
        if (*(int *)(local_20 + 0xcb) - iVar1 < 0) {
          local_20[0xe3] = '\0';
          local_20[0xe4] = '\0';
          local_20[0xe5] = -0x20;
          local_20[0xe6] = -0x40;
          piStack_94 = (int *)0x25013;
          iVar1 = FUN_000151d5(local_20);
          *(float *)(local_20 + 0xe7) = (float)(*(int *)(local_20 + 0xcf) - iVar1) / _DAT_00050024;
          return;
        }
        local_20[0xe3] = '\0';
        local_20[0xe4] = '\0';
        local_20[0xe5] = -0x20;
        local_20[0xe6] = '@';
        piStack_94 = (int *)0x2504c;
        iVar1 = FUN_000151d5(local_20);
        *(float *)(local_20 + 0xe7) = (float)(*(int *)(local_20 + 0xcf) - iVar1) / _DAT_00050028;
        return;
      }
      piStack_94 = (int *)0x2507b;
      iVar1 = FUN_000151d5(local_20);
      if (*(int *)(local_20 + 0xcf) - iVar1 < 0) {
        piStack_94 = (int *)0x25094;
        iVar1 = FUN_0001519f(local_20);
        *(float *)(local_20 + 0xe3) = (float)(*(int *)(local_20 + 0xcb) - iVar1) / _DAT_0005002c;
        local_20[0xe7] = '\0';
        local_20[0xe8] = '\0';
        local_20[0xe9] = -0x80;
        local_20[0xea] = -0x40;
        return;
      }
      piStack_94 = (int *)0x250cd;
      iVar1 = FUN_0001519f(local_20);
      *(float *)(local_20 + 0xe3) = (float)(*(int *)(local_20 + 0xcb) - iVar1) / _DAT_00050030;
      local_20[0xe7] = '\0';
      local_20[0xe8] = '\0';
      local_20[0xe9] = -0x80;
      local_20[0xea] = '@';
      return;
    }
    if (*(int *)(local_20 + 0x11) < 1) {
      return;
    }
    if (0x46 < *(int *)(local_20 + 0x11)) {
      return;
    }
    uStack_5c = (undefined4)((ulonglong)(double)ABS(*(float *)(local_20 + 0xe3)) >> 0x20);
    if (ABS(*(float *)(local_20 + 0xe3)) <= ABS(*(float *)(local_20 + 0xe7))) {
      _local_60 = CONCAT44(uStack_5c,*(int *)(local_20 + 0x11) + -0x48);
      fVar5 = (float10)(*(int *)(local_20 + 0x11) + -0x48) * (float10)*(float *)(local_20 + 0xe7);
      piStack_94 = (int *)0x251a4;
      FUN_00041f7e();
      local_24 = (int)ROUND(fVar5);
      fVar5 = (float10)local_24 / (float10)*(float *)(local_20 + 0xdb);
      piStack_94 = (int *)0x251b8;
      FUN_00041f7e();
      local_24 = local_24 + *(int *)(local_20 + 0xd7);
      local_28 = (int)ROUND(fVar5) + *(int *)(local_20 + 0xd3);
    }
    else {
      _local_60 = CONCAT44(uStack_5c,*(int *)(local_20 + 0x11) + -0x48);
      fVar5 = (float10)(*(int *)(local_20 + 0x11) + -0x48) * (float10)*(float *)(local_20 + 0xe3);
      piStack_94 = (int *)0x25156;
      FUN_00041f7e();
      local_28 = (int)ROUND(fVar5);
      fVar5 = (float10)local_28 * (float10)*(float *)(local_20 + 0xdb);
      piStack_94 = (int *)0x2516a;
      FUN_00041f7e();
      local_28 = local_28 + *(int *)(local_20 + 0xd3);
      local_24 = (int)ROUND(fVar5) + *(int *)(local_20 + 0xd7);
    }
    if (((-1 < local_28) || (*(int *)(local_20 + 0xd3) < 0xb)) &&
       (((local_28 < 0x140 || (0x135 < *(int *)(local_20 + 0xd3))) &&
        ((*(int *)(DAT_00063354 + 0x12) + DAT_000639e0 <= local_24 &&
         (local_24 <= *(int *)(DAT_00063354 + 0x16) + DAT_000639e0)))))) {
      piStack_94 = (int *)0x252b5;
      iVar1 = FUN_0001519f(local_20);
      piStack_94 = (int *)0x252c2;
      FUN_0001545e(local_20,local_28 - iVar1);
      piStack_94 = (int *)0x252ca;
      iVar1 = FUN_000151d5(local_20);
      piStack_94 = (int *)0x252d7;
      FUN_0001586b(local_20,local_24 - iVar1);
      return;
    }
    if (local_28 == *(int *)(local_20 + 0xd3) || local_28 - *(int *)(local_20 + 0xd3) < 0) {
      if (local_28 - *(int *)(local_20 + 0xd3) < 0) {
        piStack_94 = (int *)0x2526b;
        FUN_0001545e(local_20,0xfffffffd);
      }
    }
    else {
      piStack_94 = (int *)0x2524a;
      FUN_0001545e(local_20,3);
    }
    if (local_24 != *(int *)(local_20 + 0xd7) && -1 < local_24 - *(int *)(local_20 + 0xd7)) {
      piStack_94 = (int *)0x2528a;
      FUN_0001586b(local_20,1);
      return;
    }
    if (-1 < local_24 - *(int *)(local_20 + 0xd7)) {
      return;
    }
    piStack_94 = (int *)0x252ab;
    FUN_0001586b(local_20,0xffffffff);
    return;
  }
  if (*(int *)(local_20 + 0x7f) == 1) {
    piStack_94 = (int *)0x25302;
    iVar1 = FUN_0001519f(local_20);
    piStack_94 = (int *)0x2530a;
    iVar1 = FUN_00041f76(iVar1 - local_50);
    if (iVar1 < 0x46) {
      piStack_94 = (int *)0x25317;
      iVar1 = FUN_000151d5(local_20);
      piStack_94 = (int *)0x2531f;
      iVar1 = FUN_00041f76(iVar1 - local_4c);
      if (iVar1 < 10) {
        local_20[0xaf] = '\n';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\x0e';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[0xab] = '\0';
        local_20[0xac] = '\0';
        local_20[0xad] = '\0';
        local_20[0xae] = '\0';
      }
    }
    local_20[0x7f] = '\0';
    local_20[0x80] = '\0';
    local_20[0x81] = '\0';
    local_20[0x82] = '\0';
  }
  if (*local_20 != '\x01') {
    return;
  }
  local_64 = *(uint *)(local_20 + 0x1f);
  if (8 < local_64) {
    if (local_64 < 10) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00017637(local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        local_20[0xaf] = '\t';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[199] = '\x01';
        local_20[200] = '\0';
        local_20[0xc9] = '\0';
        local_20[0xca] = '\0';
        ppiVar4 = &piStack_98;
      }
    }
    else if (local_64 < 0xe) {
      if (local_64 < 0xb) {
        if (*(int *)(local_20 + 0x10f) == 1) {
          local_20[0xaf] = '\n';
          local_20[0xb0] = '\0';
          local_20[0xb1] = '\0';
          local_20[0xb2] = '\0';
          local_20[0x1f] = '\v';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          local_20[0x11b] = '\x01';
          local_20[0x11c] = '\0';
          local_20[0x11d] = '\0';
          local_20[0x11e] = '\0';
          *(undefined4 *)(local_20 + 0xab) =
               *(undefined4 *)(&DAT_00053f20 + *(int *)(local_20 + 0xd) * 0x88);
        }
        ppiVar4 = (int **)local_90;
        if (*(int *)(local_20 + 0x10f) == 2) {
          local_20[0xaf] = '\n';
          local_20[0xb0] = '\0';
          local_20[0xb1] = '\0';
          local_20[0xb2] = '\0';
          local_20[0x1f] = '\v';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          local_20[0x11b] = '\x01';
          local_20[0x11c] = '\0';
          local_20[0x11d] = '\0';
          local_20[0x11e] = '\0';
          *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053f20 + *(int *)(local_20 + 0xd) * 0x88) >> 1
          ;
          ppiVar4 = (int **)local_90;
        }
      }
      else if (local_64 < 0xc) {
        if (*(int *)(local_20 + 0xab) < 0) {
          piStack_94 = (int *)0x26270;
          iVar1 = FUN_0001519f(local_20,(&DAT_00053eb8)
                                        [*(int *)(local_20 + 0x113) * 10 +
                                         *(int *)(local_20 + 0xd) * 0x88]);
          piStack_94 = (int *)0x26278;
          uVar6 = FUN_00041f76(iVar1 - local_50);
          if ((int)uVar6 < (int)((ulonglong)uVar6 >> 0x20)) {
            piStack_94 = (int *)0x26284;
            iVar1 = FUN_000151d5(local_20);
            piStack_94 = (int *)0x2628c;
            iVar1 = FUN_00041f76(iVar1 - local_4c);
            if (iVar1 < 6) {
              if (((&DAT_00053eb8)
                   [*(int *)(local_20 + 0x11b) +
                    *(int *)(local_20 + 0xd) * 0x88 + *(int *)(local_20 + 0x113) * 10] == '\0') ||
                 (9 < *(int *)(local_20 + 0x11b))) {
                local_20[0xaf] = '\n';
                local_20[0xb0] = '\0';
                local_20[0xb1] = '\0';
                local_20[0xb2] = '\0';
                local_20[0x1f] = '\0';
                local_20[0x20] = '\0';
                local_20[0x21] = '\0';
                local_20[0x22] = '\0';
                ppiVar4 = (int **)local_90;
              }
              else {
                *(uint *)(local_20 + 0x17) =
                     (uint)(byte)(&DAT_00053eb8)
                                 [*(int *)(local_20 + 0x11b) +
                                  *(int *)(local_20 + 0xd) * 0x88 + *(int *)(local_20 + 0x113) * 10]
                ;
                local_20[0x11] = '\0';
                local_20[0x12] = '\0';
                local_20[0x13] = '\0';
                local_20[0x14] = '\0';
                *(int *)(local_20 + 0x11b) = *(int *)(local_20 + 0x11b) + 1;
                ppiVar4 = (int **)local_90;
              }
              goto LAB_00026553;
            }
          }
          piStack_94 = (int *)0x26337;
          iVar1 = FUN_00041e9f();
          if (iVar1 % 0x14 == 2) {
            local_20[0xaf] = '\x04';
            local_20[0xb0] = '\0';
            local_20[0xb1] = '\0';
            local_20[0xb2] = '\0';
            local_20[0x1f] = '\0';
            local_20[0x20] = '\0';
            local_20[0x21] = '\0';
            local_20[0x22] = '\0';
            ppiVar4 = (int **)local_90;
          }
          else {
            local_20[0xaf] = '\n';
            local_20[0xb0] = '\0';
            local_20[0xb1] = '\0';
            local_20[0xb2] = '\0';
            local_20[0x1f] = '\0';
            local_20[0x20] = '\0';
            local_20[0x21] = '\0';
            local_20[0x22] = '\0';
            ppiVar4 = (int **)local_90;
          }
        }
        else {
          *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
          local_20[0x17] = '\x01';
          local_20[0x18] = '\0';
          local_20[0x19] = '\0';
          local_20[0x1a] = '\0';
          ppiVar4 = (int **)local_90;
        }
      }
      else if (local_64 < 0xd) {
        *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_90;
        if (*(int *)(local_20 + 0xab) < 0) {
          local_20[0x17] = '2';
          local_20[0x18] = '\0';
          local_20[0x19] = '\0';
          local_20[0x1a] = '\0';
          local_20[0x11] = '\0';
          local_20[0x12] = '\0';
          local_20[0x13] = '\0';
          local_20[0x14] = '\0';
          local_20[0xaf] = '\n';
          local_20[0xb0] = '\0';
          local_20[0xb1] = '\0';
          local_20[0xb2] = '\0';
          local_20[0x1f] = '\0';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          ppiVar4 = (int **)local_90;
        }
      }
      else {
        *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_90;
        if (*(int *)(local_20 + 0xab) < 0) {
          local_20[0x17] = '1';
          local_20[0x18] = '\0';
          local_20[0x19] = '\0';
          local_20[0x1a] = '\0';
          local_20[0x11] = '\0';
          local_20[0x12] = '\0';
          local_20[0x13] = '\0';
          local_20[0x14] = '\0';
          local_20[0xaf] = '\n';
          local_20[0xb0] = '\0';
          local_20[0xb1] = '\0';
          local_20[0xb2] = '\0';
          local_20[0x1f] = '\0';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          ppiVar4 = (int **)local_90;
        }
      }
    }
    else if (local_64 < 0xf) {
      *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
      ppiVar4 = (int **)local_90;
      if (*(int *)(local_20 + 0xab) < 0) {
        local_20[0x17] = '0';
        local_20[0x18] = '\0';
        local_20[0x19] = '\0';
        local_20[0x1a] = '\0';
        local_20[0x11] = '\0';
        local_20[0x12] = '\0';
        local_20[0x13] = '\0';
        local_20[0x14] = '\0';
        local_20[0xaf] = '\n';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        ppiVar4 = (int **)local_90;
      }
    }
    else if (local_64 < 0x15) {
      ppiVar4 = (int **)local_90;
      if (local_64 == 0x14) {
        piStack_94 = &local_40;
        piStack_98 = &local_44;
        iVar1 = FUN_0001776f(local_20,local_48,local_50,local_4c);
        ppiVar4 = &piStack_98;
        if (iVar1 != 0) {
          local_20[0xaf] = '\x14';
          local_20[0xb0] = '\0';
          local_20[0xb1] = '\0';
          local_20[0xb2] = '\0';
          local_20[0x1f] = '\0';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          local_20[199] = '\x01';
          local_20[200] = '\0';
          local_20[0xc9] = '\0';
          local_20[0xca] = '\0';
          ppiVar4 = &piStack_98;
        }
      }
    }
    else if (local_64 < 0x16) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00017927(local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        local_20[0xaf] = '\x15';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[199] = '\x01';
        local_20[200] = '\0';
        local_20[0xc9] = '\0';
        local_20[0xca] = '\0';
        ppiVar4 = &piStack_98;
      }
    }
    else if (local_64 < 0x17) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00017ca6(local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        local_20[0xaf] = '\x16';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[199] = '\x01';
        local_20[200] = '\0';
        local_20[0xc9] = '\0';
        local_20[0xca] = '\0';
        ppiVar4 = &piStack_98;
      }
    }
    else {
      ppiVar4 = (int **)local_90;
      if (local_64 == 0x6a) {
        fVar5 = (float10)_DAT_00051d68;
        piStack_94 = (int *)0x25ee1;
        FUN_00041f7e();
        local_48 = (int)ROUND(fVar5);
        fVar5 = (float10)*(float *)(local_20 + 99) + (float10)_DAT_00051d68;
        piStack_94 = (int *)0x25ef5;
        FUN_00041f7e();
        _local_60 = CONCAT44(uStack_5c,(int)ROUND(fVar5));
        fVar5 = (float10)*(float *)(local_20 + 99);
        piStack_94 = (int *)0x25f03;
        FUN_00041f7e();
        local_58 = (float)(int)ROUND(fVar5);
        if ((int)local_58 + local_48 < local_60) {
          local_48 = local_48 + 1;
        }
        *(float *)(local_20 + 99) = _DAT_00051d68 + *(float *)(local_20 + 99);
        piStack_94 = &local_40;
        piStack_98 = &local_44;
        iVar1 = FUN_0001730b(local_20,local_48,local_50,local_4c);
        ppiVar4 = &piStack_98;
        if (iVar1 != 0) {
          if ((((*(int *)(DAT_00053c7c + 0x17) == 0x15) || (*(int *)(DAT_00053c7c + 0x17) == 0x16))
              || (*(int *)(DAT_00053c7c + 0x17) == 0x17)) ||
             (((*(int *)(DAT_00053c7c + 0x17) == 0x18 || (*(int *)(DAT_00053c7c + 0x17) == 0x1a)) ||
              ((*(int *)(DAT_00053c7c + 0x17) == 0x1d ||
               ((*(int *)(DAT_00053c7c + 0x17) == 0x1e || (*(int *)(DAT_00053c7c + 0x17) == 0x20))))
              )))) {
            iVar1 = FUN_0001519f(local_20);
            iVar1 = FUN_00041f76(iVar1 - local_50);
            ppiVar4 = &piStack_98;
            if (iVar1 < 0x32) {
              iVar1 = FUN_000151d5(local_20);
              iVar1 = FUN_00041f76(iVar1 - local_4c);
              ppiVar4 = &piStack_98;
              if (iVar1 < 10) {
                local_20[0xaf] = '\n';
                local_20[0xb0] = '\0';
                local_20[0xb1] = '\0';
                local_20[0xb2] = '\0';
                local_20[0x1f] = '\r';
                local_20[0x20] = '\0';
                local_20[0x21] = '\0';
                local_20[0x22] = '\0';
                local_20[0xab] = '\0';
                local_20[0xac] = '\0';
                local_20[0xad] = '\0';
                local_20[0xae] = '\0';
                ppiVar4 = &piStack_98;
              }
            }
          }
          else {
            local_20[0xaf] = '\x06';
            local_20[0xb0] = '\0';
            local_20[0xb1] = '\0';
            local_20[0xb2] = '\0';
            local_20[0x1f] = '\0';
            local_20[0x20] = '\0';
            local_20[0x21] = '\0';
            local_20[0x22] = '\0';
            ppiVar4 = &piStack_98;
          }
        }
      }
    }
    goto LAB_00026553;
  }
  if (3 < local_64) {
    if (local_64 < 5) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      local_68 = FUN_00016fec(local_20,local_48,local_50,local_4c);
      if (((((*(int *)(DAT_00053c7c + 0x17) == 0x15) || (*(int *)(DAT_00053c7c + 0x17) == 0x16)) ||
           ((*(int *)(DAT_00053c7c + 0x17) == 0x17 ||
            ((*(int *)(DAT_00053c7c + 0x17) == 0x18 || (*(int *)(DAT_00053c7c + 0x17) == 0x1a))))))
          || (*(int *)(DAT_00053c7c + 0x17) == 0x1d)) ||
         ((*(int *)(DAT_00053c7c + 0x17) == 0x1e || (*(int *)(DAT_00053c7c + 0x17) == 0x20)))) {
        local_68 = 4;
      }
      if (local_68 == 2) {
        local_20[0x1f] = '\x01';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[0xab] = '\n';
        local_20[0xac] = '\0';
        local_20[0xad] = '\0';
        local_20[0xae] = '\0';
        ppiVar4 = &piStack_98;
      }
      else if (local_68 == 1) {
        local_20[0xaf] = '\x04';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        ppiVar4 = &piStack_98;
      }
      else if (local_68 == 3) {
        local_20[0xaf] = '\x04';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\f';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[0xab] = '\x1e';
        local_20[0xac] = '\0';
        local_20[0xad] = '\0';
        local_20[0xae] = '\0';
        ppiVar4 = &piStack_98;
      }
      else {
        ppiVar4 = &piStack_98;
        if (local_68 == 4) {
          uVar2 = FUN_0001519f(DAT_00053c7c);
          *(undefined4 *)(local_20 + 0xb3) = uVar2;
          uVar2 = FUN_000151d5(DAT_00053c7c);
          *(undefined4 *)(local_20 + 0xb7) = uVar2;
          uVar2 = FUN_0001519f(DAT_00053c7c);
          uVar6 = FUN_0001519f(local_20,uVar2);
          if ((int)((ulonglong)uVar6 >> 0x20) - (int)uVar6 < 0) {
            *(int *)(local_20 + 0xb3) = *(int *)(local_20 + 0xb3) + 0x1e;
          }
          else {
            *(int *)(local_20 + 0xb3) = *(int *)(local_20 + 0xb3) + -0x1e;
          }
          local_20[0xaf] = '\0';
          local_20[0xb0] = '\0';
          local_20[0xb1] = '\0';
          local_20[0xb2] = '\0';
          local_20[0x1f] = 'j';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          ppiVar4 = &piStack_98;
        }
      }
    }
    else if (local_64 < 6) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00016959(local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        local_20[0xaf] = '\x05';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[199] = '\x01';
        local_20[200] = '\0';
        local_20[0xc9] = '\0';
        local_20[0xca] = '\0';
        ppiVar4 = &piStack_98;
      }
    }
    else if (local_64 < 7) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_0001730b(local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        local_20[0xaf] = '\x06';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        ppiVar4 = &piStack_98;
      }
    }
    else if (local_64 < 8) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_000174a2(local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        local_20[0xaf] = '\x04';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        ppiVar4 = &piStack_98;
      }
    }
    else {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00017530(local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        local_20[0xaf] = '\b';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        ppiVar4 = &piStack_98;
      }
    }
    goto LAB_00026553;
  }
  if (local_64 != 0) {
    if (local_64 < 2) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00017218(local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        local_20[0xaf] = '\x01';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        ppiVar4 = &piStack_98;
      }
    }
    else if (local_64 < 3) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00016a60(local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        local_20[0xaf] = '\x02';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        ppiVar4 = &piStack_98;
      }
    }
    else {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00016b8f(local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        local_20[0xaf] = '\x03';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        ppiVar4 = &piStack_98;
      }
    }
    goto LAB_00026553;
  }
  if (*(int *)(local_20 + 0xaf) != 4) {
    if (*(int *)(local_20 + 0xaf) == 10) {
      *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb4 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
    }
    else {
      piStack_94 = (int *)0x253b1;
      local_38 = FUN_00041e9f();
      local_38 = local_38 % 0x32;
      *(int *)(local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)(local_20 + 0xd) * 0x88) + local_38;
    }
    local_20[0x1f] = '\x04';
    local_20[0x20] = '\0';
    local_20[0x21] = '\0';
    local_20[0x22] = '\0';
    piStack_94 = (int *)0x253ed;
    iVar1 = FUN_00041e9f();
    *(int *)(local_20 + 0x113) = iVar1 % 10;
  }
  if (*(int *)(local_20 + 0xaf) == 4) {
    if (*(int *)(local_20 + 0x8f) == 1) {
      *(int *)(local_20 + 0x97) = *(int *)(&DAT_00053eb0 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
      local_20[0x1f] = '\x03';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
    }
    else {
      piStack_94 = (int *)0x25452;
      iVar1 = FUN_0001519f(local_20);
      if (iVar1 < 0) {
LAB_00025465:
        local_20[0x1f] = '\a';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        return;
      }
      piStack_94 = (int *)0x2545e;
      iVar1 = FUN_0001519f(local_20);
      if (0x13f < iVar1) goto LAB_00025465;
      *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb0 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
      if (*(int *)(local_20 + 199) == 1) {
        do {
          piStack_94 = (int *)0x254a4;
          iVar1 = FUN_00041e9f();
          local_8c = iVar1 % 4;
          if (local_8c == 2) {
            piStack_94 = (int *)0x254c0;
            iVar1 = FUN_00041e9f(iVar1 / 4);
            local_8c = local_8c + iVar1 % 2;
          }
          else if (local_8c == 3) {
            local_8c = 4;
            piStack_94 = (int *)0x254e5;
            iVar1 = FUN_00041e9f(iVar1 / 4);
            local_8c = local_8c + iVar1 % 2;
          }
        } while (local_8c == *(int *)(local_20 + 0xff));
        if ((local_8c == 0) || (local_8c == 1)) {
          local_20[0x6f] = '\x01';
          local_20[0x70] = '\0';
          local_20[0x71] = '\0';
          local_20[0x72] = '\0';
        }
        else {
          local_20[0x6f] = '\0';
          local_20[0x70] = '\0';
          local_20[0x71] = '\0';
          local_20[0x72] = '\0';
        }
        *(int *)(local_20 + 0xff) = local_8c;
        local_88 = *(undefined4 *)(local_20 + 0xbf);
        local_84 = *(undefined4 *)(local_20 + 0xc3);
        if (local_8c == 0) {
          if ((uint)((int)(uint)(byte)(&DAT_00053eb8)
                                      [*(int *)(local_20 + 0x113) * 10 +
                                       *(int *)(local_20 + 0xd) * 0x88] >> 1) < 0x3c) {
            *(uint *)(local_20 + 0xbf) =
                 local_50 -
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)(local_20 + 0x113) * 10 + *(int *)(local_20 + 0xd) * 0x88];
          }
          else {
            *(int *)(local_20 + 0xbf) =
                 (local_50 -
                 ((int)(uint)(byte)(&DAT_00053eb8)
                                   [*(int *)(local_20 + 0x113) * 10 +
                                    *(int *)(local_20 + 0xd) * 0x88] >> 1)) + 1;
          }
          *(int *)(local_20 + 0xc3) = local_4c;
        }
        else if (local_8c == 1) {
          if ((uint)((int)(uint)(byte)(&DAT_00053eb8)
                                      [*(int *)(local_20 + 0x113) * 10 +
                                       *(int *)(local_20 + 0xd) * 0x88] >> 1) < 0x3c) {
            *(uint *)(local_20 + 0xbf) =
                 local_50 +
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)(local_20 + 0x113) * 10 + *(int *)(local_20 + 0xd) * 0x88];
          }
          else {
            *(int *)(local_20 + 0xbf) =
                 local_50 +
                 ((int)(uint)(byte)(&DAT_00053eb8)
                                   [*(int *)(local_20 + 0x113) * 10 +
                                    *(int *)(local_20 + 0xd) * 0x88] >> 1) + -1;
          }
          *(int *)(local_20 + 0xc3) = local_4c;
        }
        else {
          iVar3 = *(int *)(&DAT_00051c9c + local_8c * 0x10) + local_50;
          piStack_94 = (int *)0x256be;
          iVar1 = FUN_00041e9f();
          piStack_94 = (int *)0x256ce;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX);
          *(int *)(local_20 + 0xbf) = iVar3 + iVar1;
          iVar3 = *(int *)(&DAT_00051ca8 + local_8c * 0x10) -
                  *(int *)(&DAT_00051ca0 + local_8c * 0x10);
          piStack_94 = (int *)0x25707;
          iVar1 = FUN_00041e9f(local_8c * 0x10,iVar3,iVar3,
                               *(int *)(&DAT_00051ca0 + local_8c * 0x10) + local_4c);
          piStack_94 = (int *)0x25717;
          iVar1 = FUN_00041f76(iVar1 % iVar3);
          *(int *)(local_20 + 0xc3) = extraout_ECX_00 + iVar1;
        }
        if (*(int *)(local_20 + 0xbf) < 0) {
          local_20[0xbf] = '\x14';
          local_20[0xc0] = '\0';
          local_20[0xc1] = '\0';
          local_20[0xc2] = '\0';
        }
        else if (0x13f < *(int *)(local_20 + 0xbf)) {
          local_20[0xbf] = ',';
          local_20[0xc0] = '\x01';
          local_20[0xc1] = '\0';
          local_20[0xc2] = '\0';
        }
        piStack_94 = (int *)0x25763;
        iVar1 = FUN_0001519f(local_20);
        piStack_94 = (int *)0x25775;
        iVar1 = FUN_00041f76(*(int *)(local_20 + 0xbf) - iVar1);
        if (iVar1 < 10) {
          piStack_94 = (int *)0x25782;
          iVar1 = FUN_000151d5(local_20);
          piStack_94 = (int *)0x25794;
          iVar1 = FUN_00041f76(*(int *)(local_20 + 0xc3) - iVar1);
          if (iVar1 < 5) {
            *(undefined4 *)(local_20 + 0xbf) = local_88;
            *(undefined4 *)(local_20 + 0xc3) = local_84;
            goto LAB_000257c2;
          }
        }
        local_20[199] = '\0';
        local_20[200] = '\0';
        local_20[0xc9] = '\0';
        local_20[0xca] = '\0';
      }
LAB_000257c2:
      if (*(int *)(local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
        *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
      }
      if (*(int *)(DAT_00063354 + 0x16) <= *(int *)(local_20 + 0xc3)) {
        *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
      }
      piStack_94 = (int *)0x25816;
      iVar1 = FUN_0001519f(local_20);
      if (iVar1 < *(int *)(local_20 + 0xbf)) {
        local_80 = *(int *)(local_20 + 0xbf);
        piStack_94 = (int *)0x25837;
        local_7c = FUN_0001519f(local_20);
      }
      else {
        piStack_94 = (int *)0x25844;
        local_80 = FUN_0001519f(local_20);
        local_7c = *(int *)(local_20 + 0xbf);
      }
      piStack_94 = (int *)0x2585b;
      iVar1 = FUN_000151d5(local_20);
      if (iVar1 < *(int *)(local_20 + 0xc3)) {
        local_78 = *(int *)(local_20 + 0xc3);
        piStack_94 = (int *)0x2587c;
        local_74 = FUN_000151d5(local_20);
      }
      else {
        piStack_94 = (int *)0x25889;
        local_78 = FUN_000151d5(local_20);
        local_74 = *(int *)(local_20 + 0xc3);
      }
      piStack_94 = (int *)0x258a0;
      local_70 = FUN_0001519f(local_20);
      piStack_94 = (int *)0x258ab;
      local_6c = FUN_000151d5(local_20);
      piStack_94 = (int *)0x258bf;
      iVar1 = FUN_00041f76(*(int *)(local_20 + 0xbf) - local_70);
      if (iVar1 < 5) {
        piStack_94 = (int *)0x258d5;
        iVar1 = FUN_00041f76(*(int *)(local_20 + 0xc3) - local_6c);
        if ((((5 < iVar1) && (local_4c < local_78)) && (local_74 < local_4c)) &&
           ((local_70 + -8 < local_50 && (local_50 < local_70 + 8)))) {
          *(int *)(local_20 + 0xcb) = local_70;
          *(undefined4 *)(local_20 + 0xeb) = *(undefined4 *)(local_20 + 0xcb);
          *(int *)(local_20 + 0xcf) = local_6c;
          *(undefined4 *)(local_20 + 0xef) = *(undefined4 *)(local_20 + 0xcf);
          local_20[0x1f] = '\x14';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          local_20[0x8b] = '\0';
          local_20[0x8c] = '\0';
          local_20[0x8d] = '\0';
          local_20[0x8e] = '\0';
          return;
        }
      }
      piStack_94 = (int *)0x25972;
      iVar1 = FUN_00041f76(*(int *)(local_20 + 0xc3) - local_6c);
      if (iVar1 < 10) {
        piStack_94 = (int *)0x25988;
        iVar1 = FUN_00041f76(*(int *)(local_20 + 0xbf) - local_70);
        if (((5 < iVar1) && (local_4c < local_6c + 8)) &&
           ((local_6c + -8 < local_4c && ((local_7c < local_50 && (local_50 < local_80)))))) {
          *(int *)(local_20 + 0xcb) = local_70;
          *(undefined4 *)(local_20 + 0xeb) = *(undefined4 *)(local_20 + 0xcb);
          *(int *)(local_20 + 0xcf) = local_6c;
          *(undefined4 *)(local_20 + 0xef) = *(undefined4 *)(local_20 + 0xcf);
          if (*(int *)(local_20 + 0xcf) <
              *(int *)(DAT_00063354 + 0x12) +
              (*(int *)(DAT_00063354 + 0x16) - *(int *)(DAT_00063354 + 0x12) >> 1)) {
            if (*(int *)(local_20 + 0xcf) < *(int *)(local_20 + 0xc3)) {
              *(int *)(local_20 + 0xcb) =
                   *(int *)(local_20 + 0xcb) -
                   (*(int *)(local_20 + 0xc3) - *(int *)(local_20 + 0xcf));
            }
            else {
              *(int *)(local_20 + 0xcb) =
                   *(int *)(local_20 + 0xcb) +
                   (*(int *)(local_20 + 0xc3) - *(int *)(local_20 + 0xcf));
            }
          }
          else if (*(int *)(local_20 + 0xcf) < *(int *)(local_20 + 0xc3)) {
            *(int *)(local_20 + 0xcb) =
                 *(int *)(local_20 + 0xcb) + (*(int *)(local_20 + 0xc3) - *(int *)(local_20 + 0xcf))
            ;
          }
          else {
            *(int *)(local_20 + 0xcb) =
                 *(int *)(local_20 + 0xcb) - (*(int *)(local_20 + 0xc3) - *(int *)(local_20 + 0xcf))
            ;
          }
          local_20[0x1f] = '\x15';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          local_20[0x8b] = '\0';
          local_20[0x8c] = '\0';
          local_20[0x8d] = '\0';
          local_20[0x8e] = '\0';
          return;
        }
      }
      if ((local_4c < local_78) && (local_74 < local_4c)) {
        piStack_94 = (int *)0x25b03;
        iVar1 = FUN_0001519f(local_20);
        if (local_50 <= iVar1 + -8) goto LAB_00025b1b;
        piStack_94 = (int *)0x25b13;
        iVar1 = FUN_0001519f(local_20);
        if (iVar1 + 8 <= local_50) goto LAB_00025b1b;
        local_20[0x1f] = '\b';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[0x8b] = '\0';
        local_20[0x8c] = '\0';
        local_20[0x8d] = '\0';
        local_20[0x8e] = '\0';
      }
      else {
LAB_00025b1b:
        local_20[0x1f] = '\x02';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[0x8b] = '\0';
        local_20[0x8c] = '\0';
        local_20[0x8d] = '\0';
        local_20[0x8e] = '\0';
      }
      if ((((local_50 < local_80) && (local_7c < local_50)) &&
          (*(int *)(local_20 + 0xc3) + -8 < local_4c)) && (local_4c < *(int *)(local_20 + 0xc3) + 8)
         ) {
        local_20[0x1f] = '\b';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[0x8b] = '\0';
        local_20[0x8c] = '\0';
        local_20[0x8d] = '\0';
        local_20[0x8e] = '\0';
      }
      else {
        local_20[0x1f] = '\x02';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        local_20[0x8b] = '\0';
        local_20[0x8c] = '\0';
        local_20[0x8d] = '\0';
        local_20[0x8e] = '\0';
      }
    }
  }
  if (*(int *)(local_20 + 0xaf) == 2) {
    *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb0 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
    local_20[0x1f] = '\x05';
    local_20[0x20] = '\0';
    local_20[0x21] = '\0';
    local_20[0x22] = '\0';
    local_20[0x8b] = '\0';
    local_20[0x8c] = '\0';
    local_20[0x8d] = '\0';
    local_20[0x8e] = '\0';
    ppiVar4 = (int **)local_90;
  }
  else {
    ppiVar4 = (int **)local_90;
    if (*(int *)(local_20 + 0xaf) == 8) {
      *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb0 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
      local_20[0x1f] = '\t';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      local_20[0x8b] = '\0';
      local_20[0x8c] = '\0';
      local_20[0x8d] = '\0';
      local_20[0x8e] = '\0';
      ppiVar4 = (int **)local_90;
    }
  }
LAB_00026553:
  if (local_44 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x26564;
    FUN_0001545e(local_20,local_44);
  }
  if (local_40 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x26575;
    FUN_0001586b(local_20,local_40);
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x2657d;
  iVar1 = FUN_0001519f(local_20);
  if (local_50 < iVar1) {
    local_20[0x1b] = '\x01';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x26594;
  iVar1 = FUN_0001519f(local_20);
  if (iVar1 < local_50) {
    local_20[0x1b] = '\0';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  return;
}


