// func #25  FUN_00013c4d  00013c4d  size=1810


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


