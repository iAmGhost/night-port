// func #27  FUN_0001441e  0001441e  size=3457


char * FUN_0001441e(void)

{
  char cVar1;
  short sVar2;
  char *pcVar3;
  char *pcVar4;
  int iVar5;
  uint uVar6;
  char *pcVar7;
  char *local_4c;
  char *local_48;
  int local_3c;
  int *local_2c;
  char *local_24;
  
  pcVar4 = (char *)FUN_000418ee();
  local_2c = *(int **)(*(int *)(*(int *)(pcVar4 + 0x123) + 4) + *(int *)(pcVar4 + 0x17) * 4);
  uVar6 = (uint)*(ushort *)
                 ((*(int *)(pcVar4 + 0x11) % *local_2c) * 2 + *(int *)((int)local_2c + 0x6d));
  pcVar7 = *(char **)(*(int *)(*(int *)(&DAT_000639ec + *(int *)(pcVar4 + 0xd) * 4) + 0x1c) +
                     uVar6 * 4);
  pcVar3 = *(char **)(*(int *)(*(int *)(&DAT_000639ec + *(int *)(pcVar4 + 0xd) * 4) + 0x20) +
                     uVar6 * 4);
  if ((pcVar4 == DAT_00053c7c) && (*(int *)(pcVar4 + 0x17) == 2)) {
    local_24 = DAT_00053c70;
    for (local_4c = (char *)0x0; (int)local_4c < DAT_00053c68;
        local_4c = (char *)((int)local_4c + 1)) {
      if (((((*local_24 == '\x01') &&
            (*(char *)(*(int *)(*(int *)(*(int *)(local_24 + 0x123) + 4) +
                               *(int *)(local_24 + 0x17) * 4) + 0x6c) != '\x01')) &&
           (*local_24 == '\x01')) &&
          ((*(int *)(local_24 + 0x17) == 2 || (*(int *)(local_24 + 0x17) == 1)))) &&
         (iVar5 = FUN_00041f76(*(int *)(pcVar4 + 9) - *(int *)(local_24 + 9)), iVar5 < 6)) {
        if ((*(int *)(pcVar4 + 0x1b) == 0) && (DAT_00063ed5 != '\0')) {
          if ((*(int *)(local_24 + 1) - *(int *)(pcVar4 + 1) < 0xd) &&
             (*(int *)(local_24 + 1) != *(int *)(pcVar4 + 1) &&
              -1 < *(int *)(local_24 + 1) - *(int *)(pcVar4 + 1))) {
            if ((char *)(uint)(byte)DAT_00053c7c[0x27] < (char *)0x2) {
              return (char *)(uint)(byte)DAT_00053c7c[0x27];
            }
            pcVar4[0x17] = -0x66;
            pcVar4[0x18] = '\0';
            pcVar4[0x19] = '\0';
            pcVar4[0x1a] = '\0';
            pcVar4[0x11] = '\0';
            pcVar4[0x12] = '\0';
            pcVar4[0x13] = '\0';
            pcVar4[0x14] = '\0';
            return pcVar4;
          }
        }
        else if ((DAT_00063ed3 != '\0') &&
                ((*(int *)(pcVar4 + 1) - *(int *)(local_24 + 1) < 0xd &&
                 (*(int *)(pcVar4 + 1) != *(int *)(local_24 + 1) &&
                  -1 < *(int *)(pcVar4 + 1) - *(int *)(local_24 + 1))))) {
          if ((char *)(uint)(byte)DAT_00053c7c[0x27] < (char *)0x2) {
            return (char *)(uint)(byte)DAT_00053c7c[0x27];
          }
          pcVar4[0x17] = -0x66;
          pcVar4[0x18] = '\0';
          pcVar4[0x19] = '\0';
          pcVar4[0x1a] = '\0';
          pcVar4[0x11] = '\0';
          pcVar4[0x12] = '\0';
          pcVar4[0x13] = '\0';
          pcVar4[0x14] = '\0';
          return pcVar4;
        }
      }
      local_24 = *(char **)(local_24 + 0x13f);
    }
  }
  if (pcVar7 != pcVar3) {
    for (local_4c = (char *)0x0; (int)local_4c < 6; local_4c = local_4c + 1) {
      pcVar7 = local_4c;
      if (((int)(uint)*(ushort *)(local_2c + (int)(local_4c + 2)) <= *(int *)(pcVar4 + 0x11)) &&
         (*(int *)(pcVar4 + 0x11) < (int)(uint)*(ushort *)((int)local_2c + (int)local_4c * 4 + 10)))
      {
        local_48 = local_4c;
        break;
      }
    }
    if (local_4c != (char *)0x6) {
      local_24 = DAT_00053c70;
      for (local_4c = (char *)0x0; (int)local_4c < DAT_00053c68;
          local_4c = (char *)((int)local_4c + 1)) {
        if ((((*pcVar4 != *local_24) && (*local_24 != '\x02')) &&
            (((uint)(byte)local_24[0x4e] != *(uint *)(pcVar4 + 0x17) ||
             ((char *)(uint)(byte)local_24[0x4f] != local_48)))) &&
           (iVar5 = FUN_00013c4d(pcVar4,local_24,local_2c,local_48), iVar5 == 1)) {
          sVar2 = *(short *)(local_24 + 0x29);
          local_2c = *(int **)(*(int *)(*(int *)(pcVar4 + 0x123) + 4) + *(int *)(pcVar4 + 0x17) * 4)
          ;
          if (((local_24 == DAT_00053c7c) && (*(int *)(local_24 + 5) == 0)) &&
             ((*(uint *)(local_24 + 0x17) < 0x15 || (0x1e < *(uint *)(local_24 + 0x17))))) {
            cVar1 = '\0';
          }
          else {
            cVar1 = *(char *)(*(int *)(*(int *)(*(int *)(local_24 + 0x123) + 4) +
                                      *(int *)(local_24 + 0x17) * 4) + 0x6c);
          }
          DAT_00063364 = (uint)(byte)((char *)((int)local_2c + (int)local_48))[99];
          DAT_00063368 = 0;
          local_24[0x31] = '\0';
          if (local_24 != DAT_00053c7c) {
            DAT_00053c88 = local_24;
          }
          *(char **)(local_24 + 0x46) = pcVar4;
          local_24[0x4e] = pcVar4[0x17];
          local_24[0x4f] = (char)local_48;
          uVar6 = (uint)(byte)((char *)((int)local_2c + (int)local_48))[0x38];
          if ((local_24 == DAT_00053c7c) && (*(int *)(DAT_00053c7c + 0x4a) != 0)) {
            *(undefined4 *)(*(int *)(local_24 + 0x4a) + 0x4a) = 0;
            *(undefined2 *)(*(int *)(local_24 + 0x4a) + 0x50) = 0;
            *(undefined4 *)(*(int *)(local_24 + 0x4a) + 0x17) = 1;
            *(undefined4 *)(*(int *)(local_24 + 0x4a) + 0x11) = 0;
            local_24[0x4a] = '\0';
            local_24[0x4b] = '\0';
            local_24[0x4c] = '\0';
            local_24[0x4d] = '\0';
            DAT_00053c7c[0x31] = '\0';
          }
          if ((cVar1 == '\x01') && (local_24 != DAT_00053c7c)) {
            if ((*(uint *)(local_24 + 0x17) < 0x33) || (0x35 < *(uint *)(local_24 + 0x17))) {
              local_3c = local_2c[(int)(local_48 + 8)] * 2;
            }
            else {
              local_3c = local_2c[(int)(local_48 + 8)] * 3;
            }
          }
          else {
            local_3c = local_2c[(int)(local_48 + 8)];
          }
          if (*(short *)(local_24 + 0x23) == 0) {
            *(short *)(local_24 + 0x29) = *(short *)(local_24 + 0x29) - (short)local_3c;
            if (*(short *)(local_24 + 0x29) < 1) {
              local_24[0x29] = '\0';
              local_24[0x2a] = '\0';
              local_24[0x30] = '\x01';
            }
          }
          else {
            if ((local_24 == DAT_00053c7c) || (*(int *)(local_24 + 0x56) == 0)) {
              *(short *)(local_24 + 0x23) = *(short *)(local_24 + 0x23) - (short)local_3c;
            }
            else {
              *(int *)(local_24 + 0x56) = *(int *)(local_24 + 0x56) - local_3c;
              if (*(int *)(local_24 + 0x56) < 0) {
                *(short *)(local_24 + 0x23) =
                     *(short *)(local_24 + 0x23) + *(short *)(local_24 + 0x56);
                local_24[0x56] = '\0';
                local_24[0x57] = '\0';
                local_24[0x58] = '\0';
                local_24[0x59] = '\0';
              }
            }
            if (*(short *)(local_24 + 0x23) < 1) {
              *(short *)(local_24 + 0x29) =
                   *(short *)(local_24 + 0x29) + *(short *)(local_24 + 0x23);
              local_24[0x23] = '\0';
              local_24[0x24] = '\0';
              local_24[0x30] = '\x02';
              if (*(short *)(local_24 + 0x29) < 1) {
                local_24[0x30] = '\x01';
                local_24[0x29] = '\0';
                local_24[0x2a] = '\0';
              }
            }
          }
          if (cVar1 == '\x01') {
            local_24[0x5a] = '\0';
            FUN_00031f10(local_24,5);
            if (*(int *)(local_24 + 5) == 0) {
              if (*(int *)(local_24 + 0x1b) == 0) {
                FUN_0001545e(local_24,0xfffffff6);
              }
              else {
                FUN_0001545e(local_24,10);
              }
              if (local_24 == DAT_00053c7c) {
                iVar5 = FUN_00041e9f();
                FUN_0001586b(local_24,iVar5 % 5 + -2);
              }
              else {
                FUN_00015295(local_24,0xfffffffc);
                local_24[0x16] = '\x05';
                local_24[0x42] = '\0';
                local_24[0x43] = '\0';
                local_24[0x44] = '\0';
                local_24[0x45] = '\0';
                local_24[0x3e] = '\x01';
                local_24[0x3f] = '\0';
                local_24[0x40] = '\0';
                local_24[0x41] = '\0';
              }
            }
            else {
              local_24[0x50] = 'P';
              local_24[0x51] = '\0';
              if ((uVar6 < 0xb) || (0x14 < uVar6)) {
                local_24[0x42] = '\x01';
                local_24[0x43] = '\0';
                local_24[0x44] = '\0';
                local_24[0x45] = '\0';
              }
              else {
                local_24[0x42] = '\x04';
                local_24[0x43] = '\0';
                local_24[0x44] = '\0';
                local_24[0x45] = '\0';
              }
              if ((local_24 == DAT_00053c7c) && (local_24[0x16] == '\0')) {
                local_24[0x17] = '\x1e';
                local_24[0x18] = '\0';
                local_24[0x19] = '\0';
                local_24[0x1a] = '\0';
                local_24[0x31] = '\0';
                local_24[0x16] = '\a';
                *(int *)(local_24 + 5) = *(int *)(local_24 + 5) + -0xf;
                if ((uVar6 < 0xb) || (0x14 < uVar6)) {
                  local_24[0x42] = '\x02';
                  local_24[0x43] = '\0';
                  local_24[0x44] = '\0';
                  local_24[0x45] = '\0';
                }
                else {
                  local_24[0x42] = '\x03';
                  local_24[0x43] = '\0';
                  local_24[0x44] = '\0';
                  local_24[0x45] = '\0';
                }
                if (*(int *)(local_24 + 0x1b) == 0) {
                  FUN_0001545e(local_24,0xffffffe7);
                }
                else {
                  FUN_0001545e(local_24,0x19);
                }
                *(int *)(local_24 + 0x11) = (*(int *)(local_24 + 5) + 99) / 3;
                if ((int)(CONCAT44(*(int *)(local_24 + 5) >> 0x1f,*(undefined4 *)(local_24 + 5)) % 3
                         ) != 0) {
                  *(int *)(local_24 + 5) =
                       *(int *)(local_24 + 5) - (3 - -*(int *)(local_24 + 5) % 3);
                }
                if (*(int *)(local_24 + 0x11) < 0) {
                  local_24[0x11] = '\0';
                  local_24[0x12] = '\0';
                  local_24[0x13] = '\0';
                  local_24[0x14] = '\0';
                  local_24[5] = -99;
                  local_24[6] = -1;
                  local_24[7] = -1;
                  local_24[8] = -1;
                }
                FUN_00031f10(local_24,5);
              }
              local_24[0x3e] = '\0';
              local_24[0x3f] = '\0';
              local_24[0x40] = '\0';
              local_24[0x41] = '\0';
              if ((byte)local_24[0x16] < 2) {
                local_24[0x16] = '\v';
              }
              else if ((uVar6 < 0xb) || (0x14 < uVar6)) {
                if (*(int *)(local_24 + 0x1b) == 0) {
                  FUN_0001545e(local_24,0xfffffff6);
                }
                else {
                  FUN_0001545e(local_24,10);
                }
              }
              else if (*(int *)(local_24 + 0x1b) == 0) {
                FUN_0001545e(local_24,0xffffffec);
              }
              else {
                FUN_0001545e(local_24,0x14);
              }
            }
            if (local_24[0x30] == '\x02') {
              local_24[0x30] = '\x01';
              local_24[0x31] = '\0';
            }
            if ((pcVar4 == DAT_00053c7c) && (*(int *)(pcVar4 + 0x17) == 0x8c)) {
              local_24[0x50] = '(';
              local_24[0x51] = '\0';
            }
          }
          else {
            *(undefined2 *)(local_24 + 0x50) =
                 *(undefined2 *)((int)local_2c + (int)local_48 * 2 + 0x4a);
            local_24[0x11] = '\0';
            local_24[0x12] = '\0';
            local_24[0x13] = '\0';
            local_24[0x14] = '\0';
            if ((((local_24 == DAT_00053c7c) && (*(int *)(local_24 + 0x17) == 0x97)) && (uVar6 != 0)
                ) && (uVar6 < 10)) {
              local_24[0x31] = '\0';
              local_24[0x17] = '\x14';
              local_24[0x18] = '\0';
              local_24[0x19] = '\0';
              local_24[0x1a] = '\0';
            }
            else {
              *(undefined4 *)(local_24 + 0x17) = *(undefined4 *)(&DAT_00051b60 + uVar6 * 4);
            }
            if (((*(short *)(local_24 + 0x23) == 0) && (10 < uVar6)) && (uVar6 < 0x15)) {
              local_24[0x30] = '\x01';
            }
            else if ((*(short *)(local_24 + 0x23) == 0) && (*(short *)(local_24 + 0x29) == 0)) {
              local_24[0x30] = '\x01';
              local_24[0x17] = '\x15';
              local_24[0x18] = '\0';
              local_24[0x19] = '\0';
              local_24[0x1a] = '\0';
              local_24[0x50] = -0x6a;
              local_24[0x51] = '\0';
            }
            if ((10 < uVar6) && (uVar6 < 0x15)) {
              if (*(int *)(local_24 + 0x1b) == 0) {
                iVar5 = FUN_00041e9f();
                FUN_0001545e(local_24,-iVar5 % 0x14);
              }
              else {
                iVar5 = FUN_00041e9f();
                FUN_0001545e(local_24,iVar5 % 0x14);
              }
              if (local_24[0x30] != '\x01') {
                local_24[0x31] = '\0';
                FUN_00031f10(local_24,5);
              }
              iVar5 = FUN_00041e9f();
              FUN_0001545e(local_24,iVar5 % 7 + -3);
              iVar5 = FUN_00041e9f();
              *(short *)(local_24 + 0x50) =
                   *(short *)(local_24 + 0x50) + (short)(iVar5 % 0x15) + -10;
            }
          }
          if ((*(int *)(pcVar4 + 0x17) == 0x65) || (*(int *)(pcVar4 + 0x17) == 0x99)) {
            if (cVar1 == '\0') {
              pcVar4[0x17] = 'f';
              pcVar4[0x18] = '\0';
              pcVar4[0x19] = '\0';
              pcVar4[0x1a] = '\0';
              *(undefined2 *)(pcVar4 + 0x50) =
                   *(undefined2 *)((int)local_2c + (int)local_48 * 2 + 0x4a);
              pcVar4[0x11] = '\0';
              pcVar4[0x12] = '\0';
              pcVar4[0x13] = '\0';
              pcVar4[0x14] = '\0';
              FUN_0001545e(local_24,0xffffffff);
              local_24[0x17] = '\x1f';
              local_24[0x18] = '\0';
              local_24[0x19] = '\0';
              local_24[0x1a] = '\0';
              *(char **)(local_24 + 0x4a) = pcVar4;
              *(char **)(pcVar4 + 0x4a) = local_24;
              if (*(int *)(pcVar4 + 0x1b) == 0) {
                FUN_0001545e(local_24,0x18 - (*(int *)(local_24 + 1) - *(int *)(pcVar4 + 1)));
              }
              else {
                FUN_0001545e(local_24,(*(int *)(pcVar4 + 1) - *(int *)(local_24 + 1)) + -0x18);
              }
              *(undefined4 *)(local_24 + 9) = *(undefined4 *)(pcVar4 + 9);
              local_24[0x52] = '\0';
              local_24[0x53] = '\0';
              local_24[0x54] = '\0';
              local_24[0x55] = '\0';
              return local_24;
            }
          }
          else {
            if ((*(int *)(local_24 + 0x17) == 0x16) && (*local_24 == '\x01')) {
              *local_24 = '\0';
            }
            if ((((*(int *)(pcVar4 + 0x17) == 0x67) || (*(int *)(pcVar4 + 0x17) == 0x68)) ||
                (*(int *)(pcVar4 + 0x17) == 0x6f)) ||
               (((*(int *)(pcVar4 + 0x17) == 0x71 || (*(int *)(pcVar4 + 0x17) == 0x73)) ||
                (*(int *)(pcVar4 + 0x17) == 0x72)))) {
              if (*(int *)(pcVar4 + 0x17) == 0x67) {
                FUN_000348e2();
                *(int *)(local_24 + 0x1b) = 1 - *(int *)(local_24 + 0x1b);
                if (*local_24 == '\x01') {
                  *local_24 = '\0';
                }
              }
              pcVar4[0x31] = '\0';
              *(undefined4 *)(*(int *)(pcVar4 + 0x4a) + 0x4a) = 0;
              pcVar4[0x4a] = '\0';
              pcVar4[0x4b] = '\0';
              pcVar4[0x4c] = '\0';
              pcVar4[0x4d] = '\0';
              if (*(int *)(pcVar4 + 0x17) == 0x71) {
                local_24[0x52] = '\0';
                local_24[0x53] = '\0';
                local_24[0x54] = '\0';
                local_24[0x55] = '\0';
              }
            }
            if (*(char *)(*(int *)(pcVar4 + 0x4a) + 0x30) == '\x01') {
              *(undefined4 *)(*(int *)(pcVar4 + 0x4a) + 0x4a) = 0;
              pcVar4[0x4a] = '\0';
              pcVar4[0x4b] = '\0';
              pcVar4[0x4c] = '\0';
              pcVar4[0x4d] = '\0';
              if (*(int *)(pcVar4 + 0x17) == 0x70) {
                pcVar4[0x17] = '\x01';
                pcVar4[0x18] = '\0';
                pcVar4[0x19] = '\0';
                pcVar4[0x1a] = '\0';
                pcVar4[0x11] = '\0';
                pcVar4[0x12] = '\0';
                pcVar4[0x13] = '\0';
                pcVar4[0x14] = '\0';
              }
              pcVar4[0x31] = '\0';
              FUN_000348e2();
            }
          }
          if (((int)sVar2 != (int)*(short *)(local_24 + 0x29)) && (pcVar4 == DAT_00053c7c)) {
            *(int *)(pcVar4 + 0xbb) =
                 *(int *)(pcVar4 + 0xbb) + ((int)sVar2 - (int)*(short *)(local_24 + 0x29));
          }
          if ((local_24[0x30] == '\x01') && (local_24 != DAT_00053c7c)) {
            if ((local_24[0x5a] == '\0') && (*(short *)(local_24 + 0x29) != 0)) {
              local_24[0x5a] = '\x01';
              builtin_strncpy(local_24 + 0x30,"\x02\x01\x05",4);
              local_24[0x34] = '\0';
              local_24[0x35] = '\0';
            }
            else {
              *(undefined4 *)(local_24 + 0x32) =
                   *(undefined4 *)(&DAT_00053ea8 + *(int *)(local_24 + 0xd) * 0x88);
            }
          }
          if (local_24[0x30] == '\x01') {
            if (local_24 == DAT_00053c7c) {
              local_24[0x31] = '\0';
              local_24[0x50] = -0x38;
              local_24[0x51] = '\0';
              DAT_00063364 = 0x28;
              DAT_00063368 = 0;
            }
            else {
              local_24[0x50] = 'x';
              local_24[0x51] = '\0';
              local_24[0x31] = '\0';
            }
            if (local_24 == DAT_00053c80) {
              FUN_0004185e(*(undefined4 *)
                            (&DAT_00053d0c +
                            *(int *)(&DAT_00051c68 + *(int *)(pcVar4 + 0xd) * 4) * 4));
              FUN_0003ee0a();
              FUN_00035c5c();
              DAT_00053104 = 0x23;
            }
          }
          iVar5 = *(int *)(*(int *)(local_24 + 0x17) * 4 + *(int *)(*(int *)(local_24 + 0x123) + 4))
          ;
          if (*(char *)(iVar5 + 0x69) != '\0') {
            DAT_00063364 = (uint)*(byte *)(iVar5 + 0x69);
            DAT_00063368 = (uint)*(ushort *)(iVar5 + 0x6a);
          }
        }
        local_24 = *(char **)(local_24 + 0x13f);
      }
      pcVar7 = pcVar4;
      if ((pcVar4 == DAT_00053c7c) && (*(int *)(pcVar4 + 0xbb) != 0)) {
        *(int *)(DAT_00053c7c + 0x52) = *(int *)(DAT_00053c7c + 0x52) + *(int *)(pcVar4 + 0xbb);
        pcVar4[0xbb] = '\0';
        pcVar4[0xbc] = '\0';
        pcVar4[0xbd] = '\0';
        pcVar4[0xbe] = '\0';
      }
    }
  }
  return pcVar7;
}


