// === move_dx @ 0001545e ===

void FUN_0001545e(void)

{
  byte *pbVar1;
  int iVar2;
  undefined8 uVar3;
  int local_2c;
  int local_28;
  int local_24;
  int local_1c;
  
  uVar3 = FUN_000418ee();
  local_1c = (int)((ulonglong)uVar3 >> 0x20);
  pbVar1 = (byte *)uVar3;
  if (local_1c != 0) {
    if (*(int *)(pbVar1 + 0xd) == 0) {
      local_2c = DAT_000639dc + 0xf;
      local_28 = DAT_000639dc + 0x131;
    }
    else if (*(int *)(pbVar1 + 0x17) == 2) {
      local_2c = DAT_000639dc + -100;
      local_28 = DAT_000639dc + 0x1a4;
    }
    else {
      local_2c = DAT_000639dc + 10;
      local_28 = DAT_000639dc + 0x136;
    }
    if (*pbVar1 < 2) {
      for (local_24 = 0; local_24 < DAT_00062a8c; local_24 = local_24 + 1) {
        if ((*(int *)(&DAT_0005c8c0 + local_24 * 0x14) < *(int *)(pbVar1 + 9)) &&
           (*(int *)(pbVar1 + 9) < *(int *)(&DAT_0005c8c8 + local_24 * 0x14))) {
          if (local_1c < 1) {
            if (((((*(int *)(&DAT_0005c8bc + local_24 * 0x14) < *(int *)(pbVar1 + 1) + local_1c) &&
                  (*(int *)(pbVar1 + 1) + local_1c < *(int *)(&DAT_0005c8c4 + local_24 * 0x14))) &&
                 (local_1c = *(int *)(&DAT_0005c8c4 + local_24 * 0x14) - *(int *)(pbVar1 + 1),
                 *(int *)(pbVar1 + 0x17) == 0x16)) &&
                ((pbVar1 != DAT_00053c7c && (-1 < *(int *)(pbVar1 + 0x11))))) &&
               (*(int *)(pbVar1 + 0x11) < 0x16)) {
              DAT_00063364 = 0x12;
              DAT_00063368 = 1;
              pbVar1[0x17] = 0x20;
              pbVar1[0x18] = 0;
              pbVar1[0x19] = 0;
              pbVar1[0x1a] = 0;
              pbVar1[0x11] = 0;
              pbVar1[0x12] = 0;
              pbVar1[0x13] = 0;
              pbVar1[0x14] = 0;
              pbVar1[5] = 0;
              pbVar1[6] = 0;
              pbVar1[7] = 0;
              pbVar1[8] = 0;
              if ((pbVar1 != DAT_00053c7c) && (*pbVar1 == 0)) {
                *pbVar1 = 1;
              }
            }
          }
          else if ((((*(int *)(&DAT_0005c8bc + local_24 * 0x14) < *(int *)(pbVar1 + 1) + local_1c)
                    && (*(int *)(pbVar1 + 1) + local_1c < *(int *)(&DAT_0005c8c4 + local_24 * 0x14))
                    ) && (local_1c = *(int *)(&DAT_0005c8bc + local_24 * 0x14) -
                                     *(int *)(pbVar1 + 1), *(int *)(pbVar1 + 0x17) == 0x16)) &&
                  (((pbVar1 != DAT_00053c7c && (-1 < *(int *)(pbVar1 + 0x11))) &&
                   (*(int *)(pbVar1 + 0x11) < 0x16)))) {
            DAT_00063364 = 0x12;
            DAT_00063368 = 1;
            pbVar1[0x17] = 0x20;
            pbVar1[0x18] = 0;
            pbVar1[0x19] = 0;
            pbVar1[0x1a] = 0;
            pbVar1[0x11] = 0;
            pbVar1[0x12] = 0;
            pbVar1[0x13] = 0;
            pbVar1[0x14] = 0;
            pbVar1[5] = 0;
            pbVar1[6] = 0;
            pbVar1[7] = 0;
            pbVar1[8] = 0;
            if ((pbVar1 != DAT_00053c7c) && (*pbVar1 == 0)) {
              *pbVar1 = 1;
            }
          }
        }
      }
    }
    if ((*(int *)(pbVar1 + 1) < local_2c) || (local_28 < *(int *)(pbVar1 + 1))) {
      *(int *)(pbVar1 + 1) = *(int *)(pbVar1 + 1) + local_1c;
    }
    else {
      *(int *)(pbVar1 + 1) = *(int *)(pbVar1 + 1) + local_1c;
      if (*(int *)(pbVar1 + 1) < local_2c) {
        *(int *)(pbVar1 + 1) = local_2c;
        if (((*(int *)(pbVar1 + 0x17) == 0x16) && (pbVar1 != DAT_00053c7c)) &&
           ((-1 < *(int *)(pbVar1 + 0x11) && (*(int *)(pbVar1 + 0x11) < 0x16)))) {
          DAT_00063364 = 0x12;
          DAT_00063368 = 1;
          pbVar1[0x17] = 0x20;
          pbVar1[0x18] = 0;
          pbVar1[0x19] = 0;
          pbVar1[0x1a] = 0;
          pbVar1[0x11] = 0;
          pbVar1[0x12] = 0;
          pbVar1[0x13] = 0;
          pbVar1[0x14] = 0;
          pbVar1[5] = 0;
          pbVar1[6] = 0;
          pbVar1[7] = 0;
          pbVar1[8] = 0;
          iVar2 = FUN_00041e9f();
          *(int *)(pbVar1 + 1) = *(int *)(pbVar1 + 1) + iVar2 % 0x29 + -0x14;
          if ((pbVar1 != DAT_00053c7c) && (*pbVar1 == 0)) {
            *pbVar1 = 1;
          }
        }
      }
      else if ((((local_28 < *(int *)(pbVar1 + 1)) &&
                (*(int *)(pbVar1 + 1) = local_28, *(int *)(pbVar1 + 0x17) == 0x16)) &&
               (pbVar1 != DAT_00053c7c)) &&
              ((-1 < *(int *)(pbVar1 + 0x11) && (*(int *)(pbVar1 + 0x11) < 0x16)))) {
        DAT_00063364 = 0x12;
        DAT_00063368 = 1;
        pbVar1[0x17] = 0x20;
        pbVar1[0x18] = 0;
        pbVar1[0x19] = 0;
        pbVar1[0x1a] = 0;
        pbVar1[0x11] = 0;
        pbVar1[0x12] = 0;
        pbVar1[0x13] = 0;
        pbVar1[0x14] = 0;
        pbVar1[5] = 0;
        pbVar1[6] = 0;
        pbVar1[7] = 0;
        pbVar1[8] = 0;
        iVar2 = FUN_00041e9f();
        *(int *)(pbVar1 + 1) = *(int *)(pbVar1 + 1) + iVar2 % 0x29 + -0x14;
        if ((pbVar1 != DAT_00053c7c) && (*pbVar1 == 0)) {
          *pbVar1 = 1;
        }
      }
    }
  }
  return;
}



// === move_dy @ 00015295 ===

void FUN_00015295(void)

{
  char *pcVar1;
  int iVar2;
  undefined8 uVar3;
  
  uVar3 = FUN_000418ee();
  pcVar1 = (char *)uVar3;
  *(int *)(pcVar1 + 5) = *(int *)(pcVar1 + 5) + (int)((ulonglong)uVar3 >> 0x20);
  if (0 < *(int *)(pcVar1 + 5)) {
    pcVar1[5] = '\0';
    pcVar1[6] = '\0';
    pcVar1[7] = '\0';
    pcVar1[8] = '\0';
  }
  if ((((pcVar1[0x30] == '\x01') && (pcVar1[0x25] != -1)) && (*(int *)(pcVar1 + 5) == 0)) &&
     ((0x15 < *(int *)(pcVar1 + 0x11) && (pcVar1[0x25] = -1, *pcVar1 == '\x01')))) {
    if (pcVar1 == DAT_00053c80) {
      iVar2 = FUN_00041bdc(0x19);
      if (iVar2 != 0) {
        FUN_0003b5c0(iVar2,*(undefined4 *)(pcVar1 + 1),*(int *)(pcVar1 + 9) + -0x1e,
                     *(undefined4 *)(pcVar1 + 0x32),1);
      }
    }
    else {
      iVar2 = FUN_00041bdc(0x19);
      if (iVar2 != 0) {
        FUN_0003b5c0(iVar2,*(undefined4 *)(pcVar1 + 1),*(int *)(pcVar1 + 9) + -0x1e,
                     *(undefined4 *)(pcVar1 + 0x32),0);
      }
    }
    *(int *)(DAT_00053c7c + 0x52) = *(int *)(DAT_00053c7c + 0x52) + *(int *)(pcVar1 + 0x32);
  }
  return;
}



// === move_dy2 @ 0001586b ===

void FUN_0001586b(void)

{
  byte *pbVar1;
  int iVar2;
  undefined8 uVar3;
  int local_28;
  int local_24;
  int local_1c;
  
  uVar3 = FUN_000418ee();
  local_1c = (int)((ulonglong)uVar3 >> 0x20);
  pbVar1 = (byte *)uVar3;
  if (local_1c != 0) {
    iVar2 = *(int *)(DAT_00063354 + 0x12) + DAT_000639e0;
    if (*(char *)(DAT_00063354 + 0x10) == '\0') {
      local_28 = *(int *)(DAT_00063354 + 0x16);
    }
    else {
      local_28 = *(int *)(*(int *)(DAT_00063354 + 0x34c) + 0x16);
    }
    local_28 = local_28 + DAT_000639e0;
    if (*pbVar1 < 2) {
      for (local_24 = 0; local_24 < DAT_00062a8c; local_24 = local_24 + 1) {
        if ((*(int *)(&DAT_0005c8bc + local_24 * 0x14) < *(int *)(pbVar1 + 1)) &&
           (*(int *)(pbVar1 + 1) < *(int *)(&DAT_0005c8c4 + local_24 * 0x14))) {
          if (local_1c < 1) {
            if ((*(int *)(&DAT_0005c8c0 + local_24 * 0x14) < *(int *)(pbVar1 + 9) + local_1c) &&
               (*(int *)(pbVar1 + 9) + local_1c < *(int *)(&DAT_0005c8c8 + local_24 * 0x14))) {
              local_1c = *(int *)(&DAT_0005c8c8 + local_24 * 0x14) - *(int *)(pbVar1 + 9);
            }
          }
          else if ((*(int *)(&DAT_0005c8c0 + local_24 * 0x14) < *(int *)(pbVar1 + 9) + local_1c) &&
                  (*(int *)(pbVar1 + 9) + local_1c < *(int *)(&DAT_0005c8c8 + local_24 * 0x14))) {
            local_1c = *(int *)(&DAT_0005c8c0 + local_24 * 0x14) - *(int *)(pbVar1 + 9);
          }
        }
      }
    }
    if ((*(int *)(pbVar1 + 9) < iVar2) || (local_28 < *(int *)(pbVar1 + 9))) {
      *(int *)(pbVar1 + 9) = *(int *)(pbVar1 + 9) + local_1c;
    }
    else {
      *(int *)(pbVar1 + 9) = *(int *)(pbVar1 + 9) + local_1c;
      if (*(int *)(pbVar1 + 9) < iVar2) {
        *(int *)(pbVar1 + 9) = iVar2;
      }
      else if (local_28 < *(int *)(pbVar1 + 9)) {
        *(int *)(pbVar1 + 9) = local_28;
      }
    }
  }
  return;
}



// === walkmap_x @ 0001519f ===

int FUN_0001519f(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  return *(int *)(iVar1 + 1) - DAT_000639dc;
}



// === walkmap_y @ 000151d5 ===

int FUN_000151d5(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  return *(int *)(iVar1 + 9) - DAT_000639e0;
}



// === hitbox @ 00013c4d ===

undefined4 FUN_00013c4d(undefined4 param_1,undefined4 param_2,int *param_3)

