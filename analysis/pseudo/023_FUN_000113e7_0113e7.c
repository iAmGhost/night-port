// func #17  FUN_000113e7  000113e7  size=1088


void FUN_000113e7(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int *piVar4;
  int aiStack_a8 [8];
  int local_88;
  int local_84;
  int local_80;
  int local_7c;
  int local_78;
  int local_74;
  int local_70;
  int local_6c;
  int local_68;
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
  
  local_20 = FUN_000418ee();
  piVar4 = aiStack_a8 + 7;
  if (*(int *)(local_20 + 0x1b) == 0) {
    aiStack_a8[6] = 0x11419;
    local_58 = FUN_00041bdc(0x147);
    local_54 = local_58;
    if (local_58 == 0) {
      local_38 = 0;
      piVar4 = aiStack_a8 + 7;
    }
    else {
      aiStack_a8[6] = 2;
      aiStack_a8[5] = 0xfa;
      local_5c = FUN_00010efc(local_58,*(int *)(local_20 + 1) + -0xc,0,*(int *)(local_20 + 9) + 1);
      piVar4 = aiStack_a8 + 5;
      local_38 = local_5c;
    }
    local_2c = local_38;
    *(undefined4 *)((int)piVar4 + -4) = 0x1146e;
    local_4c = FUN_00041bdc(0x147);
    local_48 = local_4c;
    if (local_4c == 0) {
      local_34 = 0;
    }
    else {
      *(undefined4 *)((int)piVar4 + -4) = 2;
      *(undefined4 *)((int)piVar4 + -8) = 0xfa;
      iVar1 = *(int *)(local_20 + 9);
      iVar2 = *(int *)(local_20 + 1);
      *(undefined4 *)((int)piVar4 + -0xc) = 0x114a2;
      local_50 = FUN_00010efc(local_4c,iVar2 + -0x2c,0,iVar1 + 1);
      piVar4 = (int *)((int)piVar4 + -8);
      local_34 = local_50;
    }
    local_28 = local_34;
    *(undefined4 *)((int)piVar4 + -4) = 0x114c3;
    local_40 = FUN_00041bdc(0x147);
    local_3c = local_40;
    if (local_40 == 0) {
      local_30 = 0;
    }
    else {
      *(undefined4 *)((int)piVar4 + -4) = 2;
      *(undefined4 *)((int)piVar4 + -8) = 0xfa;
      iVar1 = *(int *)(local_20 + 9);
      iVar2 = *(int *)(local_20 + 1);
      *(undefined4 *)((int)piVar4 + -0xc) = 0x114f7;
      local_44 = FUN_00010efc(local_40,iVar2 + 0x16,0,iVar1 + 1);
      piVar4 = (int *)((int)piVar4 + -8);
      local_30 = local_44;
    }
    local_24 = local_30;
    *(undefined4 *)(local_2c + 0x1b) = 0;
    *(undefined4 *)(local_28 + 0x1b) = 0;
    *(undefined4 *)(local_30 + 0x1b) = 0;
  }
  else {
    aiStack_a8[6] = 0x1153b;
    local_88 = FUN_00041bdc(0x147);
    local_84 = local_88;
    if (local_88 == 0) {
      local_68 = 0;
    }
    else {
      aiStack_a8[6] = 2;
      piVar4 = aiStack_a8 + 5;
      aiStack_a8[5] = 0xfa;
      aiStack_a8[7] =
           FUN_00010efc(local_88,*(int *)(local_20 + 1) + 0xc,0,*(int *)(local_20 + 9) + 1);
      local_68 = aiStack_a8[7];
    }
    local_2c = local_68;
    *(undefined4 *)((int)piVar4 + -4) = 0x11590;
    local_7c = FUN_00041bdc(0x147);
    local_78 = local_7c;
    if (local_7c == 0) {
      local_64 = 0;
    }
    else {
      *(undefined4 *)((int)piVar4 + -4) = 2;
      *(undefined4 *)((int)piVar4 + -8) = 0xfa;
      iVar1 = *(int *)(local_20 + 9);
      iVar2 = *(int *)(local_20 + 1);
      *(undefined4 *)((int)piVar4 + -0xc) = 0x115c4;
      local_80 = FUN_00010efc(local_7c,iVar2 + 0x2c,0,iVar1 + 1);
      piVar4 = (int *)((int)piVar4 + -8);
      local_64 = local_80;
    }
    local_28 = local_64;
    *(undefined4 *)((int)piVar4 + -4) = 0x115e5;
    local_70 = FUN_00041bdc(0x147);
    local_6c = local_70;
    if (local_70 == 0) {
      local_60 = 0;
    }
    else {
      *(undefined4 *)((int)piVar4 + -4) = 2;
      *(undefined4 *)((int)piVar4 + -8) = 0xfa;
      iVar1 = *(int *)(local_20 + 9);
      iVar2 = *(int *)(local_20 + 1);
      *(undefined4 *)((int)piVar4 + -0xc) = 0x11619;
      local_74 = FUN_00010efc(local_70,iVar2 + -0x16,0,iVar1 + 1);
      piVar4 = (int *)((int)piVar4 + -8);
      local_60 = local_74;
    }
    local_24 = local_60;
    *(undefined4 *)(local_2c + 0x1b) = 1;
    *(undefined4 *)(local_28 + 0x1b) = 1;
    *(undefined4 *)(local_60 + 0x1b) = 1;
  }
  *(undefined1 *)(local_2c + 0x30) = 1;
  *(undefined4 *)(local_2c + 0x17) = 2;
  *(undefined1 *)(local_28 + 0x30) = 1;
  *(undefined4 *)(local_28 + 0x17) = 3;
  *(undefined1 *)(local_24 + 0x30) = 1;
  *(undefined4 *)(local_24 + 0x17) = 3;
  if (*(char *)(local_20 + 0x30) == '\x01') {
    if ((*(int *)(local_20 + 0xd) < 0xc) || (0x11 < *(int *)(local_20 + 0xd))) {
      if ((*(int *)(local_20 + 0xd) < 0x1f) || (0x21 < *(int *)(local_20 + 0xd))) {
        if ((*(int *)(local_20 + 0xd) < 0x24) || (0x27 < *(int *)(local_20 + 0xd))) {
          if ((*(int *)(local_20 + 0xd) < 0x29) || (0x2b < *(int *)(local_20 + 0xd))) {
            if ((*(int *)(local_20 + 0xd) < 0x2e) || (0x31 < *(int *)(local_20 + 0xd))) {
              if ((*(int *)(local_20 + 0xd) < 0x33) || (0x36 < *(int *)(local_20 + 0xd))) {
                if ((*(int *)(local_20 + 0xd) < 0x38) || (0x3b < *(int *)(local_20 + 0xd))) {
                  if ((*(int *)(local_20 + 0xd) < 0x3d) || (0x40 < *(int *)(local_20 + 0xd))) {
                    if ((*(int *)(local_20 + 0xd) < 0x47) || (0x4a < *(int *)(local_20 + 0xd))) {
                      if ((*(int *)(local_20 + 0xd) < 0x4c) || (0x4f < *(int *)(local_20 + 0xd))) {
                        if ((0x50 < *(int *)(local_20 + 0xd)) && (*(int *)(local_20 + 0xd) < 0x55))
                        {
                          *(undefined4 *)((int)piVar4 + -4) = 0x1181e;
                          FUN_0004185e(DAT_00053e48);
                        }
                      }
                      else {
                        *(undefined4 *)((int)piVar4 + -4) = 0x117fe;
                        FUN_0004185e(DAT_00053e44);
                      }
                    }
                    else {
                      *(undefined4 *)((int)piVar4 + -4) = 0x117de;
                      FUN_0004185e(DAT_00053e40);
                    }
                  }
                  else {
                    *(undefined4 *)((int)piVar4 + -4) = 0x117bb;
                    FUN_0004185e(DAT_00053e3c);
                  }
                }
                else {
                  *(undefined4 *)((int)piVar4 + -4) = 0x11798;
                  FUN_0004185e(DAT_00053e38);
                }
              }
              else {
                *(undefined4 *)((int)piVar4 + -4) = 0x11775;
                FUN_0004185e(DAT_00053e34);
              }
            }
            else {
              *(undefined4 *)((int)piVar4 + -4) = 0x11752;
              FUN_0004185e(DAT_00053e30);
            }
          }
          else {
            *(undefined4 *)((int)piVar4 + -4) = 0x1172f;
            FUN_0004185e(DAT_00053e2c);
          }
        }
        else {
          *(undefined4 *)((int)piVar4 + -4) = 0x1170c;
          FUN_0004185e(DAT_00053e28);
        }
      }
      else {
        *(undefined4 *)((int)piVar4 + -4) = 0x116e9;
        FUN_0004185e(DAT_00053e24);
      }
    }
    else {
      uVar3 = *(undefined4 *)
               (&DAT_00053d0c + *(int *)(&DAT_00051c24 + *(int *)(local_20 + 0xd) * 4) * 4);
      *(undefined4 *)((int)piVar4 + -4) = 0x116c6;
      FUN_0004185e(uVar3);
    }
  }
  return;
}


