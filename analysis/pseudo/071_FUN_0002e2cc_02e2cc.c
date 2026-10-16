// func #71  FUN_0002e2cc  0002e2cc  size=6980


/* WARNING: Removing unreachable block (ram,0x0002fdc1) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0002e2cc(void)

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
  int *piStack_88;
  int *piStack_84;
  undefined1 local_80 [4];
  int local_7c;
  undefined4 local_78;
  undefined4 local_74;
  int local_70;
  int local_6c;
  int local_68;
  int local_64;
  int local_60;
  int local_5c;
  int local_58;
  uint local_54;
  float local_50;
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
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  piStack_84 = (int *)0x2e2f3;
  local_48 = FUN_0001519f(DAT_00053c7c);
  piStack_84 = (int *)0x2e300;
  local_44 = FUN_000151d5(DAT_00053c7c);
  if ((char *)local_20 == (char *)DAT_00053c7c) {
    return;
  }
  if ((*(int *)((int)(char *)local_20 + 0x1f) < 10) ||
     (0x13 < *(int *)((int)(char *)local_20 + 0x1f))) {
    local_30 = *(int *)(&DAT_00053ea0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  else {
    local_30 = *(int *)(&DAT_00053f1c + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  if (*(int *)((int)(char *)local_20 + 0x6f) == 1) {
    local_30 = *(int *)(&DAT_00053f1c + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  fVar5 = (float10)*(float *)(&DAT_00051d5c + local_30 * 4);
  piStack_84 = (int *)0x2e379;
  FUN_00041f7e();
  local_40 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)((int)(char *)local_20 + 99) +
          (float10)*(float *)(&DAT_00051d5c + local_30 * 4);
  piStack_84 = (int *)0x2e393;
  FUN_00041f7e();
  local_4c = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)((int)(char *)local_20 + 99);
  piStack_84 = (int *)0x2e3a1;
  FUN_00041f7e();
  local_50 = (float)(int)ROUND(fVar5);
  if ((int)local_50 + local_40 < local_4c) {
    local_40 = local_40 + 1;
  }
  *(float *)((int)(char *)local_20 + 99) =
       *(float *)(&DAT_00051d5c + local_30 * 4) + *(float *)((int)(char *)local_20 + 99);
  local_3c = 0;
  local_38 = 0;
  if (((((*(int *)(DAT_00053c7c + 0x17) != 0x15) && (*(int *)(DAT_00053c7c + 0x17) != 0x16)) &&
       (*(int *)(DAT_00053c7c + 0x17) != 0x17)) &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x18 && (*(int *)(DAT_00053c7c + 0x17) != 0x1a)))) &&
     ((*(int *)(DAT_00053c7c + 0x17) != 0x1d &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x1e && (*(int *)(DAT_00053c7c + 0x17) != 0x20)))))) {
    piStack_84 = (int *)0x2e446;
    local_34 = FUN_0001656f((char *)local_20);
    if (local_34 != 0) {
      *(int *)((int)(char *)local_20 + 0x10f) = local_34;
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
    if (*(int *)((int)(char *)local_20 + 0x17) == 0x2c) {
      if (*(int *)((int)(char *)local_20 + 0x11) == 1) {
        piStack_84 = (int *)0x2e5bf;
        uVar2 = FUN_0001519f((char *)local_20);
        *(undefined4 *)((int)(char *)local_20 + 0xd3) = uVar2;
        piStack_84 = (int *)0x2e5d2;
        uVar2 = FUN_000151d5((char *)local_20);
        *(undefined4 *)((int)(char *)local_20 + 0xd7) = uVar2;
        *(int *)((int)(char *)local_20 + 0xcb) = local_48;
        *(int *)((int)(char *)local_20 + 0xcf) = local_44;
        *(float *)((int)(char *)local_20 + 0xe7) =
             (float)(*(int *)((int)(char *)local_20 + 0xcf) - *(int *)((int)(char *)local_20 + 0xd7)
                    ) / (float)_DAT_0005007c;
        return;
      }
      if (*(int *)((int)(char *)local_20 + 0x11) < 2) {
        return;
      }
      if (0x14 < *(int *)((int)(char *)local_20 + 0x11)) {
        return;
      }
      fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd7) +
              (float10)*(int *)((int)(char *)local_20 + 0x11) *
              (float10)*(float *)((int)(char *)local_20 + 0xe7);
      piStack_84 = (int *)0x2e651;
      FUN_00041f7e();
      local_24 = (int)ROUND(fVar5);
      piStack_84 = (int *)0x2e65c;
      iVar1 = FUN_000151d5((char *)local_20);
      piStack_84 = (int *)0x2e669;
      FUN_0001586b((char *)local_20,local_24 - iVar1);
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x17) != 0x2d) {
      if (*(int *)((int)(char *)local_20 + 0x17) != 5) {
        return;
      }
      *(undefined4 *)((int)(char *)local_20 + 0x7f) = 1;
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) == 1) {
      piStack_84 = (int *)0x2e690;
      uVar2 = FUN_0001519f((char *)local_20);
      *(undefined4 *)((int)(char *)local_20 + 0xd3) = uVar2;
      piStack_84 = (int *)0x2e6a3;
      uVar2 = FUN_000151d5((char *)local_20);
      *(undefined4 *)((int)(char *)local_20 + 0xd7) = uVar2;
      *(int *)((int)(char *)local_20 + 0xcb) = local_48;
      *(int *)((int)(char *)local_20 + 0xcf) = local_44;
      piStack_84 = (int *)0x2e6ce;
      iVar1 = FUN_0001519f((char *)local_20);
      if (*(int *)((int)(char *)local_20 + 0xcb) == iVar1) {
        *(undefined4 *)((int)(char *)local_20 + 0xdb) = 0x47c35000;
      }
      else {
        piStack_84 = (int *)0x2e6f6;
        iVar1 = FUN_000151d5((char *)local_20);
        local_50 = (float)(*(int *)((int)(char *)local_20 + 0xcf) - iVar1);
        piStack_84 = (int *)0x2e714;
        iVar1 = FUN_0001519f((char *)local_20);
        *(float *)((int)(char *)local_20 + 0xdb) =
             local_50 / (float)(*(int *)((int)(char *)local_20 + 0xcb) - iVar1);
      }
      *(float *)((int)(char *)local_20 + 0xe3) =
           (float)*(int *)((int)(char *)local_20 + 0xcf) -
           (float)*(int *)((int)(char *)local_20 + 0xcb) * *(float *)((int)(char *)local_20 + 0xdb);
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) < 2) {
      return;
    }
    if (0x37 < *(int *)((int)(char *)local_20 + 0x11)) {
      return;
    }
    piStack_84 = (int *)0x2e78c;
    iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcb) -
                         *(int *)((int)(char *)local_20 + 0xd3));
    piStack_84 = (int *)0x2e7a5;
    iVar3 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcf) -
                         *(int *)((int)(char *)local_20 + 0xd7));
    if (iVar3 < iVar1) {
      local_50 = (float)(*(int *)((int)(char *)local_20 + 0xcb) -
                        *(int *)((int)(char *)local_20 + 0xd3));
      fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd3) +
              (float10)*(int *)((int)(char *)local_20 + 0x11) *
              ((float10)(int)local_50 / (float10)_DAT_00050084);
      piStack_84 = (int *)0x2e7e3;
      FUN_00041f7e();
      local_28 = (int)ROUND(fVar5);
      local_50 = (float)(*(int *)((int)(char *)local_20 + 0xcb) -
                        *(int *)((int)(char *)local_20 + 0xd3));
      fVar5 = (float10)*(float *)((int)(char *)local_20 + 0xe3) +
              (float10)*(float *)((int)(char *)local_20 + 0xdb) *
              ((float10)*(int *)((int)(char *)local_20 + 0xd3) +
              (float10)*(int *)((int)(char *)local_20 + 0x11) *
              ((float10)(int)local_50 / (float10)_DAT_00050084));
      piStack_84 = (int *)0x2e832;
      FUN_00041f7e();
      local_24 = (int)ROUND(fVar5);
    }
    else {
      local_50 = (float)(*(int *)((int)(char *)local_20 + 0xcf) -
                        *(int *)((int)(char *)local_20 + 0xd7));
      fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd7) +
              (float10)*(int *)((int)(char *)local_20 + 0x11) *
              ((float10)(int)local_50 / (float10)_DAT_0005008c);
      piStack_84 = (int *)0x2e870;
      FUN_00041f7e();
      local_24 = (int)ROUND(fVar5);
      if ((*(uint *)((int)(char *)local_20 + 0xdb) & 0x7fffffff) == 0) {
        local_28 = 100;
      }
      else {
        local_50 = (float)(*(int *)((int)(char *)local_20 + 0xcf) -
                          *(int *)((int)(char *)local_20 + 0xd7));
        fVar5 = (((float10)*(int *)((int)(char *)local_20 + 0xd7) +
                 (float10)*(int *)((int)(char *)local_20 + 0x11) *
                 ((float10)(int)local_50 / (float10)_DAT_00050094)) -
                (float10)*(float *)((int)(char *)local_20 + 0xe3)) /
                (float10)*(float *)((int)(char *)local_20 + 0xdb);
        piStack_84 = (int *)0x2e8d7;
        FUN_00041f7e();
        local_28 = (int)ROUND(fVar5);
      }
    }
    piStack_84 = (int *)0x2e8e2;
    iVar1 = FUN_0001519f((char *)local_20);
    piStack_84 = (int *)0x2e8ef;
    FUN_0001545e((char *)local_20,local_28 - iVar1);
    piStack_84 = (int *)0x2e8f7;
    iVar1 = FUN_000151d5((char *)local_20);
    piStack_84 = (int *)0x2e904;
    FUN_0001586b((char *)local_20,local_24 - iVar1);
    return;
  }
  if (*(int *)((int)(char *)local_20 + 0x7f) == 1) {
    piStack_84 = (int *)0x2e92f;
    iVar1 = FUN_0001519f((char *)local_20);
    piStack_84 = (int *)0x2e937;
    iVar1 = FUN_00041f76(iVar1 - local_48);
    if (iVar1 < 0x46) {
      piStack_84 = (int *)0x2e944;
      iVar1 = FUN_000151d5((char *)local_20);
      piStack_84 = (int *)0x2e94c;
      iVar1 = FUN_00041f76(iVar1 - local_44);
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
  local_54 = *(uint *)((char *)local_20 + 0x1f);
  if (8 < local_54) {
    if (local_54 < 10) {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      iVar1 = FUN_00017637((char *)local_20,local_40,local_48,local_44);
      ppiVar4 = &piStack_88;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 9;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_88;
      }
    }
    else if (local_54 < 0xe) {
      if (local_54 < 0xb) {
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
        ppiVar4 = (int **)local_80;
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
          ppiVar4 = (int **)local_80;
        }
      }
      else if (local_54 < 0xc) {
        if (*(int *)((char *)local_20 + 0xab) < 0) {
          if (((&DAT_00053eb9)
               [*(int *)((char *)local_20 + 0x113) * 10 + *(int *)((char *)local_20 + 0xd) * 0x88]
               == ',') ||
             ((&DAT_00053eb9)
              [*(int *)((char *)local_20 + 0x113) * 10 + *(int *)((char *)local_20 + 0xd) * 0x88] ==
              '-')) {
            piStack_84 = (int *)0x2f9bf;
            iVar1 = FUN_0001519f((char *)local_20,
                                 (&DAT_00053eb8)
                                 [*(int *)((char *)local_20 + 0x113) * 10 +
                                  *(int *)((char *)local_20 + 0xd) * 0x88]);
            piStack_84 = (int *)0x2f9c7;
            uVar6 = FUN_00041f76(iVar1 - local_48);
            if ((int)uVar6 < (int)((ulonglong)uVar6 >> 0x20)) {
              if (((&DAT_00053eb8)
                   [*(int *)((int)(char *)local_20 + 0x11b) +
                    *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                    *(int *)((int)(char *)local_20 + 0x113) * 10] == '\0') ||
                 (9 < *(int *)((int)(char *)local_20 + 0x11b))) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_80;
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
                ppiVar4 = (int **)local_80;
              }
            }
            else {
              piStack_84 = (int *)0x2fa70;
              iVar1 = FUN_00041e9f();
              if (iVar1 % 0x14 == 2) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_80;
              }
              else {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_80;
              }
            }
          }
          else {
            piStack_84 = (int *)0x2fae3;
            iVar1 = FUN_0001519f((char *)local_20,
                                 (&DAT_00053eb8)
                                 [*(int *)((char *)local_20 + 0x113) * 10 +
                                  *(int *)((char *)local_20 + 0xd) * 0x88]);
            piStack_84 = (int *)0x2faeb;
            uVar6 = FUN_00041f76(iVar1 - local_48);
            if ((int)uVar6 < (int)((ulonglong)uVar6 >> 0x20)) {
              piStack_84 = (int *)0x2faf7;
              iVar1 = FUN_000151d5((char *)local_20);
              piStack_84 = (int *)0x2faff;
              iVar1 = FUN_00041f76(iVar1 - local_44);
              if (iVar1 < 10) {
                if (((&DAT_00053eb8)
                     [*(int *)((int)(char *)local_20 + 0x11b) +
                      *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                      *(int *)((int)(char *)local_20 + 0x113) * 10] == '\0') ||
                   (9 < *(int *)((int)(char *)local_20 + 0x11b))) {
                  *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                  *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                  ppiVar4 = (int **)local_80;
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
                  ppiVar4 = (int **)local_80;
                }
                goto LAB_0002fdc6;
              }
            }
            piStack_84 = (int *)0x2fbaa;
            iVar1 = FUN_00041e9f();
            if (iVar1 % 0x14 == 2) {
              *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
              *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
              ppiVar4 = (int **)local_80;
            }
            else {
              *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
              *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
              ppiVar4 = (int **)local_80;
            }
          }
        }
        else {
          *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
          ((char *)local_20)[0x17] = '\x01';
          ((char *)local_20)[0x18] = '\0';
          ((char *)local_20)[0x19] = '\0';
          ((char *)local_20)[0x1a] = '\0';
          ppiVar4 = (int **)local_80;
        }
      }
      else if (local_54 < 0xd) {
        *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_80;
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
          ppiVar4 = (int **)local_80;
        }
      }
      else {
        *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_80;
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
          ppiVar4 = (int **)local_80;
        }
      }
    }
    else if (local_54 < 0xf) {
      *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
      ppiVar4 = (int **)local_80;
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
        ppiVar4 = (int **)local_80;
      }
    }
    else if (local_54 < 0x15) {
      ppiVar4 = (int **)local_80;
      if (local_54 == 0x14) {
        piStack_84 = &local_38;
        piStack_88 = &local_3c;
        iVar1 = FUN_0001776f((char *)local_20,local_40,local_48,local_44);
        ppiVar4 = &piStack_88;
        if (iVar1 != 0) {
          *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x14;
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
          *(undefined4 *)((int)(char *)local_20 + 199) = 1;
          ppiVar4 = &piStack_88;
        }
      }
    }
    else if (local_54 < 0x16) {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      iVar1 = FUN_00017927((char *)local_20,local_40,local_48,local_44);
      ppiVar4 = &piStack_88;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x15;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_88;
      }
    }
    else if (local_54 < 0x17) {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      iVar1 = FUN_00017ca6((char *)local_20,local_40,local_48,local_44);
      ppiVar4 = &piStack_88;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x16;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_88;
      }
    }
    else {
      ppiVar4 = (int **)local_80;
      if (local_54 == 0x6a) {
        fVar5 = (float10)_DAT_00051d68;
        piStack_84 = (int *)0x2f5e0;
        FUN_00041f7e();
        local_40 = (int)ROUND(fVar5);
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 99) + (float10)_DAT_00051d68;
        piStack_84 = (int *)0x2f5f4;
        FUN_00041f7e();
        local_50 = (float)(int)ROUND(fVar5);
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 99);
        piStack_84 = (int *)0x2f602;
        FUN_00041f7e();
        local_4c = (int)ROUND(fVar5);
        if (local_4c + local_40 < (int)local_50) {
          local_40 = local_40 + 1;
        }
        *(float *)((int)(char *)local_20 + 99) =
             _DAT_00051d68 + *(float *)((int)(char *)local_20 + 99);
        piStack_84 = &local_38;
        piStack_88 = &local_3c;
        iVar1 = FUN_0001730b((char *)local_20,local_40,local_48,local_44);
        ppiVar4 = &piStack_88;
        if (iVar1 != 0) {
          if ((((*(int *)(DAT_00053c7c + 0x17) == 0x15) || (*(int *)(DAT_00053c7c + 0x17) == 0x16))
              || (*(int *)(DAT_00053c7c + 0x17) == 0x17)) ||
             (((*(int *)(DAT_00053c7c + 0x17) == 0x18 || (*(int *)(DAT_00053c7c + 0x17) == 0x1a)) ||
              ((*(int *)(DAT_00053c7c + 0x17) == 0x1d ||
               ((*(int *)(DAT_00053c7c + 0x17) == 0x1e || (*(int *)(DAT_00053c7c + 0x17) == 0x20))))
              )))) {
            iVar1 = FUN_0001519f((char *)local_20);
            iVar1 = FUN_00041f76(iVar1 - local_48);
            ppiVar4 = &piStack_88;
            if (iVar1 < 0x37) {
              iVar1 = FUN_000151d5((char *)local_20);
              iVar1 = FUN_00041f76(iVar1 - local_44);
              ppiVar4 = &piStack_88;
              if (iVar1 < 10) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xd;
                *(undefined4 *)((int)(char *)local_20 + 0xab) = 0;
                ppiVar4 = &piStack_88;
              }
            }
          }
          else {
            *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
            *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
            ppiVar4 = &piStack_88;
          }
        }
      }
    }
    goto LAB_0002fdc6;
  }
  if (3 < local_54) {
    if (local_54 < 5) {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      local_58 = FUN_00016fec((char *)local_20,local_40,local_48,local_44);
      if (((((*(int *)(DAT_00053c7c + 0x17) == 0x15) || (*(int *)(DAT_00053c7c + 0x17) == 0x16)) ||
           ((*(int *)(DAT_00053c7c + 0x17) == 0x17 ||
            ((*(int *)(DAT_00053c7c + 0x17) == 0x18 || (*(int *)(DAT_00053c7c + 0x17) == 0x1a))))))
          || (*(int *)(DAT_00053c7c + 0x17) == 0x1d)) ||
         ((*(int *)(DAT_00053c7c + 0x17) == 0x1e || (*(int *)(DAT_00053c7c + 0x17) == 0x20)))) {
        local_58 = 4;
      }
      if (local_58 == 2) {
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 1;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 10;
        ppiVar4 = &piStack_88;
      }
      else if (local_58 == 1) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_88;
      }
      else if (local_58 == 3) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xc;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 0x1e;
        ppiVar4 = &piStack_88;
      }
      else {
        ppiVar4 = &piStack_88;
        if (local_58 == 4) {
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
          ppiVar4 = &piStack_88;
        }
      }
    }
    else if (local_54 < 6) {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      iVar1 = FUN_00016959((char *)local_20,local_40,local_48,local_44);
      ppiVar4 = &piStack_88;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 5;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_88;
      }
    }
    else if (local_54 < 7) {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      iVar1 = FUN_0001730b((char *)local_20,local_40,local_48,local_44);
      ppiVar4 = &piStack_88;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_88;
      }
    }
    else if (local_54 < 8) {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      iVar1 = FUN_000174a2((char *)local_20,local_40,local_48,local_44);
      ppiVar4 = &piStack_88;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_88;
      }
    }
    else {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      iVar1 = FUN_00017530((char *)local_20,local_40,local_48,local_44);
      ppiVar4 = &piStack_88;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_88;
      }
    }
    goto LAB_0002fdc6;
  }
  if (local_54 != 0) {
    if (local_54 < 2) {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      iVar1 = FUN_00017218((char *)local_20,local_40,local_48,local_44);
      ppiVar4 = &piStack_88;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 1;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_88;
      }
    }
    else if (local_54 < 3) {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      iVar1 = FUN_00016a60((char *)local_20,local_40,local_48,local_44);
      ppiVar4 = &piStack_88;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_88;
      }
    }
    else {
      piStack_84 = &local_38;
      piStack_88 = &local_3c;
      iVar1 = FUN_00016b8f((char *)local_20,local_40,local_48,local_44);
      ppiVar4 = &piStack_88;
      if (iVar1 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 3;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_88;
      }
    }
    goto LAB_0002fdc6;
  }
  if (*(int *)((char *)local_20 + 0xaf) != 4) {
    if (*(int *)((char *)local_20 + 0xaf) == 10) {
      *(int *)((char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)((char *)local_20 + 0xd) * 0x88) >> 1;
    }
    else {
      piStack_84 = (int *)0x2e9de;
      local_2c = FUN_00041e9f();
      local_2c = local_2c % 0x32;
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) + local_2c;
    }
    *(undefined4 *)((int)(char *)local_20 + 0x1f) = 4;
    piStack_84 = (int *)0x2ea1a;
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
      piStack_84 = (int *)0x2ea7f;
      iVar1 = FUN_0001519f((char *)local_20);
      if (iVar1 < 0) {
LAB_0002ea92:
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 7;
        return;
      }
      piStack_84 = (int *)0x2ea8b;
      iVar1 = FUN_0001519f((char *)local_20);
      if (0x13f < iVar1) goto LAB_0002ea92;
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      if (*(int *)((int)(char *)local_20 + 199) == 1) {
        do {
          piStack_84 = (int *)0x2ead1;
          iVar1 = FUN_00041e9f();
          if (iVar1 % 100 < 0x32) {
            piStack_84 = (int *)0x2eae9;
            iVar1 = FUN_00041e9f(iVar1 / 100);
            local_7c = iVar1 % 8 + 2;
            if (local_7c == 2) {
              piStack_84 = (int *)0x2eb08;
              iVar1 = FUN_00041e9f(iVar1 / 8);
              local_7c = local_7c + iVar1 % 2;
            }
            else if (local_7c == 3) {
              local_7c = 4;
              piStack_84 = (int *)0x2eb2d;
              iVar1 = FUN_00041e9f(iVar1 / 8);
              local_7c = local_7c + iVar1 % 2;
            }
          }
          else {
            piStack_84 = (int *)0x2eb45;
            local_7c = FUN_00041e9f(iVar1 / 100);
            local_7c = local_7c % 2;
          }
        } while (local_7c == *(int *)((int)(char *)local_20 + 0xff));
        if ((local_7c == 0) || (local_7c == 1)) {
          *(undefined4 *)((int)(char *)local_20 + 0x6f) = 1;
        }
        else {
          *(undefined4 *)((int)(char *)local_20 + 0x6f) = 0;
        }
        *(int *)((int)(char *)local_20 + 0xff) = local_7c;
        local_78 = *(undefined4 *)((int)(char *)local_20 + 0xbf);
        local_74 = *(undefined4 *)((int)(char *)local_20 + 0xc3);
        if (local_7c == 0) {
          if ((uint)((int)(uint)(byte)(&DAT_00053eb8)
                                      [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                       *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) < 0x3c) {
            *(uint *)((int)(char *)local_20 + 0xbf) =
                 local_48 -
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88];
          }
          else {
            *(int *)((int)(char *)local_20 + 0xbf) =
                 (local_48 -
                 ((int)(uint)(byte)(&DAT_00053eb8)
                                   [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                    *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1)) + 1;
          }
          *(int *)((int)(char *)local_20 + 0xc3) = local_44;
        }
        else if (local_7c == 1) {
          if ((uint)((int)(uint)(byte)(&DAT_00053eb8)
                                      [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                       *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) < 0x3c) {
            *(uint *)((int)(char *)local_20 + 0xbf) =
                 local_48 +
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88];
          }
          else {
            *(int *)((int)(char *)local_20 + 0xbf) =
                 local_48 +
                 ((int)(uint)(byte)(&DAT_00053eb8)
                                   [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                    *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) + -1;
          }
          *(int *)((int)(char *)local_20 + 0xc3) = local_44;
        }
        else if (local_7c < 6) {
          iVar3 = *(int *)(&DAT_00051c9c + local_7c * 0x10) + local_48;
          piStack_84 = (int *)0x2ed2c;
          iVar1 = FUN_00041e9f();
          piStack_84 = (int *)0x2ed3c;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX);
          *(int *)((int)(char *)local_20 + 0xbf) = iVar3 + iVar1;
          iVar3 = *(int *)(&DAT_00051ca8 + local_7c * 0x10) -
                  *(int *)(&DAT_00051ca0 + local_7c * 0x10);
          piStack_84 = (int *)0x2ed75;
          iVar1 = FUN_00041e9f(local_7c * 0x10,iVar3,iVar3,
                               *(int *)(&DAT_00051ca0 + local_7c * 0x10) + local_44);
          piStack_84 = (int *)0x2ed85;
          iVar1 = FUN_00041f76(iVar1 % iVar3);
          *(int *)((int)(char *)local_20 + 0xc3) = extraout_ECX_00 + iVar1;
        }
        else {
          iVar3 = local_7c << 4;
          piStack_84 = (int *)0x2edbc;
          iVar1 = FUN_00041e9f();
          piStack_84 = (int *)0x2edcc;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX_01);
          *(int *)((int)(char *)local_20 + 0xbf) = *(int *)(&DAT_00051c9c + iVar3) + iVar1;
          iVar3 = local_7c << 4;
          piStack_84 = (int *)0x2ee02;
          iVar1 = FUN_00041e9f();
          piStack_84 = (int *)0x2ee12;
          iVar1 = FUN_00041f76(iVar1 % extraout_ECX_02);
          *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(&DAT_00051ca0 + iVar3) + iVar1;
        }
        if (*(int *)((int)(char *)local_20 + 0xbf) < 0) {
          *(undefined4 *)((int)(char *)local_20 + 0xbf) = 0x14;
        }
        else if (0x13f < *(int *)((int)(char *)local_20 + 0xbf)) {
          *(undefined4 *)((int)(char *)local_20 + 0xbf) = 300;
        }
        piStack_84 = (int *)0x2ee62;
        iVar1 = FUN_0001519f((char *)local_20);
        piStack_84 = (int *)0x2ee74;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - iVar1);
        if (iVar1 < 10) {
          piStack_84 = (int *)0x2ee81;
          iVar1 = FUN_000151d5((char *)local_20);
          piStack_84 = (int *)0x2ee93;
          iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - iVar1);
          if (iVar1 < 5) {
            *(undefined4 *)((int)(char *)local_20 + 0xbf) = local_78;
            *(undefined4 *)((int)(char *)local_20 + 0xc3) = local_74;
            goto LAB_0002eec1;
          }
        }
        *(undefined4 *)((int)(char *)local_20 + 199) = 0;
      }
LAB_0002eec1:
      if (*(int *)((int)(char *)local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
        *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
      }
      if (*(int *)(DAT_00063354 + 0x16) <= *(int *)((int)(char *)local_20 + 0xc3)) {
        *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
      }
      piStack_84 = (int *)0x2ef15;
      iVar1 = FUN_0001519f((char *)local_20);
      if (iVar1 < *(int *)((int)(char *)local_20 + 0xbf)) {
        local_70 = *(int *)((int)(char *)local_20 + 0xbf);
        piStack_84 = (int *)0x2ef36;
        local_6c = FUN_0001519f((char *)local_20);
      }
      else {
        piStack_84 = (int *)0x2ef43;
        local_70 = FUN_0001519f((char *)local_20);
        local_6c = *(int *)((int)(char *)local_20 + 0xbf);
      }
      piStack_84 = (int *)0x2ef5a;
      iVar1 = FUN_000151d5((char *)local_20);
      if (iVar1 < *(int *)((int)(char *)local_20 + 0xc3)) {
        local_68 = *(int *)((int)(char *)local_20 + 0xc3);
        piStack_84 = (int *)0x2ef7b;
        local_64 = FUN_000151d5((char *)local_20);
      }
      else {
        piStack_84 = (int *)0x2ef88;
        local_68 = FUN_000151d5((char *)local_20);
        local_64 = *(int *)((int)(char *)local_20 + 0xc3);
      }
      piStack_84 = (int *)0x2ef9f;
      local_60 = FUN_0001519f((char *)local_20);
      piStack_84 = (int *)0x2efaa;
      local_5c = FUN_000151d5((char *)local_20);
      piStack_84 = (int *)0x2efbe;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_60);
      if (iVar1 < 5) {
        piStack_84 = (int *)0x2efd4;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_5c);
        if ((((5 < iVar1) && (local_44 < local_68)) && (local_64 < local_44)) &&
           ((local_60 + -8 < local_48 && (local_48 < local_60 + 8)))) {
          *(int *)((int)(char *)local_20 + 0xcb) = local_60;
          *(undefined4 *)((int)(char *)local_20 + 0xeb) =
               *(undefined4 *)((int)(char *)local_20 + 0xcb);
          *(int *)((int)(char *)local_20 + 0xcf) = local_5c;
          *(undefined4 *)((int)(char *)local_20 + 0xef) =
               *(undefined4 *)((int)(char *)local_20 + 0xcf);
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x14;
          *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
          return;
        }
      }
      piStack_84 = (int *)0x2f071;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_5c);
      if (iVar1 < 10) {
        piStack_84 = (int *)0x2f087;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_60);
        if (((5 < iVar1) && (local_44 < local_5c + 8)) &&
           ((local_5c + -8 < local_44 && ((local_6c < local_48 && (local_48 < local_70)))))) {
          *(int *)((int)(char *)local_20 + 0xcb) = local_60;
          *(undefined4 *)((int)(char *)local_20 + 0xeb) =
               *(undefined4 *)((int)(char *)local_20 + 0xcb);
          *(int *)((int)(char *)local_20 + 0xcf) = local_5c;
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
      if ((local_44 < local_68) && (local_64 < local_44)) {
        piStack_84 = (int *)0x2f202;
        iVar1 = FUN_0001519f((char *)local_20);
        if (local_48 <= iVar1 + -8) goto LAB_0002f21a;
        piStack_84 = (int *)0x2f212;
        iVar1 = FUN_0001519f((char *)local_20);
        if (iVar1 + 8 <= local_48) goto LAB_0002f21a;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      else {
LAB_0002f21a:
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      if ((((local_48 < local_70) && (local_6c < local_48)) &&
          (*(int *)((int)(char *)local_20 + 0xc3) + -8 < local_44)) &&
         (local_44 < *(int *)((int)(char *)local_20 + 0xc3) + 8)) {
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
    ppiVar4 = (int **)local_80;
  }
  else {
    ppiVar4 = (int **)local_80;
    if (*(int *)((int)(char *)local_20 + 0xaf) == 8) {
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 9;
      *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      ppiVar4 = (int **)local_80;
    }
  }
LAB_0002fdc6:
  if (local_3c != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x2fdd7;
    FUN_0001545e((char *)local_20,local_3c);
  }
  if (local_38 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x2fde8;
    FUN_0001586b((char *)local_20,local_38);
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x2fdf0;
  iVar1 = FUN_0001519f((char *)local_20);
  if (local_48 < iVar1) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 1;
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x2fe07;
  iVar1 = FUN_0001519f((char *)local_20);
  if (iVar1 < local_48) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 0;
  }
  return;
}