{
  byte bVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  int extraout_ECX;
  int iVar9;
  undefined8 uVar10;
  int local_70;
  undefined4 local_6c;
  int local_68;
  int local_64;
  int local_60;
  int local_5c;
  int local_58;
  int local_54;
  int local_50;
  int local_4c;
  uint local_48;
  int local_2c;
  
  uVar10 = FUN_000418ee();
  iVar9 = (int)((ulonglong)uVar10 >> 0x20);
  iVar4 = (int)uVar10;
  if (*(char *)(iVar9 + 0x30) == '\x01') {
    local_6c = 0;
  }
  else {
    piVar2 = *(int **)(*(int *)(*(int *)(iVar9 + 0x123) + 4) + *(int *)(iVar9 + 0x17) * 4);
    if ((*(int *)(iVar9 + 0x11) < (int)(uint)*(ushort *)(piVar2 + 1)) ||
       ((int)(uint)*(ushort *)((int)piVar2 + 6) <= *(int *)(iVar9 + 0x11))) {
      if (((*(int *)(iVar4 + 0x17) == 0x67) || (*(int *)(iVar4 + 0x17) == 0x68)) &&
         (*(int *)(iVar9 + 0x4a) != iVar4)) {
        local_6c = 0;
      }
      else {
        uVar5 = (uint)*(ushort *)
                       ((*(int *)(iVar4 + 0x11) % *param_3) * 2 + *(int *)((int)param_3 + 0x6d));
        uVar6 = (uint)*(ushort *)
                       ((*(int *)(iVar9 + 0x11) % *piVar2) * 2 + *(int *)((int)piVar2 + 0x6d));
        uVar7 = (uint)*(byte *)((int)param_3 + extraout_ECX + 0x5d);
        iVar8 = *(int *)(iVar9 + 0xd);
        local_50 = *(int *)(iVar9 + 9) + *(int *)(iVar9 + 5);
        if (((int)((uint)*(byte *)((int)param_3 + extraout_ECX + 0x56) + *(int *)(iVar4 + 9)) <
             *(int *)(iVar9 + 9)) ||
           (*(int *)(iVar9 + 9) <
            (int)(*(int *)(iVar4 + 9) - (uint)*(byte *)((int)param_3 + extraout_ECX + 0x56)))) {
          local_6c = 0;
        }
        else {
          if (*(int *)(iVar4 + 0x1b) == 0) {
            local_60 = -*(int *)(*(int *)(*(int *)(&DAT_000639ec + iVar8 * 4) + 0x18) + uVar6 * 4);
            iVar3 = *(int *)(uVar6 * 4 + *(int *)(*(int *)(&DAT_000639ec + iVar8 * 4) + 0x24));
            local_5c = -*(int *)(*(int *)(*(int *)(&DAT_000639ec + iVar8 * 4) + 0x14) + uVar6 * 4);
            local_4c = *(int *)(*(int *)(*(int *)(&DAT_000639ec + iVar8 * 4) + 0x28) + uVar6 * 4);
            local_68 = *(int *)(iVar4 + 1) +
                       *(int *)(*(int *)(*(int *)(&DAT_000639ec + *(int *)(iVar4 + 0xd) * 4) + 0x1c)
                               + uVar5 * 4);
            local_58 = *(int *)(*(int *)(*(int *)(&DAT_000639ec + *(int *)(iVar4 + 0xd) * 4) + 0x2c)
                               + uVar5 * 4) + *(int *)(iVar4 + 9) + *(int *)(iVar4 + 5);
            local_64 = *(int *)(iVar4 + 1) +
                       *(int *)(*(int *)(*(int *)(&DAT_000639ec + *(int *)(iVar4 + 0xd) * 4) + 0x20)
                               + uVar5 * 4);
            local_54 = *(int *)(*(int *)(*(int *)(&DAT_000639ec + *(int *)(iVar4 + 0xd) * 4) + 0x30)
                               + uVar5 * 4) + *(int *)(iVar4 + 9) + *(int *)(iVar4 + 5);
          }
          else {
            local_60 = *(int *)(*(int *)(*(int *)(&DAT_000639ec + iVar8 * 4) + 0x14) + uVar6 * 4);
            iVar3 = *(int *)(uVar6 * 4 + *(int *)(*(int *)(&DAT_000639ec + iVar8 * 4) + 0x24));
            local_5c = *(int *)(*(int *)(*(int *)(&DAT_000639ec + iVar8 * 4) + 0x18) + uVar6 * 4);
            local_4c = *(int *)(*(int *)(*(int *)(&DAT_000639ec + iVar8 * 4) + 0x28) + uVar6 * 4);
            local_68 = *(int *)(iVar4 + 1) -
                       *(int *)(*(int *)(*(int *)(&DAT_000639ec + *(int *)(iVar4 + 0xd) * 4) + 0x20)
                               + uVar5 * 4);
            local_58 = *(int *)(*(int *)(*(int *)(&DAT_000639ec + *(int *)(iVar4 + 0xd) * 4) + 0x2c)
                               + uVar5 * 4) + *(int *)(iVar4 + 9) + *(int *)(iVar4 + 5);
            local_64 = *(int *)(iVar4 + 1) -
                       *(int *)(*(int *)(*(int *)(&DAT_000639ec + *(int *)(iVar4 + 0xd) * 4) + 0x1c)
                               + uVar5 * 4);
            local_54 = *(int *)(*(int *)(*(int *)(&DAT_000639ec + *(int *)(iVar4 + 0xd) * 4) + 0x30)
                               + uVar5 * 4) + *(int *)(iVar4 + 9) + *(int *)(iVar4 + 5);
          }
          local_4c = local_50 + local_4c;
          local_50 = local_50 + iVar3;
          local_5c = *(int *)(iVar9 + 1) + local_5c;
          local_60 = *(int *)(iVar9 + 1) + local_60;
          if (((local_64 < local_60) || (local_5c < local_64)) &&
             ((local_68 < local_60 || (local_5c < local_68)))) {
            if ((local_60 < local_68) || (local_64 < local_5c)) {
              return 0;
            }
            if ((((local_54 < local_50) || (local_4c < local_54)) &&
                ((local_58 < local_50 || (local_4c < local_58)))) &&
               ((local_50 < local_58 || (local_54 < local_4c)))) {
              return 0;
            }
          }
          else if ((((local_54 < local_50) || (local_4c < local_54)) &&
                   ((local_58 < local_50 || (local_4c < local_58)))) &&
                  ((local_50 < local_58 || (local_54 < local_4c)))) {
            return 0;
          }
          bVar1 = *(byte *)((int)param_3 + extraout_ECX + 0x44);
          local_48 = (uint)bVar1;
          if ((bVar1 & 0x80) != 0) {
            local_48 = (local_48 & 0x7f) - 0x80;
          }
          if (*(int *)(iVar4 + 0x1b) == 0) {
            local_2c = *(int *)(iVar4 + 1) + local_48;
          }
          else {
            local_2c = *(int *)(iVar4 + 1) - local_48;
          }
          if (local_2c < local_60) {
            local_2c = local_60;
          }
          else if (local_5c < local_2c) {
            local_2c = local_5c;
          }
          if (uVar7 != 0) {
            iVar8 = FUN_00041bdc(0x147);
            if (iVar8 == 0) {
              local_70 = 0;
            }
            else {
              local_70 = FUN_00010efc(iVar8,local_2c,
                                      (-(uint)*(byte *)((int)param_3 + extraout_ECX + 0x56) -
                                      (uint)*(byte *)((int)param_3 + extraout_ECX + 0x3e)) + -1 +
                                      *(int *)(iVar4 + 5),
                                      (uint)*(byte *)((int)param_3 + extraout_ECX + 0x56) +
                                      *(int *)(iVar4 + 9) + 1,0xc9,2);
            }
            *(undefined1 *)(local_70 + 0x30) = 1;
            *(uint *)(local_70 + 0x17) = uVar7;
            if ((iVar4 == DAT_00053c7c) &&
               ((*(int *)(iVar4 + 0x17) == 0x38 || (*(int *)(iVar4 + 0x17) == 0xa4)))) {
              *(int *)(local_70 + 0x1b) = 1 - *(int *)(iVar4 + 0x1b);
            }
            else {
              *(undefined4 *)(local_70 + 0x1b) = *(undefined4 *)(iVar4 + 0x1b);
            }
          }
          if (((*(int *)(iVar4 + 0x17) == 0x65) || (*(int *)(iVar4 + 0x17) == 0x99)) &&
             (iVar4 == DAT_00053c7c)) {
            if ((char)piVar2[0x1b] == '\x01') {
              *(undefined4 *)(iVar4 + 0x17) = 0xa1;
              *(undefined4 *)(iVar4 + 0x11) = 0;
              *(undefined2 *)(iVar4 + 0x50) = 0;
              *(int *)(iVar9 + 0x1b) = 1 - *(int *)(iVar4 + 0x1b);
              return 0;
            }
            if (*(int *)(iVar4 + 0x1b) == *(int *)(iVar9 + 0x1b)) {
              if (*(char *)(DAT_00053c7c + 0x27) == '\x04') {
                *(undefined4 *)(iVar4 + 0x17) = 0xab;
              }
              else {
                *(undefined4 *)(iVar4 + 0x17) = 0xa0;
              }
              *(undefined4 *)(iVar4 + 0x11) = 0;
              *(undefined2 *)(iVar4 + 0x50) = 0;
              *(int *)(iVar9 + 0x1b) = 1 - *(int *)(iVar9 + 0x1b);
              return 0;
            }
          }
          else if (*(int *)(iVar4 + 0x1b) == 0) {
            if ((*(int *)(iVar9 + 0x1b) == 0) && (*(int *)(iVar4 + 1) < *(int *)(iVar9 + 1))) {
              *(undefined4 *)(iVar9 + 0x1b) = 1;
            }
          }
          else if ((*(int *)(iVar9 + 0x1b) == 1) && (*(int *)(iVar9 + 1) < *(int *)(iVar4 + 1))) {
            *(undefined4 *)(iVar9 + 0x1b) = 0;
          }
          local_6c = 1;
        }
      }
    }
    else {
      local_6c = 0;
    }
  }
  return local_6c;
}



// === ent_t @ 0001194c ===

void FUN_0001194c(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = FUN_000418ee();
  uVar1 = *(uint *)(iVar2 + 0x17);
  if (uVar1 < 0x1a) {
    if (uVar1 < 0x16) {
      if (3 < uVar1) {
        if (uVar1 < 5) {
          if ((*(int *)(iVar2 + 0x11) == 0x12) || (*(int *)(iVar2 + 0x11) == 0xb)) {
            FUN_00011827(iVar2,9,0,1,1);
          }
        }
        else if (uVar1 == 0x15) {
          if (iVar2 == DAT_00053c7c) {
            if (*(int *)(iVar2 + 0x11) == 0x3a) {
              FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
            }
            else if (*(int *)(iVar2 + 0x11) == 0x50) {
              FUN_000113e7(iVar2);
            }
          }
          else if ((*(int *)(iVar2 + 0xd) < 0x1f) || (0x2d < *(int *)(iVar2 + 0xd))) {
            if ((*(int *)(iVar2 + 0xd) == 0x32) || (*(int *)(iVar2 + 0xd) == 7)) {
              if (*(int *)(iVar2 + 0x11) == 0x2b) {
                iVar3 = FUN_00041e9f();
                FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
                FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
              }
              else if (*(int *)(iVar2 + 0x11) == 0x3e) {
                FUN_000113e7(iVar2);
              }
            }
            else if ((*(int *)(iVar2 + 0xd) == 1) || (*(int *)(iVar2 + 0xd) == 2)) {
              if (*(int *)(iVar2 + 0x11) == 0x24) {
                FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
              }
              else if (*(int *)(iVar2 + 0x11) == 0x37) {
                FUN_000113e7(iVar2);
              }
            }
            else if (*(int *)(iVar2 + 0x11) == 0x3a) {
              iVar3 = FUN_00041e9f();
              FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
              FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
            }
            else if (*(int *)(iVar2 + 0x11) == 0x4d) {
              FUN_000113e7(iVar2);
            }
          }
          else if (*(int *)(iVar2 + 0x11) == 0x3a) {
            iVar3 = FUN_00041e9f();
            FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
            FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
          }
          else if (*(int *)(iVar2 + 0x11) == 0x4d) {
            FUN_000113e7(iVar2);
          }
        }
      }
    }
    else if (uVar1 < 0x17) {
      if (iVar2 == DAT_00053c7c) {
        if (*(int *)(iVar2 + 0x11) == 0x25) {
          FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
        }
        else if (*(int *)(iVar2 + 0x11) == 0x37) {
          FUN_000113e7(iVar2);
        }
      }
      else if (*(int *)(iVar2 + 0x11) == 0x22) {
        iVar3 = FUN_00041e9f();
        FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
        FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
      }
      else if (*(int *)(iVar2 + 0x11) == 0x33) {
        FUN_000113e7(iVar2);
      }
    }
    else if (uVar1 < 0x18) {
      if ((*(int *)(iVar2 + 0xd) < 0x1f) || (0x23 < *(int *)(iVar2 + 0xd))) {
        if ((*(int *)(iVar2 + 0xd) < 0x24) || (0x28 < *(int *)(iVar2 + 0xd))) {
          if ((*(int *)(iVar2 + 0xd) < 0x29) || (0x2d < *(int *)(iVar2 + 0xd))) {
            if (((*(int *)(iVar2 + 0xd) < 0x2e) || (0x32 < *(int *)(iVar2 + 0xd))) &&
               (*(int *)(iVar2 + 0xd) != 7)) {
              if ((*(int *)(iVar2 + 0xd) == 1) || (*(int *)(iVar2 + 0xd) == 2)) {
                if (*(int *)(iVar2 + 0x11) == 0x35) {
                  FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
                }
                else if (*(int *)(iVar2 + 0x11) == 0x4e) {
                  FUN_000113e7(iVar2);
                }
              }
              else if (*(int *)(iVar2 + 0x11) == 0x4b) {
                iVar3 = FUN_00041e9f();
                FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
                FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
              }
              else if (*(int *)(iVar2 + 0x11) == 100) {
                FUN_000113e7(iVar2);
              }
            }
            else if (*(int *)(iVar2 + 0x11) == 0x41) {
              iVar3 = FUN_00041e9f();
              FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
              FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
            }
            else if (*(int *)(iVar2 + 0x11) == 0x5a) {
              FUN_000113e7(iVar2);
            }
          }
          else if (*(int *)(iVar2 + 0x11) == 0x4c) {
            iVar3 = FUN_00041e9f();
            FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
            FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
          }
          else if (*(int *)(iVar2 + 0x11) == 0x65) {
            FUN_000113e7(iVar2);
          }
        }
        else if (*(int *)(iVar2 + 0x11) == 0x4f) {
          iVar3 = FUN_00041e9f();
          FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
          FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
        }
        else if (*(int *)(iVar2 + 0x11) == 0x68) {
          FUN_000113e7(iVar2);
        }
      }
      else if (*(int *)(iVar2 + 0x11) == 0x4b) {
        iVar3 = FUN_00041e9f();
        FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
        FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
      }
      else if (*(int *)(iVar2 + 0x11) == 100) {
        FUN_000113e7(iVar2);
      }
    }
    else if (uVar1 == 0x18) {
      if ((*(int *)(iVar2 + 0xd) < 0x1f) || (0x23 < *(int *)(iVar2 + 0xd))) {
        if ((*(int *)(iVar2 + 0xd) < 0x24) || (0x28 < *(int *)(iVar2 + 0xd))) {
          if ((*(int *)(iVar2 + 0xd) < 0x29) || (0x2d < *(int *)(iVar2 + 0xd))) {
            if (((*(int *)(iVar2 + 0xd) < 0x2e) || (0x32 < *(int *)(iVar2 + 0xd))) &&
               (*(int *)(iVar2 + 0xd) != 7)) {
              if (*(int *)(iVar2 + 0x11) == 10) {
                iVar3 = FUN_00041e9f();
                FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
                FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
              }
              else if ((*(int *)(iVar2 + 0x11) == 0x3f) || (*(int *)(iVar2 + 0x11) == 0x59)) {
                FUN_000113e7(iVar2);
              }
            }
            else if (*(int *)(iVar2 + 0x11) == 10) {
              iVar3 = FUN_00041e9f();
              FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
              FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
            }
            else if ((*(int *)(iVar2 + 0x11) == 0x32) || (*(int *)(iVar2 + 0x11) == 0x43)) {
              FUN_000113e7(iVar2);
            }
          }
          else if (*(int *)(iVar2 + 0x11) == 10) {
            iVar3 = FUN_00041e9f();
            FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
            FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
          }
          else if ((*(int *)(iVar2 + 0x11) == 0x3e) || (*(int *)(iVar2 + 0x11) == 0x50)) {
            FUN_000113e7(iVar2);
          }
        }
        else if (*(int *)(iVar2 + 0x11) == 0xe) {
          iVar3 = FUN_00041e9f();
          FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
          FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
        }
        else if ((*(int *)(iVar2 + 0x11) == 0x3f) || (*(int *)(iVar2 + 0x11) == 0x50)) {
          FUN_000113e7(iVar2);
        }
      }
      else if (*(int *)(iVar2 + 0x11) == 10) {
        iVar3 = FUN_00041e9f();
        FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
        FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
      }
      else if ((*(int *)(iVar2 + 0x11) == 0x3f) || (*(int *)(iVar2 + 0x11) == 0x59)) {
        FUN_000113e7(iVar2);
      }
    }
  }
  else if (uVar1 < 0x1b) {
    if ((*(int *)(iVar2 + 0xd) < 0x1f) || (0x23 < *(int *)(iVar2 + 0xd))) {
      if ((*(int *)(iVar2 + 0xd) < 0x24) || (0x2d < *(int *)(iVar2 + 0xd))) {
        if (((*(int *)(iVar2 + 0xd) < 0x2e) || (0x32 < *(int *)(iVar2 + 0xd))) &&
           (*(int *)(iVar2 + 0xd) != 7)) {
          if (*(int *)(iVar2 + 0x11) == 0x20) {
            iVar3 = FUN_00041e9f();
            FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
            FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
          }
          else if (*(int *)(iVar2 + 0x11) == 0x33) {
            FUN_000113e7(iVar2);
          }
        }
        else if (*(int *)(iVar2 + 0x11) == 0x1d) {
          iVar3 = FUN_00041e9f();
          FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
          FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
        }
        else if (*(int *)(iVar2 + 0x11) == 0x31) {
          FUN_000113e7(iVar2);
        }
      }
      else if (*(int *)(iVar2 + 0x11) == 0x20) {
        iVar3 = FUN_00041e9f();
        FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
        FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
      }
      else if (*(int *)(iVar2 + 0x11) == 0x34) {
        FUN_000113e7(iVar2);
      }
    }
    else if (*(int *)(iVar2 + 0x11) == 0x20) {
      iVar3 = FUN_00041e9f();
      FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
      FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
    }
    else if (*(int *)(iVar2 + 0x11) == 0x33) {
      FUN_000113e7(iVar2);
    }
  }
  else if (uVar1 < 0x20) {
    if (0x1c < uVar1) {
      if (uVar1 < 0x1e) {
        if (*(int *)(iVar2 + 0x11) == 0x30) {
          iVar3 = FUN_00041e9f();
          FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
          FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
        }
        else if (*(int *)(iVar2 + 0x11) == 0x42) {
          FUN_000113e7(iVar2);
        }
      }
      else if (uVar1 == 0x1e) {
        if (*(int *)(iVar2 + 0x11) == 0x31) {
          iVar3 = FUN_00041e9f();
          FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
          FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
        }
        else if (*(int *)(iVar2 + 0x11) == 0x3b) {
          FUN_000113e7(iVar2);
        }
      }
    }
  }
  else if (uVar1 < 0x21) {
    if ((*(int *)(iVar2 + 0xd) < 0x1f) || (0x23 < *(int *)(iVar2 + 0xd))) {
      if ((*(int *)(iVar2 + 0xd) < 0x24) || (0x28 < *(int *)(iVar2 + 0xd))) {
        if (((*(int *)(iVar2 + 0xd) < 0x29) || (0x2d < *(int *)(iVar2 + 0xd))) &&
           (DAT_00053c7c != iVar2)) {
          if (((*(int *)(iVar2 + 0xd) < 0x2e) || (0x32 < *(int *)(iVar2 + 0xd))) &&
             (*(int *)(iVar2 + 0xd) != 7)) {
            if (*(int *)(iVar2 + 0x11) == 0x2d) {
              iVar3 = FUN_00041e9f();
              FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
              FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
            }
            else if (*(int *)(iVar2 + 0x11) == 0x40) {
              FUN_000113e7(iVar2);
            }
          }
          else if (*(int *)(iVar2 + 0x11) == 0x24) {
            iVar3 = FUN_00041e9f();
            FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
            FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
          }
          else if (*(int *)(iVar2 + 0x11) == 0x36) {
            FUN_000113e7(iVar2);
          }
        }
        else if (*(int *)(iVar2 + 0x11) == 0x2d) {
          iVar3 = FUN_00041e9f();
          FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
          FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
        }
        else if (*(int *)(iVar2 + 0x11) == 0x40) {
          FUN_000113e7(iVar2);
        }
      }
      else if (*(int *)(iVar2 + 0x11) == 0x33) {
        iVar3 = FUN_00041e9f();
        FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
        FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
      }
      else if (*(int *)(iVar2 + 0x11) == 0x45) {
        FUN_000113e7(iVar2);
      }
    }
    else if (*(int *)(iVar2 + 0x11) == 0x2d) {
      iVar3 = FUN_00041e9f();
      FUN_0001545e(iVar2,iVar3 % 0x15 + -10);
      FUN_00011827(iVar2,0xfffffff6,0xfffffffc,1,4);
    }
    else if (*(int *)(iVar2 + 0x11) == 0x40) {
      FUN_000113e7(iVar2);
    }
  }
  else if (0x8b < uVar1) {
    if (uVar1 < 0x8d) {
      if ((iVar2 == DAT_00053c7c) && (*(int *)(iVar2 + 0x11) == 0x3f)) {
        FUN_00011827(iVar2,0x20,0xfffffffc,1,4);
      }
    }
    else if (((uVar1 == 0x8d) && (iVar2 == DAT_00053c7c)) && (*(int *)(iVar2 + 0x11) == 0x38)) {
      FUN_00011827(iVar2,0x20,0xfffffffc,1,4);
    }
  }
  return;
}



