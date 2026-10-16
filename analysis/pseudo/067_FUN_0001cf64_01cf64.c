// func #58  FUN_0001cf64  0001cf64  size=4040


void FUN_0001cf64(void)

{
  int iVar1;
  int **ppiVar2;
  float10 fVar3;
  undefined8 uVar4;
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
  undefined4 local_54;
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
  undefined4 local_28;
  undefined4 local_24;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  piStack_84 = (int *)0x1cf8b;
  local_48 = FUN_0001519f(DAT_00053c7c);
  piStack_84 = (int *)0x1cf98;
  local_44 = FUN_000151d5(DAT_00053c7c);
  piStack_84 = (int *)0x1cfa3;
  local_28 = FUN_0001519f((char *)local_20);
  piStack_84 = (int *)0x1cfae;
  local_24 = FUN_000151d5((char *)local_20);
  if ((char *)local_20 == (char *)DAT_00053c7c) {
    return;
  }
  if ((*(int *)((int)(char *)local_20 + 0x1f) < 10) ||
     (0x13 < *(int *)((int)(char *)local_20 + 0x1f))) {
    local_2c = *(int *)(&DAT_00053ea0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  else {
    local_2c = *(int *)(&DAT_00053f1c + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  if (*(int *)((int)(char *)local_20 + 0x6f) == 1) {
    local_2c = *(int *)(&DAT_00053f1c + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  fVar3 = (float10)*(float *)(&DAT_00051d5c + local_2c * 4);
  piStack_84 = (int *)0x1d027;
  FUN_00041f7e();
  local_40 = (int)ROUND(fVar3);
  fVar3 = (float10)*(float *)((int)(char *)local_20 + 99) +
          (float10)*(float *)(&DAT_00051d5c + local_2c * 4);
  piStack_84 = (int *)0x1d041;
  FUN_00041f7e();
  local_4c = (int)ROUND(fVar3);
  fVar3 = (float10)*(float *)((int)(char *)local_20 + 99);
  piStack_84 = (int *)0x1d04f;
  FUN_00041f7e();
  local_50 = (int)ROUND(fVar3);
  if (local_50 + local_40 < local_4c) {
    local_40 = local_40 + 1;
  }
  *(float *)((int)(char *)local_20 + 99) =
       *(float *)(&DAT_00051d5c + local_2c * 4) + *(float *)((int)(char *)local_20 + 99);
  local_3c = 0;
  local_38 = 0;
  if ((((*(int *)(DAT_00053c7c + 0x17) != 0x15) && (*(int *)(DAT_00053c7c + 0x17) != 0x16)) &&
      (*(int *)(DAT_00053c7c + 0x17) != 0x17)) &&
     (((*(int *)(DAT_00053c7c + 0x17) != 0x18 && (*(int *)(DAT_00053c7c + 0x17) != 0x1a)) &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x1d &&
       ((*(int *)(DAT_00053c7c + 0x17) != 0x1e && (*(int *)(DAT_00053c7c + 0x17) != 0x20)))))))) {
    piStack_84 = (int *)0x1d0f4;
    local_34 = FUN_00015a1f((char *)local_20);
    if (local_34 != 0) {
      *(int *)((int)(char *)local_20 + 0x10f) = local_34;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 10;
    }
  }
  if ((*(int *)((char *)local_20 + 0x17) != 2) && (*(int *)((char *)local_20 + 0x17) != 1)) {
    return;
  }
  if (*(char *)local_20 != '\x01') {
    return;
  }
  local_54 = *(undefined4 *)((char *)local_20 + 0x1f);
  switch(local_54) {
  case 0:
    if (*(int *)((char *)local_20 + 0x8b) == 0) {
      if (*(int *)(&DAT_00053eac + *(int *)((char *)local_20 + 0xd) * 0x88) == 0) {
        piStack_84 = (int *)0x1d187;
        uVar4 = FUN_0001519f((char *)local_20,(local_48 + -0x41) - local_30);
        if ((int)((ulonglong)uVar4 >> 0x20) <= (int)uVar4) {
          piStack_84 = (int *)0x1d19e;
          uVar4 = FUN_0001519f((char *)local_20,local_30 + local_48 + 0x41);
          if ((int)uVar4 <= (int)((ulonglong)uVar4 >> 0x20)) {
            piStack_84 = (int *)0x1d1c6;
            uVar4 = FUN_000151d5((char *)local_20,(local_44 + -10) - local_30 % 5);
            if ((int)((ulonglong)uVar4 >> 0x20) <= (int)uVar4) {
              piStack_84 = (int *)0x1d1ea;
              FUN_000151d5((char *)local_20,local_30 % 5 + local_44 + 10);
            }
          }
        }
      }
      *(undefined4 *)((int)(char *)local_20 + 0x8b) = 1;
      ppiVar2 = (int **)local_80;
      break;
    }
    if (*(int *)((char *)local_20 + 0xaf) != 4) {
      if (*(int *)((char *)local_20 + 0xaf) == 10) {
        *(int *)((char *)local_20 + 0xab) =
             *(int *)(&DAT_00053eb4 + *(int *)((char *)local_20 + 0xd) * 0x88) >> 1;
      }
      else {
        piStack_84 = (int *)0x1d23c;
        local_30 = FUN_00041e9f();
        local_30 = local_30 % 0x32;
        *(int *)((int)(char *)local_20 + 0xab) =
             *(int *)(&DAT_00053eb4 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) + local_30;
      }
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 4;
      piStack_84 = (int *)0x1d278;
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
        piStack_84 = (int *)0x1d2dd;
        iVar1 = FUN_0001519f((char *)local_20);
        if (iVar1 < 0) {
LAB_0001d2f0:
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 7;
          return;
        }
        piStack_84 = (int *)0x1d2e9;
        iVar1 = FUN_0001519f((char *)local_20);
        if (0x13f < iVar1) goto LAB_0001d2f0;
        *(int *)((int)(char *)local_20 + 0xab) =
             *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
        if (*(int *)((int)(char *)local_20 + 199) == 1) {
          if (local_20._4_4_ == 0) {
            local_7c = 0;
          }
          else if (local_20._4_4_ == 1) {
            local_7c = 1;
          }
          else if (local_20._4_4_ == 2) {
            do {
              piStack_84 = (int *)0x1d353;
              local_7c = FUN_00041e9f();
              local_7c = local_7c % 2;
            } while (local_7c == *(int *)((int)(char *)local_20 + 0xff));
            *(int *)((int)(char *)local_20 + 0xff) = local_7c;
          }
          if ((local_7c == 0) || (local_7c == 1)) {
            *(undefined4 *)((int)(char *)local_20 + 0x6f) = 1;
          }
          else {
            *(undefined4 *)((int)(char *)local_20 + 0x6f) = 0;
          }
          local_78 = *(undefined4 *)((int)(char *)local_20 + 0xbf);
          local_74 = *(undefined4 *)((int)(char *)local_20 + 0xc3);
          if (local_7c == 0) {
            *(uint *)((int)(char *)local_20 + 0xbf) =
                 local_48 -
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88];
            *(int *)((int)(char *)local_20 + 0xc3) = local_44;
          }
          else if (local_7c == 1) {
            *(uint *)((int)(char *)local_20 + 0xbf) =
                 local_48 +
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88];
            *(int *)((int)(char *)local_20 + 0xc3) = local_44;
          }
          if (*(int *)((int)(char *)local_20 + 0xbf) < 0) {
            *(undefined4 *)((int)(char *)local_20 + 0xbf) = 0x14;
          }
          else if (0x13f < *(int *)((int)(char *)local_20 + 0xbf)) {
            *(undefined4 *)((int)(char *)local_20 + 0xbf) = 300;
          }
          piStack_84 = (int *)0x1d475;
          iVar1 = FUN_0001519f((char *)local_20);
          piStack_84 = (int *)0x1d487;
          iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - iVar1);
          if (iVar1 < 10) {
            piStack_84 = (int *)0x1d494;
            iVar1 = FUN_000151d5((char *)local_20);
            piStack_84 = (int *)0x1d4a6;
            iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - iVar1);
            if (iVar1 < 5) {
              *(undefined4 *)((int)(char *)local_20 + 0xbf) = local_78;
              *(undefined4 *)((int)(char *)local_20 + 0xc3) = local_74;
              goto LAB_0001d4d4;
            }
          }
          *(undefined4 *)((int)(char *)local_20 + 199) = 0;
        }
LAB_0001d4d4:
        if (*(int *)((int)(char *)local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
          *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
        }
        if (*(int *)(DAT_00063354 + 0x16) <= *(int *)((int)(char *)local_20 + 0xc3)) {
          *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
        }
        piStack_84 = (int *)0x1d528;
        iVar1 = FUN_0001519f((char *)local_20);
        if (iVar1 < *(int *)((int)(char *)local_20 + 0xbf)) {
          local_70 = *(int *)((int)(char *)local_20 + 0xbf);
          piStack_84 = (int *)0x1d549;
          local_6c = FUN_0001519f((char *)local_20);
        }
        else {
          piStack_84 = (int *)0x1d556;
          local_70 = FUN_0001519f((char *)local_20);
          local_6c = *(int *)((int)(char *)local_20 + 0xbf);
        }
        piStack_84 = (int *)0x1d56d;
        iVar1 = FUN_000151d5((char *)local_20);
        if (iVar1 < *(int *)((int)(char *)local_20 + 0xc3)) {
          local_68 = *(int *)((int)(char *)local_20 + 0xc3);
          piStack_84 = (int *)0x1d58e;
          local_64 = FUN_000151d5((char *)local_20);
        }
        else {
          piStack_84 = (int *)0x1d59b;
          local_68 = FUN_000151d5((char *)local_20);
          local_64 = *(int *)((int)(char *)local_20 + 0xc3);
        }
        piStack_84 = (int *)0x1d5b2;
        local_60 = FUN_0001519f((char *)local_20);
        piStack_84 = (int *)0x1d5bd;
        local_5c = FUN_000151d5((char *)local_20);
        piStack_84 = (int *)0x1d5d1;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_60);
        if (iVar1 < 5) {
          piStack_84 = (int *)0x1d5e7;
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
        piStack_84 = (int *)0x1d684;
        iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_5c);
        if (iVar1 < 10) {
          piStack_84 = (int *)0x1d69a;
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
                     (*(int *)((int)(char *)local_20 + 0xc3) -
                     *(int *)((int)(char *)local_20 + 0xcf));
              }
              else {
                *(int *)((int)(char *)local_20 + 0xcb) =
                     *(int *)((int)(char *)local_20 + 0xcb) +
                     (*(int *)((int)(char *)local_20 + 0xc3) -
                     *(int *)((int)(char *)local_20 + 0xcf));
              }
            }
            else if (*(int *)((int)(char *)local_20 + 0xcf) < *(int *)((int)(char *)local_20 + 0xc3)
                    ) {
              *(int *)((int)(char *)local_20 + 0xcb) =
                   *(int *)((int)(char *)local_20 + 0xcb) +
                   (*(int *)((int)(char *)local_20 + 0xc3) - *(int *)((int)(char *)local_20 + 0xcf))
              ;
            }
            else {
              *(int *)((int)(char *)local_20 + 0xcb) =
                   *(int *)((int)(char *)local_20 + 0xcb) -
                   (*(int *)((int)(char *)local_20 + 0xc3) - *(int *)((int)(char *)local_20 + 0xcf))
              ;
            }
            *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x15;
            *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
            return;
          }
        }
        if ((local_44 < local_68) && (local_64 < local_44)) {
          piStack_84 = (int *)0x1d815;
          iVar1 = FUN_0001519f((char *)local_20);
          if (local_48 <= iVar1 + -8) goto LAB_0001d82d;
          piStack_84 = (int *)0x1d825;
          iVar1 = FUN_0001519f((char *)local_20);
          if (iVar1 + 8 <= local_48) goto LAB_0001d82d;
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 8;
          *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
        }
        else {
LAB_0001d82d:
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
      ppiVar2 = (int **)local_80;
    }
    else {
      ppiVar2 = (int **)local_80;
      if (*(int *)((int)(char *)local_20 + 0xaf) == 8) {
        *(int *)((int)(char *)local_20 + 0xab) =
             *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 9;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
        ppiVar2 = (int **)local_80;
      }
    }
    break;
  case 1:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_00017218((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 1;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      ppiVar2 = &piStack_88;
    }
    break;
  case 2:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_00016a60((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 2;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      ppiVar2 = &piStack_88;
    }
    break;
  case 3:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_00016b8f((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 3;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      ppiVar2 = &piStack_88;
    }
    break;
  case 4:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    local_58 = FUN_00016fec((char *)local_20,local_40,local_48,local_44);
    if (local_58 == 2) {
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 1;
      *(undefined4 *)((int)(char *)local_20 + 0xab) = 10;
      ppiVar2 = &piStack_88;
    }
    else if (local_58 == 1) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      ppiVar2 = &piStack_88;
    }
    else {
      ppiVar2 = &piStack_88;
      if (local_58 == 3) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xc;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 0x1e;
        ppiVar2 = &piStack_88;
      }
    }
    break;
  case 5:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_00016959((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 5;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      *(undefined4 *)((int)(char *)local_20 + 199) = 1;
      ppiVar2 = &piStack_88;
    }
    break;
  case 6:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_0001730b((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      ppiVar2 = &piStack_88;
    }
    break;
  case 7:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_000174a2((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      ppiVar2 = &piStack_88;
    }
    break;
  case 8:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_00017530((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 8;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      ppiVar2 = &piStack_88;
    }
    break;
  case 9:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_00017637((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 9;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      *(undefined4 *)((int)(char *)local_20 + 199) = 1;
      ppiVar2 = &piStack_88;
    }
    break;
  case 10:
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
    ppiVar2 = (int **)local_80;
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
      ppiVar2 = (int **)local_80;
    }
    break;
  case 0xb:
    if (*(int *)((char *)local_20 + 0xab) < 0) {
      piStack_84 = (int *)0x1dd54;
      iVar1 = FUN_0001519f((char *)local_20,
                           (&DAT_00053eb8)
                           [*(int *)((char *)local_20 + 0x113) * 10 +
                            *(int *)((char *)local_20 + 0xd) * 0x88]);
      piStack_84 = (int *)0x1dd5c;
      uVar4 = FUN_00041f76(iVar1 - local_48);
      if ((int)uVar4 < (int)((ulonglong)uVar4 >> 0x20)) {
        piStack_84 = (int *)0x1dd68;
        iVar1 = FUN_000151d5((char *)local_20);
        piStack_84 = (int *)0x1dd70;
        iVar1 = FUN_00041f76(iVar1 - local_44);
        if (iVar1 < 6) {
          if (((&DAT_00053eb8)
               [*(int *)((int)(char *)local_20 + 0x11b) +
                *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                *(int *)((int)(char *)local_20 + 0x113) * 10] == '\0') ||
             (9 < *(int *)((int)(char *)local_20 + 0x11b))) {
            *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
            *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
            ppiVar2 = (int **)local_80;
          }
          else {
            *(uint *)((int)(char *)local_20 + 0x17) =
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x11b) +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                              *(int *)((int)(char *)local_20 + 0x113) * 10];
            *(undefined4 *)((int)(char *)local_20 + 0x11) = 0;
            *(int *)((int)(char *)local_20 + 0x11b) = *(int *)((int)(char *)local_20 + 0x11b) + 1;
            ppiVar2 = (int **)local_80;
          }
          break;
        }
      }
      piStack_84 = (int *)0x1de1b;
      iVar1 = FUN_00041e9f();
      if (iVar1 % 0x14 == 2) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar2 = (int **)local_80;
      }
      else {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar2 = (int **)local_80;
      }
    }
    else {
      *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
      ((char *)local_20)[0x17] = '\x01';
      ((char *)local_20)[0x18] = '\0';
      ((char *)local_20)[0x19] = '\0';
      ((char *)local_20)[0x1a] = '\0';
      ppiVar2 = (int **)local_80;
    }
    break;
  case 0xc:
    *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
    ppiVar2 = (int **)local_80;
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
      ppiVar2 = (int **)local_80;
    }
    break;
  default:
    ppiVar2 = (int **)local_80;
    break;
  case 0x14:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_0001776f((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x14;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      *(undefined4 *)((int)(char *)local_20 + 199) = 1;
      ppiVar2 = &piStack_88;
    }
    break;
  case 0x15:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_00017927((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x15;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      *(undefined4 *)((int)(char *)local_20 + 199) = 1;
      ppiVar2 = &piStack_88;
    }
    break;
  case 0x16:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar1 = FUN_00017ca6((char *)local_20,local_40,local_48,local_44);
    ppiVar2 = &piStack_88;
    if (iVar1 != 0) {
      *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x16;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
      *(undefined4 *)((int)(char *)local_20 + 199) = 1;
      ppiVar2 = &piStack_88;
    }
  }
  if (local_3c != 0) {
    *(undefined4 *)((int)ppiVar2 + -4) = 0x1df44;
    FUN_0001545e((char *)local_20,local_3c);
  }
  if (local_38 != 0) {
    *(undefined4 *)((int)ppiVar2 + -4) = 0x1df55;
    FUN_0001586b((char *)local_20,local_38);
  }
  *(undefined4 *)((int)ppiVar2 + -4) = 0x1df5d;
  iVar1 = FUN_0001519f((char *)local_20);
  if (local_48 < iVar1) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 1;
  }
  *(undefined4 *)((int)ppiVar2 + -4) = 0x1df74;
  iVar1 = FUN_0001519f((char *)local_20);
  if (iVar1 < local_48) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 0;
  }
  return;
}


