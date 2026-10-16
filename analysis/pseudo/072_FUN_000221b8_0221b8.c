// func #63  FUN_000221b8  000221b8  size=3974


void FUN_000221b8(void)

{
  int iVar1;
  int iVar2;
  int extraout_ECX;
  int **ppiVar3;
  float10 fVar4;
  undefined8 uVar5;
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
  piStack_7c = (int *)0x221dd;
  local_40 = FUN_0001519f(DAT_00053c7c);
  piStack_7c = (int *)0x221ea;
  local_3c = FUN_000151d5(DAT_00053c7c);
  if (local_20 == DAT_00053c7c) {
    return;
  }
  if ((*(int *)(local_20 + 0x1f) < 10) || (0x13 < *(int *)(local_20 + 0x1f))) {
    local_24 = *(int *)(&DAT_00053ea0 + *(int *)(local_20 + 0xd) * 0x88);
  }
  else {
    local_24 = *(int *)(&DAT_00053f1c + *(int *)(local_20 + 0xd) * 0x88);
  }
  if (*(int *)(local_20 + 0x6f) == 1) {
    local_24 = *(int *)(&DAT_00053f1c + *(int *)(local_20 + 0xd) * 0x88);
  }
  fVar4 = (float10)*(float *)(&DAT_00051d5c + local_24 * 4);
  piStack_7c = (int *)0x22263;
  FUN_00041f7e();
  local_38 = (int)ROUND(fVar4);
  fVar4 = (float10)*(float *)(local_20 + 99) + (float10)*(float *)(&DAT_00051d5c + local_24 * 4);
  piStack_7c = (int *)0x2227d;
  FUN_00041f7e();
  local_44 = (int)ROUND(fVar4);
  fVar4 = (float10)*(float *)(local_20 + 99);
  piStack_7c = (int *)0x2228b;
  FUN_00041f7e();
  local_48 = (int)ROUND(fVar4);
  if (local_48 + local_38 < local_44) {
    local_38 = local_38 + 1;
  }
  *(float *)(local_20 + 99) = *(float *)(&DAT_00051d5c + local_24 * 4) + *(float *)(local_20 + 99);
  local_34 = 0;
  local_30 = 0;
  if ((((*(int *)(DAT_00053c7c + 0x17) != 0x15) && (*(int *)(DAT_00053c7c + 0x17) != 0x16)) &&
      (*(int *)(DAT_00053c7c + 0x17) != 0x17)) &&
     (((*(int *)(DAT_00053c7c + 0x17) != 0x18 && (*(int *)(DAT_00053c7c + 0x17) != 0x1a)) &&
      ((*(int *)(DAT_00053c7c + 0x17) != 0x1d &&
       ((*(int *)(DAT_00053c7c + 0x17) != 0x1e && (*(int *)(DAT_00053c7c + 0x17) != 0x20)))))))) {
    piStack_7c = (int *)0x22330;
    local_2c = FUN_00015ed1(local_20);
    if (local_2c != 0) {
      *(int *)(local_20 + 0x10f) = local_2c;
      local_20[0x1f] = '\n';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
    }
  }
  if ((((0x14 < *(uint *)(local_20 + 0x17)) && (*(uint *)(local_20 + 0x17) < 0x19)) ||
      (*(int *)(local_20 + 0x17) == 0x1a)) ||
     ((*(int *)(local_20 + 0x17) == 0x1d || (*(int *)(local_20 + 0x17) == 0x1e)))) {
    *(int *)(local_20 + 0x73) = (int)*(short *)(local_20 + 0x23);
  }
  if (0x32 < *(int *)(local_20 + 0x73) - (int)*(short *)(local_20 + 0x23)) {
    local_20[0x17] = '0';
    local_20[0x18] = '\0';
    local_20[0x19] = '\0';
    local_20[0x1a] = '\0';
    local_20[0x11] = '\0';
    local_20[0x12] = '\0';
    local_20[0x13] = '\0';
    local_20[0x14] = '\0';
    *(int *)(local_20 + 0x73) = (int)*(short *)(local_20 + 0x23);
    return;
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
        piStack_7c = (int *)0x2243f;
        local_28 = FUN_00041e9f();
        local_28 = local_28 % 0x32;
        *(int *)(local_20 + 0xab) =
             *(int *)(&DAT_00053eb4 + *(int *)(local_20 + 0xd) * 0x88) + local_28;
      }
      local_20[0x1f] = '\x04';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      piStack_7c = (int *)0x2247b;
      iVar2 = FUN_00041e9f();
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
        piStack_7c = (int *)0x224e0;
        iVar2 = FUN_0001519f(local_20);
        if (iVar2 < 0) {
LAB_000224f3:
          local_20[0x1f] = '\a';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          return;
        }
        piStack_7c = (int *)0x224ec;
        iVar2 = FUN_0001519f(local_20);
        if (0x13f < iVar2) goto LAB_000224f3;
        *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb0 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
        if (*(int *)(local_20 + 199) == 1) {
          do {
            piStack_7c = (int *)0x22532;
            iVar2 = FUN_00041e9f();
            local_74 = iVar2 % 4;
            if (local_74 == 2) {
              piStack_7c = (int *)0x2254e;
              iVar2 = FUN_00041e9f(iVar2 / 4);
              local_74 = local_74 + iVar2 % 2;
            }
            else if (local_74 == 3) {
              local_74 = 4;
              piStack_7c = (int *)0x22573;
              iVar2 = FUN_00041e9f(iVar2 / 4);
              local_74 = local_74 + iVar2 % 2;
            }
          } while (local_74 == *(int *)(local_20 + 0xff));
          if ((local_74 == 0) || (local_74 == 1)) {
            *(int *)(local_20 + 0xc3) = local_3c;
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
          local_70 = *(undefined4 *)(local_20 + 0xbf);
          local_6c = *(undefined4 *)(local_20 + 0xc3);
          iVar2 = *(int *)(&DAT_00051ca4 + local_74 * 0x10) -
                  *(int *)(&DAT_00051c9c + local_74 * 0x10);
          piStack_7c = (int *)0x2261e;
          iVar1 = FUN_00041e9f(iVar2,local_74 * 0x10,iVar2,
                               *(int *)(&DAT_00051c9c + local_74 * 0x10) + local_40);
          piStack_7c = (int *)0x2262e;
          iVar2 = FUN_00041f76(iVar1 % iVar2);
          *(int *)(local_20 + 0xbf) = extraout_ECX + iVar2;
          *(int *)(local_20 + 0xc3) = local_3c;
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
          piStack_7c = (int *)0x22686;
          iVar2 = FUN_0001519f(local_20);
          piStack_7c = (int *)0x22698;
          iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - iVar2);
          if (iVar2 < 10) {
            piStack_7c = (int *)0x226a5;
            iVar2 = FUN_000151d5(local_20);
            piStack_7c = (int *)0x226b7;
            iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - iVar2);
            if (iVar2 < 5) {
              *(undefined4 *)(local_20 + 0xbf) = local_70;
              *(undefined4 *)(local_20 + 0xc3) = local_6c;
              goto LAB_000226e5;
            }
          }
          local_20[199] = '\0';
          local_20[200] = '\0';
          local_20[0xc9] = '\0';
          local_20[0xca] = '\0';
        }