// === adv_act @ 00031f5f ===

void FUN_00031f5f(void)

{
  int iVar1;
  int iVar2;
  undefined4 local_28;
  
  iVar2 = FUN_000418ee();
  if (*(char *)(iVar2 + 0x31) != '\0') {
    *(char *)(iVar2 + 0x31) = *(char *)(iVar2 + 0x31) + -1;
    *(undefined4 *)(iVar2 + 0x17) = *(undefined4 *)(iVar2 + 0x32);
    iVar1 = *(int *)(*(int *)(*(int *)(iVar2 + 0x123) + 4) + *(int *)(iVar2 + 0x17) * 4);
    if (*(char *)(iVar1 + 0x69) != '\0') {
      DAT_00063364 = (uint)*(byte *)(iVar1 + 0x69);
      DAT_00063368 = (uint)*(ushort *)(iVar1 + 0x6a);
    }
    *(undefined4 *)(iVar2 + 0x11) = 0;
    for (local_28 = 0; local_28 < (int)(uint)*(byte *)(iVar2 + 0x31); local_28 = local_28 + 1) {
      *(undefined4 *)(local_28 * 4 + iVar2 + 0x32) = *(undefined4 *)(local_28 * 4 + iVar2 + 0x36);
    }
  }
  return;
}



// === set_act @ 00031f10 ===

void FUN_00031f10(void)

{
  int iVar1;
  undefined8 uVar2;
  
  uVar2 = FUN_000418ee();
  iVar1 = (int)uVar2;
  if (*(byte *)(iVar1 + 0x31) < 5) {
    *(int *)((uint)*(byte *)(iVar1 + 0x31) * 4 + iVar1 + 0x32) = (int)((ulonglong)uVar2 >> 0x20);
    *(char *)(iVar1 + 0x31) = *(char *)(iVar1 + 0x31) + '\x01';
  }
  return;
}



// === ent_free @ 000112fd ===

int FUN_000112fd(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_00053c68 == 1) {
    DAT_00053c70 = 0;
    DAT_00053c74 = 0;
    DAT_00053c78 = 0;
  }
  else if (DAT_00053c70 == iVar1) {
    DAT_00053c70 = *(int *)(iVar1 + 0x13f);
    *(undefined4 *)(*(int *)(iVar1 + 0x13f) + 0x143) = 0;
  }
  else if (DAT_00053c74 == iVar1) {
    DAT_00053c74 = *(int *)(iVar1 + 0x143);
    *(undefined4 *)(*(int *)(iVar1 + 0x143) + 0x13f) = 0;
  }
  else {
    *(undefined4 *)(*(int *)(iVar1 + 0x143) + 0x13f) = *(undefined4 *)(iVar1 + 0x13f);
    *(undefined4 *)(*(int *)(iVar1 + 0x13f) + 0x143) = *(undefined4 *)(iVar1 + 0x143);
  }
  DAT_00053c68 = DAT_00053c68 + -1;
  return iVar1;
}



// === chk46 @ 0001435f ===

undefined4 FUN_0001435f(undefined4 param_1,undefined4 param_2,byte param_3)

{
  int iVar1;
  int iVar2;
  byte extraout_DL;
  undefined4 local_2c;
  uint local_28;
  
  iVar2 = FUN_000418ee();
  iVar1 = *(int *)(*(int *)(*(int *)(iVar2 + 0x123) + 4) + *(int *)(iVar2 + 0x17) * 4);
  local_28 = 0;
  while (((int)local_28 < 6 &&
         ((*(int *)(iVar2 + 0x11) < (int)(uint)*(ushort *)(local_28 * 4 + iVar1 + 8) ||
          ((int)(uint)*(ushort *)(local_28 * 4 + iVar1 + 10) <= *(int *)(iVar2 + 0x11)))))) {
    local_28 = local_28 + 1;
  }
  if (((uint)extraout_DL == *(uint *)(iVar2 + 0x17)) && (param_3 == local_28)) {
    local_2c = 0;
  }
  else {
    local_2c = 1;
  }
  return local_2c;
}



// === key_disp @ 00034353 ===

void FUN_00034353(void)

{
  undefined1 uVar1;
  int extraout_EDX;
  
  uVar1 = FUN_000418ee();
  (&DAT_00063f14)[DAT_00063f50] = uVar1;
  *(int *)(&DAT_00063f28 + (uint)DAT_00063f50 * 4) = extraout_EDX - DAT_00063f0c;
  (&DAT_00063f1e)[DAT_00063f50] = 0;
  DAT_00063f0c = extraout_EDX;
  DAT_00063f50 = DAT_00063f50 + 1;
  if (9 < DAT_00063f50) {
    DAT_00063f50 = 0;
  }
  return;
}



// === key_get @ 0003493e ===

undefined8 FUN_0003493e(void)

{
  undefined4 extraout_EDX;
  byte local_20;
  
  FUN_000418ee();
  if (DAT_00063f50 == 0) {
    local_20 = DAT_00063f1d;
  }
  else {
    local_20 = (&DAT_00063f13)[DAT_00063f50];
  }
  return CONCAT44(extraout_EDX,(uint)local_20);
}



// === key_rec2 @ 000342ff ===

void FUN_000342ff(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_00063ed3 == '\0') {
    if (DAT_00063ed5 != '\0') {
      FUN_0001545e(DAT_00053c7c,iVar1);
    }
  }
  else {
    FUN_0001545e(DAT_00053c7c,-iVar1);
  }
  return;
}



// === key_rec1 @ 00034295 ===

void FUN_00034295(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_00063ed3 == '\0') {
    if ((DAT_00063ed5 != '\0') && (*(int *)(DAT_00053c7c + 0x1b) == 1)) {
      FUN_0001545e(DAT_00053c7c,iVar1);
    }
  }
  else if (*(int *)(DAT_00053c7c + 0x1b) == 0) {
    FUN_0001545e(DAT_00053c7c,-iVar1);
  }
  return;
}



// === key_hold @ 0003442e ===

void FUN_0003442e(void)

{
  bool bVar1;
  byte bVar2;
  byte local_2c;
  
  bVar2 = FUN_000418ee();
  bVar1 = false;
  if (bVar2 < 0x2a) {
    if (bVar2 == 0x1d) {
      if (DAT_00062c90 == '\0') {
        if ((DAT_00062c82 != '\0') || (DAT_00062c8e != '\0')) {
          (&DAT_00063f14)[DAT_00063f50] = 0x68;
          local_2c = 0x2a;
          bVar1 = true;
        }
      }
      else {
        (&DAT_00063f14)[DAT_00063f50] = 0x67;
        local_2c = 0x38;
        bVar1 = true;
      }
    }
    goto LAB_00034580;
  }
  if (0x2a < bVar2) {
    if (bVar2 < 0x36) goto LAB_00034580;
    if (0x36 < bVar2) {
      if (bVar2 == 0x38) {
        if (DAT_00062c75 == '\0') {
          if ((DAT_00062c82 != '\0') || (DAT_00062c8e != '\0')) {
            (&DAT_00063f14)[DAT_00063f50] = 0x69;
            local_2c = 0x2a;
            bVar1 = true;
          }
        }
        else {
          (&DAT_00063f14)[DAT_00063f50] = 0x67;
          local_2c = 0x1d;
          bVar1 = true;
        }
      }
      goto LAB_00034580;
    }
  }
  if (DAT_00062c75 == '\0') {
    if (DAT_00062c90 != '\0') {
      (&DAT_00063f14)[DAT_00063f50] = 0x69;
      local_2c = 0x38;
      bVar1 = true;
    }
  }
  else {
    (&DAT_00063f14)[DAT_00063f50] = 0x68;
    local_2c = 0x1d;
    bVar1 = true;
  }
LAB_00034580:
  if (!bVar1) {
    return;
  }
  (&DAT_00063f1e)[DAT_00063f50] = 0;
  *(int *)(&DAT_00063f28 + (uint)DAT_00063f50 * 4) = DAT_00063f08 - DAT_00063f0c;
  if (DAT_00063f50 == 9) {
    DAT_00063f50 = '\0';
  }
  else {
    DAT_00063f50 = DAT_00063f50 + 1;
  }
  FUN_000343d1(bVar2);
  if (local_2c < 0x2a) {
    if (local_2c != 0x1d) {
      return;
    }
  }
  else {
    if (local_2c < 0x2b) {
      FUN_000343d1(0x2a);
      FUN_000343d1(0x36);
      return;
    }
    if (local_2c != 0x38) {
      return;
    }
  }
  FUN_000343d1(local_2c);
  return;
}



// === spawn_seg @ 0003cad3 ===

void FUN_0003cad3(void)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_000418ee();
  if (*(char *)((int)piVar1 + 0x11) == '\0') {
    DAT_000639dc = *piVar1;
  }
  else {
    DAT_000639dc = piVar1[2] + -0x140;
  }
  DAT_000639e0 = piVar1[1];
  DAT_000639e4 = (char)piVar1[4];
  return;
}



// === reset @ 00038f3d ===

