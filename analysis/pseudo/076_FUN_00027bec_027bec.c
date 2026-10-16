// func #67  FUN_00027bec  00027bec  size=6927


/* WARNING: Removing unreachable block (ram,0x000296ac) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00027bec(void)

{
  int iVar1;
  undefined4 uVar2;
  int extraout_ECX;
  int extraout_ECX_00;
  int extraout_ECX_01;
  int extraout_ECX_02;
  int iVar3;
  int **ppiVar4;
  float10 fVar5;
  undefined8 uVar6;
  int *piStack_94;
  int *piStack_90;
  undefined1 local_8c [4];
  int local_88;
  undefined4 local_84;
  undefined4 local_80;
  int local_7c;
  int local_78;
  int local_74;
  int local_70;
  int local_6c;
  int local_68;
  int local_64;
  uint local_60;
  int local_5c;
  undefined4 uStack_58;
  float local_54;
  int local_50;
  int local_4c;
  int local_48;
  int local_44;
  int local_40;
  int local_3c;
  int local_38;
  int local_34;
  int local_30;
  int local_28;
  int local_24;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  piStack_90 = (int *)0x27c13;
  local_4c = FUN_0001519f(DAT_00053c7c);
  piStack_90 = (int *)0x27c20;
  local_48 = FUN_000151d5(DAT_00053c7c);
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
  piStack_90 = (int *)0x27c99;
  FUN_00041f7e();
  local_44 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)((int)(char *)local_20 + 99) +
          (float10)*(float *)(&DAT_00051d5c + local_34 * 4);
  piStack_90 = (int *)0x27cb3;
  FUN_00041f7e();
  local_50 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)((int)(char *)local_20 + 99);
  piStack_90 = (int *)0x27cc1;
  FUN_00041f7e();
  local_54 = (float)(int)ROUND(fVar5);
  if ((int)local_54 + local_44 < local_50) {
    local_44 = local_44 + 1;
  }
  *(float *)((int)(char *)local_20 + 99) =
       *(float *)(&DAT_00051d5c + local_34 * 4) + *(float *)((int)(char *)local_20 + 99);
  local_40 = 0;
  local_3c = 0;
  if (((((*(int *)(DAT_00053c7c + 0x17) != 0x15) && (*(int *)(DAT_00053c7c + 0x17) != 0x16)) &&
       (*(int *)(DAT_00053c7c + 0x17) != 0x17)) &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x18 && (*(int *)(DAT_00053c7c + 0x17) != 0x1a)))) &&
     ((*(int *)(DAT_00053c7c + 0x17) != 0x1d &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x1e && (*(int *)(DAT_00053c7c + 0x17) != 0x20)))))) {
    piStack_90 = (int *)0x27d66;
    local_38 = FUN_0001603e((char *)local_20);
    if (local_38 != 0) {
      *(int *)((int)(char *)local_20 + 0x10f) = local_38;
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
    if (*(int *)((int)(char *)local_20 + 0x17) != 0x2b) {
      if (*(int *)((int)(char *)local_20 + 0x17) != 5) {
        return;
      }
      *(undefined4 *)((int)(char *)local_20 + 0x7f) = 1;
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) == 0) {
      piStack_90 = (int *)0x27ee3;
      uVar2 = FUN_0001519f((char *)local_20);
      *(undefined4 *)((int)(char *)local_20 + 0xd3) = uVar2;
      piStack_90 = (int *)0x27ef6;
      uVar2 = FUN_000151d5((char *)local_20);
      *(undefined4 *)((int)(char *)local_20 + 0xd7) = uVar2;
      *(int *)((int)(char *)local_20 + 0xcb) = local_4c;
      *(int *)((int)(char *)local_20 + 0xcf) = local_48;
      piStack_90 = (int *)0x27f21;
      iVar1 = FUN_0001519f((char *)local_20);
      if (*(int *)((int)(char *)local_20 + 0xcb) == iVar1) {
        *(undefined4 *)((int)(char *)local_20 + 0xdb) = 0x47c35000;
      }
      else {
        piStack_90 = (int *)0x27f49;
        iVar1 = FUN_000151d5((char *)local_20);
        local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcf) - iVar1);
        piStack_90 = (int *)0x27f67;
        local_50 = FUN_0001519f((char *)local_20);
        local_50 = *(int *)((int)(char *)local_20 + 0xcb) - local_50;
        *(float *)((int)(char *)local_20 + 0xdb) = local_54 / (float)local_50;
      }
      piStack_90 = (int *)0x27f8e;
      iVar1 = FUN_0001519f((char *)local_20);
      piStack_90 = (int *)0x27fa0;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcb) - iVar1);
      piStack_90 = (int *)0x27faa;
      iVar3 = FUN_000151d5((char *)local_20);
      piStack_90 = (int *)0x27fbc;
      iVar3 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcf) - iVar3);
      if (iVar3 < iVar1) {
        piStack_90 = (int *)0x27fcc;
        iVar1 = FUN_0001519f((char *)local_20);
        if (*(int *)((int)(char *)local_20 + 0xcb) - iVar1 < 0) {
          *(undefined4 *)((int)(char *)local_20 + 0xe3) = 0xc0e00000;
          piStack_90 = (int *)0x27ff2;
          iVar1 = FUN_000151d5((char *)local_20);
          *(float *)((int)(char *)local_20 + 0xe7) =
               (float)(*(int *)((int)(char *)local_20 + 0xcf) - iVar1) / _DAT_00050034;
          return;
        }
        *(undefined4 *)((int)(char *)local_20 + 0xe3) = 0x40e00000;
        piStack_90 = (int *)0x2802b;
        iVar1 = FUN_000151d5((char *)local_20);
        *(float *)((int)(char *)local_20 + 0xe7) =
             (float)(*(int *)((int)(char *)local_20 + 0xcf) - iVar1) / _DAT_00050038;
        return;
      }
      piStack_90 = (int *)0x2805a;
      iVar1 = FUN_000151d5((char *)local_20);
      if (*(int *)((int)(char *)local_20 + 0xcf) - iVar1 < 0) {
        piStack_90 = (int *)0x28073;
        iVar1 = FUN_0001519f((char *)local_20);
        *(float *)((int)(char *)local_20 + 0xe3) =
             (float)(*(int *)((int)(char *)local_20 + 0xcb) - iVar1) / _DAT_0005003c;
        *(undefined4 *)((int)(char *)local_20 + 0xe7) = 0xc0800000;
        return;
      }
      piStack_90 = (int *)0x280ac;
      iVar1 = FUN_0001519f((char *)local_20);
      *(float *)((int)(char *)local_20 + 0xe3) =
           (float)(*(int *)((int)(char *)local_20 + 0xcb) - iVar1) / _DAT_00050040;
      *(undefined4 *)((int)(char *)local_20 + 0xe7) = 0x40800000;
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) < 1) {
      return;
    }
    if (0x10 < *(int *)((int)(char *)local_20 + 0x11)) {
      return;
    }
    uStack_58 = (undefined4)
                ((ulonglong)(double)ABS(*(float *)((int)(char *)local_20 + 0xe3)) >> 0x20);
    if (ABS(*(float *)((int)(char *)local_20 + 0xe3)) <=
        ABS(*(float *)((int)(char *)local_20 + 0xe7))) {
      iVar1 = *(int *)((int)(char *)local_20 + 0x11) + -0x48;
      _local_5c = CONCAT44(uStack_58,iVar1);
      fVar5 = (float10)iVar1 * (float10)*(float *)((int)(char *)local_20 + 0xe7);
      piStack_90 = (int *)0x28183;
      FUN_00041f7e();
      local_24 = (int)ROUND(fVar5);
      fVar5 = (float10)local_24 / (float10)*(float *)((int)(char *)local_20 + 0xdb);
      piStack_90 = (int *)0x28197;
      FUN_00041f7e();
      local_24 = local_24 + *(int *)((int)(char *)local_20 + 0xd7);
      local_28 = (int)ROUND(fVar5) + *(int *)((int)(char *)local_20 + 0xd3);
    }
    else {
      iVar1 = *(int *)((int)(char *)local_20 + 0x11) + -0x48;
      _local_5c = CONCAT44(uStack_58,iVar1);
      fVar5 = (float10)iVar1 * (float10)*(float *)((int)(char *)local_20 + 0xe3);
      piStack_90 = (int *)0x28135;
      FUN_00041f7e();
      local_28 = (int)ROUND(fVar5);
      fVar5 = (float10)local_28 * (float10)*(float *)((int)(char *)local_20 + 0xdb);
      piStack_90 = (int *)0x28149;
      FUN_00041f7e();
      local_28 = local_28 + *(int *)((int)(char *)local_20 + 0xd3);
      local_24 = (int)ROUND(fVar5) + *(int *)((int)(char *)local_20 + 0xd7);
    }
    if (((-1 < local_28) || (*(int *)((int)(char *)local_20 + 0xd3) < 0xb)) &&
       (((local_28 < 0x140 || (0x135 < *(int *)((int)(char *)local_20 + 0xd3))) &&
        ((*(int *)(DAT_00063354 + 0x12) + DAT_000639e0 <= local_24 &&
         (local_24 <= *(int *)(DAT_00063354 + 0x16) + DAT_000639e0)))))) {
      piStack_90 = (int *)0x28294;
      iVar1 = FUN_0001519f((char *)local_20);
      piStack_90 = (int *)0x282a1;
      FUN_0001545e((char *)local_20,local_28 - iVar1);
      piStack_90 = (int *)0x282a9;
      iVar1 = FUN_000151d5((char *)local_20);
      piStack_90 = (int *)0x282b6;
      FUN_0001586b((char *)local_20,local_24 - iVar1);
      return;
    }
    if (local_28 == *(int *)((int)(char *)local_20 + 0xd3) ||
        local_28 - *(int *)((int)(char *)local_20 + 0xd3) < 0) {
      if (local_28 - *(int *)((int)(char *)local_20 + 0xd3) < 0) {
        piStack_90 = (int *)0x2824a;
        FUN_0001545e((char *)local_20,0xfffffffd);
      }
    }
    else {
      piStack_90 = (int *)0x28229;
      FUN_0001545e((char *)local_20,3);
    }
    if (local_24 != *(int *)((int)(char *)local_20 + 0xd7) &&
        -1 < local_24 - *(int *)((int)(char *)local_20 + 0xd7)) {
      piStack_90 = (int *)0x28269;
      FUN_0001586b((char *)local_20,1);
      return;
    }
    if (-1 < local_24 - *(int *)((int)(char *)local_20 + 0xd7)) {
      return;
    }
    piStack_90 = (int *)0x2828a;
    FUN_0001586b((char *)local_20,0xffffffff);
    return;
  }
  if (*(int *)((int)(char *)local_20 + 0x7f) == 1) {
    piStack_90 = (int *)0x282e1;
    iVar1 = FUN_0001519f((char *)local_20);
    piStack_90 = (int *)0x282e9;
    iVar1 = FUN_00041f76(iVar1 - local_4c);
    if (iVar1 < 0x46) {
      piStack_90 = (int *)0x282f6;
      iVar1 = FUN_000151d5((char *)local_20);
      piStack_90 = (int *)0x282fe;
      iVar1 = FUN_00041f76(iVar1 - local_48);
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
  local_60 = *(uint *)((char *)local_20 + 0x1f);
  if (8 < local_60) {
    if (local_60 < 10) {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      iVar1 = FUN_00017637((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_94;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 9;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_94;
      }
    }
    else if (local_60 < 0xe) {
      if (local_60 < 0xb) {
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
        ppiVar4 = (int **)local_8c;
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
          ppiVar4 = (int **)local_8c;
        }
      }
      else if (local_60 < 0xc) {
        if (*(int *)((char *)local_20 + 0xab) < 0) {
          if ((&DAT_00053eb9)
              [*(int *)((char *)local_20 + 0x113) * 10 + *(int *)((char *)local_20 + 0xd) * 0x88] ==
              '+') {
            piStack_90 = (int *)0x292aa;
            iVar1 = FUN_0001519f((char *)local_20,
                                 (&DAT_00053eb8)
                                 [*(int *)((char *)local_20 + 0x113) * 10 +
                                  *(int *)((char *)local_20 + 0xd) * 0x88]);
            piStack_90 = (int *)0x292b2;
            uVar6 = FUN_00041f76(iVar1 - local_4c);
            if ((int)uVar6 < (int)((ulonglong)uVar6 >> 0x20)) {
              if (((&DAT_00053eb8)
                   [*(int *)((int)(char *)local_20 + 0x11b) +
                    *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                    *(int *)((int)(char *)local_20 + 0x113) * 10] == '\0') ||
                 (9 < *(int *)((int)(char *)local_20 + 0x11b))) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_8c;
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
                ppiVar4 = (int **)local_8c;
              }
            }
            else {
              piStack_90 = (int *)0x2935b;
              iVar1 = FUN_00041e9f();
              if (iVar1 % 0x14 == 2) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_8c;
              }
              else {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_8c;
              }
            }
          }
          else {
            piStack_90 = (int *)0x293ce;
            iVar1 = FUN_0001519f((char *)local_20,
                                 (&DAT_00053eb8)
                                 [*(int *)((char *)local_20 + 0x113) * 10 +
                                  *(int *)((char *)local_20 + 0xd) * 0x88]);
            piStack_90 = (int *)0x293d6;
            uVar6 = FUN_00041f76(iVar1 - local_4c);
            if ((int)uVar6 < (int)((ulonglong)uVar6 >> 0x20)) {
              piStack_90 = (int *)0x293e2;
              iVar1 = FUN_000151d5((char *)local_20);
              piStack_90 = (int *)0x293ea;
              iVar1 = FUN_00041f76(iVar1 - local_48);
              if (iVar1 < 10) {
                if (((&DAT_00053eb8)
                     [*(int *)((int)(char *)local_20 + 0x11b) +
                      *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                      *(int *)((int)(char *)local_20 + 0x113) * 10] == '\0') ||
                   (9 < *(int *)((int)(char *)local_20 + 0x11b))) {
                  *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                  *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                  ppiVar4 = (int **)local_8c;
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
                  ppiVar4 = (int **)local_8c;
                }
                goto LAB_000296b1;
              }
            }
            piStack_90 = (int *)0x29495;
            iVar1 = FUN_00041e9f();
            if (iVar1 % 0x14 == 2) {
              *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
              *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
              ppiVar4 = (int **)local_8c;
            }
            else {
              *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
              *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
              ppiVar4 = (int **)local_8c;
            }
          }
        }
        else {
          *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
          ((char *)local_20)[0x17] = '\x01';
          ((char *)local_20)[0x18] = '\0';
          ((char *)local_20)[0x19] = '\0';
          ((char *)local_20)[0x1a] = '\0';
          ppiVar4 = (int **)local_8c;
        }
      }
      else if (local_60 < 0xd) {
        *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_8c;
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
          ppiVar4 = (int **)local_8c;
        }
      }
      else {
        *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_8c;
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
          ppiVar4 = (int **)local_8c;
        }
      }
    }
    else if (local_60 < 0xf) {
      *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
      ppiVar4 = (int **)local_8c;
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
        ppiVar4 = (int **)local_8c;
      }
    }
    else if (local_60 < 0x15) {
      ppiVar4 = (int **)local_8c;
      if (local_60 == 0x14) {
        piStack_90 = &local_3c;
        piStack_94 = &local_40;
        iVar1 = FUN_0001776f((char *)local_20,local_44,local_4c,local_48);
        ppiVar4 = &piStack_94;
        if (iVar1 != 0) {
          *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x14;
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
          *(undefined4 *)((int)(char *)local_20 + 199) = 1;
          ppiVar4 = &piStack_94;
        }
      }
    }
    else if (local_60 < 0x16) {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      iVar1 = FUN_00017927((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_94;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x15;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_94;
      }
    }
    else if (local_60 < 0x17) {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      iVar1 = FUN_00017ca6((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_94;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x16;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_94;
      }
    }
    else {
      ppiVar4 = (int **)local_8c;
      if (local_60 == 0x6a) {
        fVar5 = (float10)_DAT_00051d68;
        piStack_90 = (int *)0x28ef1;
        FUN_00041f7e();
        local_44 = (int)ROUND(fVar5);
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 99) + (float10)_DAT_00051d68;
        piStack_90 = (int *)0x28f05;
        FUN_00041f7e();
        _local_5c = CONCAT44(uStack_58,(int)ROUND(fVar5));
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 99);
        piStack_90 = (int *)0x28f13;
        FUN_00041f7e();
        local_54 = (float)(int)ROUND(fVar5);
        if ((int)local_54 + local_44 < local_5c) {
          local_44 = local_44 + 1;
        }
        *(float *)((int)(char *)local_20 + 99) =
             _DAT_00051d68 + *(float *)((int)(char *)local_20 + 99);
        piStack_90 = &local_3c;
        piStack_94 = &local_40;
        iVar1 = FUN_0001730b((char *)local_20,local_44,local_4c,local_48);
        ppiVar4 = &piStack_94;
        if (iVar1 != 0) {
          if ((((*(int *)(DAT_00053c7c + 0x17) == 0x15) || (*(int *)(DAT_00053c7c + 0x17) == 0x16))
              || (*(int *)(DAT_00053c7c + 0x17) == 0x17)) ||
             (((*(int *)(DAT_00053c7c + 0x17) == 0x18 || (*(int *)(DAT_00053c7c + 0x17) == 0x1a)) ||
              ((*(int *)(DAT_00053c7c + 0x17) == 0x1d ||
               ((*(int *)(DAT_00053c7c + 0x17) == 0x1e || (*(int *)(DAT_00053c7c + 0x17) == 0x20))))
              )))) {
            iVar1 = FUN_0001519f((char *)local_20);
            iVar1 = FUN_00041f76(iVar1 - local_4c);
            ppiVar4 = &piStack_94;
            if (iVar1 < 0x4b) {
              iVar1 = FUN_000151d5((char *)local_20);
              iVar1 = FUN_00041f76(iVar1 - local_48);
              ppiVar4 = &piStack_94;
              if (iVar1 < 10) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xd;
                *(undefined4 *)((int)(char *)local_20 + 0xab) = 0;
                ppiVar4 = &piStack_94;
              }
            }
          }
          else {
            *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
            *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
            ppiVar4 = &piStack_94;
          }
        }
      }
    }
    goto LAB_000296b1;
  }
  if (3 < local_60) {
    if (local_60 < 5) {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      local_64 = FUN_00016fec((char *)local_20,local_44,local_4c,local_48);
      if (local_64 == 2) {
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 1;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 10;
        ppiVar4 = &piStack_94;
      }
      else if (local_64 == 1) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_94;
      }
      else if (local_64 == 3) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xc;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 0x1e;
        ppiVar4 = &piStack_94;
      }
      else {
        ppiVar4 = &piStack_94;
        if (local_64 == 4) {
          uVar2 = FUN_0001519f(DAT_00053c7c);
          *(undefined4 *)((int)(char *)local_20 + 0xb3) = uVar2;
          uVar2 = FUN_000151d5(DAT_00053c7c);
          *(undefined4 *)((int)(char *)local_20 + 0xb7) = uVar2;
          uVar2 = FUN_0001519f(DAT_00053c7c);
          uVar6 = FUN_0001519f((char *)local_20,uVar2);
          if ((int)((ulonglong)uVar6 >> 0x20) - (int)uVar6 < 0) {
            *(int *)((int)(char *)local_20 + 0xb3) = *(int *)((int)(char *)local_20 + 0xb3) + 0x46;
          }
          else {
            *(int *)((int)(char *)local_20 + 0xb3) = *(int *)((int)(char *)local_20 + 0xb3) + -0x46;
          }
          *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0;
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x6a;
          ppiVar4 = &piStack_94;
        }
      }
    }
    else if (local_60 < 6) {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      iVar1 = FUN_00016959((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_94;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 5;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_94;
      }
    }
    else if (local_60 < 7) {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      iVar1 = FUN_0001730b((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_94;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_94;
      }
    }
    else if (local_60 < 8) {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      iVar1 = FUN_000174a2((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_94;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_94;
      }
    }
    else {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      iVar1 = FUN_00017530((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_94;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_94;
      }
    }
    goto LAB_000296b1;
  }
  if (local_60 != 0) {
    if (local_60 < 2) {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      iVar1 = FUN_00017218((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_94;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 1;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_94;
      }
    }
    else if (local_60 < 3) {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      iVar1 = FUN_00016a60((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_94;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_94;
      }
    }
    else {
      piStack_90 = &local_3c;
      piStack_94 = &local_40;
      iVar1 = FUN_00016b8f((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_94;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 3;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_94;
      }
    }
    goto LAB_000296b1;
  }
  if (*(int *)((char *)local_20 + 0xaf) != 4) {
    if (*(int *)((char *)local_20 + 0xaf) == 10) {
      *(int *)((char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)((char *)local_20 + 0xd) * 0x88) >> 1;
    }
    else {
      piStack_90 = (int *)0x28390;
      local_30 = FUN_00041e9f();
      local_30 = local_30 % 0x32;
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) + local_30;
    }
    *(undefined4 *)((int)(char *)local_20 + 0x1f) = 4;
    piStack_90 = (int *)0x283cc;
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
      piStack_90 = (int *)0x28431;
      iVar1 = FUN_0001519f((char *)local_20);
      if (iVar1 < 0) {
LAB_00028444:
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 7;
        return;
      }
      piStack_90 = (int *)0x2843d;
      iVar1 = FUN_0001519f((char *)local_20);
      if (0x13f < iVar1) goto LAB_00028444;
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      if (*(int *)((int)(char *)local_20 + 199) == 1) {
        do {
          if (*(int *)((int)(char *)local_20 + 0x5f) == 0) {
            *(undefined4 *)((int)(char *)local_20 + 0x5f) = 1;
            piStack_90 = (int *)0x284bb;
            local_88 = FUN_00041e9f();
            local_88 = local_88 % 2;
            *(undefined4 *)((int)(char *)local_20 + 0x5f) = 1;
          }
          else {
            piStack_90 = (int *)0x2848c;
            iVar1 = FUN_00041e9f();
            local_88 = iVar1 % 4 + 6;
            *(undefined4 *)((int)(char *)local_20 + 0x5f) = 0;
          }
        } while (local_88 == *(int *)((int)(char *)local_20 + 0xff));
        if ((local_88 == 0) || (local_88 == 1)) {
          *(undefined4 *)((int)(char *)local_20 + 0x6f) = 1;
        }
        else {
          *(undefined4 *)((int)(char *)local_20 + 0x6f) = 0;
        }
        *(int *)((int)(char *)local_20 + 0xff) = local_88;
        local_84 = *(undefined4 *)((int)(char *)local_20 + 0xbf);
        local_80 = *(undefined4 *)((int)(char *)local_20 + 0xc3);
        if (local_88 == 0) {
          if ((uint)((int)(uint)(byte)(&DAT_00053eb8)
                                      [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                       *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) < 0x3c) {
            *(uint *)((int)(char *)local_20 + 0xbf) =
                 local_4c -
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88];
          }
          else {
            *(int *)((int)(char *)local_20 + 0xbf) =
                 (local_4c -
                 ((int)(uint)(byte)(&DAT_00053eb8)
                                   [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                    *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1)) + 1;
          }
          *(int *)((int)(char *)local_20 + 0xc3) = local_48;
        }
        else if (local_88 == 1) {
          if ((uint)((int)(uint)(byte)(&DAT_00053eb8)
                                      [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                       *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) < 0x3c) {
            *(uint *)((int)(char *)local_20 + 0xbf) =
                 local_4c +
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88];
          }
          else {
            *(int *)((int)(char *)local_20 + 0xbf) =
                 local_4c +
                 ((int)(uint)(byte)(&DAT_00053eb8)
                                   [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                    *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) + -1;
          }
          *(int *)((int)(char *)local_20 + 0xc3) = local_48;
        }
        else if (local_88 < 6) {
          iVar3 = *(int *)(&DAT_00051c9c + local_88 * 0x10) + local_4c;
          piStack_90 = (int *)0x286a8;
          iVar1 = FUN_00041e9f();
          piStack_90 = (int *)0x286b8;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX);
          *(int *)((int)(char *)local_20 + 0xbf) = iVar3 + iVar1;
          iVar3 = *(int *)(&DAT_00051ca8 + local_88 * 0x10) -
                  *(int *)(&DAT_00051ca0 + local_88 * 0x10);
          piStack_90 = (int *)0x286f1;
          iVar1 = FUN_00041e9f(local_88 * 0x10,iVar3,iVar3,
                               *(int *)(&DAT_00051ca0 + local_88 * 0x10) + local_48);
          piStack_90 = (int *)0x28701;
          iVar1 = FUN_00041f76(iVar1 % iVar3);
          *(int *)((int)(char *)local_20 + 0xc3) = extraout_ECX_00 + iVar1;
        }
        else {
          iVar3 = local_88 << 4;
          piStack_90 = (int *)0x28738;
          iVar1 = FUN_00041e9f();
          piStack_90 = (int *)0x28748;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX_01);
          *(int *)((int)(char *)local_20 + 0xbf) = *(int *)(&DAT_00051c9c + iVar3) + iVar1;
          iVar3 = local_88 << 4;
          piStack_90 = (int *)0x2877e;
          iVar1 = FUN_00041e9f();
          piStack_90 = (int *)0x2878e;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX_02);
          *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(&DAT_00051ca0 + iVar3) + iVar1;
        }
        piStack_90 = (int *)0x287a7;
        iVar1 = FUN_0001519f((char *)local_20);
        piStack_90 = (int *)0x287b9;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - iVar1);
        if (iVar1 < 10) {
          piStack_90 = (int *)0x287c6;
          iVar1 = FUN_000151d5((char *)local_20);
          piStack_90 = (int *)0x287d8;
          iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - iVar1);
          if (iVar1 < 5) {
            *(undefined4 *)((int)(char *)local_20 + 0xbf) = local_84;
            *(undefined4 *)((int)(char *)local_20 + 0xc3) = local_80;
            goto LAB_00028806;
          }
        }
        *(undefined4 *)((int)(char *)local_20 + 199) = 0;
      }
LAB_00028806:
      if (*(int *)((int)(char *)local_20 + 0xbf) < 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xbf) = 0x14;
      }
      else if (0x13f < *(int *)((int)(char *)local_20 + 0xbf)) {
        *(undefined4 *)((int)(char *)local_20 + 0xbf) = 300;
      }
      if (*(int *)((int)(char *)local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
        *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
      }
      if (*(int *)(DAT_00063354 + 0x16) <= *(int *)((int)(char *)local_20 + 0xc3)) {
        *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
      }
      piStack_90 = (int *)0x28891;
      iVar1 = FUN_0001519f((char *)local_20);
      if (iVar1 < *(int *)((int)(char *)local_20 + 0xbf)) {
        local_7c = *(int *)((int)(char *)local_20 + 0xbf);
        piStack_90 = (int *)0x288b2;
        local_78 = FUN_0001519f((char *)local_20);
      }
      else {
        piStack_90 = (int *)0x288bf;
        local_7c = FUN_0001519f((char *)local_20);
        local_78 = *(int *)((int)(char *)local_20 + 0xbf);
      }
      piStack_90 = (int *)0x288d6;
      iVar1 = FUN_000151d5((char *)local_20);
      if (iVar1 < *(int *)((int)(char *)local_20 + 0xc3)) {
        local_74 = *(int *)((int)(char *)local_20 + 0xc3);
        piStack_90 = (int *)0x288f7;
        local_70 = FUN_000151d5((char *)local_20);
      }
      else {
        piStack_90 = (int *)0x28904;
        local_74 = FUN_000151d5((char *)local_20);
        local_70 = *(int *)((int)(char *)local_20 + 0xc3);
      }
      piStack_90 = (int *)0x2891b;
      local_6c = FUN_0001519f((char *)local_20);
      piStack_90 = (int *)0x28926;
      local_68 = FUN_000151d5((char *)local_20);
      piStack_90 = (int *)0x2893a;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_6c);
      if (iVar1 < 5) {
        piStack_90 = (int *)0x28950;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_68);
        if ((((5 < iVar1) && (local_48 < local_74)) && (local_70 < local_48)) &&
           ((local_6c + -8 < local_4c && (local_4c < local_6c + 8)))) {
          *(int *)((int)(char *)local_20 + 0xcb) = local_6c;
          *(undefined4 *)((int)(char *)local_20 + 0xeb) =
               *(undefined4 *)((int)(char *)local_20 + 0xcb);
          *(int *)((int)(char *)local_20 + 0xcf) = local_68;
          *(undefined4 *)((int)(char *)local_20 + 0xef) =
               *(undefined4 *)((int)(char *)local_20 + 0xcf);
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x14;
          *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
          return;
        }
      }
      piStack_90 = (int *)0x289ed;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_68);
      if (iVar1 < 10) {
        piStack_90 = (int *)0x28a03;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_6c);
        if (((5 < iVar1) && (local_48 < local_68 + 8)) &&
           ((local_68 + -8 < local_48 && ((local_78 < local_4c && (local_4c < local_7c)))))) {
          *(int *)((int)(char *)local_20 + 0xcb) = local_6c;
          *(undefined4 *)((int)(char *)local_20 + 0xeb) =
               *(undefined4 *)((int)(char *)local_20 + 0xcb);
          *(int *)((int)(char *)local_20 + 0xcf) = local_68;
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
      if ((local_48 < local_74) && (local_70 < local_48)) {
        piStack_90 = (int *)0x28b7e;
        iVar1 = FUN_0001519f((char *)local_20);
        if (local_4c <= iVar1 + -8) goto LAB_00028b96;
        piStack_90 = (int *)0x28b8e;
        iVar1 = FUN_0001519f((char *)local_20);
        if (iVar1 + 8 <= local_4c) goto LAB_00028b96;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      else {
LAB_00028b96:
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      if ((((local_4c < local_7c) && (local_78 < local_4c)) &&
          (*(int *)((int)(char *)local_20 + 0xc3) + -8 < local_48)) &&
         (local_48 < *(int *)((int)(char *)local_20 + 0xc3) + 8)) {
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
    ppiVar4 = (int **)local_8c;
  }
  else {
    ppiVar4 = (int **)local_8c;
    if (*(int *)((int)(char *)local_20 + 0xaf) == 8) {
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 9;
      *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      ppiVar4 = (int **)local_8c;
    }
  }
LAB_000296b1:
  if (local_40 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x296c2;
    FUN_0001545e((char *)local_20,local_40);
  }
  if (local_3c != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x296d3;
    FUN_0001586b((char *)local_20,local_3c);
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x296db;
  iVar1 = FUN_0001519f((char *)local_20);
  if (local_4c < iVar1) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 1;
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x296f2;
  iVar1 = FUN_0001519f((char *)local_20);
  if (iVar1 < local_4c) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 0;
  }
  return;
}


