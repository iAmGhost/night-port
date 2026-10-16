// func #69  FUN_0002adf5  0002adf5  size=6709


/* WARNING: Removing unreachable block (ram,0x0002c7db) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0002adf5(void)

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
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  piStack_94 = (int *)0x2ae1c;
  local_50 = FUN_0001519f(DAT_00053c7c);
  piStack_94 = (int *)0x2ae29;
  local_4c = FUN_000151d5(DAT_00053c7c);
  piStack_94 = (int *)0x2ae34;
  local_30 = FUN_0001519f((char *)local_20);
  piStack_94 = (int *)0x2ae3f;
  local_2c = FUN_000151d5((char *)local_20);
  if ((char *)local_20 == (char *)DAT_00053c7c) {
    return;
  }
  if ((*(int *)((int)(char *)local_20 + 0x1f) < 10) ||
     (0x13 < *(int *)((int)(char *)local_20 + 0x1f))) {
    local_34 = *(int *)(&DAT_00053ea0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  else {
    local_34 = *(int *)(&DAT_00053f1c + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  if (*(int *)((int)(char *)local_20 + 0x6f) == 1) {
    local_34 = *(int *)(&DAT_00053f1c + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  fVar5 = (float10)*(float *)(&DAT_00051d5c + local_34 * 4);
  piStack_94 = (int *)0x2aeb8;
  FUN_00041f7e();
  local_48 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)((int)(char *)local_20 + 99) +
          (float10)*(float *)(&DAT_00051d5c + local_34 * 4);
  piStack_94 = (int *)0x2aed2;
  FUN_00041f7e();
  local_54 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)((int)(char *)local_20 + 99);
  piStack_94 = (int *)0x2aee0;
  FUN_00041f7e();
  local_58 = (float)(int)ROUND(fVar5);
  if ((int)local_58 + local_48 < local_54) {
    local_48 = local_48 + 1;
  }
  *(float *)((int)(char *)local_20 + 99) =
       *(float *)(&DAT_00051d5c + local_34 * 4) + *(float *)((int)(char *)local_20 + 99);
  local_44 = 0;
  local_40 = 0;
  if (((((*(int *)(DAT_00053c7c + 0x17) != 0x15) && (*(int *)(DAT_00053c7c + 0x17) != 0x16)) &&
       (*(int *)(DAT_00053c7c + 0x17) != 0x17)) &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x18 && (*(int *)(DAT_00053c7c + 0x17) != 0x1a)))) &&
     ((*(int *)(DAT_00053c7c + 0x17) != 0x1d &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x1e && (*(int *)(DAT_00053c7c + 0x17) != 0x20)))))) {
    piStack_94 = (int *)0x2af85;
    local_3c = FUN_00015b5b((char *)local_20);
    if (local_3c != 0) {
      *(int *)((int)(char *)local_20 + 0x10f) = local_3c;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 10;
    }
  }
  if ((int)*(short *)((int)(char *)local_20 + 0x23) + *(int *)((int)(char *)local_20 + 0x56) +
      (int)*(short *)((int)(char *)local_20 + 0x29) != *(int *)((int)(char *)local_20 + 0x77)) {
    *(int *)((int)(char *)local_20 + 0x7b) =
         *(int *)((int)(char *)local_20 + 0x77) -
         ((int)*(short *)((int)(char *)local_20 + 0x23) + *(int *)((int)(char *)local_20 + 0x56) +
         (int)*(short *)((int)(char *)local_20 + 0x29));
    *(int *)((int)(char *)local_20 + 0x77) =
         *(int *)((int)(char *)local_20 + 0x56) + (int)*(short *)((int)(char *)local_20 + 0x23) +
         (int)*(short *)((int)(char *)local_20 + 0x29);
  }
  if ((((*(uint *)((int)(char *)local_20 + 0x17) < 0x15) ||
       (0x18 < *(uint *)((int)(char *)local_20 + 0x17))) &&
      (*(int *)((int)(char *)local_20 + 0x17) != 0x1a)) &&
     (((*(int *)((int)(char *)local_20 + 0x17) != 0x1d &&
       (*(int *)((int)(char *)local_20 + 0x17) != 0x1e)) &&
      (*(int *)((int)(char *)local_20 + 0x17) != 0x20)))) {
    if (200 < *(int *)((int)(char *)local_20 + 0x73) -
              ((int)*(short *)((int)(char *)local_20 + 0x23) +
               *(int *)((int)(char *)local_20 + 0x56) +
              (int)*(short *)((int)(char *)local_20 + 0x29))) {
      *(int *)((int)(char *)local_20 + 0x73) =
           *(int *)((int)(char *)local_20 + 0x56) + (int)*(short *)((int)(char *)local_20 + 0x23) +
           (int)*(short *)((int)(char *)local_20 + 0x29);
      if (10 < *(int *)((int)(char *)local_20 + 0x7b)) {
        return;
      }
      *(undefined4 *)((int)(char *)local_20 + 0x17) = 0x2f;
      *(undefined4 *)((int)(char *)local_20 + 0x11) = 0;
      return;
    }
  }
  else {
    *(int *)((int)(char *)local_20 + 0x73) =
         *(int *)((int)(char *)local_20 + 0x56) + (int)*(short *)((int)(char *)local_20 + 0x23) +
         (int)*(short *)((int)(char *)local_20 + 0x29);
  }
  if ((*(int *)((int)(char *)local_20 + 0x17) != 2) && (*(int *)((int)(char *)local_20 + 0x17) != 1)
     ) {
    if (*(int *)((int)(char *)local_20 + 0x17) != 0x2c) {
      if (*(int *)((int)(char *)local_20 + 0x17) != 5) {
        return;
      }
      *(undefined4 *)((int)(char *)local_20 + 0x7f) = 1;
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) == 1) {
      *(undefined4 *)((int)(char *)local_20 + 0x87) = *(undefined4 *)((int)(char *)local_20 + 0x1b);
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) == 0x48) {
      piStack_94 = (int *)0x2b11f;
      uVar2 = FUN_0001519f((char *)local_20);
      *(undefined4 *)((int)(char *)local_20 + 0xd3) = uVar2;
      piStack_94 = (int *)0x2b132;
      uVar2 = FUN_000151d5((char *)local_20);
      *(undefined4 *)((int)(char *)local_20 + 0xd7) = uVar2;
      *(int *)((int)(char *)local_20 + 0xcb) = local_50;
      *(int *)((int)(char *)local_20 + 0xcf) = local_4c;
      piStack_94 = (int *)0x2b15d;
      iVar1 = FUN_0001519f((char *)local_20);
      if (*(int *)((int)(char *)local_20 + 0xcb) == iVar1) {
        *(undefined4 *)((int)(char *)local_20 + 0xdb) = 0x47c35000;
      }
      else {
        piStack_94 = (int *)0x2b185;
        iVar1 = FUN_000151d5((char *)local_20);
        local_58 = (float)(*(int *)((int)(char *)local_20 + 0xcf) - iVar1);
        piStack_94 = (int *)0x2b1a3;
        local_54 = FUN_0001519f((char *)local_20);
        local_54 = *(int *)((int)(char *)local_20 + 0xcb) - local_54;
        *(float *)((int)(char *)local_20 + 0xdb) = local_58 / (float)local_54;
      }
      piStack_94 = (int *)0x2b1ca;
      iVar1 = FUN_0001519f((char *)local_20);
      piStack_94 = (int *)0x2b1dc;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcb) - iVar1);
      piStack_94 = (int *)0x2b1e6;
      iVar3 = FUN_000151d5((char *)local_20);
      piStack_94 = (int *)0x2b1f8;
      iVar3 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcf) - iVar3);
      if (iVar3 < iVar1) {
        piStack_94 = (int *)0x2b208;
        iVar1 = FUN_0001519f((char *)local_20);
        if (*(int *)((int)(char *)local_20 + 0xcb) - iVar1 < 0) {
          *(undefined4 *)((int)(char *)local_20 + 0xe3) = 0xc0e00000;
          piStack_94 = (int *)0x2b22e;
          iVar1 = FUN_000151d5((char *)local_20);
          *(float *)((int)(char *)local_20 + 0xe7) =
               (float)(*(int *)((int)(char *)local_20 + 0xcf) - iVar1) / _DAT_0005004c;
        }
        else {
          *(undefined4 *)((int)(char *)local_20 + 0xe3) = 0x40e00000;
          piStack_94 = (int *)0x2b267;
          iVar1 = FUN_000151d5((char *)local_20);
          *(float *)((int)(char *)local_20 + 0xe7) =
               (float)(*(int *)((int)(char *)local_20 + 0xcf) - iVar1) / _DAT_00050050;
        }
      }
      else {
        piStack_94 = (int *)0x2b296;
        iVar1 = FUN_000151d5((char *)local_20);
        if (*(int *)((int)(char *)local_20 + 0xcf) - iVar1 < 0) {
          piStack_94 = (int *)0x2b2af;
          iVar1 = FUN_0001519f((char *)local_20);
          *(float *)((int)(char *)local_20 + 0xe3) =
               (float)(*(int *)((int)(char *)local_20 + 0xcb) - iVar1) / _DAT_00050054;
          *(undefined4 *)((int)(char *)local_20 + 0xe7) = 0xc0800000;
        }
        else {
          piStack_94 = (int *)0x2b2e8;
          iVar1 = FUN_0001519f((char *)local_20);
          *(float *)((int)(char *)local_20 + 0xe3) =
               (float)(*(int *)((int)(char *)local_20 + 0xcb) - iVar1) / _DAT_00050058;
          *(undefined4 *)((int)(char *)local_20 + 0xe7) = 0x40800000;
        }
      }
      if ((*(int *)((int)(char *)local_20 + 0x87) == 1) &&
         (0.0 < *(float *)((int)(char *)local_20 + 0xe3))) {
        *(undefined4 *)((int)(char *)local_20 + 0xe3) = 0;
        *(undefined4 *)((int)(char *)local_20 + 0xdb) = 0x47c35000;
        return;
      }
      if (*(int *)((int)(char *)local_20 + 0x87) != 0) {
        return;
      }
      if (0.0 <= *(float *)((int)(char *)local_20 + 0xe3)) {
        return;
      }
      *(undefined4 *)((int)(char *)local_20 + 0xe3) = 0;
      *(undefined4 *)((int)(char *)local_20 + 0xdb) = 0x47c35000;
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) < 0x49) {
      return;
    }
    if (0x71 < *(int *)((int)(char *)local_20 + 0x11)) {
      return;
    }
    uStack_5c = (undefined4)
                ((ulonglong)(double)ABS(*(float *)((int)(char *)local_20 + 0xe3)) >> 0x20);
    if (ABS(*(float *)((int)(char *)local_20 + 0xe3)) <=
        ABS(*(float *)((int)(char *)local_20 + 0xe7))) {
      iVar1 = *(int *)((int)(char *)local_20 + 0x11) + -0x48;
      _local_60 = CONCAT44(uStack_5c,iVar1);
      fVar5 = (float10)iVar1 * (float10)*(float *)((int)(char *)local_20 + 0xe7);
      piStack_94 = (int *)0x2b431;
      FUN_00041f7e();
      local_24 = (int)ROUND(fVar5);
      fVar5 = (float10)local_24 / (float10)*(float *)((int)(char *)local_20 + 0xdb);
      piStack_94 = (int *)0x2b445;
      FUN_00041f7e();
      local_24 = local_24 + *(int *)((int)(char *)local_20 + 0xd7);
      local_28 = (int)ROUND(fVar5) + *(int *)((int)(char *)local_20 + 0xd3);
    }
    else {
      iVar1 = *(int *)((int)(char *)local_20 + 0x11) + -0x48;
      _local_60 = CONCAT44(uStack_5c,iVar1);
      fVar5 = (float10)iVar1 * (float10)*(float *)((int)(char *)local_20 + 0xe3);
      piStack_94 = (int *)0x2b3e3;
      FUN_00041f7e();
      local_28 = (int)ROUND(fVar5);
      fVar5 = (float10)local_28 * (float10)*(float *)((int)(char *)local_20 + 0xdb);
      piStack_94 = (int *)0x2b3f7;
      FUN_00041f7e();
      local_28 = local_28 + *(int *)((int)(char *)local_20 + 0xd3);
      local_24 = (int)ROUND(fVar5) + *(int *)((int)(char *)local_20 + 0xd7);
    }
    if (((-1 < local_28) || (*(int *)((int)(char *)local_20 + 0xd3) < 0xb)) &&
       (((local_28 < 0x140 || (0x135 < *(int *)((int)(char *)local_20 + 0xd3))) &&
        ((*(int *)(DAT_00063354 + 0x12) + DAT_000639e0 <= local_24 &&
         (local_24 <= *(int *)(DAT_00063354 + 0x16) + DAT_000639e0)))))) {
      piStack_94 = (int *)0x2b542;
      iVar1 = FUN_0001519f((char *)local_20);
      piStack_94 = (int *)0x2b54f;
      FUN_0001545e((char *)local_20,local_28 - iVar1);
      piStack_94 = (int *)0x2b557;
      iVar1 = FUN_000151d5((char *)local_20);
      piStack_94 = (int *)0x2b564;
      FUN_0001586b((char *)local_20,local_24 - iVar1);
      return;
    }
    if (local_28 == *(int *)((int)(char *)local_20 + 0xd3) ||
        local_28 - *(int *)((int)(char *)local_20 + 0xd3) < 0) {
      if (local_28 - *(int *)((int)(char *)local_20 + 0xd3) < 0) {
        piStack_94 = (int *)0x2b4f8;
        FUN_0001545e((char *)local_20,0xfffffffd);
      }
    }
    else {
      piStack_94 = (int *)0x2b4d7;
      FUN_0001545e((char *)local_20,3);
    }
    if (local_24 != *(int *)((int)(char *)local_20 + 0xd7) &&
        -1 < local_24 - *(int *)((int)(char *)local_20 + 0xd7)) {
      piStack_94 = (int *)0x2b517;
      FUN_0001586b((char *)local_20,1);
      return;
    }
    if (-1 < local_24 - *(int *)((int)(char *)local_20 + 0xd7)) {
      return;
    }
    piStack_94 = (int *)0x2b538;
    FUN_0001586b((char *)local_20,0xffffffff);
    return;
  }
  if (*(int *)((int)(char *)local_20 + 0x7f) == 1) {
    piStack_94 = (int *)0x2b58f;
    iVar1 = FUN_0001519f((char *)local_20);
    piStack_94 = (int *)0x2b597;
    iVar1 = FUN_00041f76(iVar1 - local_50);
    if (iVar1 < 0x3c) {
      piStack_94 = (int *)0x2b5a4;
      iVar1 = FUN_000151d5((char *)local_20);
      piStack_94 = (int *)0x2b5ac;
      iVar1 = FUN_00041f76(iVar1 - local_4c);
      if (iVar1 < 10) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xe;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 0;
      }
    }
    *(undefined4 *)((int)(char *)local_20 + 0x7f) = 0;
  }
  if (*(char *)local_20 != '\x01') {
    return;
  }
  local_64 = *(uint *)((char *)local_20 + 0x1f);
  if (8 < local_64) {
    if (local_64 < 10) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00017637((char *)local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 9;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_98;
      }
    }
    else if (local_64 < 0xe) {
      if (local_64 < 0xb) {
        if (*(int *)((char *)local_20 + 0x10f) == 1) {
          ((char *)local_20)[0xaf] = '\n';
          ((char *)local_20)[0xb0] = '\0';
          ((char *)local_20)[0xb1] = '\0';
          ((char *)local_20)[0xb2] = '\0';
          ((char *)local_20)[0x1f] = '\v';
          ((char *)local_20)[0x20] = '\0';
          ((char *)local_20)[0x21] = '\0';
          ((char *)local_20)[0x22] = '\0';
          ((char *)local_20)[0x11b] = '\x01';
          ((char *)local_20)[0x11c] = '\0';
          ((char *)local_20)[0x11d] = '\0';
          ((char *)local_20)[0x11e] = '\0';
          *(undefined4 *)((char *)local_20 + 0xab) =
               *(undefined4 *)(&DAT_00053f20 + *(int *)((char *)local_20 + 0xd) * 0x88);
        }
        ppiVar4 = (int **)local_90;
        if (*(int *)((char *)local_20 + 0x10f) == 2) {
          ((char *)local_20)[0xaf] = '\n';
          ((char *)local_20)[0xb0] = '\0';
          ((char *)local_20)[0xb1] = '\0';
          ((char *)local_20)[0xb2] = '\0';
          ((char *)local_20)[0x1f] = '\v';
          ((char *)local_20)[0x20] = '\0';
          ((char *)local_20)[0x21] = '\0';
          ((char *)local_20)[0x22] = '\0';
          ((char *)local_20)[0x11b] = '\x01';
          ((char *)local_20)[0x11c] = '\0';
          ((char *)local_20)[0x11d] = '\0';
          ((char *)local_20)[0x11e] = '\0';
          *(int *)((char *)local_20 + 0xab) =
               *(int *)(&DAT_00053f20 + *(int *)((char *)local_20 + 0xd) * 0x88) >> 1;
          ppiVar4 = (int **)local_90;
        }
      }
      else if (local_64 < 0xc) {
        if (*(int *)((char *)local_20 + 0xab) < 0) {
          piStack_94 = (int *)0x2c4fd;
          iVar1 = FUN_0001519f((char *)local_20,
                               (&DAT_00053eb8)
                               [*(int *)((char *)local_20 + 0x113) * 10 +
                                *(int *)((char *)local_20 + 0xd) * 0x88]);
          piStack_94 = (int *)0x2c505;
          uVar6 = FUN_00041f76(iVar1 - local_50);
          if ((int)uVar6 < (int)((ulonglong)uVar6 >> 0x20)) {
            piStack_94 = (int *)0x2c511;
            iVar1 = FUN_000151d5((char *)local_20);
            piStack_94 = (int *)0x2c519;
            iVar1 = FUN_00041f76(iVar1 - local_4c);
            if (iVar1 < 6) {
              if (((&DAT_00053eb8)
                   [*(int *)((int)(char *)local_20 + 0x11b) +
                    *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                    *(int *)((int)(char *)local_20 + 0x113) * 10] == '\0') ||
                 (9 < *(int *)((int)(char *)local_20 + 0x11b))) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_90;
              }
              else {
                *(uint *)((int)(char *)local_20 + 0x17) =
                     (uint)(byte)(&DAT_00053eb8)
                                 [*(int *)((int)(char *)local_20 + 0x11b) +
                                  *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                                  *(int *)((int)(char *)local_20 + 0x113) * 10];
                *(undefined4 *)((int)(char *)local_20 + 0x11) = 0;
                *(int *)((int)(char *)local_20 + 0x11b) =
                     *(int *)((int)(char *)local_20 + 0x11b) + 1;
                ppiVar4 = (int **)local_90;
              }
              goto LAB_0002c7e0;
            }
          }
          piStack_94 = (int *)0x2c5c4;
          iVar1 = FUN_00041e9f();
          if (iVar1 % 0x14 == 2) {
            *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
            *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
            ppiVar4 = (int **)local_90;
          }
          else {
            *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
            *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
            ppiVar4 = (int **)local_90;
          }
        }
        else {
          *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
          ((char *)local_20)[0x17] = '\x01';
          ((char *)local_20)[0x18] = '\0';
          ((char *)local_20)[0x19] = '\0';
          ((char *)local_20)[0x1a] = '\0';
          ppiVar4 = (int **)local_90;
        }
      }
      else if (local_64 < 0xd) {
        *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_90;
        if (*(int *)((char *)local_20 + 0xab) < 0) {
          ((char *)local_20)[0x17] = '2';
          ((char *)local_20)[0x18] = '\0';
          ((char *)local_20)[0x19] = '\0';
          ((char *)local_20)[0x1a] = '\0';
          ((char *)local_20)[0x11] = '\0';
          ((char *)local_20)[0x12] = '\0';
          ((char *)local_20)[0x13] = '\0';
          ((char *)local_20)[0x14] = '\0';
          ((char *)local_20)[0xaf] = '\n';
          ((char *)local_20)[0xb0] = '\0';
          ((char *)local_20)[0xb1] = '\0';
          ((char *)local_20)[0xb2] = '\0';
          ((char *)local_20)[0x1f] = '\0';
          ((char *)local_20)[0x20] = '\0';
          ((char *)local_20)[0x21] = '\0';
          ((char *)local_20)[0x22] = '\0';
          ppiVar4 = (int **)local_90;
        }
      }
      else {
        *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_90;
        if (*(int *)((char *)local_20 + 0xab) < 0) {
          ((char *)local_20)[0x17] = '1';
          ((char *)local_20)[0x18] = '\0';
          ((char *)local_20)[0x19] = '\0';
          ((char *)local_20)[0x1a] = '\0';
          ((char *)local_20)[0x11] = '\0';
          ((char *)local_20)[0x12] = '\0';
          ((char *)local_20)[0x13] = '\0';
          ((char *)local_20)[0x14] = '\0';
          ((char *)local_20)[0xaf] = '\n';
          ((char *)local_20)[0xb0] = '\0';
          ((char *)local_20)[0xb1] = '\0';
          ((char *)local_20)[0xb2] = '\0';
          ((char *)local_20)[0x1f] = '\0';
          ((char *)local_20)[0x20] = '\0';
          ((char *)local_20)[0x21] = '\0';
          ((char *)local_20)[0x22] = '\0';
          ppiVar4 = (int **)local_90;
        }
      }
    }
    else if (local_64 < 0xf) {
      *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
      ppiVar4 = (int **)local_90;
      if (*(int *)((char *)local_20 + 0xab) < 0) {
        ((char *)local_20)[0x17] = '0';
        ((char *)local_20)[0x18] = '\0';
        ((char *)local_20)[0x19] = '\0';
        ((char *)local_20)[0x1a] = '\0';
        ((char *)local_20)[0x11] = '\0';
        ((char *)local_20)[0x12] = '\0';
        ((char *)local_20)[0x13] = '\0';
        ((char *)local_20)[0x14] = '\0';
        ((char *)local_20)[0xaf] = '\n';
        ((char *)local_20)[0xb0] = '\0';
        ((char *)local_20)[0xb1] = '\0';
        ((char *)local_20)[0xb2] = '\0';
        ((char *)local_20)[0x1f] = '\0';
        ((char *)local_20)[0x20] = '\0';
        ((char *)local_20)[0x21] = '\0';
        ((char *)local_20)[0x22] = '\0';
        ppiVar4 = (int **)local_90;
      }
    }
    else if (local_64 < 0x15) {
      ppiVar4 = (int **)local_90;
      if (local_64 == 0x14) {
        piStack_94 = &local_40;
        piStack_98 = &local_44;
        iVar1 = FUN_0001776f((char *)local_20,local_48,local_50,local_4c);
        ppiVar4 = &piStack_98;
        if (iVar1 != 0) {
          *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x14;
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
          *(undefined4 *)((int)(char *)local_20 + 199) = 1;
          ppiVar4 = &piStack_98;
        }
      }
    }
    else if (local_64 < 0x16) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00017927((char *)local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x15;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_98;
      }
    }
    else if (local_64 < 0x17) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00017ca6((char *)local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x16;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_98;
      }
    }
    else {
      ppiVar4 = (int **)local_90;
      if (local_64 == 0x6a) {
        fVar5 = (float10)_DAT_00051d68;
        piStack_94 = (int *)0x2c16e;
        FUN_00041f7e();
        local_48 = (int)ROUND(fVar5);
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 99) + (float10)_DAT_00051d68;
        piStack_94 = (int *)0x2c182;
        FUN_00041f7e();
        _local_60 = CONCAT44(uStack_5c,(int)ROUND(fVar5));
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 99);
        piStack_94 = (int *)0x2c190;
        FUN_00041f7e();
        local_58 = (float)(int)ROUND(fVar5);
        if ((int)local_58 + local_48 < local_60) {
          local_48 = local_48 + 1;
        }
        *(float *)((int)(char *)local_20 + 99) =
             _DAT_00051d68 + *(float *)((int)(char *)local_20 + 99);
        piStack_94 = &local_40;
        piStack_98 = &local_44;
        iVar1 = FUN_0001730b((char *)local_20,local_48,local_50,local_4c);
        ppiVar4 = &piStack_98;
        if (iVar1 != 0) {
          if ((((*(int *)(DAT_00053c7c + 0x17) == 0x15) || (*(int *)(DAT_00053c7c + 0x17) == 0x16))
              || (*(int *)(DAT_00053c7c + 0x17) == 0x17)) ||
             (((*(int *)(DAT_00053c7c + 0x17) == 0x18 || (*(int *)(DAT_00053c7c + 0x17) == 0x1a)) ||
              ((*(int *)(DAT_00053c7c + 0x17) == 0x1d ||
               ((*(int *)(DAT_00053c7c + 0x17) == 0x1e || (*(int *)(DAT_00053c7c + 0x17) == 0x20))))
              )))) {
            iVar1 = FUN_0001519f((char *)local_20);
            iVar1 = FUN_00041f76(iVar1 - local_50);
            ppiVar4 = &piStack_98;
            if (iVar1 < 0x37) {
              iVar1 = FUN_000151d5((char *)local_20);
              iVar1 = FUN_00041f76(iVar1 - local_4c);
              ppiVar4 = &piStack_98;
              if (iVar1 < 10) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xd;
                *(undefined4 *)((int)(char *)local_20 + 0xab) = 0;
                ppiVar4 = &piStack_98;
              }
            }
          }
          else {
            *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
            *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
            ppiVar4 = &piStack_98;
          }
        }
      }
    }
    goto LAB_0002c7e0;
  }
  if (3 < local_64) {
    if (local_64 < 5) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      local_68 = FUN_00016fec((char *)local_20,local_48,local_50,local_4c);
      if (((((*(int *)(DAT_00053c7c + 0x17) == 0x15) || (*(int *)(DAT_00053c7c + 0x17) == 0x16)) ||
           ((*(int *)(DAT_00053c7c + 0x17) == 0x17 ||
            ((*(int *)(DAT_00053c7c + 0x17) == 0x18 || (*(int *)(DAT_00053c7c + 0x17) == 0x1a))))))
          || (*(int *)(DAT_00053c7c + 0x17) == 0x1d)) ||
         ((*(int *)(DAT_00053c7c + 0x17) == 0x1e || (*(int *)(DAT_00053c7c + 0x17) == 0x20)))) {
        local_68 = 4;
      }
      if (local_68 == 2) {
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 1;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 10;
        ppiVar4 = &piStack_98;
      }
      else if (local_68 == 1) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_98;
      }
      else if (local_68 == 3) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xc;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 0x1e;
        ppiVar4 = &piStack_98;
      }
      else {
        ppiVar4 = &piStack_98;
        if (local_68 == 4) {
          uVar2 = FUN_0001519f(DAT_00053c7c);
          *(undefined4 *)((int)(char *)local_20 + 0xb3) = uVar2;
          uVar2 = FUN_000151d5(DAT_00053c7c);
          *(undefined4 *)((int)(char *)local_20 + 0xb7) = uVar2;
          uVar2 = FUN_0001519f(DAT_00053c7c);
          uVar6 = FUN_0001519f((char *)local_20,uVar2);
          if ((int)((ulonglong)uVar6 >> 0x20) - (int)uVar6 < 0) {
            *(int *)((int)(char *)local_20 + 0xb3) = *(int *)((int)(char *)local_20 + 0xb3) + 0x32;
          }
          else {
            *(int *)((int)(char *)local_20 + 0xb3) = *(int *)((int)(char *)local_20 + 0xb3) + -0x32;
          }
          *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0;
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x6a;
          ppiVar4 = &piStack_98;
        }
      }
    }
    else if (local_64 < 6) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00016959((char *)local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 5;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_98;
      }
    }
    else if (local_64 < 7) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_0001730b((char *)local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_98;
      }
    }
    else if (local_64 < 8) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_000174a2((char *)local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_98;
      }
    }
    else {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00017530((char *)local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_98;
      }
    }
    goto LAB_0002c7e0;
  }
  if (local_64 != 0) {
    if (local_64 < 2) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00017218((char *)local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 1;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_98;
      }
    }
    else if (local_64 < 3) {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00016a60((char *)local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_98;
      }
    }
    else {
      piStack_94 = &local_40;
      piStack_98 = &local_44;
      iVar1 = FUN_00016b8f((char *)local_20,local_48,local_50,local_4c);
      ppiVar4 = &piStack_98;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 3;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_98;
      }
    }
    goto LAB_0002c7e0;
  }
  if (*(int *)((char *)local_20 + 0xaf) != 4) {
    if (*(int *)((char *)local_20 + 0xaf) == 10) {
      *(int *)((char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)((char *)local_20 + 0xd) * 0x88) >> 1;
    }
    else {
      piStack_94 = (int *)0x2b63e;
      local_38 = FUN_00041e9f();
      local_38 = local_38 % 0x32;
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) + local_38;
    }
    *(undefined4 *)((int)(char *)local_20 + 0x1f) = 4;
    piStack_94 = (int *)0x2b67a;
    iVar1 = FUN_00041e9f();
    *(int *)((int)(char *)local_20 + 0x113) = iVar1 % 10;
  }
  if (*(int *)((int)(char *)local_20 + 0xaf) == 4) {
    if (*(int *)((int)(char *)local_20 + 0x8f) == 1) {
      *(int *)((int)(char *)local_20 + 0x97) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 3;
    }
    else {
      piStack_94 = (int *)0x2b6df;
      iVar1 = FUN_0001519f((char *)local_20);
      if (iVar1 < 0) {
LAB_0002b6f2:
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 7;
        return;
      }
      piStack_94 = (int *)0x2b6eb;
      iVar1 = FUN_0001519f((char *)local_20);
      if (0x13f < iVar1) goto LAB_0002b6f2;
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      if (*(int *)((int)(char *)local_20 + 199) == 1) {
        do {
          piStack_94 = (int *)0x2b731;
          iVar1 = FUN_00041e9f();
          local_8c = iVar1 % 4;
          if (local_8c == 2) {
            piStack_94 = (int *)0x2b74d;
            iVar1 = FUN_00041e9f(iVar1 / 4);
            local_8c = local_8c + iVar1 % 2;
          }
          else if (local_8c == 3) {
            local_8c = 4;
            piStack_94 = (int *)0x2b772;
            iVar1 = FUN_00041e9f(iVar1 / 4);
            local_8c = local_8c + iVar1 % 2;
          }
        } while (local_8c == *(int *)((int)(char *)local_20 + 0xff));
        if ((local_8c == 0) || (local_8c == 1)) {
          *(undefined4 *)((int)(char *)local_20 + 0x6f) = 1;
        }
        else {
          *(undefined4 *)((int)(char *)local_20 + 0x6f) = 0;
        }
        *(int *)((int)(char *)local_20 + 0xff) = local_8c;
        local_88 = *(undefined4 *)((int)(char *)local_20 + 0xbf);
        local_84 = *(undefined4 *)((int)(char *)local_20 + 0xc3);
        if (local_8c == 0) {
          if ((uint)((int)(uint)(byte)(&DAT_00053eb8)
                                      [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                       *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) < 0x3c) {
            *(uint *)((int)(char *)local_20 + 0xbf) =
                 local_50 -
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88];
          }
          else {
            *(int *)((int)(char *)local_20 + 0xbf) =
                 (local_50 -
                 ((int)(uint)(byte)(&DAT_00053eb8)
                                   [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                    *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1)) + 1;
          }
          *(int *)((int)(char *)local_20 + 0xc3) = local_4c;
        }
        else if (local_8c == 1) {
          if ((uint)((int)(uint)(byte)(&DAT_00053eb8)
                                      [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                       *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) < 0x3c) {
            *(uint *)((int)(char *)local_20 + 0xbf) =
                 local_50 +
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88];
          }
          else {
            *(int *)((int)(char *)local_20 + 0xbf) =
                 local_50 +
                 ((int)(uint)(byte)(&DAT_00053eb8)
                                   [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                    *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) + -1;
          }
          *(int *)((int)(char *)local_20 + 0xc3) = local_4c;
        }
        else {
          iVar3 = *(int *)(&DAT_00051c9c + local_8c * 0x10) + local_50;
          piStack_94 = (int *)0x2b94b;
          iVar1 = FUN_00041e9f();
          piStack_94 = (int *)0x2b95b;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX);
          *(int *)((int)(char *)local_20 + 0xbf) = iVar3 + iVar1;
          iVar3 = *(int *)(&DAT_00051ca8 + local_8c * 0x10) -
                  *(int *)(&DAT_00051ca0 + local_8c * 0x10);
          piStack_94 = (int *)0x2b994;
          iVar1 = FUN_00041e9f(local_8c * 0x10,iVar3,iVar3,
                               *(int *)(&DAT_00051ca0 + local_8c * 0x10) + local_4c);
          piStack_94 = (int *)0x2b9a4;
          iVar1 = FUN_00041f76(iVar1 % iVar3);
          *(int *)((int)(char *)local_20 + 0xc3) = extraout_ECX_00 + iVar1;
        }
        if (*(int *)((int)(char *)local_20 + 0xbf) < 0) {
          *(undefined4 *)((int)(char *)local_20 + 0xbf) = 0x14;
        }
        else if (0x13f < *(int *)((int)(char *)local_20 + 0xbf)) {
          *(undefined4 *)((int)(char *)local_20 + 0xbf) = 300;
        }
        piStack_94 = (int *)0x2b9f0;
        iVar1 = FUN_0001519f((char *)local_20);
        piStack_94 = (int *)0x2ba02;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - iVar1);
        if (iVar1 < 10) {
          piStack_94 = (int *)0x2ba0f;
          iVar1 = FUN_000151d5((char *)local_20);
          piStack_94 = (int *)0x2ba21;
          iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - iVar1);
          if (iVar1 < 5) {
            *(undefined4 *)((int)(char *)local_20 + 0xbf) = local_88;
            *(undefined4 *)((int)(char *)local_20 + 0xc3) = local_84;
            goto LAB_0002ba4f;
          }
        }
        *(undefined4 *)((int)(char *)local_20 + 199) = 0;
      }
LAB_0002ba4f:
      if (*(int *)((int)(char *)local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
        *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
      }
      if (*(int *)(DAT_00063354 + 0x16) <= *(int *)((int)(char *)local_20 + 0xc3)) {
        *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
      }
      piStack_94 = (int *)0x2baa3;
      iVar1 = FUN_0001519f((char *)local_20);
      if (iVar1 < *(int *)((int)(char *)local_20 + 0xbf)) {
        local_80 = *(int *)((int)(char *)local_20 + 0xbf);
        piStack_94 = (int *)0x2bac4;
        local_7c = FUN_0001519f((char *)local_20);
      }
      else {
        piStack_94 = (int *)0x2bad1;
        local_80 = FUN_0001519f((char *)local_20);
        local_7c = *(int *)((int)(char *)local_20 + 0xbf);
      }
      piStack_94 = (int *)0x2bae8;
      iVar1 = FUN_000151d5((char *)local_20);
      if (iVar1 < *(int *)((int)(char *)local_20 + 0xc3)) {
        local_78 = *(int *)((int)(char *)local_20 + 0xc3);
        piStack_94 = (int *)0x2bb09;
        local_74 = FUN_000151d5((char *)local_20);
      }
      else {
        piStack_94 = (int *)0x2bb16;
        local_78 = FUN_000151d5((char *)local_20);
        local_74 = *(int *)((int)(char *)local_20 + 0xc3);
      }
      piStack_94 = (int *)0x2bb2d;
      local_70 = FUN_0001519f((char *)local_20);
      piStack_94 = (int *)0x2bb38;
      local_6c = FUN_000151d5((char *)local_20);
      piStack_94 = (int *)0x2bb4c;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_70);
      if (iVar1 < 5) {
        piStack_94 = (int *)0x2bb62;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_6c);
        if ((((5 < iVar1) && (local_4c < local_78)) && (local_74 < local_4c)) &&
           ((local_70 + -8 < local_50 && (local_50 < local_70 + 8)))) {
          *(int *)((int)(char *)local_20 + 0xcb) = local_70;
          *(undefined4 *)((int)(char *)local_20 + 0xeb) =
               *(undefined4 *)((int)(char *)local_20 + 0xcb);
          *(int *)((int)(char *)local_20 + 0xcf) = local_6c;
          *(undefined4 *)((int)(char *)local_20 + 0xef) =
               *(undefined4 *)((int)(char *)local_20 + 0xcf);
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x14;
          *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
          return;
        }
      }
      piStack_94 = (int *)0x2bbff;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_6c);
      if (iVar1 < 10) {
        piStack_94 = (int *)0x2bc15;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_70);
        if (((5 < iVar1) && (local_4c < local_6c + 8)) &&
           ((local_6c + -8 < local_4c && ((local_7c < local_50 && (local_50 < local_80)))))) {
          *(int *)((int)(char *)local_20 + 0xcb) = local_70;
          *(undefined4 *)((int)(char *)local_20 + 0xeb) =
               *(undefined4 *)((int)(char *)local_20 + 0xcb);
          *(int *)((int)(char *)local_20 + 0xcf) = local_6c;
          *(undefined4 *)((int)(char *)local_20 + 0xef) =
               *(undefined4 *)((int)(char *)local_20 + 0xcf);
          if (*(int *)((int)(char *)local_20 + 0xcf) <
              *(int *)(DAT_00063354 + 0x12) +
              (*(int *)(DAT_00063354 + 0x16) - *(int *)(DAT_00063354 + 0x12) >> 1)) {
            if (*(int *)((int)(char *)local_20 + 0xcf) < *(int *)((int)(char *)local_20 + 0xc3)) {
              *(int *)((int)(char *)local_20 + 0xcb) =
                   *(int *)((int)(char *)local_20 + 0xcb) -
                   (*(int *)((int)(char *)local_20 + 0xc3) - *(int *)((int)(char *)local_20 + 0xcf))
              ;
            }
            else {
              *(int *)((int)(char *)local_20 + 0xcb) =
                   *(int *)((int)(char *)local_20 + 0xcb) +
                   (*(int *)((int)(char *)local_20 + 0xc3) - *(int *)((int)(char *)local_20 + 0xcf))
              ;
            }
          }
          else if (*(int *)((int)(char *)local_20 + 0xcf) < *(int *)((int)(char *)local_20 + 0xc3))
          {
            *(int *)((int)(char *)local_20 + 0xcb) =
                 *(int *)((int)(char *)local_20 + 0xcb) +
                 (*(int *)((int)(char *)local_20 + 0xc3) - *(int *)((int)(char *)local_20 + 0xcf));
          }
          else {
            *(int *)((int)(char *)local_20 + 0xcb) =
                 *(int *)((int)(char *)local_20 + 0xcb) -
                 (*(int *)((int)(char *)local_20 + 0xc3) - *(int *)((int)(char *)local_20 + 0xcf));
          }
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x15;
          *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
          return;
        }
      }
      if ((local_4c < local_78) && (local_74 < local_4c)) {
        piStack_94 = (int *)0x2bd90;
        iVar1 = FUN_0001519f((char *)local_20);
        if (local_50 <= iVar1 + -8) goto LAB_0002bda8;
        piStack_94 = (int *)0x2bda0;
        iVar1 = FUN_0001519f((char *)local_20);
        if (iVar1 + 8 <= local_50) goto LAB_0002bda8;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      else {
LAB_0002bda8:
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      if ((((local_50 < local_80) && (local_7c < local_50)) &&
          (*(int *)((int)(char *)local_20 + 0xc3) + -8 < local_4c)) &&
         (local_4c < *(int *)((int)(char *)local_20 + 0xc3) + 8)) {
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      else {
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
    }
  }
  if (*(int *)((int)(char *)local_20 + 0xaf) == 2) {
    *(int *)((int)(char *)local_20 + 0xab) =
         *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
    *(undefined4 *)((int)(char *)local_20 + 0x1f) = 5;
    *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
    ppiVar4 = (int **)local_90;
  }
  else {
    ppiVar4 = (int **)local_90;
    if (*(int *)((int)(char *)local_20 + 0xaf) == 8) {
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 9;
      *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      ppiVar4 = (int **)local_90;
    }
  }
LAB_0002c7e0:
  if (local_44 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x2c7f1;
    FUN_0001545e((char *)local_20,local_44);
  }
  if (local_40 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x2c802;
    FUN_0001586b((char *)local_20,local_40);
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x2c80a;
  iVar1 = FUN_0001519f((char *)local_20);
  if (local_50 < iVar1) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 1;
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x2c821;
  iVar1 = FUN_0001519f((char *)local_20);
  if (iVar1 < local_50) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 0;
  }
  return;
}