void FUN_00038f3d(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  int extraout_EDX;
  int extraout_EDX_00;
  int extraout_EDX_01;
  int extraout_EDX_02;
  int extraout_EDX_03;
  int extraout_EDX_04;
  int extraout_EDX_05;
  undefined4 *puVar6;
  undefined1 *puVar7;
  undefined1 *puVar8;
  int *piVar9;
  undefined4 *puVar10;
  int *piVar11;
  byte bVar12;
  undefined4 local_15c;
  undefined4 local_158;
  undefined4 local_154;
  undefined1 local_150 [16];
  undefined4 local_140;
  int local_13c;
  uint local_138;
  uint local_134;
  uint local_130;
  uint local_12c;
  undefined4 local_128;
  int local_120 [6];
  int local_108 [6];
  undefined4 local_f0 [6];
  undefined4 local_d8 [6];
  undefined4 local_c0 [6];
  undefined4 local_a8 [6];
  int local_90;
  int local_8c;
  int local_88;
  int local_84;
  int local_80;
  int local_7c;
  int local_78;
  int local_74;
  int local_70;
  uint local_6c;
  uint local_68;
  int local_64;
  int local_60;
  int local_5c;
  int local_58;
  int local_54;
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
  int local_28;
  int local_24;
  int local_20;
  
  bVar12 = 0;
  local_20 = FUN_000418ee();
  local_7c = 0;
  local_78 = 0;
  local_74 = 0;
  local_8c = 0;
  local_90 = 0;
  local_64 = 0;
  local_84 = 0;
  puVar6 = &DAT_00053150;
  puVar10 = local_a8;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar10 = *puVar6;
    puVar6 = puVar6 + (uint)bVar12 * -2 + 1;
    puVar10 = puVar10 + (uint)bVar12 * -2 + 1;
  }
  puVar6 = &DAT_00053168;
  puVar10 = local_c0;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar10 = *puVar6;
    puVar6 = puVar6 + (uint)bVar12 * -2 + 1;
    puVar10 = puVar10 + (uint)bVar12 * -2 + 1;
  }
  puVar6 = &DAT_00053180;
  puVar10 = local_d8;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar10 = *puVar6;
    puVar6 = puVar6 + (uint)bVar12 * -2 + 1;
    puVar10 = puVar10 + (uint)bVar12 * -2 + 1;
  }
  puVar6 = &DAT_00053198;
  puVar10 = local_f0;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar10 = *puVar6;
    puVar6 = puVar6 + (uint)bVar12 * -2 + 1;
    puVar10 = puVar10 + (uint)bVar12 * -2 + 1;
  }
  piVar9 = &DAT_000531b0;
  piVar11 = local_108;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *piVar11 = *piVar9;
    piVar9 = piVar9 + (uint)bVar12 * -2 + 1;
    piVar11 = piVar11 + (uint)bVar12 * -2 + 1;
  }
  piVar9 = &DAT_000531c8;
  piVar11 = local_120;
  for (iVar5 = 6; iVar5 != 0; iVar5 = iVar5 + -1) {
    *piVar11 = *piVar9;
    piVar9 = piVar9 + (uint)bVar12 * -2 + 1;
    piVar11 = piVar11 + (uint)bVar12 * -2 + 1;
  }
  *(int *)(DAT_00053c7c + 0x2b) = *(int *)(DAT_00053c7c + 0x2b) + *(int *)(DAT_00053c7c + 0x52);
  *(undefined4 *)(DAT_00053c7c + 0x52) = 0;
  FUN_00041e20(&local_138,0,0x18);
  local_138 = (uint)*(byte *)(DAT_00053c7c + 0x27);
  local_134 = (uint)*(byte *)(DAT_00053c7c + 0x28);
  local_130 = (uint)*(byte *)(DAT_00053c7c + 0x26);
  local_12c = (uint)*(byte *)(DAT_00053c7c + 0x25);
  local_58 = FUN_00041bdc(0x3c);
  local_54 = local_58;
  if (local_58 == 0) {
    local_2c = 0;
  }
  else {
    local_5c = FUN_0003d8d4(local_58,"levelup.spr");
    local_2c = local_5c;
  }
  local_60 = local_2c;
  local_4c = FUN_00041bdc(0x3c);
  local_48 = local_4c;
  if (local_4c == 0) {
    local_28 = 0;
  }
  else {
    local_50 = FUN_0003d8d4(local_4c,"number1.spr");
    local_28 = local_50;
  }
  local_13c = local_28;
  local_40 = FUN_00041bdc(0x3c);
  local_3c = local_40;
  if (local_40 == 0) {
    local_24 = 0;
  }
  else {
    local_44 = FUN_0003d8d4(local_40,"number2.spr");
    local_24 = local_44;
  }
  local_80 = local_24;
  local_140 = FUN_00041af8(64000);
  if (local_20 == 0) {
    FUN_00036190("levelup.pcx",local_140);
  }
  else {
    FUN_00036190("levelup2.pcx",local_140);
  }
  FUN_00041e20(&DAT_000636b8,0,0x300);
  FUN_00040042();
  if (DAT_000633a4 == 1) {
    FUN_00041e6c(&DAT_000a0000,local_140,64000);
    FUN_00040660(&DAT_000633b8,0x32);
  }
  else {
    FUN_00035f1b("help1.pcx",0);
    FUN_00040660(&DAT_000633b8,0x32);
    DAT_000633a4 = 1;
    FUN_00035d5d();
    FUN_00041e6c(&DAT_000a0000,local_140,64000);
  }
  if (local_20 == 1) {
    FUN_00010b1d(0xb);
  }
  FUN_0004185e(DAT_00053de4);
  iVar5 = extraout_EDX;
  puVar6 = &local_15c;
  while (((DAT_00063e89 == '\0' && (DAT_00063eb2 == '\0')) && (DAT_00063ebe == '\0'))) {
    if (DAT_00063ec3 != '\0') {
      *(undefined4 *)((int)puVar6 + -4) = 0x39241;
      FUN_0004185e(DAT_00053dd8,iVar5);
      if (local_7c == 0) {
        local_88 = 10;
        *(undefined4 *)((int)puVar6 + -4) = 0x39261;
        iVar5 = FUN_00035f1b("help10.pcx",1);
        while (((DAT_00063eb2 == '\0' && (DAT_00063ebe == '\0')) &&
               ((DAT_00063e89 == '\0' &&
                ((((DAT_00063ea4 == '\0' && (DAT_00063ec1 == '\0')) && (DAT_00063ea5 == '\0')) &&
                 (DAT_00063ec0 == '\0'))))))) {
          if ((DAT_00063ed3 != '\0') && (local_90 == 0)) {
            if (local_88 == 10) {
              local_88 = 0x23;
            }
            else {
              local_88 = local_88 + -1;
            }
            *(int *)((int)puVar6 + -4) = local_88;
            *(char **)((int)puVar6 + -8) = "help%d.pcx";
            *(undefined1 **)((int)puVar6 + -0xc) = local_150;
            *(undefined4 *)((int)puVar6 + -0x10) = 0x392eb;
            FUN_000428f8();
            *(undefined4 *)((int)puVar6 + -4) = 0x392fe;
            iVar5 = FUN_00035f1b(local_150,1);
            local_90 = 1;
          }
          iVar4 = local_90;
          if (DAT_00063ed3 == '\0') {
            local_90 = 0x13;
            iVar4 = iVar5;
          }
          else {
            local_90 = local_90 + 1;
            if (local_90 == 0x14) {
              local_90 = 0;
            }
          }
          if ((DAT_00063ed5 != '\0') && (local_64 == 0)) {
            if (local_88 == 0x23) {
              local_88 = 10;
            }
            else {
              local_88 = local_88 + 1;
            }
            *(int *)((int)puVar6 + -4) = local_88;
            *(char **)((int)puVar6 + -8) = "help%d.pcx";
            *(undefined1 **)((int)puVar6 + -0xc) = local_150;
            *(undefined4 *)((int)puVar6 + -0x10) = 0x39366;
            FUN_000428f8();
            *(undefined4 *)((int)puVar6 + -4) = 0x39379;
            iVar4 = FUN_00035f1b(local_150,1);
            local_64 = 1;
          }
          iVar5 = local_64;
          if (DAT_00063ed5 == '\0') {
            local_64 = 0x13;
            iVar5 = iVar4;
          }
          else {
            local_64 = local_64 + 1;
            if (local_64 == 0x14) {
              local_64 = 0;
            }
          }
          *(undefined4 *)((int)puVar6 + -4) = 0x393aa;
          FUN_00035ec2(iVar5);
          if (local_84 % 3 == 0) {
            *(undefined4 *)((int)puVar6 + -4) = 0x393d2;
            FUN_0003d67c(&DAT_000636b8,0x40,0x4f);
            *(undefined4 *)((int)puVar6 + -4) = 0x393e6;
            FUN_0003d67c(&DAT_000636b8,0x50,0x5f);
            *(undefined4 *)((int)puVar6 + -4) = 0x393fa;
            FUN_0003d67c(&DAT_000636b8,0x60,0x6f);
            *(undefined **)((int)puVar6 + -4) = &DAT_000636b8;
            *(undefined4 *)((int)puVar6 + -8) = 0xff;
            *(undefined4 *)((int)puVar6 + -0xc) = 0;
            *(undefined4 *)((int)puVar6 + -0x10) = 0x3940e;
            FUN_00040042();
          }
          iVar5 = local_84;
          local_84 = local_84 + 1;
        }
        do {
          do {
          } while (DAT_00063ec1 != '\0');
        } while ((((DAT_00063ea4 != '\0') || (DAT_00063ea5 != '\0')) || (DAT_00063ec0 != '\0')) ||
                (((DAT_00063eb2 != '\0' || (DAT_00063ebe != '\0')) || (DAT_00063e89 != '\0'))));
      }
      else {
        *(int *)((int)puVar6 + -4) = local_7c + 1;
        *(char **)((int)puVar6 + -8) = "help%d.pcx";
        *(undefined1 **)((int)puVar6 + -0xc) = local_150;
        *(undefined4 *)((int)puVar6 + -0x10) = 0x3947e;
        FUN_000428f8();
        *(undefined4 *)((int)puVar6 + -4) = 0x39491;
        FUN_00035f1b(local_150,1);
        *(undefined4 *)((int)puVar6 + -4) = 0x39496;
        FUN_00035d5d();
      }
      *(undefined4 *)((int)puVar6 + -4) = 0x394a0;
      FUN_0004185e(DAT_00053dd8);
      iVar5 = extraout_EDX_00;
    }
    if (((DAT_00063ea4 != '\0') || (DAT_00063ec1 != '\0')) ||
       ((DAT_00063ea5 != '\0' || (DAT_00063ec0 != '\0')))) {
      if ((*(int *)(DAT_00053c7c + 0x2b) <
           *(int *)(&DAT_00053c94 + local_7c * 0x14 + (&local_138)[local_7c] * 4)) ||
         (local_108[local_7c] <= (int)(&local_138)[local_7c])) {
        if ((int)(&local_138)[local_7c] < local_108[local_7c]) {
          *(undefined4 *)((int)puVar6 + -4) = 0x39640;
          FUN_00035f1b("help8.pcx",1);
        }
        else {
          *(undefined4 *)((int)puVar6 + -4) = 0x3962f;
          FUN_00035f1b("help9.pcx",1);
        }
        *(undefined4 *)((int)puVar6 + -4) = 0x3964a;
        FUN_0004185e(DAT_00053de0);
        *(undefined4 *)((int)puVar6 + -4) = 0x3964f;
        FUN_00035d5d();
        iVar5 = extraout_EDX_03;
      }
      else {
        *(int *)(DAT_00053c7c + 0x2b) =
             *(int *)(DAT_00053c7c + 0x2b) -
             *(int *)(&DAT_00053c94 + local_7c * 0x14 + (&local_138)[local_7c] * 4);
        if (local_7c == 5) {
          DAT_00053100 = DAT_00053100 + 1;
          *(undefined4 *)((int)puVar6 + -4) = 0x39602;
          FUN_0004185e(DAT_00053ddc);
          iVar5 = extraout_EDX_02;
        }
        else {
          (&local_138)[local_7c] = (&local_138)[local_7c] + 1;
          if ((local_7c == 4) || (local_7c == 1)) {
            if (local_7c == 4) {
              local_128 = 0;
            }
            if ((int)*(short *)(DAT_00053c7c + 0x23) == *(int *)(&DAT_00052738 + local_134 * 4)) {
              *(int *)(DAT_00053c7c + 0x2b) =
                   *(int *)(DAT_00053c7c + 0x2b) +
                   *(int *)(&DAT_00053c94 + (&local_138)[local_7c] * 4 + local_7c * 0x14);
              *(undefined4 *)((int)puVar6 + -4) = 0x395b6;
              FUN_0004185e(DAT_00053de0);
            }
            else {
              *(undefined4 *)((int)puVar6 + -4) = 0x395c2;
              FUN_0004185e(DAT_00053ddc);
            }
            iVar5 = CONCAT22((short)(local_134 * 4 >> 0x10),
                             *(undefined2 *)(&DAT_00052738 + local_134 * 4));
            *(undefined2 *)(DAT_00053c7c + 0x23) = *(undefined2 *)(&DAT_00052738 + local_134 * 4);
            *(undefined1 *)(DAT_00053c7c + 0x30) = 0;
          }
          else {
            *(undefined4 *)((int)puVar6 + -4) = 0x395f0;
            FUN_0004185e(DAT_00053ddc);
            iVar5 = extraout_EDX_01;
          }
        }
      }
      do {
        do {
        } while (DAT_00063ea4 != '\0');
      } while (((DAT_00063ec1 != '\0') || (DAT_00063ea5 != '\0')) || (DAT_00063ec0 != '\0'));
    }
    if ((DAT_00063ed0 != '\0') && (local_78 == 0)) {
      if (local_7c == 0) {
        local_7c = 5;
      }
      else {
        local_7c = local_7c + -1;
      }
      if ((local_20 == 0) && (local_7c == 5)) {
        local_7c = 3;
      }
      local_78 = 1;
      *(undefined4 *)((int)puVar6 + -4) = 0x396c3;
      FUN_0004185e(DAT_00053dd8,iVar5);
    }
    if (DAT_00063ed0 == '\0') {
      local_78 = 0x13;
    }
    else {
      local_78 = local_78 + 1;
      if (local_78 == 0x14) {
        local_78 = 0;
      }
    }
    if ((DAT_00063ed8 != '\0') && (local_74 == 0)) {
      if (local_7c == 5) {
        local_7c = 0;
      }
      else {
        local_7c = local_7c + 1;
      }
      if ((local_20 == 0) && (local_7c == 4)) {
        local_7c = 0;
      }
      local_74 = 1;
      *(undefined4 *)((int)puVar6 + -4) = 0x39732;
      FUN_0004185e(DAT_00053dd8);
    }
    uVar1 = local_140;
    if (DAT_00063ed8 == '\0') {
      local_74 = 0x13;
    }
    else {
      local_74 = local_74 + 1;
      if (local_74 == 0x14) {
        local_74 = 0;
      }
    }
    *(undefined4 *)((int)puVar6 + -4) = 0x3976c;
    FUN_00041e6c(DAT_000633b0,uVar1,64000);
    *(undefined4 *)((int)puVar6 + -4) = *(undefined4 *)(DAT_00053c7c + 0x2b);
    iVar5 = local_13c;
    *(undefined4 *)((int)puVar6 + -8) = 0x3978e;
    FUN_00038b30(DAT_000633b0,iVar5,0x49,0x19);
    *(undefined4 *)((int)puVar6 + -8) =
         *(undefined4 *)(&DAT_00053c94 + (&local_138)[local_7c] * 4 + local_7c * 0x14);
    iVar5 = local_80;
    *(undefined4 *)((int)puVar6 + -0xc) = 0x397c1;
    FUN_00038b30(DAT_000633b0,iVar5,0x5e,0x26);
    *(undefined4 *)((int)puVar6 + -0xc) = 0;
    *(int *)((int)puVar6 + -0x10) =
         *(int *)(local_60 + 0x38) + *(int *)(*(int *)(local_60 + 0x34) + local_7c * 4);
    *(undefined4 *)((int)puVar6 + -0x14) = *(undefined4 *)(local_7c * 4 + *(int *)(local_60 + 8));
    uVar1 = *(undefined4 *)(local_7c * 4 + *(int *)(local_60 + 4));
    uVar2 = local_a8[local_7c];
    uVar3 = local_c0[local_7c];
    *(undefined4 *)((int)puVar6 + -0x18) = 0x3981f;
    FUN_0003bb42(DAT_000633b0,uVar3,uVar2,uVar1);
    puVar8 = (undefined1 *)((int)puVar6 + -0x18);
    *(int *)((int)puVar6 + -0x18) = DAT_00053100;
    iVar5 = local_13c;
    *(undefined4 *)((int)puVar6 + -0x1c) = 0x3983f;
    FUN_00038c23(DAT_000633b0,iVar5,0x68,0xac);
    for (local_70 = 0; local_70 < 6; local_70 = local_70 + 1) {
      local_6c = (&local_138)[local_70];
      local_68 = (&local_138)[local_70];
      if (((0x19 < local_8c % 0x32) && (local_70 == local_7c)) &&
         ((int)local_6c < local_108[local_70])) {
        local_6c = local_6c + 1;
      }
      puVar7 = puVar8;
      if (local_6c != 0) {
        *(undefined4 *)(puVar8 + -4) = 0;
        *(int *)(puVar8 + -8) =
             *(int *)(local_60 + 0x38) +
             *(int *)((local_120[local_70] + local_6c) * 4 + *(int *)(local_60 + 0x34) + -4);
        puVar7 = puVar8 + -0xc;
        *(undefined4 *)(puVar8 + -0xc) =
             *(undefined4 *)(*(int *)(local_60 + 8) + (local_120[local_70] + local_6c) * 4 + -4);
        uVar1 = *(undefined4 *)((local_120[local_70] + local_6c) * 4 + *(int *)(local_60 + 4) + -4);
        uVar2 = local_d8[local_70];
        uVar3 = local_f0[local_70];
        *(undefined4 *)(puVar8 + -0x10) = 0x39943;
        FUN_0003bb42(DAT_000633b0,uVar3,uVar2,uVar1);
      }
      puVar8 = puVar7;
      if ((local_70 < 2) && (local_68 != 0)) {
        *(undefined4 *)(puVar7 + -4) = 0;
        *(int *)(puVar7 + -8) =
             *(int *)(local_60 + 0x38) + *(int *)(local_68 * 4 + *(int *)(local_60 + 0x34) + 0x30);
        puVar8 = puVar7 + -0xc;
        *(undefined4 *)(puVar7 + -0xc) =
             *(undefined4 *)(*(int *)(local_60 + 8) + local_68 * 4 + 0x30);
        uVar1 = *(undefined4 *)(*(int *)(local_60 + 4) + local_68 * 4 + 0x30);
        *(undefined4 *)(puVar7 + -0x10) = 0x399a2;
        FUN_0003bb42(DAT_000633b0,0x8a,local_70 * 0x19 + 0x38,uVar1);
      }
    }
    iVar5 = local_8c / 4;
    if (local_8c % 4 == 0) {
      *(undefined4 *)(puVar8 + -4) = 0x399cf;
      FUN_0003d67c(&DAT_000633b8,0x40,0x4f);
      *(undefined4 *)(puVar8 + -4) = 0x399e3;
      iVar5 = FUN_0003d67c(&DAT_000633b8,0x50,0x5f);
    }
    *(undefined4 *)(puVar8 + -4) = 0x399e8;
    FUN_00035ec2(iVar5);
    *(undefined **)(puVar8 + -4) = &DAT_000633b8;
    *(undefined4 *)(puVar8 + -8) = 0x100;
    *(undefined4 *)(puVar8 + -0xc) = 0;
    *(undefined4 *)(puVar8 + -0x10) = 0x399fc;
    FUN_00040042();
    *(undefined4 *)(puVar8 + -4) = 0x39a14;
    FUN_00041e6c(&DAT_000a0000,DAT_000633b0,64000);
    local_8c = local_8c + 1;
    iVar5 = extraout_EDX_04;
    puVar6 = (undefined4 *)puVar8;
    if ((local_20 == 1) && (iVar5 = DAT_00063e38 % 0x1ea, DAT_00053c48 < DAT_00063e38 / 0x1ea)) {
      DAT_00063e38 = 0;
      *(undefined4 *)(puVar8 + -4) = 0x39a51;
      FUN_00010b1d(0xb);
      iVar5 = extraout_EDX_05;
    }
  }
  if (*(byte *)(DAT_00053c7c + 0x27) != local_138) {
    DAT_00062a98 = (&DAT_00062aa0)[local_138];
    DAT_00062a9c = (&DAT_00062ab4)[local_138];
    *(undefined1 *)(DAT_00053c7c + 0x27) = (undefined1)local_138;
  }
  *(undefined1 *)(DAT_00053c7c + 0x28) = (undefined1)local_134;
  if (*(byte *)(DAT_00053c7c + 0x26) != local_130) {
    *(undefined1 *)(DAT_00053c7c + 0x26) = (undefined1)local_130;
    DAT_00053818 = (&DAT_00053c28)[local_130];
    *(undefined4 *)(DAT_00053c7c + 0x123) = DAT_00053818;
  }
  *(undefined1 *)(DAT_00053c7c + 0x25) = (undefined1)local_12c;
  *(undefined4 *)((int)puVar6 + -4) = 0x39b16;
  FUN_0004185e(DAT_00053ddc);
  *(undefined4 *)((int)puVar6 + -4) = 0x39b25;
  FUN_000406f5(&DAT_000633b8,0x32);
  uVar1 = local_140;
  do {
    do {
    } while (DAT_00063e89 != '\0');
  } while ((DAT_00063eb2 != '\0') || (DAT_00063ebe != '\0'));
  *(undefined4 *)((int)puVar6 + -4) = 0x39b4d;
  FUN_00041c31(uVar1);
  iVar5 = local_80;
  local_38 = local_80;
  if (local_80 == 0) {
    local_154 = 0;
  }
  else {
    *(undefined4 *)((int)puVar6 + -4) = 0x39b63;
    FUN_0003d999(iVar5,0);
    *(undefined4 *)((int)puVar6 + -4) = 0x39b68;
    local_154 = FUN_00041c28();
  }
  iVar5 = local_13c;
  local_34 = local_13c;
  if (local_13c == 0) {
    local_158 = 0;
  }
  else {
    *(undefined4 *)((int)puVar6 + -4) = 0x39b93;
    FUN_0003d999(iVar5,0);
    *(undefined4 *)((int)puVar6 + -4) = 0x39b98;
    local_158 = FUN_00041c28();
  }
  iVar5 = local_60;
  local_30 = local_60;
  if (local_60 == 0) {
    local_15c = 0;
  }
  else {
    *(undefined4 *)((int)puVar6 + -4) = 0x39bc0;
    FUN_0003d999(iVar5,0);
    *(undefined4 *)((int)puVar6 + -4) = 0x39bc5;
    local_15c = FUN_00041c28();
  }
  local_80 = 0;
  local_13c = 0;
  local_60 = 0;
  if (local_20 == 1) {
    *(undefined4 *)((int)puVar6 + -4) = 0x39bfa;
    FUN_00010dde();
  }
  return;
}



