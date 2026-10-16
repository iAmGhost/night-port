// func #56  FUN_0001aec8  0001aec8  size=3969


void FUN_0001aec8(void)

{
  char *pcVar1;
  int iVar2;
  int iVar3;
  int **ppiVar4;
  float10 fVar5;
  undefined8 uVar6;
  int *piStack_80;
  int *piStack_7c;
  undefined1 local_78 [4];
  int local_74;
  undefined4 local_70;
  undefined4 local_6c;
  int local_68;
  int local_64;
  int local_60;
  int local_5c;
  int local_58;
  int local_54;
  int local_50;
  undefined4 local_4c;
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
  char *local_20;
  
  local_20 = (char *)FUN_000418ee();
  piStack_7c = (int *)0x1aeed;
  local_40 = FUN_0001519f(DAT_00053c7c);
  piStack_7c = (int *)0x1aefa;
  local_3c = FUN_000151d5(DAT_00053c7c);
  if (local_20 == DAT_00053c7c) {
    return;
  }
  if ((*(int *)(local_20 + 0x1f) < 10) || (0x13 < *(int *)(local_20 + 0x1f))) {
    local_28 = *(int *)(&DAT_00053ea0 + *(int *)(local_20 + 0xd) * 0x88);
  }
  else {
    local_28 = *(int *)(&DAT_00053f1c + *(int *)(local_20 + 0xd) * 0x88);
  }
  if (*(int *)(local_20 + 0x6f) == 1) {
    local_28 = *(int *)(&DAT_00053f1c + *(int *)(local_20 + 0xd) * 0x88);
  }
  fVar5 = (float10)*(float *)(&DAT_00051d5c + local_28 * 4);
  piStack_7c = (int *)0x1af73;
  FUN_00041f7e();
  local_38 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)(local_20 + 99) + (float10)*(float *)(&DAT_00051d5c + local_28 * 4);
  piStack_7c = (int *)0x1af8d;
  FUN_00041f7e();
  local_44 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)(local_20 + 99);
  piStack_7c = (int *)0x1af9b;
  FUN_00041f7e();
  local_48 = (int)ROUND(fVar5);
  if (local_48 + local_38 < local_44) {
    local_38 = local_38 + 1;
  }
  *(float *)(local_20 + 99) = *(float *)(&DAT_00051d5c + local_28 * 4) + *(float *)(local_20 + 99);
  local_34 = 0;
  local_30 = 0;
  if ((((*(int *)(DAT_00053c7c + 0x17) != 0x15) && (*(int *)(DAT_00053c7c + 0x17) != 0x16)) &&
      (*(int *)(DAT_00053c7c + 0x17) != 0x17)) &&
     (((*(int *)(DAT_00053c7c + 0x17) != 0x18 && (*(int *)(DAT_00053c7c + 0x17) != 0x1a)) &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x1d &&
       ((*(int *)(DAT_00053c7c + 0x17) != 0x1e && (*(int *)(DAT_00053c7c + 0x17) != 0x20)))))))) {
    piStack_7c = (int *)0x1b040;
    local_2c = FUN_00015a1f(local_20);
    if (local_2c != 0) {
      *(int *)(local_20 + 0x10f) = local_2c;
      local_20[0x1f] = '\n';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
    }
  }
  if ((*(int *)(local_20 + 0x17) != 2) && (*(int *)(local_20 + 0x17) != 1)) {
    return;
  }
  if (*local_20 != '\x01') {
    return;
  }
  local_4c = *(undefined4 *)(local_20 + 0x1f);
  switch(local_4c) {
  case 0:
    if (*(int *)(local_20 + 0xaf) != 4) {
      if (*(int *)(local_20 + 0xaf) == 10) {
        *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb4 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
      }
      else {
        piStack_7c = (int *)0x1b0d7;
        local_24 = FUN_00041e9f();
        local_24 = local_24 % 0x32;
        *(int *)(local_20 + 0xab) =
             *(int *)(&DAT_00053eb4 + *(int *)(local_20 + 0xd) * 0x88) + local_24;
      }
      local_20[0x1f] = '\x04';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      piStack_7c = (int *)0x1b113;
      iVar2 = FUN_00041e9f();
      pcVar1 = (char *)(iVar2 / 6);
      if (iVar2 % 6 == 1) {
        local_20[0xaf] = '\x04';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        pcVar1 = local_20;
      }
      piStack_7c = (int *)0x1b138;
      iVar2 = FUN_00041e9f(pcVar1);
      *(int *)(local_20 + 0x113) = iVar2 % 10;
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
        piStack_7c = (int *)0x1b19d;
        iVar2 = FUN_0001519f(local_20);
        if (iVar2 < 0) {
LAB_0001b1b0:
          local_20[0x1f] = '\a';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          return;
        }
        piStack_7c = (int *)0x1b1a9;
        iVar2 = FUN_0001519f(local_20);
        if (0x13f < iVar2) goto LAB_0001b1b0;
        *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb0 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
        if (*(int *)(local_20 + 199) == 1) {
          local_70 = *(undefined4 *)(local_20 + 0xbf);
          local_6c = *(undefined4 *)(local_20 + 0xc3);
          if (*(int *)(local_20 + 0x5f) == 0) {
            piStack_7c = (int *)0x1b2bb;
            local_74 = FUN_00041e9f();
            local_74 = local_74 % 2;
            iVar2 = *(int *)(&DAT_00051ca4 + local_74 * 0x10) -
                    *(int *)(&DAT_00051c9c + local_74 * 0x10);
            piStack_7c = (int *)0x1b2eb;
            iVar3 = FUN_00041e9f();
            *(int *)(local_20 + 0xbf) =
                 *(int *)(&DAT_00051c9c + local_74 * 0x10) + local_40 + iVar3 % iVar2;
            *(int *)(local_20 + 0xc3) = local_3c;
            local_20[0x5f] = '\x01';
            local_20[0x60] = '\0';
            local_20[0x61] = '\0';
            local_20[0x62] = '\0';
          }
          else {
            if (local_40 < 0xa1) {
              local_74 = 0xb;
            }
            else {
              local_74 = 10;
            }
            iVar3 = *(int *)(&DAT_00051ca4 + local_74 * 0x10) -
                    *(int *)(&DAT_00051c9c + local_74 * 0x10);
            piStack_7c = (int *)0x1b247;
            iVar2 = FUN_00041e9f();
            *(int *)(local_20 + 0xbf) = *(int *)(&DAT_00051c9c + local_74 * 0x10) + iVar2 % iVar3;
            iVar3 = *(int *)(&DAT_00051ca8 + local_74 * 0x10) -
                    *(int *)(&DAT_00051ca0 + local_74 * 0x10);
            piStack_7c = (int *)0x1b288;
            iVar2 = FUN_00041e9f();
            *(int *)(local_20 + 0xc3) = *(int *)(&DAT_00051ca0 + local_74 * 0x10) + iVar2 % iVar3;
            local_20[0x5f] = '\0';
            local_20[0x60] = '\0';
            local_20[0x61] = '\0';
            local_20[0x62] = '\0';
          }
          if ((local_74 == 0) || (local_74 == 1)) {
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
          *(int *)(local_20 + 0xff) = local_74;
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
          piStack_7c = (int *)0x1b391;
          iVar2 = FUN_0001519f(local_20);
          piStack_7c = (int *)0x1b3a3;
          iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - iVar2);
          if (iVar2 < 10) {
            piStack_7c = (int *)0x1b3b0;
            iVar2 = FUN_000151d5(local_20);
            piStack_7c = (int *)0x1b3c2;
            iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - iVar2);
            if (iVar2 < 5) {
              *(undefined4 *)(local_20 + 0xbf) = local_70;
              *(undefined4 *)(local_20 + 0xc3) = local_6c;
              goto LAB_0001b3f0;
            }
          }
          local_20[199] = '\0';
          local_20[200] = '\0';
          local_20[0xc9] = '\0';
          local_20[0xca] = '\0';
        }
LAB_0001b3f0:
        if (*(int *)(local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
          *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
        }
        if (*(int *)(DAT_00063354 + 0x16) <= *(int *)(local_20 + 0xc3)) {
          *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
        }
        piStack_7c = (int *)0x1b444;
        iVar2 = FUN_0001519f(local_20);
        if (iVar2 < *(int *)(local_20 + 0xbf)) {
          local_68 = *(int *)(local_20 + 0xbf);
          piStack_7c = (int *)0x1b465;
          local_64 = FUN_0001519f(local_20);
        }
        else {
          piStack_7c = (int *)0x1b472;
          local_68 = FUN_0001519f(local_20);
          local_64 = *(int *)(local_20 + 0xbf);
        }
        piStack_7c = (int *)0x1b489;
        iVar2 = FUN_000151d5(local_20);
        if (iVar2 < *(int *)(local_20 + 0xc3)) {
          local_60 = *(int *)(local_20 + 0xc3);
          piStack_7c = (int *)0x1b4aa;
          local_5c = FUN_000151d5(local_20);
        }
        else {
          piStack_7c = (int *)0x1b4b7;
          local_60 = FUN_000151d5(local_20);
          local_5c = *(int *)(local_20 + 0xc3);
        }
        piStack_7c = (int *)0x1b4ce;
        local_58 = FUN_0001519f(local_20);
        piStack_7c = (int *)0x1b4d9;
        local_54 = FUN_000151d5(local_20);
        piStack_7c = (int *)0x1b4ed;
        iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - local_58);
        if (iVar2 < 5) {
          piStack_7c = (int *)0x1b503;
          iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - local_54);
          if ((((5 < iVar2) && (local_3c < local_60)) && (local_5c < local_3c)) &&
             ((local_58 + -8 < local_40 && (local_40 < local_58 + 8)))) {
            *(int *)(local_20 + 0xcb) = local_58;
            *(undefined4 *)(local_20 + 0xeb) = *(undefined4 *)(local_20 + 0xcb);
            *(int *)(local_20 + 0xcf) = local_54;
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
        piStack_7c = (int *)0x1b5a0;
        iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - local_54);
        if (iVar2 < 10) {
          piStack_7c = (int *)0x1b5b6;
          iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - local_58);
          if (((5 < iVar2) && (local_3c < local_54 + 8)) &&
             ((local_54 + -8 < local_3c && ((local_64 < local_40 && (local_40 < local_68)))))) {
            *(int *)(local_20 + 0xcb) = local_58;
            *(undefined4 *)(local_20 + 0xeb) = *(undefined4 *)(local_20 + 0xcb);
            *(int *)(local_20 + 0xcf) = local_54;
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
                   *(int *)(local_20 + 0xcb) +
                   (*(int *)(local_20 + 0xc3) - *(int *)(local_20 + 0xcf));
            }
            else {
              *(int *)(local_20 + 0xcb) =
                   *(int *)(local_20 + 0xcb) -
                   (*(int *)(local_20 + 0xc3) - *(int *)(local_20 + 0xcf));
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
        if ((local_3c < local_60) && (local_5c < local_3c)) {
          piStack_7c = (int *)0x1b731;
          iVar2 = FUN_0001519f(local_20);
          if (local_40 <= iVar2 + -8) goto LAB_0001b749;
          piStack_7c = (int *)0x1b741;
          iVar2 = FUN_0001519f(local_20);
          if (iVar2 + 8 <= local_40) goto LAB_0001b749;
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
LAB_0001b749:
          local_20[0x1f] = '\x02';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          local_20[0x8b] = '\0';
          local_20[0x8c] = '\0';
          local_20[0x8d] = '\0';
          local_20[0x8e] = '\0';
        }
        if ((((local_40 < local_68) && (local_64 < local_40)) &&
            (*(int *)(local_20 + 0xc3) + -8 < local_3c)) &&
           (local_3c < *(int *)(local_20 + 0xc3) + 8)) {
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
      ppiVar4 = (int **)local_78;
    }
    else {
      ppiVar4 = (int **)local_78;
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
        ppiVar4 = (int **)local_78;
      }
    }
    break;
  case 1:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00017218(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x01';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_80;
    }
    break;
  case 2:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00016a60(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x02';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_80;
    }
    break;
  case 3:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00016b8f(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x03';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_80;
    }
    break;
  case 4:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    local_50 = FUN_00016fec(local_20,local_38,local_40,local_3c);
    if (local_50 == 2) {
      local_20[0x1f] = '\x01';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      local_20[0xab] = '\n';
      local_20[0xac] = '\0';
      local_20[0xad] = '\0';
      local_20[0xae] = '\0';
      ppiVar4 = &piStack_80;
    }
    else if (local_50 == 1) {
      local_20[0xaf] = '\x04';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_80;
    }
    else {
      ppiVar4 = &piStack_80;
      if (local_50 == 3) {
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
        ppiVar4 = &piStack_80;
      }
    }
    break;
  case 5:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00016959(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
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
      ppiVar4 = &piStack_80;
    }
    break;
  case 6:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_0001730b(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x06';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_80;
    }
    break;
  case 7:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_000174a2(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x04';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_80;
    }
    break;
  case 8:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00017530(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\b';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_80;
    }
    break;
  case 9:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00017637(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
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
      ppiVar4 = &piStack_80;
    }
    break;
  case 10:
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
    ppiVar4 = (int **)local_78;
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
      *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053f20 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
      ppiVar4 = (int **)local_78;
    }
    break;
  case 0xb:
    if (*(int *)(local_20 + 0xab) < 0) {
      piStack_7c = (int *)0x1bc70;
      iVar2 = FUN_0001519f(local_20,(&DAT_00053eb8)
                                    [*(int *)(local_20 + 0x113) * 10 +
                                     *(int *)(local_20 + 0xd) * 0x88]);
      piStack_7c = (int *)0x1bc78;
      uVar6 = FUN_00041f76(iVar2 - local_40);
      if ((int)uVar6 < (int)((ulonglong)uVar6 >> 0x20)) {
        piStack_7c = (int *)0x1bc84;
        iVar2 = FUN_000151d5(local_20);
        piStack_7c = (int *)0x1bc8c;
        iVar2 = FUN_00041f76(iVar2 - local_3c);
        if (iVar2 < 6) {
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
            ppiVar4 = (int **)local_78;
          }
          else {
            *(uint *)(local_20 + 0x17) =
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)(local_20 + 0x11b) +
                              *(int *)(local_20 + 0xd) * 0x88 + *(int *)(local_20 + 0x113) * 10];
            local_20[0x11] = '\0';
            local_20[0x12] = '\0';
            local_20[0x13] = '\0';
            local_20[0x14] = '\0';
            *(int *)(local_20 + 0x11b) = *(int *)(local_20 + 0x11b) + 1;
            ppiVar4 = (int **)local_78;
          }
          break;
        }
      }
      piStack_7c = (int *)0x1bd37;
      iVar2 = FUN_00041e9f();
      if (iVar2 % 0x28 == 2) {
        local_20[0xaf] = '\x04';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        ppiVar4 = (int **)local_78;
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
        ppiVar4 = (int **)local_78;
      }
    }
    else {
      *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
      local_20[0x17] = '\x01';
      local_20[0x18] = '\0';
      local_20[0x19] = '\0';
      local_20[0x1a] = '\0';
      ppiVar4 = (int **)local_78;
    }
    break;
  case 0xc:
    *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
    ppiVar4 = (int **)local_78;
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
      ppiVar4 = (int **)local_78;
    }
    break;
  default:
    ppiVar4 = (int **)local_78;
    break;
  case 0x14:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_0001776f(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
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
      ppiVar4 = &piStack_80;
    }
    break;
  case 0x15:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00017927(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
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
      ppiVar4 = &piStack_80;
    }
    break;
  case 0x16:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00017ca6(local_20,local_38,local_40,local_3c);
    ppiVar4 = &piStack_80;
    if (iVar2 != 0) {
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
      ppiVar4 = &piStack_80;
    }
  }
  if (local_34 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x1be60;
    FUN_0001545e(local_20,local_34);
  }
  if (local_30 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x1be71;
    FUN_0001586b(local_20,local_30);
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x1be79;
  iVar2 = FUN_0001519f(local_20);
  if (local_40 < iVar2) {
    local_20[0x1b] = '\x01';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x1be90;
  iVar2 = FUN_0001519f(local_20);
  if (iVar2 < local_40) {
    local_20[0x1b] = '\0';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  return;
}


