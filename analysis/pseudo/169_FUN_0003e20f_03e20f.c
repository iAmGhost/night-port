// func #169  FUN_0003e20f  0003e20f  size=2862


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