// === drip @ 0003d67c ===

void FUN_0003d67c(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined1 uVar1;
  undefined1 uVar2;
  undefined1 uVar3;
  int iVar4;
  int iVar5;
  undefined8 uVar6;
  undefined4 local_30;
  
  uVar6 = FUN_000418ee();
  iVar5 = (int)((ulonglong)uVar6 >> 0x20);
  iVar4 = (int)uVar6;
  uVar1 = *(undefined1 *)(param_3 * 3 + iVar4);
  uVar2 = *(undefined1 *)(param_3 * 3 + iVar4 + 1);
  uVar3 = *(undefined1 *)(param_3 * 3 + iVar4 + 2);
  while (local_30 = param_3 + -1, iVar5 <= local_30) {
    *(undefined1 *)(param_3 * 3 + iVar4) = *(undefined1 *)(local_30 * 3 + iVar4);
    *(undefined1 *)(param_3 * 3 + iVar4 + 1) = *(undefined1 *)(local_30 * 3 + iVar4 + 1);
    *(undefined1 *)(param_3 * 3 + iVar4 + 2) = *(undefined1 *)(local_30 * 3 + iVar4 + 2);
    param_3 = local_30;
  }
  *(undefined1 *)(iVar5 * 3 + iVar4) = uVar1;
  *(undefined1 *)(iVar5 * 3 + iVar4 + 1) = uVar2;
  *(undefined1 *)(iVar5 * 3 + iVar4 + 2) = uVar3;
  return;
}



// === stg_load @ 00037b03 ===

ulonglong FUN_00037b03(void)

{
  int iVar1;
  undefined4 uVar2;
  ulonglong uVar3;
  undefined4 local_80;
  undefined4 local_7c;
  char *local_6c;
  undefined1 local_68 [20];
  undefined4 local_54;
  int local_50;
  int local_4c;
  undefined4 local_48;
  int local_44;
  int local_40;
  undefined4 local_3c;
  undefined4 local_38;
  int local_30;
  int local_2c;
  undefined4 local_28;
  char *local_24;
  int local_20;
  
  uVar3 = FUN_000418ee();
  local_20 = (int)uVar3;
  DAT_00062a8c = 0;
  DAT_00062a90 = 0;
  FUN_00041e20(&DAT_0005d08c,0,0x14);
  DAT_00053c80 = 0;
  if (local_20 == 0) {
    DAT_00063358 = 0;
    FUN_000428f8();
    local_50 = FUN_00041bdc(0x354);
    local_4c = local_50;
    if (local_50 == 0) {
      local_3c = 0;
    }
    else {
      local_54 = FUN_0003c6ec(local_50);
      local_3c = local_54;
    }
    DAT_0006334c = local_3c;
    FUN_000367c9(local_68);
    DAT_0006335c = 0;
    FUN_0004192e(&DAT_000639d8,1,4,DAT_000639b8);
    DAT_000639e8 = 0;
    FUN_0003c853(DAT_0006334c,DAT_000639b8);
    FUN_0003cad3(DAT_0006334c);
    FUN_000428f8();
    local_44 = FUN_00041bdc(0x3c);
    local_40 = local_44;
    if (local_44 == 0) {
      local_38 = 0;
    }
    else {
      local_48 = FUN_0003d8d4(local_44,local_68);
      local_38 = local_48;
    }
    DAT_00063dfc = local_38;
  }
  FUN_000428f8();
  FUN_000367c9(local_68);
  FUN_0004192e(&DAT_00063e14,1,4,DAT_000639b8);
  DAT_00062a90 = 0;
  for (local_30 = 0; local_30 < DAT_00063e14; local_30 = local_30 + 1) {
    iVar1 = FUN_00041bdc(0x59);
    if (iVar1 == 0) {
      local_6c = (char *)0x0;
    }
    else {
      local_6c = (char *)FUN_0003e0a0(iVar1,DAT_000639b8);
    }
    local_24 = local_6c;
    if (*local_6c == '\x01') {
      local_2c = *(int *)(local_6c + 1);
      if (*(int *)(&DAT_000639ec + local_2c * 4) == 0) {
        local_28 = FUN_00042928(DAT_000639b8);
        iVar1 = FUN_00041bdc(0x3c);
        if (iVar1 == 0) {
          local_80 = 0;
        }
        else {
          local_80 = FUN_0003d8d4(iVar1,(&PTR_s_gun_spr_00051d88)[local_2c]);
        }
        *(undefined4 *)(&DAT_000639ec + local_2c * 4) = local_80;
        iVar1 = FUN_00041bdc(8);
        if (iVar1 == 0) {
          local_7c = 0;
        }
        else {
          local_7c = FUN_00010010(iVar1,(&PTR_s_gun_act_00052198)[local_2c]);
        }
        (&DAT_00053818)[local_2c] = local_7c;
        FUN_0004247a(DAT_000639b8,local_28);
      }
    }
    else if (*local_6c == '\x05') {
      (&DAT_0005d0b4)[DAT_00062a90 * 5] = *(undefined4 *)(local_6c + 1);
      (&DAT_0005d0b8)[DAT_00062a90 * 5] = *(undefined4 *)(local_6c + 9);
      (&DAT_0005d0bc)[DAT_00062a90 * 5] = *(undefined4 *)(local_6c + 0xd);
      (&DAT_0005d0c0)[DAT_00062a90 * 5] = *(undefined4 *)(local_6c + 5);
      (&DAT_0005d0c4)[DAT_00062a90 * 5] = 0;
      DAT_00062a90 = DAT_00062a90 + 1;
      *local_6c = '\0';
    }
  }
  FUN_000428f8();
  FUN_000367c9(local_68);
  FUN_00036853(local_68,2,0);
  for (local_30 = 0; local_30 < 0x65; local_30 = local_30 + 1) {
    FUN_0004192e(&DAT_00053e9c + local_30 * 0x22,1,2,DAT_000639b8);
    uVar2 = FUN_00041c8f(DAT_000639b8);
    *(undefined4 *)(&DAT_00053ea0 + local_30 * 0x88) = uVar2;
    FUN_0004192e(&DAT_00053ea4 + local_30 * 0x44,1,2,DAT_000639b8);
    FUN_0004192e(&DAT_00053ea8 + local_30 * 0x88,1,2,DAT_000639b8);
    uVar2 = FUN_00041c8f(DAT_000639b8);
    *(undefined4 *)(&DAT_00053eac + local_30 * 0x88) = uVar2;
    FUN_0004192e(&DAT_00053eb0 + local_30 * 0x88,1,2,DAT_000639b8);
    FUN_0004192e(&DAT_00053eb4 + local_30 * 0x88,1,2,DAT_000639b8);
    FUN_0004192e(&DAT_00053eb8 + local_30 * 0x88,10,10,DAT_000639b8);
    FUN_0004192e(&DAT_00053f1c + local_30 * 0x88,1,2,DAT_000639b8);
    FUN_0004192e(&DAT_00053f20 + local_30 * 0x88,1,2,DAT_000639b8);
  }
  return uVar3 & 0xffffffff00000000;
}



// === player_mk @ 00010efc ===

undefined1 *
FUN_00010efc(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined1 param_5)

{
  undefined1 *puVar1;
  int iVar2;
  undefined4 extraout_ECX;
  undefined8 uVar3;
  int in_stack_00000000;
  
  uVar3 = FUN_000418ee();
  puVar1 = (undefined1 *)uVar3;
  if (in_stack_00000000 == DAT_00063370) {
    DAT_00053c80 = puVar1;
  }
  *puVar1 = param_5;
  *(int *)(puVar1 + 1) = (int)((ulonglong)uVar3 >> 0x20);
  *(undefined4 *)(puVar1 + 5) = param_3;
  *(undefined4 *)(puVar1 + 9) = extraout_ECX;
  *(int *)(puVar1 + 0xd) = in_stack_00000000;
  *(undefined4 *)(puVar1 + 0x11) = 0;
  puVar1[0x15] = 0;
  puVar1[0x16] = 0;
  *(undefined4 *)(puVar1 + 0x17) = 0;
  *(undefined4 *)(puVar1 + 0x1b) = 0;
  *(undefined4 *)(puVar1 + 0x1f) = 0;
  puVar1[0x25] = (&DAT_00053ea0)[*(int *)(puVar1 + 0xd) * 0x88];
  puVar1[0x26] = 0;
  puVar1[0x27] = 0;
  puVar1[0x28] = 0;
  *(undefined2 *)(puVar1 + 0x29) = (&DAT_00053ea4)[*(int *)(puVar1 + 0xd) * 0x44];
  *(undefined4 *)(puVar1 + 0x2b) = 0;
  puVar1[0x30] = 0;
  puVar1[0x31] = 0;
  FUN_00041e20(puVar1 + 0x32,0,0x14);
  *(undefined4 *)(puVar1 + 0x46) = 0;
  *(undefined4 *)(puVar1 + 0x4a) = 0;
  puVar1[0x4e] = 0;
  puVar1[0x4f] = 0;
  *(undefined2 *)(puVar1 + 0x50) = 0;
  *(undefined4 *)(puVar1 + 0x52) = 0;
  *(undefined4 *)(puVar1 + 0x56) = 0;
  *(undefined2 *)(puVar1 + 0x23) = 0;
  puVar1[0x5a] = 0;
  if (in_stack_00000000 == 0) {
    *(undefined4 *)(puVar1 + 0x56) = (&DAT_00053e9c)[*(int *)(puVar1 + 0xd) * 0x22];
  }
  else {
    *(undefined2 *)(puVar1 + 0x23) = *(undefined2 *)(&DAT_00053e9c + *(int *)(puVar1 + 0xd) * 0x22);
    if ((0 < in_stack_00000000) && (in_stack_00000000 < 10)) {
      *(undefined4 *)(puVar1 + 0x56) = 0x30c;
    }
  }
  *(undefined4 *)(puVar1 + 0x123) = (&DAT_00053818)[*(int *)(puVar1 + 0xd)];
  *(undefined4 *)(puVar1 + 0x127) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 0x38);
  *(undefined4 *)(puVar1 + 299) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 4);
  *(undefined4 *)(puVar1 + 0x12f) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 8);
  *(undefined4 *)(puVar1 + 0x133) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 0x34);
  *(undefined4 *)(puVar1 + 0x137) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 0xc);
  *(undefined4 *)(puVar1 + 0x13b) =
       *(undefined4 *)(*(int *)(&DAT_000639ec + *(int *)(puVar1 + 0xd) * 4) + 0x10);
  if (DAT_00053c70 == (undefined1 *)0x0) {
    DAT_00053c70 = puVar1;
    DAT_00053c74 = puVar1;
    *(undefined4 *)(puVar1 + 0x143) = 0;
    *(undefined4 *)(puVar1 + 0x13f) = 0;
  }
  else {
    *(undefined1 **)((int)DAT_00053c74 + 0x13f) = puVar1;
    *(undefined1 **)(puVar1 + 0x143) = DAT_00053c74;
    *(undefined4 *)(puVar1 + 0x13f) = 0;
    DAT_00053c74 = puVar1;
  }
  DAT_00053c68 = DAT_00053c68 + 1;
  *(undefined4 *)(puVar1 + 0x8b) = 0;
  *(undefined4 *)(puVar1 + 0x8f) = 0;
  *(undefined4 *)(puVar1 + 0xab) = 0;
  *(undefined4 *)(puVar1 + 0xaf) = 4;
  *(undefined4 *)(puVar1 + 0xbb) = 0;
  *(undefined4 *)(puVar1 + 199) = 1;
  *(undefined4 *)(puVar1 + 99) = 0;
  *(undefined4 *)(puVar1 + 0xff) = 0xffffffff;
  *(undefined4 *)(puVar1 + 0x5f) = 0;
  *(undefined4 *)(puVar1 + 0x6b) = 0;
  *(undefined4 *)(puVar1 + 0x6f) = 0;
  *(undefined4 *)(puVar1 + 0x113) = 0;
  *(undefined4 *)(puVar1 + 0xd3) = 0;
  *(int *)(puVar1 + 0x73) =
       *(int *)(puVar1 + 0x56) + (int)*(short *)(puVar1 + 0x23) + (int)*(short *)(puVar1 + 0x29);
  *(undefined4 *)(puVar1 + 0x77) = *(undefined4 *)(puVar1 + 0x73);
  *(undefined4 *)(puVar1 + 0x7b) = 0;
  *(undefined4 *)(puVar1 + 0x7f) = 0;
  *(undefined4 *)(puVar1 + 0x10b) = 0;
  *(undefined4 *)(puVar1 + 0x107) = *(undefined4 *)(puVar1 + 0x10b);
  *(undefined4 *)(puVar1 + 0x103) = *(undefined4 *)(puVar1 + 0x107);
  iVar2 = FUN_00041e9f();
  *(int *)(puVar1 + 0x11f) = iVar2 % 0x14;
  return puVar1;
}



// === fx_upd @ 0003e20f ===

void FUN_0003e20f(void)