LAB_000226e5:
        if (*(int *)(local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
          *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
        }
        if (*(int *)(DAT_00063354 + 0x16) <= *(int *)(local_20 + 0xc3)) {
          *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
        }
        piStack_7c = (int *)0x22739;
        iVar2 = FUN_0001519f(local_20);
        if (iVar2 < *(int *)(local_20 + 0xbf)) {
          local_68 = *(int *)(local_20 + 0xbf);
          piStack_7c = (int *)0x2275a;
          local_64 = FUN_0001519f(local_20);
        }
        else {
          piStack_7c = (int *)0x22767;
          local_68 = FUN_0001519f(local_20);
          local_64 = *(int *)(local_20 + 0xbf);
        }
        piStack_7c = (int *)0x2277e;
        iVar2 = FUN_000151d5(local_20);
        if (iVar2 < *(int *)(local_20 + 0xc3)) {
          local_60 = *(int *)(local_20 + 0xc3);
          piStack_7c = (int *)0x2279f;
          local_5c = FUN_000151d5(local_20);
        }
        else {
          piStack_7c = (int *)0x227ac;
          local_60 = FUN_000151d5(local_20);
          local_5c = *(int *)(local_20 + 0xc3);
        }
        piStack_7c = (int *)0x227c3;
        local_58 = FUN_0001519f(local_20);
        piStack_7c = (int *)0x227ce;
        local_54 = FUN_000151d5(local_20);
        piStack_7c = (int *)0x227e2;
        iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - local_58);
        if (iVar2 < 5) {
          piStack_7c = (int *)0x227f8;
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
        piStack_7c = (int *)0x22895;
        iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - local_54);
        if (iVar2 < 10) {
          piStack_7c = (int *)0x228ab;
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
          piStack_7c = (int *)0x22a26;
          iVar2 = FUN_0001519f(local_20);
          if (local_40 <= iVar2 + -8) goto LAB_00022a3e;
          piStack_7c = (int *)0x22a36;
          iVar2 = FUN_0001519f(local_20);
          if (iVar2 + 8 <= local_40) goto LAB_00022a3e;
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
LAB_00022a3e:
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
      ppiVar3 = (int **)local_78;
    }
    else {
      ppiVar3 = (int **)local_78;
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
        ppiVar3 = (int **)local_78;
      }
    }
    break;
  case 1:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00017218(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x01';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_80;
    }
    break;
  case 2:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00016a60(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x02';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_80;
    }
    break;
  case 3:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00016b8f(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x03';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_80;
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
      ppiVar3 = &piStack_80;
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
      ppiVar3 = &piStack_80;
    }
    else {
      ppiVar3 = &piStack_80;
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
        ppiVar3 = &piStack_80;
      }
    }
    break;
  case 5:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00016959(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
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
      ppiVar3 = &piStack_80;
    }
    break;
  case 6:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_0001730b(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x06';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_80;
    }
    break;
  case 7:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_000174a2(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x04';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_80;
    }
    break;
  case 8:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00017530(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
    if (iVar2 != 0) {
      local_20[0xaf] = '\b';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_80;
    }
    break;
  case 9:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00017637(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
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
      ppiVar3 = &piStack_80;
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
    ppiVar3 = (int **)local_78;
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
      ppiVar3 = (int **)local_78;
    }
    break;
  case 0xb:
    if (*(int *)(local_20 + 0xab) < 0) {
      piStack_7c = (int *)0x22f65;
      iVar2 = FUN_0001519f(local_20,(&DAT_00053eb8)
                                    [*(int *)(local_20 + 0x113) * 10 +
                                     *(int *)(local_20 + 0xd) * 0x88]);
      piStack_7c = (int *)0x22f6d;
      uVar5 = FUN_00041f76(iVar2 - local_40);
      if ((int)uVar5 < (int)((ulonglong)uVar5 >> 0x20)) {
        piStack_7c = (int *)0x22f79;
        iVar2 = FUN_000151d5(local_20);
        piStack_7c = (int *)0x22f81;
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
            ppiVar3 = (int **)local_78;
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
            ppiVar3 = (int **)local_78;
          }
          break;
        }
      }
      piStack_7c = (int *)0x2302c;
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
        ppiVar3 = (int **)local_78;
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
        ppiVar3 = (int **)local_78;
      }
    }
    else {
      *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
      local_20[0x17] = '\x01';
      local_20[0x18] = '\0';
      local_20[0x19] = '\0';
      local_20[0x1a] = '\0';
      ppiVar3 = (int **)local_78;
    }
    break;
  case 0xc:
    *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
    ppiVar3 = (int **)local_78;
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
      ppiVar3 = (int **)local_78;
    }
    break;
  default:
    ppiVar3 = (int **)local_78;
    break;
  case 0x14:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_0001776f(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
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
      ppiVar3 = &piStack_80;
    }
    break;
  case 0x15:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00017927(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
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
      ppiVar3 = &piStack_80;
    }
    break;
  case 0x16:
    piStack_7c = &local_30;
    piStack_80 = &local_34;
    iVar2 = FUN_00017ca6(local_20,local_38,local_40,local_3c);
    ppiVar3 = &piStack_80;
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
      ppiVar3 = &piStack_80;
    }
  }
  if (local_34 != 0) {
    *(undefined4 *)((int)ppiVar3 + -4) = 0x23154;
    FUN_0001545e(local_20,local_34);
  }
  if (local_30 != 0) {
    *(undefined4 *)((int)ppiVar3 + -4) = 0x23165;
    FUN_0001586b(local_20,local_30);
  }
  *(undefined4 *)((int)ppiVar3 + -4) = 0x2316d;
  iVar2 = FUN_0001519f(local_20);
  if (local_40 < iVar2) {
    local_20[0x1b] = '\x01';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  *(undefined4 *)((int)ppiVar3 + -4) = 0x23184;
  iVar2 = FUN_0001519f(local_20);
  if (iVar2 < local_40) {
    local_20[0x1b] = '\0';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  return;
}


