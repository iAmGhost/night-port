// func #70  FUN_0002c838  0002c838  size=6790


/* WARNING: Removing unreachable block (ram,0x0002e26f) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0002c838(void)

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
  int *piStack_8c;
  int *piStack_88;
  undefined1 local_84 [4];
  int local_80;
  undefined4 local_7c;
  undefined4 local_78;
  int local_74;
  int local_70;
  int local_6c;
  int local_68;
  int local_64;
  int local_60;
  int local_5c;
  uint local_58;
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
  piStack_88 = (int *)0x2c85f;
  local_4c = FUN_0001519f(DAT_00053c7c);
  piStack_88 = (int *)0x2c86c;
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
  piStack_88 = (int *)0x2c8e5;
  FUN_00041f7e();
  local_44 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)((int)(char *)local_20 + 99) +
          (float10)*(float *)(&DAT_00051d5c + local_34 * 4);
  piStack_88 = (int *)0x2c8ff;
  FUN_00041f7e();
  local_50 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)((int)(char *)local_20 + 99);
  piStack_88 = (int *)0x2c90d;
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
    piStack_88 = (int *)0x2c9b2;
    local_38 = FUN_000163b4((char *)local_20);
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
    if (*(int *)((int)(char *)local_20 + 0x17) == 0x2a) {
      if (*(int *)((int)(char *)local_20 + 0x11) == 0x32) {
        piStack_88 = (int *)0x2cb2b;
        uVar2 = FUN_0001519f((char *)local_20);
        *(undefined4 *)((int)(char *)local_20 + 0xd3) = uVar2;
        piStack_88 = (int *)0x2cb3e;
        uVar2 = FUN_000151d5((char *)local_20);
        *(undefined4 *)((int)(char *)local_20 + 0xd7) = uVar2;
        *(int *)((int)(char *)local_20 + 0xcb) = local_4c;
        *(int *)((int)(char *)local_20 + 0xcf) = local_48;
        *(float *)((int)(char *)local_20 + 0xe7) =
             (float)(*(int *)((int)(char *)local_20 + 0xcf) - *(int *)((int)(char *)local_20 + 0xd7)
                    ) / (float)_DAT_0005005c;
        return;
      }
      if (*(int *)((int)(char *)local_20 + 0x11) < 0x33) {
        return;
      }
      if (0x46 < *(int *)((int)(char *)local_20 + 0x11)) {
        return;
      }
      local_54 = (float)(*(int *)((int)(char *)local_20 + 0x11) + -0x32);
      fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd7) +
              (float10)(int)local_54 * (float10)*(float *)((int)(char *)local_20 + 0xe7);
      piStack_88 = (int *)0x2cbc6;
      FUN_00041f7e();
      local_24 = (int)ROUND(fVar5);
      piStack_88 = (int *)0x2cbd1;
      iVar1 = FUN_000151d5((char *)local_20);
      piStack_88 = (int *)0x2cbde;
      FUN_0001586b((char *)local_20,local_24 - iVar1);
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x17) != 0x2c) {
      if (*(int *)((int)(char *)local_20 + 0x17) != 5) {
        return;
      }
      *(undefined4 *)((int)(char *)local_20 + 0x7f) = 1;
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) == 1) {
      piStack_88 = (int *)0x2cc05;
      uVar2 = FUN_0001519f((char *)local_20);
      *(undefined4 *)((int)(char *)local_20 + 0xd3) = uVar2;
      piStack_88 = (int *)0x2cc18;
      uVar2 = FUN_000151d5((char *)local_20);
      *(undefined4 *)((int)(char *)local_20 + 0xd7) = uVar2;
      *(int *)((int)(char *)local_20 + 0xcb) = local_4c;
      *(int *)((int)(char *)local_20 + 0xcf) = local_48;
      piStack_88 = (int *)0x2cc43;
      iVar1 = FUN_0001519f((char *)local_20);
      if (*(int *)((int)(char *)local_20 + 0xcb) == iVar1) {
        *(undefined4 *)((int)(char *)local_20 + 0xdb) = 0x47c35000;
      }
      else {
        piStack_88 = (int *)0x2cc6b;
        iVar1 = FUN_000151d5((char *)local_20);
        local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcf) - iVar1);
        piStack_88 = (int *)0x2cc89;
        iVar1 = FUN_0001519f((char *)local_20);
        *(float *)((int)(char *)local_20 + 0xdb) =
             local_54 / (float)(*(int *)((int)(char *)local_20 + 0xcb) - iVar1);
      }
      *(float *)((int)(char *)local_20 + 0xe3) =
           (float)*(int *)((int)(char *)local_20 + 0xcf) -
           (float)*(int *)((int)(char *)local_20 + 0xcb) * *(float *)((int)(char *)local_20 + 0xdb);
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) < 2) {
      return;
    }
    if (0x32 < *(int *)((int)(char *)local_20 + 0x11)) {
      return;
    }
    piStack_88 = (int *)0x2cd01;
    iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcb) -
                         *(int *)((int)(char *)local_20 + 0xd3));
    piStack_88 = (int *)0x2cd1a;
    iVar3 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcf) -
                         *(int *)((int)(char *)local_20 + 0xd7));
    if (iVar3 < iVar1) {
      local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcb) -
                        *(int *)((int)(char *)local_20 + 0xd3));
      fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd3) +
              (float10)*(int *)((int)(char *)local_20 + 0x11) *
              ((float10)(int)local_54 / (float10)_DAT_00050064);
      piStack_88 = (int *)0x2cd58;
      FUN_00041f7e();
      local_28 = (int)ROUND(fVar5);
      local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcb) -
                        *(int *)((int)(char *)local_20 + 0xd3));
      fVar5 = (float10)*(float *)((int)(char *)local_20 + 0xe3) +
              (float10)*(float *)((int)(char *)local_20 + 0xdb) *
              ((float10)*(int *)((int)(char *)local_20 + 0xd3) +
              (float10)*(int *)((int)(char *)local_20 + 0x11) *
              ((float10)(int)local_54 / (float10)_DAT_00050064));
      piStack_88 = (int *)0x2cda7;
      FUN_00041f7e();
      local_24 = (int)ROUND(fVar5);
    }
    else {
      local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcf) -
                        *(int *)((int)(char *)local_20 + 0xd7));
      fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd7) +
              (float10)*(int *)((int)(char *)local_20 + 0x11) *
              ((float10)(int)local_54 / (float10)_DAT_0005006c);
      piStack_88 = (int *)0x2cde5;
      FUN_00041f7e();
      local_24 = (int)ROUND(fVar5);
      if ((*(uint *)((int)(char *)local_20 + 0xdb) & 0x7fffffff) == 0) {
        local_28 = 100;
      }
      else {
        local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcf) -
                          *(int *)((int)(char *)local_20 + 0xd7));
        fVar5 = (((float10)*(int *)((int)(char *)local_20 + 0xd7) +
                 (float10)*(int *)((int)(char *)local_20 + 0x11) *
                 ((float10)(int)local_54 / (float10)_DAT_00050074)) -
                (float10)*(float *)((int)(char *)local_20 + 0xe3)) /
                (float10)*(float *)((int)(char *)local_20 + 0xdb);
        piStack_88 = (int *)0x2ce4c;
        FUN_00041f7e();
        local_28 = (int)ROUND(fVar5);
      }
    }
    piStack_88 = (int *)0x2ce57;
    iVar1 = FUN_0001519f((char *)local_20);
    piStack_88 = (int *)0x2ce64;
    FUN_0001545e((char *)local_20,local_28 - iVar1);
    piStack_88 = (int *)0x2ce6c;
    iVar1 = FUN_000151d5((char *)local_20);
    piStack_88 = (int *)0x2ce79;
    FUN_0001586b((char *)local_20,local_24 - iVar1);
    return;
  }
  if (*(int *)((int)(char *)local_20 + 0x7f) == 1) {
    piStack_88 = (int *)0x2cea4;
    iVar1 = FUN_0001519f((char *)local_20);
    piStack_88 = (int *)0x2ceac;
    iVar1 = FUN_00041f76(iVar1 - local_4c);
    if (iVar1 < 0x3c) {
      piStack_88 = (int *)0x2ceb9;
      iVar1 = FUN_000151d5((char *)local_20);
      piStack_88 = (int *)0x2cec1;
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
  local_58 = *(uint *)((char *)local_20 + 0x1f);
  if (8 < local_58) {
    if (local_58 < 10) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar1 = FUN_00017637((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 9;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_8c;
      }
    }
    else if (local_58 < 0xe) {
      if (local_58 < 0xb) {
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
        ppiVar4 = (int **)local_84;
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
          ppiVar4 = (int **)local_84;
        }
      }
      else if (local_58 < 0xc) {
        if (*(int *)((char *)local_20 + 0xab) < 0) {
          if ((&DAT_00053eb9)
              [*(int *)((char *)local_20 + 0x113) * 10 + *(int *)((char *)local_20 + 0xd) * 0x88] ==
              ',') {
            piStack_88 = (int *)0x2de6d;
            iVar1 = FUN_0001519f((char *)local_20,
                                 (&DAT_00053eb8)
                                 [*(int *)((char *)local_20 + 0x113) * 10 +
                                  *(int *)((char *)local_20 + 0xd) * 0x88]);
            piStack_88 = (int *)0x2de75;
            uVar6 = FUN_00041f76(iVar1 - local_4c);
            if ((int)uVar6 < (int)((ulonglong)uVar6 >> 0x20)) {
              if (((&DAT_00053eb8)
                   [*(int *)((int)(char *)local_20 + 0x11b) +
                    *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                    *(int *)((int)(char *)local_20 + 0x113) * 10] == '\0') ||
                 (9 < *(int *)((int)(char *)local_20 + 0x11b))) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_84;
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
                ppiVar4 = (int **)local_84;
              }
            }
            else {
              piStack_88 = (int *)0x2df1e;
              iVar1 = FUN_00041e9f();
              if (iVar1 % 0x14 == 2) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_84;
              }
              else {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_84;
              }
            }
          }
          else {
            piStack_88 = (int *)0x2df91;
            iVar1 = FUN_0001519f((char *)local_20,
                                 (&DAT_00053eb8)
                                 [*(int *)((char *)local_20 + 0x113) * 10 +
                                  *(int *)((char *)local_20 + 0xd) * 0x88]);
            piStack_88 = (int *)0x2df99;
            uVar6 = FUN_00041f76(iVar1 - local_4c);
            if ((int)uVar6 < (int)((ulonglong)uVar6 >> 0x20)) {
              piStack_88 = (int *)0x2dfa5;
              iVar1 = FUN_000151d5((char *)local_20);
              piStack_88 = (int *)0x2dfad;
              iVar1 = FUN_00041f76(iVar1 - local_48);
              if (iVar1 < 10) {
                if (((&DAT_00053eb8)
                     [*(int *)((int)(char *)local_20 + 0x11b) +
                      *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                      *(int *)((int)(char *)local_20 + 0x113) * 10] == '\0') ||
                   (9 < *(int *)((int)(char *)local_20 + 0x11b))) {
                  *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                  *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                  ppiVar4 = (int **)local_84;
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
                  ppiVar4 = (int **)local_84;
                }
                goto LAB_0002e274;
              }
            }
            piStack_88 = (int *)0x2e058;
            iVar1 = FUN_00041e9f();
            if (iVar1 % 0x14 == 2) {
              *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
              *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
              ppiVar4 = (int **)local_84;
            }
            else {
              *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
              *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
              ppiVar4 = (int **)local_84;
            }
          }
        }
        else {
          *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
          ((char *)local_20)[0x17] = '\x01';
          ((char *)local_20)[0x18] = '\0';
          ((char *)local_20)[0x19] = '\0';
          ((char *)local_20)[0x1a] = '\0';
          ppiVar4 = (int **)local_84;
        }
      }
      else if (local_58 < 0xd) {
        *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_84;
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
          ppiVar4 = (int **)local_84;
        }
      }
      else {
        *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_84;
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
          ppiVar4 = (int **)local_84;
        }
      }
    }
    else if (local_58 < 0xf) {
      *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
      ppiVar4 = (int **)local_84;
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
        ppiVar4 = (int **)local_84;
      }
    }
    else if (local_58 < 0x15) {
      ppiVar4 = (int **)local_84;
      if (local_58 == 0x14) {
        piStack_88 = &local_3c;
        piStack_8c = &local_40;
        iVar1 = FUN_0001776f((char *)local_20,local_44,local_4c,local_48);
        ppiVar4 = &piStack_8c;
        if (iVar1 != 0) {
          *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x14;
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
          *(undefined4 *)((int)(char *)local_20 + 199) = 1;
          ppiVar4 = &piStack_8c;
        }
      }
    }
    else if (local_58 < 0x16) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar1 = FUN_00017927((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x15;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_8c;
      }
    }
    else if (local_58 < 0x17) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar1 = FUN_00017ca6((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x16;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_8c;
      }
    }
    else {
      ppiVar4 = (int **)local_84;
      if (local_58 == 0x6a) {
        fVar5 = (float10)_DAT_00051d68;
        piStack_88 = (int *)0x2dab4;
        FUN_00041f7e();
        local_44 = (int)ROUND(fVar5);
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 99) + (float10)_DAT_00051d68;
        piStack_88 = (int *)0x2dac8;
        FUN_00041f7e();
        local_54 = (float)(int)ROUND(fVar5);
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 99);
        piStack_88 = (int *)0x2dad6;
        FUN_00041f7e();
        local_50 = (int)ROUND(fVar5);
        if (local_50 + local_44 < (int)local_54) {
          local_44 = local_44 + 1;
        }
        *(float *)((int)(char *)local_20 + 99) =
             _DAT_00051d68 + *(float *)((int)(char *)local_20 + 99);
        piStack_88 = &local_3c;
        piStack_8c = &local_40;
        iVar1 = FUN_0001730b((char *)local_20,local_44,local_4c,local_48);
        ppiVar4 = &piStack_8c;
        if (iVar1 != 0) {
          if ((((*(int *)(DAT_00053c7c + 0x17) == 0x15) || (*(int *)(DAT_00053c7c + 0x17) == 0x16))
              || (*(int *)(DAT_00053c7c + 0x17) == 0x17)) ||
             (((*(int *)(DAT_00053c7c + 0x17) == 0x18 || (*(int *)(DAT_00053c7c + 0x17) == 0x1a)) ||
              ((*(int *)(DAT_00053c7c + 0x17) == 0x1d ||
               ((*(int *)(DAT_00053c7c + 0x17) == 0x1e || (*(int *)(DAT_00053c7c + 0x17) == 0x20))))
              )))) {
            iVar1 = FUN_0001519f((char *)local_20);
            iVar1 = FUN_00041f76(iVar1 - local_4c);
            ppiVar4 = &piStack_8c;
            if (iVar1 < 0x4b) {
              iVar1 = FUN_000151d5((char *)local_20);
              iVar1 = FUN_00041f76(iVar1 - local_48);
              ppiVar4 = &piStack_8c;
              if (iVar1 < 10) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xd;
                *(undefined4 *)((int)(char *)local_20 + 0xab) = 0;
                ppiVar4 = &piStack_8c;
              }
            }
          }
          else {
            *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
            *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
            ppiVar4 = &piStack_8c;
          }
        }
      }
    }
    goto LAB_0002e274;
  }
  if (3 < local_58) {
    if (local_58 < 5) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      local_5c = FUN_00016fec((char *)local_20,local_44,local_4c,local_48);
      if (local_5c == 2) {
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 1;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 10;
        ppiVar4 = &piStack_8c;
      }
      else if (local_5c == 1) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
      else if (local_5c == 3) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xc;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 0x1e;
        ppiVar4 = &piStack_8c;
      }
      else {
        ppiVar4 = &piStack_8c;
        if (local_5c == 4) {
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
          ppiVar4 = &piStack_8c;
        }
      }
    }
    else if (local_58 < 6) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar1 = FUN_00016959((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 5;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_8c;
      }
    }
    else if (local_58 < 7) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar1 = FUN_0001730b((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    else if (local_58 < 8) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar1 = FUN_000174a2((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    else {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar1 = FUN_00017530((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    goto LAB_0002e274;
  }
  if (local_58 != 0) {
    if (local_58 < 2) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar1 = FUN_00017218((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 1;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    else if (local_58 < 3) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar1 = FUN_00016a60((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    else {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar1 = FUN_00016b8f((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 3;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    goto LAB_0002e274;
  }
  if (*(int *)((char *)local_20 + 0xaf) != 4) {
    if (*(int *)((char *)local_20 + 0xaf) == 10) {
      *(int *)((char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)((char *)local_20 + 0xd) * 0x88) >> 1;
    }
    else {
      piStack_88 = (int *)0x2cf53;
      local_30 = FUN_00041e9f();
      local_30 = local_30 % 0x32;
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) + local_30;
    }
    *(undefined4 *)((int)(char *)local_20 + 0x1f) = 4;
    piStack_88 = (int *)0x2cf8f;
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
      piStack_88 = (int *)0x2cff4;
      iVar1 = FUN_0001519f((char *)local_20);
      if (iVar1 < 0) {
LAB_0002d007:
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 7;
        return;
      }
      piStack_88 = (int *)0x2d000;
      iVar1 = FUN_0001519f((char *)local_20);
      if (0x13f < iVar1) goto LAB_0002d007;
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      if (*(int *)((int)(char *)local_20 + 199) == 1) {
        do {
          if (*(int *)((int)(char *)local_20 + 0x5f) == 0) {
            *(undefined4 *)((int)(char *)local_20 + 0x5f) = 1;
            piStack_88 = (int *)0x2d07e;
            local_80 = FUN_00041e9f();
            local_80 = local_80 % 2;
            *(undefined4 *)((int)(char *)local_20 + 0x5f) = 1;
          }
          else {
            piStack_88 = (int *)0x2d04f;
            iVar1 = FUN_00041e9f();
            local_80 = iVar1 % 4 + 6;
            *(undefined4 *)((int)(char *)local_20 + 0x5f) = 0;
          }
        } while (local_80 == *(int *)((int)(char *)local_20 + 0xff));
        if ((local_80 == 0) || (local_80 == 1)) {
          *(undefined4 *)((int)(char *)local_20 + 0x6f) = 1;
        }
        else {
          *(undefined4 *)((int)(char *)local_20 + 0x6f) = 0;
        }
        *(int *)((int)(char *)local_20 + 0xff) = local_80;
        local_7c = *(undefined4 *)((int)(char *)local_20 + 0xbf);
        local_78 = *(undefined4 *)((int)(char *)local_20 + 0xc3);
        if (local_80 == 0) {
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
        else if (local_80 == 1) {
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
        else if (local_80 < 6) {
          iVar3 = *(int *)(&DAT_00051c9c + local_80 * 0x10) + local_4c;
          piStack_88 = (int *)0x2d26b;
          iVar1 = FUN_00041e9f();
          piStack_88 = (int *)0x2d27b;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX);
          *(int *)((int)(char *)local_20 + 0xbf) = iVar3 + iVar1;
          iVar3 = *(int *)(&DAT_00051ca8 + local_80 * 0x10) -
                  *(int *)(&DAT_00051ca0 + local_80 * 0x10);
          piStack_88 = (int *)0x2d2b4;
          iVar1 = FUN_00041e9f(local_80 * 0x10,iVar3,iVar3,
                               *(int *)(&DAT_00051ca0 + local_80 * 0x10) + local_48);
          piStack_88 = (int *)0x2d2c4;
          iVar1 = FUN_00041f76(iVar1 % iVar3);
          *(int *)((int)(char *)local_20 + 0xc3) = extraout_ECX_00 + iVar1;
        }
        else {
          iVar3 = local_80 << 4;
          piStack_88 = (int *)0x2d2fb;
          iVar1 = FUN_00041e9f();
          piStack_88 = (int *)0x2d30b;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX_01);
          *(int *)((int)(char *)local_20 + 0xbf) = *(int *)(&DAT_00051c9c + iVar3) + iVar1;
          iVar3 = local_80 << 4;
          piStack_88 = (int *)0x2d341;
          iVar1 = FUN_00041e9f();
          piStack_88 = (int *)0x2d351;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX_02);
          *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(&DAT_00051ca0 + iVar3) + iVar1;
        }
        piStack_88 = (int *)0x2d36a;
        iVar1 = FUN_0001519f((char *)local_20);
        piStack_88 = (int *)0x2d37c;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - iVar1);
        if (iVar1 < 10) {
          piStack_88 = (int *)0x2d389;
          iVar1 = FUN_000151d5((char *)local_20);
          piStack_88 = (int *)0x2d39b;
          iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - iVar1);
          if (iVar1 < 5) {
            *(undefined4 *)((int)(char *)local_20 + 0xbf) = local_7c;
            *(undefined4 *)((int)(char *)local_20 + 0xc3) = local_78;
            goto LAB_0002d3c9;
          }
        }
        *(undefined4 *)((int)(char *)local_20 + 199) = 0;
      }
LAB_0002d3c9:
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
      piStack_88 = (int *)0x2d454;
      iVar1 = FUN_0001519f((char *)local_20);
      if (iVar1 < *(int *)((int)(char *)local_20 + 0xbf)) {
        local_74 = *(int *)((int)(char *)local_20 + 0xbf);
        piStack_88 = (int *)0x2d475;
        local_70 = FUN_0001519f((char *)local_20);
      }
      else {
        piStack_88 = (int *)0x2d482;
        local_74 = FUN_0001519f((char *)local_20);
        local_70 = *(int *)((int)(char *)local_20 + 0xbf);
      }
      piStack_88 = (int *)0x2d499;
      iVar1 = FUN_000151d5((char *)local_20);
      if (iVar1 < *(int *)((int)(char *)local_20 + 0xc3)) {
        local_6c = *(int *)((int)(char *)local_20 + 0xc3);
        piStack_88 = (int *)0x2d4ba;
        local_68 = FUN_000151d5((char *)local_20);
      }
      else {
        piStack_88 = (int *)0x2d4c7;
        local_6c = FUN_000151d5((char *)local_20);
        local_68 = *(int *)((int)(char *)local_20 + 0xc3);
      }
      piStack_88 = (int *)0x2d4de;
      local_64 = FUN_0001519f((char *)local_20);
      piStack_88 = (int *)0x2d4e9;
      local_60 = FUN_000151d5((char *)local_20);
      piStack_88 = (int *)0x2d4fd;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_64);
      if (iVar1 < 5) {
        piStack_88 = (int *)0x2d513;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_60);
        if ((((5 < iVar1) && (local_48 < local_6c)) && (local_68 < local_48)) &&
           ((local_64 + -8 < local_4c && (local_4c < local_64 + 8)))) {
          *(int *)((int)(char *)local_20 + 0xcb) = local_64;
          *(undefined4 *)((int)(char *)local_20 + 0xeb) =
               *(undefined4 *)((int)(char *)local_20 + 0xcb);
          *(int *)((int)(char *)local_20 + 0xcf) = local_60;
          *(undefined4 *)((int)(char *)local_20 + 0xef) =
               *(undefined4 *)((int)(char *)local_20 + 0xcf);
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x14;
          *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
          return;
        }
      }
      piStack_88 = (int *)0x2d5b0;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_60);
      if (iVar1 < 10) {
        piStack_88 = (int *)0x2d5c6;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_64);
        if (((5 < iVar1) && (local_48 < local_60 + 8)) &&
           ((local_60 + -8 < local_48 && ((local_70 < local_4c && (local_4c < local_74)))))) {
          *(int *)((int)(char *)local_20 + 0xcb) = local_64;
          *(undefined4 *)((int)(char *)local_20 + 0xeb) =
               *(undefined4 *)((int)(char *)local_20 + 0xcb);
          *(int *)((int)(char *)local_20 + 0xcf) = local_60;
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
      if ((local_48 < local_6c) && (local_68 < local_48)) {
        piStack_88 = (int *)0x2d741;
        iVar1 = FUN_0001519f((char *)local_20);
        if (local_4c <= iVar1 + -8) goto LAB_0002d759;
        piStack_88 = (int *)0x2d751;
        iVar1 = FUN_0001519f((char *)local_20);
        if (iVar1 + 8 <= local_4c) goto LAB_0002d759;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      else {
LAB_0002d759:
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      if ((((local_4c < local_74) && (local_70 < local_4c)) &&
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
    ppiVar4 = (int **)local_84;
  }
  else {
    ppiVar4 = (int **)local_84;
    if (*(int *)((int)(char *)local_20 + 0xaf) == 8) {
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 9;
      *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      ppiVar4 = (int **)local_84;
    }
  }
LAB_0002e274:
  if (local_40 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x2e285;
    FUN_0001545e((char *)local_20,local_40);
  }
  if (local_3c != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x2e296;
    FUN_0001586b((char *)local_20,local_3c);
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x2e29e;
  iVar1 = FUN_0001519f((char *)local_20);
  if (local_4c < iVar1) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 1;
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x2e2b5;
  iVar1 = FUN_0001519f((char *)local_20);
  if (iVar1 < local_4c) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 0;
  }
  return;
}