{
  int iVar1;
  undefined1 *puVar2;
  int iVar3;
  int local_c0;
  int local_b0;
  int local_a0;
  int local_90;
  int local_80;
  int local_70;
  int local_60;
  int local_50;
  int local_40;
  int local_2c;
  
  puVar2 = (undefined1 *)FUN_000418ee();
  switch(*puVar2) {
  case 1:
    iVar3 = *(int *)(puVar2 + 9);
    iVar1 = *(int *)(puVar2 + 0xd);
    if (*(char *)((int)DAT_00063354 + 0x11) == '\0') {
      if (*(int *)(puVar2 + 0x11) == 1) {
        if ((((DAT_0005d08c <= iVar3) && (iVar3 <= DAT_0005d094)) && (DAT_0005d090 <= iVar1)) &&
           (iVar1 <= DAT_0005d098)) {
          if (*(int *)(puVar2 + 5) == 0) {
            iVar3 = FUN_00041bdc(0x147);
            if (iVar3 == 0) {
              local_50 = 0;
            }
            else {
              local_50 = FUN_00010efc(iVar3,DAT_000639dc + -0x1e,0,*(undefined4 *)(puVar2 + 0xd),
                                      *(undefined4 *)(puVar2 + 1),1);
            }
            local_40 = local_50;
          }
          else {
            iVar3 = FUN_00041bdc(0x147);
            if (iVar3 == 0) {
              local_60 = 0;
            }
            else {
              local_60 = FUN_00010efc(iVar3,DAT_000639dc + 0x15e,0,*(undefined4 *)(puVar2 + 0xd),
                                      *(undefined4 *)(puVar2 + 1),1);
            }
            local_40 = local_60;
          }
          *(undefined4 *)(local_40 + 0x1b) = *(undefined4 *)(puVar2 + 5);
          *puVar2 = 0;
        }
      }
      else if (DAT_00053c68 <
               *(int *)(&DAT_000528b4 + DAT_00063354[0x10] * 4 + (DAT_00063370 + -1) * 0x28)) {
        if (*(int *)(puVar2 + 5) == 0) {
          if (((DAT_000639e0 <= *(int *)(puVar2 + 0xd)) &&
              (*(int *)(puVar2 + 0xd) <= DAT_000639e0 + 200)) &&
             ((*DAT_00063354 <= *(int *)(puVar2 + 9) && (*(int *)(puVar2 + 9) <= DAT_00063354[2]))))
          {
            if (*(int *)(puVar2 + 0x15) == 1) {
              local_2c = *(int *)(puVar2 + 9);
            }
            else {
              if (DAT_000639dc + -0x32 < *(int *)(puVar2 + 9)) {
                return;
              }
              local_2c = DAT_000639dc + -0x1e;
            }
            iVar3 = FUN_00041bdc(0x147);
            if (iVar3 == 0) {
              local_70 = 0;
            }
            else {
              local_70 = FUN_00010efc(iVar3,local_2c,0,*(undefined4 *)(puVar2 + 0xd),
                                      *(undefined4 *)(puVar2 + 1),1);
            }
            *(undefined4 *)(local_70 + 0x1b) = *(undefined4 *)(puVar2 + 5);
            if (*(int *)(puVar2 + 0x15) == 1) {
              *(undefined4 *)(local_70 + 0x17) = 3;
            }
            *puVar2 = 0;
          }
        }
        else if (((((DAT_000639e0 <= *(int *)(puVar2 + 0xd)) &&
                   (*(int *)(puVar2 + 0xd) <= DAT_000639e0 + 200)) &&
                  (*DAT_00063354 <= *(int *)(puVar2 + 9))) &&
                 ((*(int *)(puVar2 + 9) <= DAT_00063354[2] && (DAT_000639dc <= *(int *)(puVar2 + 9))
                  ))) && (*(int *)(puVar2 + 9) <= DAT_000639dc + 0x172)) {
          if (*(int *)(puVar2 + 0x15) == 1) {
            local_2c = *(int *)(puVar2 + 9);
          }
          else {
            local_2c = DAT_000639dc + 0x15e;
          }
          iVar3 = FUN_00041bdc(0x147);
          if (iVar3 == 0) {
            local_80 = 0;
          }
          else {
            local_80 = FUN_00010efc(iVar3,local_2c,0,*(undefined4 *)(puVar2 + 0xd),
                                    *(undefined4 *)(puVar2 + 1),1);
          }
          *(undefined4 *)(local_80 + 0x1b) = *(undefined4 *)(puVar2 + 5);
          if (*(int *)(puVar2 + 0x15) == 1) {
            *(undefined4 *)(local_80 + 0x17) = 3;
          }
          *puVar2 = 0;
        }
      }
    }
    else if (*(int *)(puVar2 + 0x11) == 1) {
      if (((DAT_0005d08c <= iVar3) && (iVar3 <= DAT_0005d094)) &&
         ((DAT_0005d090 <= iVar1 && (iVar1 <= DAT_0005d098)))) {
        if (*(int *)(puVar2 + 5) == 0) {
          iVar3 = FUN_00041bdc(0x147);
          if (iVar3 == 0) {
            local_90 = 0;
          }
          else {
            local_90 = FUN_00010efc(iVar3,DAT_000639dc + -0x1e,0,*(undefined4 *)(puVar2 + 0xd),
                                    *(undefined4 *)(puVar2 + 1),1);
          }
          local_40 = local_90;
        }
        else {
          iVar3 = FUN_00041bdc(0x147);
          if (iVar3 == 0) {
            local_a0 = 0;
          }
          else {
            local_a0 = FUN_00010efc(iVar3,DAT_000639dc + 0x15e,0,*(undefined4 *)(puVar2 + 0xd),
                                    *(undefined4 *)(puVar2 + 1),1);
          }
          local_40 = local_a0;
        }
        *(undefined4 *)(local_40 + 0x1b) = *(undefined4 *)(puVar2 + 5);
        *puVar2 = 0;
      }
    }
    else if (DAT_00053c68 <
             *(int *)(&DAT_000528b4 + DAT_00063354[0x10] * 4 + (DAT_00063370 + -1) * 0x28)) {
      if (*(int *)(puVar2 + 5) == 1) {
        if (((DAT_000639e0 <= *(int *)(puVar2 + 0xd)) &&
            (*(int *)(puVar2 + 0xd) <= DAT_000639e0 + 200)) &&
           ((*DAT_00063354 <= *(int *)(puVar2 + 9) && (*(int *)(puVar2 + 9) <= DAT_00063354[2])))) {
          if (*(int *)(puVar2 + 0x15) == 1) {
            local_2c = *(int *)(puVar2 + 9);
          }
          else {
            if (DAT_000639dc + 0x172 < *(int *)(puVar2 + 9)) {
              return;
            }
            local_2c = DAT_000639dc + 0x15e;
          }
          iVar3 = FUN_00041bdc(0x147);
          if (iVar3 == 0) {
            local_b0 = 0;
          }
          else {
            local_b0 = FUN_00010efc(iVar3,local_2c,0,*(undefined4 *)(puVar2 + 0xd),
                                    *(undefined4 *)(puVar2 + 1),1);
          }
          *(undefined4 *)(local_b0 + 0x1b) = *(undefined4 *)(puVar2 + 5);
          if (*(int *)(puVar2 + 0x15) == 1) {
            *(undefined4 *)(local_b0 + 0x17) = 3;
          }
          *puVar2 = 0;
        }
      }
      else if (((DAT_000639e0 <= *(int *)(puVar2 + 0xd)) &&
               (*(int *)(puVar2 + 0xd) <= DAT_000639e0 + 200)) &&
              ((*DAT_00063354 <= *(int *)(puVar2 + 9) &&
               (((*(int *)(puVar2 + 9) <= DAT_00063354[2] &&
                 (DAT_000639dc + -0x32 <= *(int *)(puVar2 + 9))) &&
                (*(int *)(puVar2 + 9) <= DAT_000639dc + 0x140)))))) {
        if (*(int *)(puVar2 + 0x15) == 1) {
          local_2c = *(int *)(puVar2 + 9);
        }
        else {
          local_2c = DAT_000639dc + -0x1e;
        }
        iVar3 = FUN_00041bdc(0x147);
        if (iVar3 == 0) {
          local_c0 = 0;
        }
        else {
          local_c0 = FUN_00010efc(iVar3,local_2c,0,*(undefined4 *)(puVar2 + 0xd),
                                  *(undefined4 *)(puVar2 + 1),1);
        }
        *(undefined4 *)(local_c0 + 0x1b) = *(undefined4 *)(puVar2 + 5);
        if (*(int *)(puVar2 + 0x15) == 1) {
          *(undefined4 *)(local_c0 + 0x17) = 3;
        }
        *puVar2 = 0;
      }
    }
    break;
  case 2:
    switch(*(undefined4 *)(puVar2 + 5)) {
    case 1:
      if (((DAT_000639e0 < *(int *)(puVar2 + 0xd)) && (*(int *)(puVar2 + 0xd) < DAT_000639e0 + 200))
         && (*(int *)(puVar2 + 9) < DAT_000639dc)) {
        *(undefined4 *)((int)DAT_00063354 + 0x2a) = *(undefined4 *)(puVar2 + 9);
        *puVar2 = 0;
      }
      break;
    case 2:
      if (((DAT_000639e0 < *(int *)(puVar2 + 0xd)) && (*(int *)(puVar2 + 0xd) < DAT_000639e0 + 200))
         && (DAT_000639dc < *(int *)(puVar2 + 9))) {
        *(undefined4 *)((int)DAT_00063354 + 0x2e) = *(undefined4 *)(puVar2 + 9);
        *puVar2 = 0;
      }
      break;
    case 3:
      if (((DAT_000639e0 < *(int *)(puVar2 + 0xd)) && (*(int *)(puVar2 + 0xd) < DAT_000639e0 + 200))
         && ((DAT_000639dc < *(int *)(puVar2 + 9) && (*(int *)(puVar2 + 9) < DAT_000639dc + 0x140)))
         ) {
        *(undefined4 *)((int)DAT_00063354 + 0x32) = *(undefined4 *)(puVar2 + 9);
        *puVar2 = 0;
      }
      break;
    case 4:
      if (((DAT_000639e0 < *(int *)(puVar2 + 0xd)) && (*(int *)(puVar2 + 0xd) < DAT_000639e0 + 200))
         && ((DAT_000639dc < *(int *)(puVar2 + 9) && (*(int *)(puVar2 + 9) < DAT_000639dc + 0x140)))
         ) {
        *(undefined4 *)((int)DAT_00063354 + 0x36) = *(undefined4 *)(puVar2 + 9);
        *puVar2 = 0;
      }
      break;
    case 5:
      if (((DAT_000639e0 < *(int *)(puVar2 + 0xd)) && (*(int *)(puVar2 + 0xd) < DAT_000639e0 + 200))
         && (DAT_000639dc < *(int *)(puVar2 + 9))) {
        *(undefined1 *)((int)DAT_00063354 + 0x3a) = 1;
        *puVar2 = 0;
      }
      break;
    case 6:
      if (((DAT_000639e0 < *(int *)(puVar2 + 0xd)) && (*(int *)(puVar2 + 0xd) < DAT_000639e0 + 200))
         && (DAT_000639dc < *(int *)(puVar2 + 9))) {
        *(undefined1 *)((int)DAT_00063354 + 0x3b) = 1;
        *puVar2 = 0;
      }
    }
    break;
  case 3:
    *(undefined4 *)(&DAT_0005c8bc + DAT_00062a8c * 0x14) = *(undefined4 *)(puVar2 + 9);
    *(undefined4 *)(&DAT_0005c8c0 + DAT_00062a8c * 0x14) = *(undefined4 *)(puVar2 + 0xd);
    *(undefined4 *)(&DAT_0005c8c4 + DAT_00062a8c * 0x14) = *(undefined4 *)(puVar2 + 0x11);
    *(undefined4 *)(&DAT_0005c8c8 + DAT_00062a8c * 0x14) = *(undefined4 *)(puVar2 + 0x15);
    DAT_00062a8c = DAT_00062a8c + 1;
    *puVar2 = 0;
    break;
  case 4:
    if (DAT_00053c68 == 1) {
      iVar3 = *(int *)(puVar2 + 9);
      iVar1 = *(int *)(puVar2 + 0xd);
      if (((DAT_000639dc <= iVar3) && (iVar3 <= DAT_000639dc + 0x13f)) &&
         ((DAT_000639e0 <= iVar1 && (iVar1 <= DAT_000639e0 + 199)))) {
        DAT_0005d08c = iVar3;
        DAT_0005d090 = iVar1;
        DAT_0005d094 = *(int *)(puVar2 + 0x11);
        DAT_0005d098 = *(int *)(puVar2 + 0x15);
        *puVar2 = 0;
      }
    }
    break;
  default:
    break;
  case 6:
    if ((((DAT_000639dc <= *(int *)(puVar2 + 9)) && (*(int *)(puVar2 + 9) <= DAT_000639dc + 0x13f))
        && (DAT_000639e0 <= *(int *)(puVar2 + 0xd))) &&
       (*(int *)(puVar2 + 0xd) <= DAT_000639e0 + 199)) {
      DAT_0005d0a0 = *(undefined4 *)(puVar2 + 9);
      DAT_0005d0a4 = *(undefined4 *)(puVar2 + 0xd);
      DAT_0005d0a8 = *(undefined4 *)(puVar2 + 0x11);
      DAT_0005d0ac = *(undefined4 *)(puVar2 + 0x15);
      DAT_0005d0b0 = *(undefined4 *)(puVar2 + 0x19);
      DAT_00062a8c = DAT_00062a8c + 1;
      *puVar2 = 0;
    }
  }
  return;
}



// === ent_draw @ 00012efa ===

