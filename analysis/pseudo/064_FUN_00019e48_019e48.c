// func #55  FUN_00019e48  00019e48  size=4129


void FUN_00019e48(void)

{
  char *pcVar1;
  int iVar2;
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
  char *local_20;
  
  local_20 = (char *)FUN_000418ee();
  piStack_84 = (int *)0x19e6d;
  local_48 = FUN_0001519f(DAT_00053c7c);
  piStack_84 = (int *)0x19e7a;
  local_44 = FUN_000151d5(DAT_00053c7c);
  piStack_84 = (int *)0x19e85;
  local_28 = FUN_0001519f(local_20);
  piStack_84 = (int *)0x19e90;
  local_24 = FUN_000151d5(local_20);
  if (local_20 == DAT_00053c7c) {
    return;
  }
  if ((*(int *)(local_20 + 0x1f) < 10) || (0x13 < *(int *)(local_20 + 0x1f))) {
    local_2c = *(int *)(&DAT_00053ea0 + *(int *)(local_20 + 0xd) * 0x88);
  }
  else {
    local_2c = *(int *)(&DAT_00053f1c + *(int *)(local_20 + 0xd) * 0x88);
  }
  if (*(int *)(local_20 + 0x6f) == 1) {
    local_2c = *(int *)(&DAT_00053f1c + *(int *)(local_20 + 0xd) * 0x88);
  }
  fVar5 = (float10)*(float *)(&DAT_00051d5c + local_2c * 4);
  piStack_84 = (int *)0x19f09;
  FUN_00041f7e();
  local_40 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)(local_20 + 99) + (float10)*(float *)(&DAT_00051d5c + local_2c * 4);
  piStack_84 = (int *)0x19f23;
  FUN_00041f7e();
  local_4c = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)(local_20 + 99);
  piStack_84 = (int *)0x19f31;
  FUN_00041f7e();
  local_50 = (int)ROUND(fVar5);
  if (local_50 + local_40 < local_4c) {
    local_40 = local_40 + 1;
  }
  *(float *)(local_20 + 99) = *(float *)(&DAT_00051d5c + local_2c * 4) + *(float *)(local_20 + 99);
  local_3c = 0;
  local_38 = 0;
  if ((((*(int *)(DAT_00053c7c + 0x17) != 0x15) && (*(int *)(DAT_00053c7c + 0x17) != 0x16)) &&
      (*(int *)(DAT_00053c7c + 0x17) != 0x17)) &&
     (((*(int *)(DAT_00053c7c + 0x17) != 0x18 && (*(int *)(DAT_00053c7c + 0x17) != 0x1a)) &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x1d &&
       ((*(int *)(DAT_00053c7c + 0x17) != 0x1e && (*(int *)(DAT_00053c7c + 0x17) != 0x20)))))))) {
    piStack_84 = (int *)0x19fd6;
    local_34 = FUN_00015a1f(local_20);
    if (local_34 != 0) {
      *(int *)(local_20 + 0x10f) = local_34;
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
  local_54 = *(undefined4 *)(local_20 + 0x1f);
  switch(local_54) {
  case 0:
    if (*(int *)(local_20 + 0xaf) != 4) {
      if (*(int *)(local_20 + 0xaf) == 10) {
        *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb4 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
      }
      else {
        piStack_84 = (int *)0x1a06d;
        local_30 = FUN_00041e9f();
        local_30 = local_30 % 0x32;
        *(int *)(local_20 + 0xab) =
             *(int *)(&DAT_00053eb4 + *(int *)(local_20 + 0xd) * 0x88) + local_30;
      }
      local_20[0x1f] = '\x04';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      piStack_84 = (int *)0x1a0a9;
      iVar2 = FUN_00041e9f();
      pcVar1 = (char *)(iVar2 / 6);
      if (iVar2 % 6 == 1) {
        local_20[0xaf] = '\x04';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        pcVar1 = local_20;
      }
      piStack_84 = (int *)0x1a0ce;
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
        piStack_84 = (int *)0x1a133;
        iVar2 = FUN_0001519f(local_20);
        if (iVar2 < 0) {
LAB_0001a146:
          local_20[0x1f] = '\a';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          return;
        }
        piStack_84 = (int *)0x1a13f;
        iVar2 = FUN_0001519f(local_20);
        if (0x13f < iVar2) goto LAB_0001a146;
        *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb0 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
        if (*(int *)(local_20 + 199) == 1) {
          do {
            piStack_84 = (int *)0x1a185;
            iVar2 = FUN_00041e9f();
            local_7c = iVar2 % 10;
            if (local_7c == 2) {
              piStack_84 = (int *)0x1a1a1;
              iVar2 = FUN_00041e9f(iVar2 / 10);
              local_7c = local_7c + iVar2 % 2;
            }
            else if (local_7c == 3) {
              local_7c = 4;
              piStack_84 = (int *)0x1a1c6;
              iVar2 = FUN_00041e9f(iVar2 / 10);
              local_7c = local_7c + iVar2 % 2;
            }
          } while (local_7c == *(int *)(local_20 + 0xff));
          if ((local_7c == 0) || (local_7c == 1)) {
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
          *(int *)(local_20 + 0xff) = local_7c;
          local_78 = *(undefined4 *)(local_20 + 0xbf);
          local_74 = *(undefined4 *)(local_20 + 0xc3);
          if (local_7c < 6) {
            iVar3 = *(int *)(&DAT_00051c9c + local_7c * 0x10) + local_48;
            piStack_84 = (int *)0x1a263;
            iVar2 = FUN_00041e9f();
            piStack_84 = (int *)0x1a273;
            iVar2 = FUN_00041f76(iVar2 % extraout_ECX);
            *(int *)(local_20 + 0xbf) = iVar3 + iVar2;
            iVar3 = *(int *)(&DAT_00051ca8 + local_7c * 0x10) -
                    *(int *)(&DAT_00051ca0 + local_7c * 0x10);
            piStack_84 = (int *)0x1a2ac;
            iVar2 = FUN_00041e9f(local_7c * 0x10,iVar3,iVar3,
                                 *(int *)(&DAT_00051ca0 + local_7c * 0x10) + local_44);
            piStack_84 = (int *)0x1a2bc;
            iVar2 = FUN_00041f76(iVar2 % iVar3);
            *(int *)(local_20 + 0xc3) = extraout_ECX_00 + iVar2;
          }
          else {
            iVar3 = local_7c << 4;
            piStack_84 = (int *)0x1a2f3;
            iVar2 = FUN_00041e9f();
            piStack_84 = (int *)0x1a303;
            iVar2 = FUN_00041f76(iVar2 % extraout_ECX_01);
            *(int *)(local_20 + 0xbf) = *(int *)(&DAT_00051c9c + iVar3) + iVar2;
            iVar3 = local_7c << 4;
            piStack_84 = (int *)0x1a339;
            iVar2 = FUN_00041e9f();
            piStack_84 = (int *)0x1a349;
            iVar2 = FUN_00041f76(iVar2 % extraout_ECX_02);
            *(int *)(local_20 + 0xc3) = *(int *)(&DAT_00051ca0 + iVar3) + iVar2;
          }
          if ((local_7c == 0) || (local_7c == 1)) {
            *(int *)(local_20 + 0xc3) = local_44;
          }
          piStack_84 = (int *)0x1a37a;
          iVar2 = FUN_0001519f(local_20);
          piStack_84 = (int *)0x1a38c;
          iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - iVar2);
          if (iVar2 < 10) {
            piStack_84 = (int *)0x1a399;
            iVar2 = FUN_000151d5(local_20);
            piStack_84 = (int *)0x1a3ab;
            iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - iVar2);
            if (iVar2 < 5) {
              *(undefined4 *)(local_20 + 0xbf) = local_78;
              *(undefined4 *)(local_20 + 0xc3) = local_74;
              goto LAB_0001a3d9;
            }
          }
          local_20[199] = '\0';
          local_20[200] = '\0';
          local_20[0xc9] = '\0';
          local_20[0xca] = '\0';
        }
LAB_0001a3d9:
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
        if (*(int *)(local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
          *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
        }
        if (*(int *)(DAT_00063354 + 0x16) <= *(int *)(local_20 + 0xc3)) {
          *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
        }
        piStack_84 = (int *)0x1a464;
        iVar2 = FUN_0001519f(local_20);
        if (iVar2 < *(int *)(local_20 + 0xbf)) {
          local_70 = *(int *)(local_20 + 0xbf);
          piStack_84 = (int *)0x1a485;
          local_6c = FUN_0001519f(local_20);
        }
        else {
          piStack_84 = (int *)0x1a492;
          local_70 = FUN_0001519f(local_20);
          local_6c = *(int *)(local_20 + 0xbf);
        }
        piStack_84 = (int *)0x1a4a9;
        iVar2 = FUN_000151d5(local_20);
        if (iVar2 < *(int *)(local_20 + 0xc3)) {
          local_68 = *(int *)(local_20 + 0xc3);
          piStack_84 = (int *)0x1a4ca;
          local_64 = FUN_000151d5(local_20);
        }
        else {
          piStack_84 = (int *)0x1a4d7;
          local_68 = FUN_000151d5(local_20);
          local_64 = *(int *)(local_20 + 0xc3);
        }
        piStack_84 = (int *)0x1a4ee;
        local_60 = FUN_0001519f(local_20);
        piStack_84 = (int *)0x1a4f9;
        local_5c = FUN_000151d5(local_20);
        piStack_84 = (int *)0x1a50d;
        iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - local_60);
        if (iVar2 < 5) {
          piStack_84 = (int *)0x1a523;
          iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - local_5c);
          if ((((5 < iVar2) && (local_44 < local_68)) && (local_64 < local_44)) &&
             ((local_60 + -8 < local_48 && (local_48 < local_60 + 8)))) {
            *(int *)(local_20 + 0xcb) = local_60;
            *(undefined4 *)(local_20 + 0xeb) = *(undefined4 *)(local_20 + 0xcb);
            *(int *)(local_20 + 0xcf) = local_5c;
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
        piStack_84 = (int *)0x1a5c0;
        iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - local_5c);
        if (iVar2 < 10) {
          piStack_84 = (int *)0x1a5d6;
          iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - local_60);
          if (((5 < iVar2) && (local_44 < local_5c + 8)) &&
             ((local_5c + -8 < local_44 && ((local_6c < local_48 && (local_48 < local_70)))))) {
            *(int *)(local_20 + 0xcb) = local_60;
            *(undefined4 *)(local_20 + 0xeb) = *(undefined4 *)(local_20 + 0xcb);
            *(int *)(local_20 + 0xcf) = local_5c;
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
        if ((local_44 < local_68) && (local_64 < local_44)) {
          piStack_84 = (int *)0x1a751;
          iVar2 = FUN_0001519f(local_20);
          if (local_48 <= iVar2 + -8) goto LAB_0001a769;
          piStack_84 = (int *)0x1a761;
          iVar2 = FUN_0001519f(local_20);
          if (iVar2 + 8 <= local_48) goto LAB_0001a769;
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
LAB_0001a769:
          local_20[0x1f] = '\x02';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          local_20[0x8b] = '\0';
          local_20[0x8c] = '\0';
          local_20[0x8d] = '\0';
          local_20[0x8e] = '\0';
        }
        if ((((local_48 < local_70) && (local_6c < local_48)) &&
            (*(int *)(local_20 + 0xc3) + -8 < local_44)) &&
           (local_44 < *(int *)(local_20 + 0xc3) + 8)) {
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
      ppiVar4 = (int **)local_80;
    }
    else {
      ppiVar4 = (int **)local_80;
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
        ppiVar4 = (int **)local_80;
      }
    }
    break;
  case 1:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017218(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x01';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_88;
    }
    break;
  case 2:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00016a60(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x02';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_88;
    }
    break;
  case 3:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00016b8f(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x03';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_88;
    }
    break;
  case 4:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    local_58 = FUN_00016fec(local_20,local_40,local_48,local_44);
    if (local_58 == 2) {
      local_20[0x1f] = '\x01';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      local_20[0xab] = '\n';
      local_20[0xac] = '\0';
      local_20[0xad] = '\0';
      local_20[0xae] = '\0';
      ppiVar4 = &piStack_88;
    }
    else if (local_58 == 1) {
      local_20[0xaf] = '\x04';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_88;
    }
    else {
      ppiVar4 = &piStack_88;
      if (local_58 == 3) {
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
        ppiVar4 = &piStack_88;
      }
    }
    break;
  case 5:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00016959(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
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
      ppiVar4 = &piStack_88;
    }
    break;
  case 6:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_0001730b(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x06';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_88;
    }
    break;
  case 7:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_000174a2(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x04';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_88;
    }
    break;
  case 8:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017530(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\b';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar4 = &piStack_88;
    }
    break;
  case 9:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017637(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
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
      ppiVar4 = &piStack_88;
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
    ppiVar4 = (int **)local_80;
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
      ppiVar4 = (int **)local_80;
    }
    break;
  case 0xb:
    if (*(int *)(local_20 + 0xab) < 0) {
      piStack_84 = (int *)0x1ac90;
      iVar2 = FUN_0001519f(local_20,(&DAT_00053eb8)
                                    [*(int *)(local_20 + 0x113) * 10 +
                                     *(int *)(local_20 + 0xd) * 0x88]);
      piStack_84 = (int *)0x1ac98;
      uVar6 = FUN_00041f76(iVar2 - local_48);
      if ((int)uVar6 < (int)((ulonglong)uVar6 >> 0x20)) {
        piStack_84 = (int *)0x1aca4;
        iVar2 = FUN_000151d5(local_20);
        piStack_84 = (int *)0x1acac;
        iVar2 = FUN_00041f76(iVar2 - local_44);
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
            ppiVar4 = (int **)local_80;
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
            ppiVar4 = (int **)local_80;
          }
          break;
        }
      }
      piStack_84 = (int *)0x1ad57;
      iVar2 = FUN_00041e9f();
      if (iVar2 % 0x14 == 2) {
        local_20[0xaf] = '\x04';
        local_20[0xb0] = '\0';
        local_20[0xb1] = '\0';
        local_20[0xb2] = '\0';
        local_20[0x1f] = '\0';
        local_20[0x20] = '\0';
        local_20[0x21] = '\0';
        local_20[0x22] = '\0';
        ppiVar4 = (int **)local_80;
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
        ppiVar4 = (int **)local_80;
      }
    }
    else {
      *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
      local_20[0x17] = '\x01';
      local_20[0x18] = '\0';
      local_20[0x19] = '\0';
      local_20[0x1a] = '\0';
      ppiVar4 = (int **)local_80;
    }
    break;
  case 0xc:
    *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
    ppiVar4 = (int **)local_80;
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
      ppiVar4 = (int **)local_80;
    }
    break;
  default:
    ppiVar4 = (int **)local_80;
    break;
  case 0x14:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_0001776f(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
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
      ppiVar4 = &piStack_88;
    }
    break;
  case 0x15:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017927(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
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
      ppiVar4 = &piStack_88;
    }
    break;
  case 0x16:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017ca6(local_20,local_40,local_48,local_44);
    ppiVar4 = &piStack_88;
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
      ppiVar4 = &piStack_88;
    }
  }
  if (local_3c != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x1ae80;
    FUN_0001545e(local_20,local_3c);
  }
  if (local_38 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x1ae91;
    FUN_0001586b(local_20,local_38);
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x1ae99;
  iVar2 = FUN_0001519f(local_20);
  if (local_48 < iVar2) {
    local_20[0x1b] = '\x01';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x1aeb0;
  iVar2 = FUN_0001519f(local_20);
  if (iVar2 < local_48) {
    local_20[0x1b] = '\0';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  return;
}