void FUN_00012efa(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined8 uVar7;
  byte *pbVar8;
  undefined4 uVar9;
  int *piVar10;
  uint *puVar11;
  uint *puVar12;
  int iStack_38;
  int iStack_34;
  uint uStack_30;
  undefined4 uStack_2c;
  uint local_28;
  int *local_24;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  uVar9 = (undefined4)((ulonglong)local_20 >> 0x20);
  pbVar8 = (byte *)local_20;
  puVar12 = &local_28;
  puVar11 = &local_28;
  if (((pbVar8[0x30] != 1) || (5 < DAT_00063360 % 0x17)) || (1 < *pbVar8)) {
    local_24 = *(int **)(*(int *)(*(int *)(pbVar8 + 0x123) + 4) + *(int *)(pbVar8 + 0x17) * 4);
    local_28 = (uint)*(ushort *)
                      ((*(int *)(pbVar8 + 0x11) % *local_24) * 2 + *(int *)((int)local_24 + 0x6d));
    if ((pbVar8[0x30] == 2) && (DAT_00063360 % 0x14 == 0)) {
      if (*(int *)(pbVar8 + 0x1b) == 0) {
        uStack_2c = 0x9c;
        uStack_30 = (uint)pbVar8[0x1b];
        iStack_34 = *(int *)(pbVar8 + 0x127) + *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x133));
        iStack_38 = *(int *)(*(int *)(pbVar8 + 0x12f) + local_28 * 4);
        FUN_0003bebb(uVar9,(*(int *)(pbVar8 + 1) - *(int *)(*(int *)(pbVar8 + 0x137) + local_28 * 4)
                           ) - DAT_000639dc,
                     ((*(int *)(pbVar8 + 5) + *(int *)(pbVar8 + 9)) -
                     *(int *)(*(int *)(pbVar8 + 0x13b) + local_28 * 4)) - DAT_000639e0,
                     *(undefined4 *)(*(int *)(pbVar8 + 299) + local_28 * 4));
      }
      else {
        uStack_2c = 0x9c;
        uStack_30 = (uint)pbVar8[0x1b];
        iStack_34 = *(int *)(pbVar8 + 0x127) + *(int *)(*(int *)(pbVar8 + 0x133) + local_28 * 4);
        iStack_38 = *(int *)(*(int *)(pbVar8 + 0x12f) + local_28 * 4);
        FUN_0003bebb(uVar9,(*(int *)(pbVar8 + 1) -
                           (*(int *)(local_28 * 4 + *(int *)(pbVar8 + 299)) -
                           *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x137)))) - DAT_000639dc,
                     ((*(int *)(pbVar8 + 5) + *(int *)(pbVar8 + 9)) -
                     *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x13b))) - DAT_000639e0,
                     *(undefined4 *)(local_28 * 4 + *(int *)(pbVar8 + 299)));
      }
    }
    else if (*(int *)(pbVar8 + 0x1b) == 0) {
      uVar7 = local_20;
      if ((char)local_24[0x1b] == '\x01') {
        uStack_2c = 0xd0;
        uStack_30 = (uint)pbVar8[0x1b];
        iStack_34 = *(int *)(pbVar8 + 0x127) + *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x133));
        piVar10 = (int *)(*(int *)(pbVar8 + 0x12f) + local_28 * 4);
        iStack_38 = *piVar10 - (*piVar10 >> 0x1f) >> 1;
        puVar11 = (uint *)&iStack_38;
        piVar10 = (int *)(*(int *)(pbVar8 + 0x12f) + local_28 * 4);
        FUN_0003c231(uVar9,(*(int *)(pbVar8 + 1) - *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x137))
                           ) - DAT_000639dc,
                     (((*(int *)(pbVar8 + 9) + (*piVar10 - (*piVar10 >> 0x1f) >> 1)) -
                      *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x13b))) - DAT_000639e0) +
                     (*(int *)(pbVar8 + 5) - (*(int *)(pbVar8 + 5) >> 0x1f) >> 1),
                     *(undefined4 *)(*(int *)(pbVar8 + 299) + local_28 * 4));
        uVar7 = local_20;
      }
      local_20._4_4_ = (undefined4)((ulonglong)uVar7 >> 0x20);
      local_20._0_4_ = (int)uVar7;
      *(uint *)((int)puVar11 + -4) = (uint)*(byte *)((int)local_20 + 0x1b);
      *(int *)((int)puVar11 + -8) =
           *(int *)((int)local_20 + 0x127) +
           *(int *)(*(int *)((int)local_20 + 0x133) + local_28 * 4);
      *(undefined4 *)((int)puVar11 + -0xc) =
           *(undefined4 *)(*(int *)((int)local_20 + 0x12f) + local_28 * 4);
      uVar9 = *(undefined4 *)(*(int *)((int)local_20 + 299) + local_28 * 4);
      iVar1 = *(int *)((int)local_20 + 5);
      iVar2 = *(int *)((int)local_20 + 9);
      iVar3 = *(int *)(local_28 * 4 + *(int *)((int)local_20 + 0x13b));
      iVar4 = *(int *)((int)local_20 + 1);
      iVar5 = *(int *)(local_28 * 4 + *(int *)((int)local_20 + 0x137));
      *(undefined4 *)((int)puVar11 + -0x10) = 0x1315e;
      FUN_0003bb42(local_20._4_4_,(iVar4 - iVar5) - DAT_000639dc,
                   ((iVar1 + iVar2) - iVar3) - DAT_000639e0,uVar9);
    }
    else {
      uVar7 = local_20;
      if ((char)local_24[0x1b] == '\x01') {
        uStack_2c = 0xd0;
        uStack_30 = (uint)pbVar8[0x1b];
        iStack_34 = *(int *)(pbVar8 + 0x127) + *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x133));
        piVar10 = (int *)(local_28 * 4 + *(int *)(pbVar8 + 0x12f));
        iStack_38 = *piVar10 - (*piVar10 >> 0x1f) >> 1;
        puVar12 = (uint *)&iStack_38;
        piVar10 = (int *)(local_28 * 4 + *(int *)(pbVar8 + 0x12f));
        FUN_0003c231(uVar9,(*(int *)(pbVar8 + 1) -
                           (*(int *)(local_28 * 4 + *(int *)(pbVar8 + 299)) -
                           *(int *)(local_28 * 4 + *(int *)(pbVar8 + 0x137)))) - DAT_000639dc,
                     (((*(int *)(pbVar8 + 9) + (*piVar10 - (*piVar10 >> 0x1f) >> 1)) -
                      *(int *)(*(int *)(pbVar8 + 0x13b) + local_28 * 4)) - DAT_000639e0) +
                     (*(int *)(pbVar8 + 5) - (*(int *)(pbVar8 + 5) >> 0x1f) >> 1),
                     *(undefined4 *)(*(int *)(pbVar8 + 299) + local_28 * 4));
        uVar7 = local_20;
      }
      local_20._4_4_ = (undefined4)((ulonglong)uVar7 >> 0x20);
      local_20._0_4_ = (int)uVar7;
      *(uint *)((int)puVar12 + -4) = (uint)*(byte *)((int)local_20 + 0x1b);
      *(int *)((int)puVar12 + -8) =
           *(int *)((int)local_20 + 0x127) +
           *(int *)(*(int *)((int)local_20 + 0x133) + local_28 * 4);
      *(undefined4 *)((int)puVar12 + -0xc) =
           *(undefined4 *)(local_28 * 4 + *(int *)((int)local_20 + 0x12f));
      uVar9 = *(undefined4 *)(*(int *)((int)local_20 + 299) + local_28 * 4);
      iVar1 = *(int *)((int)local_20 + 5);
      iVar2 = *(int *)((int)local_20 + 9);
      iVar3 = *(int *)(local_28 * 4 + *(int *)((int)local_20 + 0x13b));
      iVar4 = *(int *)(local_28 * 4 + *(int *)((int)local_20 + 299));
      iVar5 = *(int *)(local_28 * 4 + *(int *)((int)local_20 + 0x137));
      iVar6 = *(int *)((int)local_20 + 1);
      *(undefined4 *)((int)puVar12 + -0x10) = 0x13316;
      FUN_0003bb42(local_20._4_4_,(iVar6 - (iVar4 - iVar5)) - DAT_000639dc,
                   ((iVar1 + iVar2) - iVar3) - DAT_000639e0,uVar9);
    }
  }
  return;
}



// === scroll @ 0003d75c ===

void FUN_0003d75c(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  float10 fVar4;
  int local_30;
  
  iVar1 = FUN_000418ee();
  for (local_30 = 0; local_30 < DAT_00062a90; local_30 = local_30 + 1) {
    iVar3 = (&DAT_0005d0b4)[local_30 * 5];
    iVar2 = ((&DAT_0005d0b8)[local_30 * 5] - *(int *)(iVar3 * 4 + *(int *)(DAT_00063dfc + 0xc))) -
            DAT_000639dc;
    iVar3 = (((&DAT_0005d0b8)[local_30 * 5] - *(int *)(*(int *)(DAT_00063dfc + 0xc) + iVar3 * 4)) +
            *(int *)(*(int *)(DAT_00063dfc + 4) + iVar3 * 4)) - DAT_000639dc;
    fVar4 = (float10)(float)(&DAT_0005d0c4)[local_30 * 5];
    FUN_00041f7e();
    iVar2 = iVar2 + (int)ROUND(fVar4);
    fVar4 = (float10)(float)(&DAT_0005d0c4)[local_30 * 5];
    FUN_00041f7e();
    iVar3 = iVar3 + (int)ROUND(fVar4);
    if (((-1 < iVar2) && (iVar2 < 0x140)) || ((-1 < iVar3 && (iVar3 < 0x140)))) {
      (&DAT_0005d0c4)[local_30 * 5] =
           (float)(&DAT_0005d0c4)[local_30 * 5] -
           (float)iVar1 *
           *(float *)(&DAT_0005274c + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28
                     );
    }
  }
  return;
}



// === bg_blit @ 00040491 ===

void FUN_00040491(void)

{
  uint uVar1;
  undefined2 *in_stack_00000004;
  int in_stack_00000008;
  uint in_stack_0000000c;
  int in_stack_00000010;
  undefined2 *in_stack_00000014;
  int in_stack_00000018;
  
  do {
    uVar1 = in_stack_0000000c >> 1;
    if ((in_stack_0000000c & 1) != 0) {
      *(undefined1 *)in_stack_00000014 = *(undefined1 *)in_stack_00000004;
      in_stack_00000004 = (undefined2 *)((int)in_stack_00000004 + 1);
      in_stack_00000014 = (undefined2 *)((int)in_stack_00000014 + 1);
    }
    for (; uVar1 != 0; uVar1 = uVar1 - 1) {
      *in_stack_00000014 = *in_stack_00000004;
      in_stack_00000004 = in_stack_00000004 + 1;
      in_stack_00000014 = in_stack_00000014 + 1;
    }
    in_stack_00000004 =
         (undefined2 *)((int)in_stack_00000004 + (in_stack_00000008 - in_stack_0000000c));
    in_stack_00000014 =
         (undefined2 *)((int)in_stack_00000014 + (in_stack_00000018 - in_stack_0000000c));
    in_stack_00000010 = in_stack_00000010 + -1;
  } while (in_stack_00000010 != 0);
  return;
}



// === flip @ 000404c6 ===

void FUN_000404c6(void)

{
  undefined4 *in_stack_00000004;
  int in_stack_00000008;
  undefined4 *in_stack_0000000c;
  
  for (; in_stack_00000008 != 0; in_stack_00000008 = in_stack_00000008 + -1) {
    *in_stack_0000000c = *in_stack_00000004;
    in_stack_00000004 = in_stack_00000004 + 1;
    in_stack_0000000c = in_stack_0000000c + 1;
  }
  return;
}



// === ai_t0 @ 00017f13 ===

void FUN_00017f13(void)

{
  int iVar1;
  int iVar2;
  int extraout_ECX;
  int **ppiVar3;
  float10 fVar4;
  undefined8 uVar5;
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
  piStack_84 = (int *)0x17f38;
  local_48 = FUN_0001519f(DAT_00053c7c);
  piStack_84 = (int *)0x17f45;
  local_44 = FUN_000151d5(DAT_00053c7c);
  piStack_84 = (int *)0x17f50;
  local_28 = FUN_0001519f(local_20);
  piStack_84 = (int *)0x17f5b;
  local_24 = FUN_000151d5(local_20);
  if (local_20 == DAT_00053c7c) {
    return;
  }
  local_2c = *(int *)(&DAT_00053f1c + *(int *)(local_20 + 0xd) * 0x88);
  fVar4 = (float10)*(float *)(&DAT_00051d5c + local_2c * 4);
  piStack_84 = (int *)0x17f91;
  FUN_00041f7e();
  local_40 = (int)ROUND(fVar4);
  fVar4 = (float10)*(float *)(local_20 + 99) + (float10)*(float *)(&DAT_00051d5c + local_2c * 4);
  piStack_84 = (int *)0x17fab;
  FUN_00041f7e();
  local_4c = (int)ROUND(fVar4);
  fVar4 = (float10)*(float *)(local_20 + 99);
  piStack_84 = (int *)0x17fb9;
  FUN_00041f7e();
  local_50 = (int)ROUND(fVar4);
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
    piStack_84 = (int *)0x1805e;
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
    if (*(int *)(local_20 + 0x8b) == 0) {
      if (*(int *)(&DAT_00053eac + *(int *)(local_20 + 0xd) * 0x88) == 0) {
        piStack_84 = (int *)0x180f1;
        uVar5 = FUN_0001519f(local_20,(local_48 + -0x41) - local_30);
        if ((int)((ulonglong)uVar5 >> 0x20) <= (int)uVar5) {
          piStack_84 = (int *)0x18108;
          uVar5 = FUN_0001519f(local_20,local_30 + local_48 + 0x41);
          if ((int)uVar5 <= (int)((ulonglong)uVar5 >> 0x20)) {
            piStack_84 = (int *)0x18130;
            uVar5 = FUN_000151d5(local_20,(local_44 + -10) - local_30 % 5);
            if ((int)((ulonglong)uVar5 >> 0x20) <= (int)uVar5) {
              piStack_84 = (int *)0x18154;
              FUN_000151d5(local_20,local_30 % 5 + local_44 + 10);
            }
          }
        }
      }
      local_20[0x8b] = '\x01';
      local_20[0x8c] = '\0';
      local_20[0x8d] = '\0';
      local_20[0x8e] = '\0';
      ppiVar3 = (int **)local_80;
      break;
    }
    if (*(int *)(local_20 + 0xaf) != 4) {
      if (*(int *)(local_20 + 0xaf) == 10) {
        *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb4 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
      }
      else {
        piStack_84 = (int *)0x181a6;
        local_30 = FUN_00041e9f();
        local_30 = local_30 % 0x32;
        *(int *)(local_20 + 0xab) =
             *(int *)(&DAT_00053eb4 + *(int *)(local_20 + 0xd) * 0x88) + local_30;
      }
      local_20[0x1f] = '\x04';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      piStack_84 = (int *)0x181e2;
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
        piStack_84 = (int *)0x18247;
        iVar2 = FUN_0001519f(local_20);
        if (iVar2 < 0) {
LAB_0001825a:
          local_20[0x1f] = '\a';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          return;
        }
        piStack_84 = (int *)0x18253;
        iVar2 = FUN_0001519f(local_20);
        if (0x13f < iVar2) goto LAB_0001825a;
        *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb0 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
        if (*(int *)(local_20 + 199) == 1) {
          do {
            piStack_84 = (int *)0x18299;
            local_7c = FUN_00041e9f();
            local_7c = local_7c % 2;
          } while (local_7c == *(int *)(local_20 + 0xff));
          *(int *)(local_20 + 0xff) = local_7c;
          local_78 = *(undefined4 *)(local_20 + 0xbf);
          local_74 = *(undefined4 *)(local_20 + 0xc3);
          iVar2 = *(int *)(&DAT_00051ca4 + local_7c * 0x10) -
                  *(int *)(&DAT_00051c9c + local_7c * 0x10);
          piStack_84 = (int *)0x1830a;
          iVar1 = FUN_00041e9f(iVar2,local_7c * 0x10,iVar2,
                               *(int *)(&DAT_00051c9c + local_7c * 0x10) + local_48);
          piStack_84 = (int *)0x1831a;
          iVar2 = FUN_00041f76(iVar1 % iVar2);
          *(int *)(local_20 + 0xbf) = extraout_ECX + iVar2;
          *(int *)(local_20 + 0xc3) = local_44;
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
          piStack_84 = (int *)0x18372;
          iVar2 = FUN_0001519f(local_20);
          piStack_84 = (int *)0x18384;
          iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - iVar2);
          if (iVar2 < 10) {
            piStack_84 = (int *)0x18391;
            iVar2 = FUN_000151d5(local_20);
            piStack_84 = (int *)0x183a3;
            iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - iVar2);
            if (iVar2 < 5) {
              *(undefined4 *)(local_20 + 0xbf) = local_78;
              *(undefined4 *)(local_20 + 0xc3) = local_74;
              goto LAB_000183d1;
            }
          }
          local_20[199] = '\0';
          local_20[200] = '\0';
          local_20[0xc9] = '\0';
          local_20[0xca] = '\0';
        }
LAB_000183d1:
        if (*(int *)(local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
          *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
        }
        if (*(int *)(DAT_00063354 + 0x16) <= *(int *)(local_20 + 0xc3)) {
          *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
        }
        piStack_84 = (int *)0x18425;
        iVar2 = FUN_0001519f(local_20);
        if (iVar2 < *(int *)(local_20 + 0xbf)) {
          local_70 = *(int *)(local_20 + 0xbf);
          piStack_84 = (int *)0x18446;
          local_6c = FUN_0001519f(local_20);
        }
        else {
          piStack_84 = (int *)0x18453;
          local_70 = FUN_0001519f(local_20);
          local_6c = *(int *)(local_20 + 0xbf);
        }
        piStack_84 = (int *)0x1846a;
        iVar2 = FUN_000151d5(local_20);
        if (iVar2 < *(int *)(local_20 + 0xc3)) {
          local_68 = *(int *)(local_20 + 0xc3);
          piStack_84 = (int *)0x1848b;
          local_64 = FUN_000151d5(local_20);
        }
        else {
          piStack_84 = (int *)0x18498;
          local_68 = FUN_000151d5(local_20);
          local_64 = *(int *)(local_20 + 0xc3);
        }
        piStack_84 = (int *)0x184af;
        local_60 = FUN_0001519f(local_20);
        piStack_84 = (int *)0x184ba;
        local_5c = FUN_000151d5(local_20);
        piStack_84 = (int *)0x184ce;
        iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - local_60);
        if (iVar2 < 5) {
          piStack_84 = (int *)0x184e4;
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
        piStack_84 = (int *)0x18581;
        iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - local_5c);
        if (iVar2 < 10) {
          piStack_84 = (int *)0x18597;
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
          piStack_84 = (int *)0x18712;
          iVar2 = FUN_0001519f(local_20);
          if (local_48 <= iVar2 + -8) goto LAB_0001872a;
          piStack_84 = (int *)0x18722;
          iVar2 = FUN_0001519f(local_20);
          if (iVar2 + 8 <= local_48) goto LAB_0001872a;
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
LAB_0001872a:
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
      ppiVar3 = (int **)local_80;
    }
    else {
      ppiVar3 = (int **)local_80;
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
        ppiVar3 = (int **)local_80;
      }
    }
    break;
  case 1:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017218(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x01';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
    }
    break;
  case 2:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00016a60(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x02';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
    }
    break;
  case 3:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00016b8f(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x03';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
    }
    else {
      ppiVar3 = &piStack_88;
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
        ppiVar3 = &piStack_88;
      }
    }
    break;
  case 5:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00016959(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
    }
    break;
  case 6:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_0001730b(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x06';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
    }
    break;
  case 7:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_000174a2(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x04';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
    }
    break;
  case 8:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017530(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\b';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
    }
    break;
  case 9:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017637(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
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
    ppiVar3 = (int **)local_80;
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
      ppiVar3 = (int **)local_80;
    }
    break;
  case 0xb:
    if (*(int *)(local_20 + 0xab) < 0) {
      piStack_84 = (int *)0x18c51;
      iVar2 = FUN_0001519f(local_20,(&DAT_00053eb8)
                                    [*(int *)(local_20 + 0x113) * 10 +
                                     *(int *)(local_20 + 0xd) * 0x88]);
      piStack_84 = (int *)0x18c59;
      uVar5 = FUN_00041f76(iVar2 - local_48);
      if ((int)uVar5 < (int)((ulonglong)uVar5 >> 0x20)) {
        piStack_84 = (int *)0x18c65;
        iVar2 = FUN_000151d5(local_20);
        piStack_84 = (int *)0x18c6d;
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
            ppiVar3 = (int **)local_80;
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
            ppiVar3 = (int **)local_80;
          }
          break;
        }
      }
      piStack_84 = (int *)0x18d18;
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
        ppiVar3 = (int **)local_80;
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
        ppiVar3 = (int **)local_80;
      }
    }
    else {
      *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
      local_20[0x17] = '\x01';
      local_20[0x18] = '\0';
      local_20[0x19] = '\0';
      local_20[0x1a] = '\0';
      ppiVar3 = (int **)local_80;
    }
    break;
  case 0xc:
    *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
    ppiVar3 = (int **)local_80;
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
      ppiVar3 = (int **)local_80;
    }
    break;
  default:
    ppiVar3 = (int **)local_80;
    break;
  case 0x14:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_0001776f(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
    }
    break;
  case 0x15:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017927(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
    }
    break;
  case 0x16:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017ca6(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
    }
  }
  if (local_3c != 0) {
    *(undefined4 *)((int)ppiVar3 + -4) = 0x18e40;
    FUN_0001545e(local_20,local_3c);
  }
  if (local_38 != 0) {
    *(undefined4 *)((int)ppiVar3 + -4) = 0x18e51;
    FUN_0001586b(local_20,local_38);
  }
  *(undefined4 *)((int)ppiVar3 + -4) = 0x18e59;
  iVar2 = FUN_0001519f(local_20);
  if (local_48 < iVar2) {
    local_20[0x1b] = '\x01';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  *(undefined4 *)((int)ppiVar3 + -4) = 0x18e70;
  iVar2 = FUN_0001519f(local_20);
  if (iVar2 < local_48) {
    local_20[0x1b] = '\0';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  return;
}



// === ai_t1 @ 00018e88 ===

void FUN_00018e88(void)

{
  int iVar1;
  int iVar2;
  int extraout_ECX;
  int extraout_ECX_00;
  int **ppiVar3;
  float10 fVar4;
  undefined8 uVar5;
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
  piStack_84 = (int *)0x18ead;
  local_48 = FUN_0001519f(DAT_00053c7c);
  piStack_84 = (int *)0x18eba;
  local_44 = FUN_000151d5(DAT_00053c7c);
  piStack_84 = (int *)0x18ec5;
  local_28 = FUN_0001519f(local_20);
  piStack_84 = (int *)0x18ed0;
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
  fVar4 = (float10)*(float *)(&DAT_00051d5c + local_2c * 4);
  piStack_84 = (int *)0x18f49;
  FUN_00041f7e();
  local_40 = (int)ROUND(fVar4);
  fVar4 = (float10)*(float *)(local_20 + 99) + (float10)*(float *)(&DAT_00051d5c + local_2c * 4);
  piStack_84 = (int *)0x18f63;
  FUN_00041f7e();
  local_4c = (int)ROUND(fVar4);
  fVar4 = (float10)*(float *)(local_20 + 99);
  piStack_84 = (int *)0x18f71;
  FUN_00041f7e();
  local_50 = (int)ROUND(fVar4);
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
    piStack_84 = (int *)0x19016;
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
        piStack_84 = (int *)0x190ad;
        local_30 = FUN_00041e9f();
        local_30 = local_30 % 0x32;
        *(int *)(local_20 + 0xab) =
             *(int *)(&DAT_00053eb4 + *(int *)(local_20 + 0xd) * 0x88) + local_30;
      }
      local_20[0x1f] = '\x04';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      piStack_84 = (int *)0x190e9;
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
        piStack_84 = (int *)0x1914e;
        iVar2 = FUN_0001519f(local_20);
        if (iVar2 < 0) {
LAB_00019161:
          local_20[0x1f] = '\a';
          local_20[0x20] = '\0';
          local_20[0x21] = '\0';
          local_20[0x22] = '\0';
          return;
        }
        piStack_84 = (int *)0x1915a;
        iVar2 = FUN_0001519f(local_20);
        if (0x13f < iVar2) goto LAB_00019161;
        *(int *)(local_20 + 0xab) = *(int *)(&DAT_00053eb0 + *(int *)(local_20 + 0xd) * 0x88) >> 1;
        if (*(int *)(local_20 + 199) == 1) {
          do {
            piStack_84 = (int *)0x191a0;
            iVar2 = FUN_00041e9f();
            local_7c = iVar2 % 4;
            if (local_7c == 2) {
              piStack_84 = (int *)0x191bc;
              iVar2 = FUN_00041e9f(iVar2 / 4);
              local_7c = local_7c + iVar2 % 2;
            }
            else if (local_7c == 3) {
              local_7c = 4;
              piStack_84 = (int *)0x191e1;
              iVar2 = FUN_00041e9f(iVar2 / 4);
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
          iVar2 = *(int *)(&DAT_00051ca4 + local_7c * 0x10) -
                  *(int *)(&DAT_00051c9c + local_7c * 0x10);
          piStack_84 = (int *)0x19274;
          iVar1 = FUN_00041e9f(iVar2,local_7c * 0x10,iVar2,
                               *(int *)(&DAT_00051c9c + local_7c * 0x10) + local_48);
          piStack_84 = (int *)0x19284;
          iVar2 = FUN_00041f76(iVar1 % iVar2);
          *(int *)(local_20 + 0xbf) = extraout_ECX + iVar2;
          if ((local_7c == 0) || (local_7c == 1)) {
            *(int *)(local_20 + 0xc3) = local_44;
          }
          else {
            iVar1 = *(int *)(&DAT_00051ca8 + local_7c * 0x10) -
                    *(int *)(&DAT_00051ca0 + local_7c * 0x10);
            piStack_84 = (int *)0x192d9;
            iVar2 = FUN_00041e9f(local_7c * 0x10,iVar1,iVar1,
                                 *(int *)(&DAT_00051ca0 + local_7c * 0x10) + local_44);
            piStack_84 = (int *)0x192e9;
            iVar2 = FUN_00041f76(iVar2 % iVar1);
            *(int *)(local_20 + 0xc3) = extraout_ECX_00 + iVar2;
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
          piStack_84 = (int *)0x19333;
          iVar2 = FUN_0001519f(local_20);
          piStack_84 = (int *)0x19345;
          iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - iVar2);
          if (iVar2 < 10) {
            piStack_84 = (int *)0x19352;
            iVar2 = FUN_000151d5(local_20);
            piStack_84 = (int *)0x19364;
            iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - iVar2);
            if (iVar2 < 5) {
              *(undefined4 *)(local_20 + 0xbf) = local_78;
              *(undefined4 *)(local_20 + 0xc3) = local_74;
              goto LAB_00019392;
            }
          }
          local_20[199] = '\0';
          local_20[200] = '\0';
          local_20[0xc9] = '\0';
          local_20[0xca] = '\0';
        }
LAB_00019392:
        if (*(int *)(local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
          *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
        }
        if (*(int *)(DAT_00063354 + 0x16) <= *(int *)(local_20 + 0xc3)) {
          *(int *)(local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
        }
        piStack_84 = (int *)0x193e6;
        iVar2 = FUN_0001519f(local_20);
        if (iVar2 < *(int *)(local_20 + 0xbf)) {
          local_70 = *(int *)(local_20 + 0xbf);
          piStack_84 = (int *)0x19407;
          local_6c = FUN_0001519f(local_20);
        }
        else {
          piStack_84 = (int *)0x19414;
          local_70 = FUN_0001519f(local_20);
          local_6c = *(int *)(local_20 + 0xbf);
        }
        piStack_84 = (int *)0x1942b;
        iVar2 = FUN_000151d5(local_20);
        if (iVar2 < *(int *)(local_20 + 0xc3)) {
          local_68 = *(int *)(local_20 + 0xc3);
          piStack_84 = (int *)0x1944c;
          local_64 = FUN_000151d5(local_20);
        }
        else {
          piStack_84 = (int *)0x19459;
          local_68 = FUN_000151d5(local_20);
          local_64 = *(int *)(local_20 + 0xc3);
        }
        piStack_84 = (int *)0x19470;
        local_60 = FUN_0001519f(local_20);
        piStack_84 = (int *)0x1947b;
        local_5c = FUN_000151d5(local_20);
        piStack_84 = (int *)0x1948f;
        iVar2 = FUN_00041f76(*(int *)(local_20 + 0xbf) - local_60);
        if (iVar2 < 5) {
          piStack_84 = (int *)0x194a5;
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
        piStack_84 = (int *)0x19542;
        iVar2 = FUN_00041f76(*(int *)(local_20 + 0xc3) - local_5c);
        if (iVar2 < 10) {
          piStack_84 = (int *)0x19558;
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
          piStack_84 = (int *)0x196d3;
          iVar2 = FUN_0001519f(local_20);
          if (local_48 <= iVar2 + -8) goto LAB_000196eb;
          piStack_84 = (int *)0x196e3;
          iVar2 = FUN_0001519f(local_20);
          if (iVar2 + 8 <= local_48) goto LAB_000196eb;
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
LAB_000196eb:
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
      ppiVar3 = (int **)local_80;
    }
    else {
      ppiVar3 = (int **)local_80;
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
        ppiVar3 = (int **)local_80;
      }
    }
    break;
  case 1:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017218(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x01';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
    }
    break;
  case 2:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00016a60(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x02';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
    }
    break;
  case 3:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00016b8f(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x03';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
    }
    else {
      ppiVar3 = &piStack_88;
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
        ppiVar3 = &piStack_88;
      }
    }
    break;
  case 5:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00016959(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
    }
    break;
  case 6:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_0001730b(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x06';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
    }
    break;
  case 7:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_000174a2(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\x04';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
    }
    break;
  case 8:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017530(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
    if (iVar2 != 0) {
      local_20[0xaf] = '\b';
      local_20[0xb0] = '\0';
      local_20[0xb1] = '\0';
      local_20[0xb2] = '\0';
      local_20[0x1f] = '\0';
      local_20[0x20] = '\0';
      local_20[0x21] = '\0';
      local_20[0x22] = '\0';
      ppiVar3 = &piStack_88;
    }
    break;
  case 9:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017637(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
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
    ppiVar3 = (int **)local_80;
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
      ppiVar3 = (int **)local_80;
    }
    break;
  case 0xb:
    if (*(int *)(local_20 + 0xab) < 0) {
      piStack_84 = (int *)0x19c12;
      iVar2 = FUN_0001519f(local_20,(&DAT_00053eb8)
                                    [*(int *)(local_20 + 0x113) * 10 +
                                     *(int *)(local_20 + 0xd) * 0x88]);
      piStack_84 = (int *)0x19c1a;
      uVar5 = FUN_00041f76(iVar2 - local_48);
      if ((int)uVar5 < (int)((ulonglong)uVar5 >> 0x20)) {
        piStack_84 = (int *)0x19c26;
        iVar2 = FUN_000151d5(local_20);
        piStack_84 = (int *)0x19c2e;
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
            ppiVar3 = (int **)local_80;
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
            ppiVar3 = (int **)local_80;
          }
          break;
        }
      }
      piStack_84 = (int *)0x19cd9;
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
        ppiVar3 = (int **)local_80;
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
        ppiVar3 = (int **)local_80;
      }
    }
    else {
      *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
      local_20[0x17] = '\x01';
      local_20[0x18] = '\0';
      local_20[0x19] = '\0';
      local_20[0x1a] = '\0';
      ppiVar3 = (int **)local_80;
    }
    break;
  case 0xc:
    *(int *)(local_20 + 0xab) = *(int *)(local_20 + 0xab) + -1;
    ppiVar3 = (int **)local_80;
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
      ppiVar3 = (int **)local_80;
    }
    break;
  default:
    ppiVar3 = (int **)local_80;
    break;
  case 0x14:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_0001776f(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
    }
    break;
  case 0x15:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017927(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
    }
    break;
  case 0x16:
    piStack_84 = &local_38;
    piStack_88 = &local_3c;
    iVar2 = FUN_00017ca6(local_20,local_40,local_48,local_44);
    ppiVar3 = &piStack_88;
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
      ppiVar3 = &piStack_88;
    }
  }
  if (local_3c != 0) {
    *(undefined4 *)((int)ppiVar3 + -4) = 0x19e00;
    FUN_0001545e(local_20,local_3c);
  }
  if (local_38 != 0) {
    *(undefined4 *)((int)ppiVar3 + -4) = 0x19e11;
    FUN_0001586b(local_20,local_38);
  }
  *(undefined4 *)((int)ppiVar3 + -4) = 0x19e19;
  iVar2 = FUN_0001519f(local_20);
  if (local_48 < iVar2) {
    local_20[0x1b] = '\x01';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  *(undefined4 *)((int)ppiVar3 + -4) = 0x19e30;
  iVar2 = FUN_0001519f(local_20);
  if (iVar2 < local_48) {
    local_20[0x1b] = '\0';
    local_20[0x1c] = '\0';
    local_20[0x1d] = '\0';
    local_20[0x1e] = '\0';
  }
  return;
}



// === boss_sfx @ 00035c5c ===

void FUN_00035c5c(void)

{
  char *pcVar1;
  char *pcVar2;
  
  FUN_000418ee();
  DAT_00053c78 = DAT_00053c70;
  pcVar1 = DAT_00053c78;
  while (DAT_00053c78 = pcVar1, DAT_00053c78 != (char *)0x0) {
    pcVar1 = *(char **)(DAT_00053c78 + 0x13f);
    if ((*DAT_00053c78 == '\x01') && (DAT_00053c78[0x30] != '\x01')) {
      DAT_00053c78[0x30] = '\x01';
      pcVar2 = DAT_00053c78;
      pcVar2[0x17] = '\x15';
      pcVar2[0x18] = '\0';
      pcVar2[0x19] = '\0';
      pcVar2[0x1a] = '\0';
      pcVar2 = DAT_00053c78;
      pcVar2[0x11] = '\0';
      pcVar2[0x12] = '\0';
      pcVar2[0x13] = '\0';
      pcVar2[0x14] = '\0';
      *(undefined4 *)(DAT_00053c78 + 0x32) =
           *(undefined4 *)(&DAT_00053ea8 + *(int *)(DAT_00053c78 + 0xd) * 0x88);
    }
  }
  return;
}



// === boss_fx @ 0003ee0a ===

void FUN_0003ee0a(void)

{
  int iVar1;
  
  FUN_000418ee();
  DAT_00063e20 = DAT_00063e18;
  iVar1 = DAT_00063e20;
  while (DAT_00063e20 = iVar1, DAT_00063e20 != 0) {
    iVar1 = *(int *)(DAT_00063e20 + 0x51);
    if (DAT_00063e20 != 0) {
      FUN_0003e14c(DAT_00063e20,0);
      FUN_00041c28();
    }
  }
  DAT_00062a8c = 0;
  return;
}



