// === frame @ 0003a829 ===

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0003a829(void)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 extraout_EDX;
  undefined **ppuVar3;
  undefined *puStack_28;
  undefined4 local_24;
  int local_20;
  
  FUN_000418ee();
  ppuVar3 = (undefined **)&local_24;
  local_20 = 0;
  DAT_000639bc = 0;
  DAT_00053104 = 0x46;
  puStack_28 = (undefined *)0x3a87d;
  FUN_00010b1d(*(undefined4 *)
                (&DAT_00052da4 + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28));
  puStack_28 = (undefined *)0x3a882;
  FUN_0003b9b7();
  *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
  DAT_00063374 = *(undefined4 *)(DAT_00053c7c + 0x2b);
  DAT_00063378 = (uint)*(byte *)(DAT_00053c7c + 0x27);
  DAT_0006337c = (uint)*(byte *)(DAT_00053c7c + 0x26);
  _DAT_00063380 = (uint)*(byte *)(DAT_00053c7c + 0x25);
  _DAT_00063384 = (uint)*(byte *)(DAT_00053c7c + 0x28);
  puStack_28 = (undefined *)0x3a8f4;
  FUN_00041e20(&DAT_000633b8,0,0x300);
  puStack_28 = &DAT_000633b8;
  FUN_00040042();
  DAT_00063360 = 1;
  if ((&DAT_00052b84)[*(int *)(DAT_00063354 + 0x40) + (DAT_00063370 + -1) * 10] == '\x01') {
    if (DAT_00063398 == (undefined *)0x0) {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) + 0x78;
    }
    else {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) + -0x78;
    }
  }
  puStack_28 = (undefined *)0x3a95a;
  FUN_0003ed6f();
  puStack_28 = (undefined *)0x3a964;
  FUN_0003801b(DAT_000633b0);
  puStack_28 = (undefined *)0x3a976;
  FUN_00040660(DAT_00063354 + 0x48,0x32);
  DAT_0006336c = 0;
  DAT_00063360 = 0;
  DAT_00063e28 = 0;
  DAT_00063e24 = 0;
  if ((&DAT_00052b84)[*(int *)(DAT_00063354 + 0x40) + (DAT_00063370 + -1) * 10] == '\0') {
    ppuVar3 = &puStack_28;
    puStack_28 = DAT_00063398;
    FUN_000381d7(DAT_000633b0,1,0,0x78);
  }
  DAT_0006336c = 0;
  DAT_00063360 = 0;
  DAT_00063e28 = 0;
  DAT_00063e24 = 0;
  *(undefined4 *)((int)ppuVar3 + -4) = 0x3aa09;
  FUN_000348e2();
  *(undefined4 *)((int)ppuVar3 + -4) = 0x3aa1a;
  FUN_00041e20(&DAT_00062c58,0,0x80);
  *(undefined4 *)((int)ppuVar3 + -4) = 0x3aa2b;
  FUN_00041e20(&DAT_00062cd8,0,0x80);
  while( true ) {
    if (((((local_20 == 0) && (DAT_00053c80 != 0)) && (DAT_000639e0 <= *(int *)(DAT_00053c80 + 9)))
        && ((*(int *)(DAT_00053c80 + 9) < DAT_000639e0 + 200 &&
            (DAT_000639dc + 0x32 <= *(int *)(DAT_00053c80 + 1))))) &&
       (*(int *)(DAT_00053c80 + 1) <= DAT_000639dc + 0x10e)) {
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3aa94;
      FUN_0003a70e();
      local_20 = 1;
    }
    if ((DAT_00063e89 != '\0') && (*(char *)(DAT_00053c7c + 0x30) != '\x01')) {
      do {
      } while (DAT_00063e89 != '\0');
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3aac6;
      iVar2 = FUN_00039c03();
      if (iVar2 == 0) {
        local_24 = 0xffffffff;
        goto LAB_0003acdd;
      }
    }
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3aadb;
    FUN_000322f5();
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3aae0;
    FUN_0003209c();
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3aae5;
    FUN_000347e3();
    if ((*(int *)(DAT_00053c7c + 0x17) == 0x66) || (*(int *)(DAT_00053c7c + 0x17) == 0x70)) {
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab13;
      FUN_0003272f(DAT_00062a9c);
    }
    else {
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab07;
      FUN_0003272f(DAT_00062a98);
    }
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab18;
    FUN_000320f5();
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab22;
    FUN_0003d4e4(DAT_00063354);
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab27;
    FUN_0003ed6f();
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab31;
    FUN_00038098(DAT_000633b0);
    if (DAT_00063364 != 0) {
      if (DAT_00063368 == 0) {
        uVar1 = *(undefined4 *)(&DAT_00053d08 + DAT_00063364 * 4);
        *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab56;
        FUN_0004185e(uVar1);
        DAT_00063364 = 0;
      }
      else {
        DAT_00063368 = DAT_00063368 + -1;
      }
    }
    DAT_00063360 = DAT_00063360 + 1;
    if (*(int *)(DAT_00053c7c + 0x17) == 1) {
      DAT_0006339c = DAT_0006339c + 1;
    }
    else {
      DAT_0006339c = 0;
    }
    iVar2 = DAT_00063360 / 10;
    if (DAT_00063360 % 10 == 0) {
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3abae;
      iVar2 = FUN_0003d606(DAT_00063354);
    }
    if (DAT_0006336c == 1) break;
    if (DAT_0006336c == 3) {
      DAT_0006336c = 0;
      local_24 = 1;
LAB_0003acdd:
      return CONCAT44(extraout_EDX,local_24);
    }
    if (DAT_0006336c == 2) {
      *(undefined **)((int)ppuVar3 + -4) = DAT_00063398;
      *(undefined4 *)((int)ppuVar3 + -8) = 0x3ac4f;
      FUN_000381d7(DAT_000633b0,1,0,0x50);
      DAT_00063398 = (undefined *)(uint)*(byte *)(*(int *)(DAT_00063354 + 0x34c) + 0x11);
      DAT_0006336c = 0;
      *(undefined4 *)((int)ppuVar3 + -8) = 0x3ac7b;
      iVar2 = FUN_0003d244(DAT_00063354);
      ppuVar3 = (undefined **)((int)ppuVar3 + -4);
    }
    else if (DAT_0006336c == 4) {
      DAT_00063398 = (undefined *)(uint)*(byte *)(*(int *)(DAT_00063354 + 0x34c) + 0x11);
      DAT_0006336c = 0;
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3acb2;
      iVar2 = FUN_0003d244(DAT_00063354);
    }
    if (DAT_00053c68 == 1) {
      *(undefined4 *)(DAT_00063354 + 0x32) = 0;
      iVar2 = DAT_00063354;
      *(undefined4 *)(DAT_00063354 + 0x36) = 0;
    }
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3acd8;
    FUN_00010d47(iVar2);
  }
  DAT_0006336c = 0;
  if (DAT_00053100 == 0) {
    DAT_00053100 = 3;
    local_24 = 0;
  }
  else {
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3abcf;
    iVar2 = FUN_00038445();
    if (iVar2 == 0) {
      DAT_00053100 = 3;
      local_24 = 0;
    }
    else {
      local_24 = 2;
    }
  }
  goto LAB_0003acdd;
}



// === ent_ai @ 00031a8e ===

void FUN_00031a8e(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  
  iVar1 = FUN_000418ee();
  if ((*(int *)(iVar1 + 0x17) == 2) &&
     (*(int *)(iVar1 + 0x103) = *(int *)(iVar1 + 0x103) + 1, *(int *)(iVar1 + 0x103) == 8)) {
    *(undefined4 *)(iVar1 + 0x103) = 0;
    iVar2 = FUN_0001519f(iVar1);
    if ((iVar2 == *(int *)(iVar1 + 0x107)) &&
       (iVar2 = FUN_000151d5(iVar1), iVar2 == *(int *)(iVar1 + 0x10b))) {
      *(undefined4 *)(iVar1 + 0x17) = 1;
      *(undefined4 *)(iVar1 + 0x1f) = 0;
      *(undefined4 *)(iVar1 + 0xaf) = 1;
      *(undefined4 *)(iVar1 + 199) = 1;
    }
    else {
      uVar3 = FUN_0001519f(iVar1);
      *(undefined4 *)(iVar1 + 0x107) = uVar3;
      uVar3 = FUN_000151d5(iVar1);
      *(undefined4 *)(iVar1 + 0x10b) = uVar3;
    }
  }
  iVar2 = FUN_0001519f(DAT_00053c7c);
  iVar4 = FUN_000151d5(DAT_00053c7c);
  if (*(int *)(iVar1 + 0x17) == 3) {
    if (((*(int *)(iVar1 + 0x6b) == 0) && (iVar5 = FUN_0001519f(iVar1), iVar5 < iVar2 + 200)) &&
       (iVar5 = FUN_0001519f(iVar1), iVar2 + -200 < iVar5)) {
      *(int *)(iVar1 + 0x6b) = *(int *)(iVar1 + 0x6b) + 1;
    }
    else if (*(int *)(iVar1 + 0x6b) != 0) {
      *(int *)(iVar1 + 0x6b) = *(int *)(iVar1 + 0x6b) + 1;
      if (*(int *)(&DAT_00053eac + *(int *)(iVar1 + 0xd) * 0x88) < 0x1f) {
        if (999 < *(int *)(iVar1 + 0x6b)) {
          *(undefined4 *)(iVar1 + 0x17) = 6;
          *(undefined4 *)(iVar1 + 0x11) = 0;
        }
      }
      else if (99 < *(int *)(iVar1 + 0x6b)) {
        *(undefined4 *)(iVar1 + 0x17) = 6;
        *(undefined4 *)(iVar1 + 0x11) = 0;
      }
      iVar5 = FUN_0001519f(iVar1);
      if (((iVar2 < iVar5 + 0x50) && (iVar5 = FUN_0001519f(iVar1), iVar5 + -0x50 < iVar2)) &&
         ((iVar5 = FUN_000151d5(iVar1), iVar4 < iVar5 + 0x28 &&
          (iVar5 = FUN_000151d5(iVar1), iVar5 + -0x28 < iVar4)))) {
        *(undefined4 *)(iVar1 + 0x17) = 6;
        *(undefined4 *)(iVar1 + 0x11) = 0;
      }
      iVar4 = FUN_0001519f(iVar1);
      if ((iVar2 < iVar4 + 10) && (iVar4 = FUN_0001519f(iVar1), iVar4 + -10 < iVar2)) {
        *(undefined4 *)(iVar1 + 0x17) = 6;
        *(undefined4 *)(iVar1 + 0x11) = 0;
      }
    }
  }
  if (iVar1 != DAT_00053c7c) {
    if ((0xb4 < *(uint *)(DAT_00053c7c + 0x17)) && (*(uint *)(DAT_00053c7c + 0x17) < 0xba)) {
      *(undefined4 *)(DAT_00053c7c + 0x4a) = 0;
    }
    switch(*(undefined4 *)(&DAT_00053eac + *(int *)(iVar1 + 0xd) * 0x88)) {
    case 0:
      FUN_00017f13(iVar1);
      break;
    case 1:
      FUN_00018e88(iVar1);
      break;
    case 2:
      FUN_00019e48(iVar1);
      break;
    case 3:
      FUN_0001aec8(iVar1);
      break;
    case 4:
      FUN_0001bea8(iVar1);
      break;
    case 5:
      FUN_0001cf64(iVar1,2);
      break;
    case 6:
      FUN_0001df8b(iVar1);
      break;
    case 7:
      FUN_0001f064(iVar1);
      break;
    case 8:
      FUN_00020170(iVar1);
      break;
    case 9:
      FUN_0001cf64(iVar1,0);
      break;
    case 10:
      FUN_0001cf64(iVar1,1);
      break;
    case 0xb:
      FUN_0002119c(iVar1);
      break;
    default:
      break;
    case 0x15:
      FUN_000221b8(iVar1);
      break;
    case 0x1f:
      FUN_0002319c(iVar1,2);
      break;
    case 0x20:
      FUN_00024bf9(iVar1);
      break;
    case 0x21:
      FUN_000265ac(iVar1,0);
      break;
    case 0x22:
      FUN_00027bec(iVar1,0);
      break;
    case 0x23:
      FUN_00029709(iVar1,0);
      break;
    case 0x24:
      FUN_0002adf5(iVar1,0);
      break;
    case 0x25:
      FUN_0002c838(iVar1,0);
      break;
    case 0x26:
      FUN_0002e2cc(iVar1,0);
      break;
    case 0x27:
      FUN_0002fe1e(iVar1,0);
    }
  }
  return;
}



// === ent_upd @ 00012616 ===

void FUN_00012616(void)

{
  byte *pbVar1;
  int iVar2;
  int local_2c;
  int *local_28;
  int *local_24;
  
  pbVar1 = (byte *)FUN_000418ee();
  if (*(int *)(pbVar1 + 0x17) == 0x29) {
    pbVar1[0x17] = 0x29;
    pbVar1[0x18] = 0;
    pbVar1[0x19] = 0;
    pbVar1[0x1a] = 0;
  }
  if ((pbVar1 != DAT_00053c7c) && (*pbVar1 == 0)) {
    if ((*(int *)(pbVar1 + 0x17) == 0x1d) && (0x2d < *(int *)(pbVar1 + 0x11))) {
      *pbVar1 = 1;
    }
    if ((*(int *)(pbVar1 + 0x17) == 0x16) && (0x12 < *(int *)(pbVar1 + 0x11))) {
      *pbVar1 = 1;
    }
  }
  if (*pbVar1 < 2) {
    FUN_0001194c(pbVar1);
  }
  local_28 = *(int **)(*(int *)(*(int *)(pbVar1 + 0x123) + 4) + *(int *)(pbVar1 + 0x17) * 4);
  if ((*(int *)(pbVar1 + 0x46) != 0) &&
     (iVar2 = FUN_0001435f(*(undefined4 *)(pbVar1 + 0x46),pbVar1[0x4e],pbVar1[0x4f]), iVar2 == 1)) {
    pbVar1[0x46] = 0;
    pbVar1[0x47] = 0;
    pbVar1[0x48] = 0;
    pbVar1[0x49] = 0;
    pbVar1[0x4e] = 0;
    pbVar1[0x4f] = 0;
  }
  if (*(short *)(pbVar1 + 0x50) == 0) {
    *(int *)(pbVar1 + 0x11) = *(int *)(pbVar1 + 0x11) + 1;
    if ((((int)(uint)*(byte *)(local_28 + 0x17) <= *(int *)(pbVar1 + 0x11)) &&
        (0xaa < *(uint *)(pbVar1 + 0x17))) && (*(uint *)(pbVar1 + 0x17) < 0xae)) {
      *(int *)(pbVar1 + 0x17) = *(int *)(pbVar1 + 0x17) + 1;
      pbVar1[0x11] = 0;
      pbVar1[0x12] = 0;
      pbVar1[0x13] = 0;
      pbVar1[0x14] = 0;
    }
    if ((((int)(uint)*(byte *)(local_28 + 0x17) <= *(int *)(pbVar1 + 0x11)) &&
        (0xb4 < *(uint *)(pbVar1 + 0x17))) && (*(uint *)(pbVar1 + 0x17) < 0xb9)) {
      *(int *)(pbVar1 + 0x17) = *(int *)(pbVar1 + 0x17) + 1;
      pbVar1[0x11] = 0;
      pbVar1[0x12] = 0;
      pbVar1[0x13] = 0;
      pbVar1[0x14] = 0;
    }
    if ((((int)(uint)*(byte *)(local_28 + 0x17) <= *(int *)(pbVar1 + 0x11)) &&
        (0xbe < *(uint *)(pbVar1 + 0x17))) && (*(uint *)(pbVar1 + 0x17) < 200)) {
      *(int *)(pbVar1 + 0x17) = *(int *)(pbVar1 + 0x17) + 1;
      pbVar1[0x11] = 0;
      pbVar1[0x12] = 0;
      pbVar1[0x13] = 0;
      pbVar1[0x14] = 0;
    }
    if ((pbVar1[0x31] == 0) || (*(int *)(pbVar1 + 0x11) < (int)(uint)*(byte *)(local_28 + 0x17))) {
      if (*local_28 <= *(int *)(pbVar1 + 0x11)) {
        if ((*(int *)(pbVar1 + 0x17) == 0x6f) || (*(int *)(pbVar1 + 0x17) == 0x71)) {
          pbVar1[0x4a] = 0;
          pbVar1[0x4b] = 0;
          pbVar1[0x4c] = 0;
          pbVar1[0x4d] = 0;
          pbVar1[0x31] = 0;
          pbVar1[0x17] = 1;
          pbVar1[0x18] = 0;
          pbVar1[0x19] = 0;
          pbVar1[0x1a] = 0;
          pbVar1[0x11] = 0;
          pbVar1[0x12] = 0;
          pbVar1[0x13] = 0;
          pbVar1[0x14] = 0;
        }
        if (pbVar1[0x31] == 0) {
          if ((*pbVar1 != 1) || (*(int *)(pbVar1 + 0x17) != 3)) {
            if (*(int *)(pbVar1 + 0x17) == 5) {
              pbVar1[5] = 0;
              pbVar1[6] = 0;
              pbVar1[7] = 0;
              pbVar1[8] = 0;
            }
            if (*(int *)(pbVar1 + 0x17) == 3) {
              pbVar1[0x17] = 3;
              pbVar1[0x18] = 0;
              pbVar1[0x19] = 0;
              pbVar1[0x1a] = 0;
            }
            else if ((*(int *)(pbVar1 + 0x17) == 0x70) &&
                    (*(char *)(*(int *)(pbVar1 + 0x4a) + 0x30) != '\x01')) {
              pbVar1[0x17] = 0x66;
              pbVar1[0x18] = 0;
              pbVar1[0x19] = 0;
              pbVar1[0x1a] = 0;
              pbVar1[0x50] = 200;
              pbVar1[0x51] = 0;
              *(undefined4 *)(*(int *)(pbVar1 + 0x4a) + 0x17) = 0x1f;
              *(undefined4 *)(*(int *)(pbVar1 + 0x4a) + 0x11) = 0;
              *(undefined2 *)(*(int *)(pbVar1 + 0x4a) + 0x50) = 200;
            }
            else if (*(int *)(pbVar1 + 0x17) != 2) {
              if (*(int *)(pbVar1 + 0x17) == 0x28) {
                pbVar1[0x17] = 0x28;
                pbVar1[0x18] = 0;
                pbVar1[0x19] = 0;
                pbVar1[0x1a] = 0;
              }
              else {
                pbVar1[0x17] = 1;
                pbVar1[0x18] = 0;
                pbVar1[0x19] = 0;
                pbVar1[0x1a] = 0;
              }
            }
          }
          pbVar1[0x11] = 0;
          pbVar1[0x12] = 0;
          pbVar1[0x13] = 0;
          pbVar1[0x14] = 0;
        }
        else {
          FUN_00031f5f(pbVar1);
          local_28 = *(int **)(*(int *)(*(int *)(pbVar1 + 0x123) + 4) + *(int *)(pbVar1 + 0x17) * 4)
          ;
        }
        if (pbVar1[0x30] == 1) {
          if (pbVar1 == DAT_00053c7c) {
            DAT_0006336c = 1;
          }
          else if (pbVar1 == DAT_00053c80) {
            DAT_0006336c = 3;
          }
          else if (DAT_00053c88 == pbVar1) {
            DAT_00053c88 = (byte *)0x0;
          }
          if ((*pbVar1 == 1) &&
             (DAT_000639bc = DAT_000639bc + 1,
             *(int *)(&DAT_00052a1c + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28
                     ) <= DAT_000639bc)) {
            DAT_0006336c = 4;
          }
          FUN_000112fd(pbVar1,0);
          FUN_00041c28();
          return;
        }
      }
    }
    else if ((pbVar1 == DAT_00053c7c) &&
            ((*(int *)(pbVar1 + 0x17) == 0x6f || (*(int *)(pbVar1 + 0x17) == 0x71)))) {
      pbVar1[0x31] = 0;
    }
    else {
      FUN_00031f5f(pbVar1);
      local_28 = *(int **)(*(int *)(*(int *)(pbVar1 + 0x123) + 4) + *(int *)(pbVar1 + 0x17) * 4);
    }
  }
  else if (*(int *)(pbVar1 + 0x11) < *local_28 + -1) {
    *(int *)(pbVar1 + 0x11) = *(int *)(pbVar1 + 0x11) + 1;
  }
  else {
    *(short *)(pbVar1 + 0x50) = *(short *)(pbVar1 + 0x50) + -1;
    if (((10 < *(uint *)(pbVar1 + 0x17)) && (*(uint *)(pbVar1 + 0x17) < 0x15)) &&
       (DAT_00063360 % 3 == 0)) {
      if (DAT_00063360 % 6 == 0) {
        FUN_0001545e(pbVar1,0xffffffff);
      }
      else {
        FUN_0001545e(pbVar1,1);
      }
    }
    if ((*(short *)(pbVar1 + 0x50) == 0) && (*(int *)(pbVar1 + 0x4a) != 0)) {
      *(undefined4 *)(*(int *)(pbVar1 + 0x4a) + 0x4a) = 0;
      pbVar1[0x4a] = 0;
      pbVar1[0x4b] = 0;
      pbVar1[0x4c] = 0;
      pbVar1[0x4d] = 0;
    }
  }
  if ((*(int *)(pbVar1 + 0x17) == 0x84) || (*(int *)(pbVar1 + 0x17) == 0x85)) {
    if (*(int *)(pbVar1 + 0x1b) == 0) {
      FUN_0001545e(pbVar1,*(int *)(pbVar1 + 0x17) + -0x82);
    }
    else {
      FUN_0001545e(pbVar1,0x82 - *(int *)(pbVar1 + 0x17));
    }
  }
  else {
    if ((*(uint *)(pbVar1 + 0x17) < 0x88) || (0x8b < *(uint *)(pbVar1 + 0x17))) {
      if ((*(int *)(pbVar1 + 0x17) == 0x8c) || (*(int *)(pbVar1 + 0x17) == 0x8d)) {
        if (*(int *)(pbVar1 + 0x11) < 0x35) {
          if (*(int *)(pbVar1 + 0x3e) == 0x84) {
            local_2c = 2;
          }
          else if (*(int *)(pbVar1 + 0x3e) == 0x85) {
            local_2c = 3;
          }
          else {
            local_2c = 0;
          }
        }
        else {
          local_2c = *(int *)(*(int *)((int)local_28 + 0x71) + *(int *)(pbVar1 + 0x11) * 4);
        }
      }
      else {
        local_2c = *(int *)(*(int *)((int)local_28 + 0x71) + *(int *)(pbVar1 + 0x11) * 4);
      }
    }
    else {
      local_24 = *(int **)(*(int *)(*(int *)(pbVar1 + 0x123) + 4) + *(int *)(pbVar1 + 0x3e) * 4);
      *(int *)(pbVar1 + 0x42) = *(int *)(pbVar1 + 0x42) + 1;
      if (*(int *)(pbVar1 + 0x3e) == 0x84) {
        local_2c = 2;
      }
      else if (*(int *)(pbVar1 + 0x3e) == 0x85) {
        local_2c = 3;
      }
      else {
        local_2c = 0;
      }
      if (*local_24 - 1U <= *(uint *)(pbVar1 + 0x42)) {
        pbVar1[0x11] = 0;
        pbVar1[0x12] = 0;
        pbVar1[0x13] = 0;
        pbVar1[0x14] = 0;
        pbVar1[0x17] = 1;
        pbVar1[0x18] = 0;
        pbVar1[0x19] = 0;
        pbVar1[0x1a] = 0;
        FUN_000348e2();
        local_2c = 0;
      }
    }
    if (*(int *)(pbVar1 + 0x1b) == 0) {
      FUN_0001545e(pbVar1,local_2c);
      if (pbVar1 == DAT_00053c7c) {
        *(int *)(DAT_00063354 + 0x1e) = *(int *)(DAT_00063354 + 0x1e) + local_2c;
      }
    }
    else {
      FUN_0001545e(pbVar1,-local_2c);
    }
  }
  if (*(char *)(*(int *)(*(int *)(*(int *)(pbVar1 + 0x123) + 4) + *(int *)(pbVar1 + 0x17) * 4) +
               0x6c) == '\x01') {
    if (pbVar1[0x16] != 0) {
      if (*(int *)(pbVar1 + 0x1b) == 0) {
        FUN_0001545e(pbVar1,-*(int *)(pbVar1 + 0x42));
      }
      else {
        FUN_0001545e(pbVar1,*(undefined4 *)(pbVar1 + 0x42));
      }
      FUN_00015295(pbVar1,*(undefined4 *)(pbVar1 + 0x3e));
    }
    if (pbVar1[0x16] < 2) {
      if ((*(uint *)(pbVar1 + 0x17) < 0x88) || (0x8b < *(uint *)(pbVar1 + 0x17))) {
        local_2c = *(undefined4 *)(*(int *)((int)local_28 + 0x75) + *(int *)(pbVar1 + 0x11) * 4);
      }
      else {
        local_2c = *(undefined4 *)(*(int *)((int)local_24 + 0x75) + *(int *)(pbVar1 + 0x42) * 4);
      }
      FUN_00015295(pbVar1,local_2c);
    }
    else {
      *(int *)(pbVar1 + 0x11) = *(int *)(pbVar1 + 0x11) + -1;
      pbVar1[0x16] = pbVar1[0x16] - 1;
    }
    if ((-1 < *(int *)(pbVar1 + 5)) &&
       (*(int *)(*(int *)((int)local_28 + 0x75) + *(int *)(pbVar1 + 0x11) * 4) != 0)) {
      pbVar1[0x16] = 0;
    }
  }
  else {
    FUN_0001586b(pbVar1,*(undefined4 *)
                         (*(int *)(pbVar1 + 0x11) * 4 + *(int *)((int)local_28 + 0x75)));
  }
  if (((((pbVar1 == DAT_00053c7c) && (*(int *)(DAT_00053c7c + 0x17) == 2)) &&
       (*(int *)(DAT_00063354 + 0x22) =
             *(int *)(DAT_00063354 + 0x22) +
             *(int *)(*(int *)((int)local_28 + 0x75) + *(int *)(pbVar1 + 0x11) * 4),
       DAT_0005d0a0 <= *(int *)(pbVar1 + 1))) &&
      ((*(int *)(pbVar1 + 1) <= DAT_0005d0a8 && (DAT_0005d0a4 <= *(int *)(pbVar1 + 9))))) &&
     (*(int *)(pbVar1 + 9) <= DAT_0005d0ac)) {
    if (DAT_0005d0b0 == 0) {
      if (DAT_00063ed5 != '\0') {
        if (*(int *)(DAT_00063354 + 0x34c) != 0) {
          DAT_00063398 = 0;
        }
        DAT_0006336c = 2;
      }
    }
    else if (DAT_00063ed3 != '\0') {
      if (*(int *)(DAT_00063354 + 0x34c) != 0) {
        DAT_00063398 = 1;
      }
      DAT_0006336c = 2;
    }
  }
  return;
}



// === ent_upd2 @ 0001441e ===

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



// === in_lo @ 00034735 ===

void FUN_00034735(void)

{
  byte bVar1;
  
  bVar1 = FUN_000418ee();
  if ((&DAT_00062c58)[bVar1] == '\x04') {
    FUN_00034353(bVar1,DAT_00063f08);
    if ((&DAT_00062cd8)[bVar1] == '\0') {
      (&DAT_00062c58)[bVar1] = 0;
    }
    else {
      (&DAT_00062c58)[bVar1] = 1;
      (&DAT_00062cd8)[bVar1] = (&DAT_00062cd8)[bVar1] + -1;
    }
  }
  else if ((byte)(&DAT_00062c58)[bVar1] < 4) {
    (&DAT_00062c58)[bVar1] = (&DAT_00062c58)[bVar1] + '\x01';
    FUN_0003442e(bVar1);
  }
  return;
}



// === in_hi @ 00034635 ===

void FUN_00034635(void)

{
  byte bVar1;
  
  bVar1 = FUN_000418ee();
  if ((&DAT_00062c58)[bVar1] == '\x04') {
    if ((bVar1 == 0x4d) || (bVar1 == 0x4b)) {
      if (DAT_00062a94 == 0) {
        FUN_00034353(-0x4f - bVar1,DAT_00063f08);
      }
      else {
        FUN_00034353(bVar1 + 0x19,DAT_00063f08);
      }
    }
    else {
      FUN_00034353(bVar1,DAT_00063f08);
    }
    if ((&DAT_00062cd8)[bVar1] == '\0') {
      (&DAT_00062c58)[bVar1] = 0;
    }
    else {
      (&DAT_00062c58)[bVar1] = 1;
      (&DAT_00062cd8)[bVar1] = (&DAT_00062cd8)[bVar1] + -1;
    }
  }
  else {
    if ((&DAT_00062c58)[bVar1] == '\x01') {
      DAT_00062a94 = *(int *)(DAT_00053c7c + 0x1b);
    }
    (&DAT_00062c58)[bVar1] = (&DAT_00062c58)[bVar1] + '\x01';
  }
  return;
}



// === spawner @ 0003d606 ===

void FUN_0003d606(void)

{
  int iVar1;
  undefined4 local_24;
  
  iVar1 = FUN_000418ee();
  for (local_24 = 0; local_24 < *(int *)(iVar1 + 0x3c); local_24 = local_24 + 1) {
    FUN_0003d67c(iVar1 + 0x48,*(undefined1 *)(*(int *)(iVar1 + 0x44) + local_24 * 2),
                 *(undefined1 *)(local_24 * 2 + *(int *)(iVar1 + 0x44) + 1));
  }
  return;
}



// === trig_a @ 0003cb38 ===

void FUN_0003cb38(void)

{
  int iVar1;
  int iVar2;
  
  iVar2 = FUN_000418ee();
  iVar1 = DAT_000639dc;
  if (DAT_000639e4 == '\0') {
    if (*(int *)(DAT_00053c7c + 0x17) == 3) {
      if (DAT_000639dc < *(int *)(iVar2 + 8) + -0x142) {
        DAT_000639dc = DAT_000639dc + 3;
      }
      else {
        DAT_000639dc = *(int *)(iVar2 + 8) + -0x140;
      }
    }
    else if (DAT_000639dc < *(int *)(iVar2 + 8) + -0x140) {
      DAT_000639dc = DAT_000639dc + 1;
    }
  }
  if (iVar1 != DAT_000639dc) {
    FUN_0003d75c(DAT_000639dc - iVar1);
  }
  return;
}



// === trig_b @ 0003cbd9 ===

void FUN_0003cbd9(void)

{
  int iVar1;
  int *piVar2;
  
  piVar2 = (int *)FUN_000418ee();
  iVar1 = DAT_000639dc;
  if (DAT_000639e4 == '\0') {
    if (*(int *)(DAT_00053c7c + 0x17) == 3) {
      if (*piVar2 + 2 < DAT_000639dc) {
        DAT_000639dc = DAT_000639dc + -3;
      }
      else {
        DAT_000639dc = *piVar2;
      }
    }
    else if (*piVar2 < DAT_000639dc) {
      DAT_000639dc = DAT_000639dc + -1;
    }
  }
  if (iVar1 != DAT_000639dc) {
    FUN_0003d75c(DAT_000639dc - iVar1);
  }
  return;
}



// === trig_c @ 0003cc6b ===

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0003cc6b(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_000639e4 == '\0') {
    if (DAT_000639dc == **(int **)(iVar1 + 0x34c)) {
      DAT_00063354 = *(undefined4 *)(iVar1 + 0x34c);
      DAT_000639e4 = '\x01';
      if (DAT_00063370 != 1) {
        DAT_00063374 = *(undefined4 *)(DAT_00053c7c + 0x2b);
        _DAT_00063384 = (uint)*(byte *)(DAT_00053c7c + 0x28);
        DAT_00063378 = (uint)*(byte *)(DAT_00053c7c + 0x27);
        DAT_0006337c = (uint)*(byte *)(DAT_00053c7c + 0x26);
        _DAT_00063380 = (uint)*(byte *)(DAT_00053c7c + 0x25);
        DAT_00063358 = DAT_00063354;
      }
      if (DAT_000639e0 < *(int *)(*(int *)(iVar1 + 0x34c) + 0xc) + -200) {
        DAT_000639e0 = DAT_000639e0 + 1;
      }
    }
  }
  else {
    if (*(int *)(*(int *)(iVar1 + 0x34c) + 4) + -1 == DAT_000639e0) {
      DAT_00063354 = *(undefined4 *)(iVar1 + 0x34c);
      DAT_000639e4 = '\0';
      if (DAT_00063370 != 1) {
        DAT_00063374 = *(undefined4 *)(DAT_00053c7c + 0x2b);
        _DAT_00063384 = (uint)*(byte *)(DAT_00053c7c + 0x28);
        DAT_00063378 = (uint)*(byte *)(DAT_00053c7c + 0x27);
        DAT_0006337c = (uint)*(byte *)(DAT_00053c7c + 0x26);
        _DAT_00063380 = (uint)*(byte *)(DAT_00053c7c + 0x25);
        DAT_00063358 = DAT_00063354;
      }
    }
    if (DAT_000639e0 < *(int *)(*(int *)(iVar1 + 0x34c) + 0xc) + -200) {
      DAT_000639e0 = DAT_000639e0 + 1;
    }
  }
  return;
}



// === wavereset @ 000376c8 ===

void FUN_000376c8(void)

{
  undefined4 *puVar1;
  undefined4 uStack_44;
  undefined4 uStack_40;
  undefined1 auStack_3c [16];
  int local_2c;
  int local_28;
  int local_24;
  int local_20;
  
  FUN_000418ee();
  puVar1 = (undefined4 *)auStack_3c;
  uStack_40 = 0x376e5;
  FUN_0003d88f();
  uStack_40 = 0x376ea;
  FUN_0003237b();
  uStack_40 = 0x376ef;
  FUN_0003ee0a();
  uStack_40 = 0x376f9;
  local_28 = FUN_00041bdc(0x147);
  local_24 = local_28;
  if (local_28 == 0) {
    local_20 = 0;
  }
  else {
    uStack_40 = 0;
    puVar1 = &uStack_44;
    uStack_44 = 0;
    local_2c = FUN_00010efc(local_28,0xffffffe2,0,
                            *(undefined4 *)
                             (&DAT_00052c3c +
                             *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28));
    local_20 = local_2c;
  }
  DAT_00053c7c = local_20;
  *(undefined2 *)(local_20 + 0x23) = 0;
  *(undefined1 *)(DAT_00053c7c + 0x28) = DAT_00063384;
  if (*(char *)(DAT_00053c7c + 0x28) == '\0') {
    *(uint *)(DAT_00053c7c + 0x56) = DAT_00053e9c & 0xffff;
  }
  else {
    *(uint *)(DAT_00053c7c + 0x56) =
         *(uint *)(&DAT_00052738 + (uint)*(byte *)(DAT_00053c7c + 0x28) * 4) & 0xffff;
  }
  *(undefined2 *)(DAT_00053c7c + 0x29) = DAT_00053ea4;
  DAT_00053c88 = 0;
  DAT_00053c84 = DAT_00053c7c;
  *(undefined4 *)((int)puVar1 + -4) = 0x377e3;
  FUN_00037b03(1);
  if (DAT_00063358 == 0) {
    DAT_00063354 = DAT_0006334c;
  }
  else {
    DAT_00063354 = DAT_00063358;
  }
  DAT_00062a98 = (&DAT_00062aa0)[DAT_00063378];
  DAT_00062a9c = (&DAT_00062ab4)[DAT_00063378];
  DAT_00053818 = (&DAT_00053c28)[DAT_0006337c];
  *(undefined4 *)(DAT_00053c7c + 0x123) = DAT_00053818;
  *(undefined4 *)(DAT_00053c7c + 0x2b) = DAT_00063374;
  *(undefined1 *)(DAT_00053c7c + 0x27) = (undefined1)DAT_00063378;
  *(undefined1 *)(DAT_00053c7c + 0x26) = (undefined1)DAT_0006337c;
  *(undefined1 *)(DAT_00053c7c + 0x25) = DAT_00063380;
  *(undefined4 *)((int)puVar1 + -4) = 0x3788e;
  FUN_0003cad3(DAT_00063354);
  if (DAT_00063398 == 0) {
    *(int *)(DAT_00053c7c + 1) = DAT_000639dc + -0x1e;
    *(undefined4 *)(DAT_00053c7c + 0x1b) = 0;
  }
  else {
    *(int *)(DAT_00053c7c + 1) = DAT_000639dc + 0x15e;
    *(undefined4 *)(DAT_00053c7c + 0x1b) = 1;
  }
  *(int *)(DAT_00053c7c + 9) =
       DAT_000639e0 +
       *(int *)(&DAT_00052c3c + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28);
  *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
  *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
  return;
}



// === stageend @ 00010d47 ===

void FUN_00010d47(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  FUN_000418ee();
  uVar2 = DAT_00063e28;
  uVar1 = DAT_00063e24;
  if (DAT_00053c48 < DAT_00063e38 / 0x1ea) {
    DAT_00063e38 = 0;
    DAT_00052ff8 = 0;
    FUN_00010b1d(*(undefined4 *)
                  (&DAT_00052da4 + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28));
    DAT_00052ff8 = 1;
  }
  DAT_00063e24 = uVar1;
  DAT_00063e28 = uVar2;
  return;
}



// === draw_ce0b @ 0003ce0b ===

int FUN_0003ce0b(void)

{
  int iVar1;
  
  iVar1 = FUN_000418ee();
  if (DAT_000639e4 == '\0') {
    FUN_00040491();
  }
  else if (DAT_000639e0 < *(int *)(iVar1 + 4)) {
    FUN_00040491();
    FUN_00040491();
  }
  else if (*(int *)(iVar1 + 0xc) < DAT_000639e0 + 200) {
    FUN_00040491();
    FUN_00040491();
  }
  else {
    FUN_00040491();
  }
  *(undefined4 *)(iVar1 + 0x22) = 0;
  *(undefined4 *)(iVar1 + 0x1e) = 0;
  iVar1 = DAT_00063360 / 10;
  if (DAT_00063360 % 10 == 0) {
    iVar1 = FUN_00040042();
  }
  return iVar1;
}



// === draw_203d @ 0003203d ===

void FUN_0003203d(void)

{
  undefined4 uVar1;
  undefined4 local_28;
  undefined4 local_24;
  
  uVar1 = FUN_000418ee();
  local_24 = DAT_00053c70;
  for (local_28 = 0; local_28 < DAT_00053c68; local_28 = local_28 + 1) {
    FUN_00012efa(local_24,uVar1);
    local_24 = *(int *)(local_24 + 0x13f);
  }
  return;
}



// === draw_df96 @ 0003df96 ===

void FUN_0003df96(void)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int extraout_EDX;
  int *piVar6;
  float10 fVar7;
  int local_2c;
  int local_28;
  int local_24;
  undefined4 local_20;
  
  local_20 = FUN_000418ee();
  piVar6 = &local_2c;
  for (local_24 = 0; local_24 < DAT_00062a90; local_24 = local_24 + 1) {
    local_28 = (&DAT_0005d0b4)[local_24 * 5];
    *(uint *)((int)piVar6 + -4) = (uint)*(byte *)(&DAT_0005d0c0 + local_24 * 5);
    *(int *)((int)piVar6 + -8) =
         *(int *)(DAT_00063dfc + 0x38) + *(int *)(*(int *)(DAT_00063dfc + 0x34) + local_28 * 4);
    *(undefined4 *)((int)piVar6 + -0xc) = *(undefined4 *)(*(int *)(DAT_00063dfc + 8) + local_28 * 4)
    ;
    uVar1 = *(undefined4 *)(*(int *)(DAT_00063dfc + 4) + local_28 * 4);
    iVar2 = (&DAT_0005d0bc)[local_24 * 5];
    iVar3 = *(int *)(*(int *)(DAT_00063dfc + 0x10) + local_28 * 4);
    iVar4 = (&DAT_0005d0b8)[local_24 * 5];
    iVar5 = *(int *)(*(int *)(DAT_00063dfc + 0xc) + local_28 * 4);
    fVar7 = (float10)(float)(&DAT_0005d0c4)[local_24 * 5];
    *(undefined4 *)((int)piVar6 + -0x10) = 0x3e083;
    FUN_00041f7e(local_24 * 0x14,(iVar4 - iVar5) - DAT_000639dc,(iVar2 - iVar3) - DAT_000639e0,uVar1
                );
    local_2c = (int)ROUND(fVar7);
    *(undefined4 *)((int)piVar6 + -0x10) = 0x3e091;
    FUN_0003bb42(local_20,extraout_EDX + local_2c);
    piVar6 = (int *)((int)piVar6 + -0xc);
  }
  return;
}



// === draw_b953 @ 0003b953 ===

void FUN_0003b953(void)

{
  undefined4 uVar1;
  undefined4 local_24;
  
  uVar1 = FUN_000418ee();
  DAT_000639d4 = DAT_000639cc;
  for (local_24 = 0; local_24 < DAT_000639c8; local_24 = local_24 + 1) {
    FUN_0003b718(DAT_000639d4,uVar1);
    DAT_000639d4 = *(int *)(DAT_000639d4 + 0x11);
  }
  return;
}



// === music_272f @ 0003272f ===

void FUN_0003272f(void)

{
  int *piVar1;
  byte bVar2;
  uint uVar3;
  int iVar4;
  char cVar5;
  int *piVar6;
  int local_38;
  uint local_34;
  int local_30;
  int local_2c;
  
  piVar6 = (int *)FUN_000418ee();
  if (*(char *)(DAT_00053c7c + 0x30) == '\x01') {
    return;
  }
  if (DAT_0006339c == 500) {
    *(undefined4 *)(DAT_00053c7c + 0x17) = 0x28;
    *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
  }
  if ((*(int *)(DAT_00053c7c + 0x17) == 0x66) &&
     ((cVar5 = FUN_0003493e(), cVar5 == '*' || (cVar5 == '6')))) {
    *(undefined4 *)(*(int *)(DAT_00053c7c + 0x4a) + 0x17) = 1;
    *(undefined4 *)(*(int *)(DAT_00053c7c + 0x4a) + 0x11) = 0;
    *(undefined4 *)(*(int *)(DAT_00053c7c + 0x4a) + 0x4a) = 0;
    *(undefined2 *)(*(int *)(DAT_00053c7c + 0x4a) + 0x50) = 0;
    *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
    *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
    *(undefined4 *)(DAT_00053c7c + 0x4a) = 0;
    *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
    FUN_000348e2();
    return;
  }
  if ((*(int *)(DAT_00053c7c + 0x17) == 0x66) || (*(int *)(DAT_00053c7c + 0x17) == 0x70))
  goto LAB_000339c5;
  uVar3 = *(uint *)(DAT_00053c7c + 0x17);
  if (uVar3 < 0x47) {
    if (uVar3 < 0x15) {
      if (uVar3 < 3) {
        if (uVar3 != 0) {
LAB_0003284e:
          cVar5 = FUN_0003493e();
          if ((((DAT_00063ed8 != '\0') || (DAT_00063ed0 != '\0')) || (DAT_00063ed3 != '\0')) ||
             (DAT_00063ed5 != '\0')) {
            iVar4 = *(int *)(&DAT_00052ffc +
                            ((DAT_00063360 % 4) * 3 + (uint)*(byte *)(DAT_00053c7c + 0x25)) * 4);
            if (DAT_00063ed0 == '\0') {
              if (DAT_00063ed8 != '\0') {
                FUN_0001586b(DAT_00053c7c,iVar4);
              }
            }
            else {
              FUN_0001586b(DAT_00053c7c,-iVar4);
            }
            if (DAT_00063ed3 == '\0') {
              if (DAT_00063ed5 != '\0') {
                FUN_0001545e(DAT_00053c7c,iVar4);
                *(undefined4 *)(DAT_00053c7c + 0x1b) = 0;
              }
            }
            else {
              FUN_0001545e(DAT_00053c7c,-iVar4);
              *(undefined4 *)(DAT_00053c7c + 0x1b) = 1;
            }
            if (*(int *)(DAT_00053c7c + 0x17) != 2) {
              *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
              *(undefined4 *)(DAT_00053c7c + 0x17) = 2;
            }
          }
          if (((DAT_00063ec1 != '\0') || (DAT_00063eb4 != '\0')) &&
             (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x97;
            *(undefined4 *)(DAT_00053c7c + 0x42) = 0;
            FUN_000348e2();
          }
          if ((cVar5 == '*') || (cVar5 == '6')) {
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            if ((DAT_00063ed3 == '\0') && (DAT_00063ed5 == '\0')) {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x83;
              *(undefined4 *)(DAT_00053c7c + 0x42) = 0;
              DAT_00062d58 = 0x83;
            }
            else {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x84;
              DAT_00062d58 = 0x84;
            }
            FUN_000348e2();
          }
        }
      }
      else if (uVar3 < 4) {
        cVar5 = FUN_0003493e();
        if (((cVar5 == '8') || (cVar5 == '\x1d')) && (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0xa3;
          DAT_00062d58 = 0xa3;
          FUN_000348e2();
        }
        if (((cVar5 == '*') || (cVar5 == '6')) && (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x85;
          DAT_00062d58 = 0x85;
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          FUN_000348e2();
        }
        if (*(int *)(DAT_00053c7c + 0x1b) == 0) {
          if ((DAT_00063ed5 == '\0') &&
             ((*(int *)(DAT_00053c7c + 0x11) == 0x13 || (*(int *)(DAT_00053c7c + 0x11) == 0x2d)))) {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 4;
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          }
        }
        else if ((DAT_00063ed3 == '\0') &&
                ((*(int *)(DAT_00053c7c + 0x11) == 0x13 || (*(int *)(DAT_00053c7c + 0x11) == 0x2d)))
                ) {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 4;
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
        }
      }
      else if (uVar3 == 5) {
        cVar5 = FUN_0003493e();
        if (*(int *)(DAT_00053c7c + 0x11) == 8) {
          if ((cVar5 == '8') && (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x9c;
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
            *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
            *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
            FUN_000348e2();
          }
        }
        else if (*(int *)(DAT_00053c7c + 0x11) == 0x11) {
          *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
          FUN_000348e2();
        }
      }
    }
    else if (uVar3 < 0x16) {
      cVar5 = FUN_0003493e();
      if ((*(int *)(DAT_00053c7c + 0x11) < 0x32) && ((cVar5 == '*' || (cVar5 == '6')))) {
        if (((*(short *)(DAT_00053c7c + 0x23) != 0) || (*(short *)(DAT_00053c7c + 0x29) != 0)) &&
           (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x98;
          *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
          *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
          if ((*(short *)(DAT_00053c7c + 0x23) == 0) && (*(char *)(DAT_00053c7c + 0x30) == '\x01'))
          {
            *(undefined1 *)(DAT_00053c7c + 0x30) = 2;
          }
        }
        FUN_000348e2();
      }
      else if ((*(int *)(DAT_00053c7c + 0x11) < 0x56) && ((cVar5 == '*' || (cVar5 == '6')))) {
        *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
      }
    }
    else if (uVar3 < 0x1e) {
      if (uVar3 == 0x16) {
        cVar5 = FUN_0003493e();
        if ((*(int *)(DAT_00053c7c + 0x11) < 0x1e) && ((cVar5 == '*' || (cVar5 == '6')))) {
          if (((*(short *)(DAT_00053c7c + 0x23) != 0) || (*(short *)(DAT_00053c7c + 0x29) != 0)) &&
             (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x9b;
            *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
            *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
            if ((*(short *)(DAT_00053c7c + 0x23) == 0) && (*(char *)(DAT_00053c7c + 0x30) == '\x01')
               ) {
              *(undefined1 *)(DAT_00053c7c + 0x30) = 2;
            }
            FUN_000348e2();
          }
        }
        else if ((*(int *)(DAT_00053c7c + 0x11) < 0x44) && ((cVar5 == '*' || (cVar5 == '6')))) {
          *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
          FUN_000348e2();
        }
      }
    }
    else if (uVar3 < 0x1f) {
      cVar5 = FUN_0003493e();
      if ((((9 < *(int *)(DAT_00053c7c + 0x11)) && (*(int *)(DAT_00053c7c + 0x11) < 0x3b)) &&
          ((cVar5 == '*' || (cVar5 == '6')))) && (*(char *)(DAT_00053c7c + 0x30) != '\x01')) {
        *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
      }
    }
    else if (0x27 < uVar3) {
      if (uVar3 < 0x29) goto LAB_0003284e;
      if ((uVar3 == 0x29) &&
         (((cVar5 = FUN_0003493e(), cVar5 == '*' || (cVar5 == '6')) &&
          (*(char *)(DAT_00053c7c + 0x27) != '\0')))) {
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0x85;
        DAT_00062d58 = 0x85;
        *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
        FUN_000348e2();
      }
    }
  }
  else if (uVar3 < 0x48) {
    cVar5 = FUN_0003493e();
    if (((cVar5 == '*') || (cVar5 == '6')) && (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
      *(undefined4 *)(DAT_00053c7c + 0x17) = 0x85;
      DAT_00062d58 = 0x85;
      *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
      FUN_000348e2();
    }
  }
  else if (uVar3 < 0x88) {
    if (uVar3 < 0x84) {
      if (uVar3 == 0x83) {
        cVar5 = FUN_0003493e();
        if (*(int *)(DAT_00053c7c + 0x11) == 0x3e) {
          FUN_000348e2();
          *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
        }
        if ((cVar5 == 'g') && (*(int *)(DAT_00053c7c + 0x11) < 0x1c)) {
          *(undefined4 *)(DAT_00053c7c + 0x11f) = 0x1fffff;
        }
        FUN_000342ff(2);
        if ((cVar5 == '8') || (cVar5 == '\x1d')) {
          if ((0xb < *(int *)(DAT_00053c7c + 0x11)) && (*(int *)(DAT_00053c7c + 0x11) < 0x33)) {
            *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
            *(undefined4 *)(DAT_00053c7c + 0x42) = *(undefined4 *)(DAT_00053c7c + 0x11);
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            if (cVar5 == '\x1d') {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x88;
              FUN_000348e2();
            }
            else if (cVar5 == '8') {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x89;
              FUN_000348e2();
            }
          }
        }
        else if ((cVar5 == 'g') || (*(int *)(DAT_00053c7c + 0x11f) == 0x1fffff)) {
          if ((*(int *)(DAT_00053c7c + 0x11) == 0x1c) && (1 < *(byte *)(DAT_00053c7c + 0x27))) {
            *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
            if (*(char *)(DAT_00053c7c + 0x26) == '\x02') {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8d;
            }
            else {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8c;
            }
            *(undefined4 *)(DAT_00053c7c + 0x11f) = 0;
            FUN_000348e2();
          }
        }
        else if ((((cVar5 == 'i') && (3 < *(byte *)(DAT_00053c7c + 0x27))) &&
                 (0xb < *(int *)(DAT_00053c7c + 0x11))) && (*(int *)(DAT_00053c7c + 0x11) < 0x33)) {
          *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
          *(undefined4 *)(DAT_00053c7c + 0x42) = *(undefined4 *)(DAT_00053c7c + 0x11);
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8b;
          FUN_000348e2();
        }
      }
    }
    else if (uVar3 < 0x85) {
      cVar5 = FUN_0003493e();
      if (*(int *)(DAT_00053c7c + 0x11) == 0x3e) {
        FUN_000348e2();
        *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
      }
      if ((cVar5 == 'g') && (*(int *)(DAT_00053c7c + 0x11) < 0x1c)) {
        *(undefined4 *)(DAT_00053c7c + 0x11f) = 0x1fffff;
      }
      FUN_00034295(3);
      if ((cVar5 == '8') || (cVar5 == '\x1d')) {
        if ((0xb < *(int *)(DAT_00053c7c + 0x11)) && (*(int *)(DAT_00053c7c + 0x11) < 0x33)) {
          *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
          *(undefined4 *)(DAT_00053c7c + 0x42) = *(undefined4 *)(DAT_00053c7c + 0x11);
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          if (cVar5 == '\x1d') {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x88;
            FUN_000348e2();
          }
          else if (cVar5 == '8') {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8a;
            FUN_000348e2();
          }
        }
      }
      else if ((((cVar5 == 'g') || (*(int *)(DAT_00053c7c + 0x11f) == 0x1fffff)) &&
               (*(int *)(DAT_00053c7c + 0x11) == 0x1c)) && (1 < *(byte *)(DAT_00053c7c + 0x27))) {
        *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
        if (*(char *)(DAT_00053c7c + 0x26) == '\x02') {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8d;
        }
        else {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8c;
        }
        *(undefined4 *)(DAT_00053c7c + 0x11f) = 0;
        FUN_000348e2();
      }
    }
    else if (uVar3 == 0x85) {
      cVar5 = FUN_0003493e();
      if (*(int *)(DAT_00053c7c + 0x11) == 0x3e) {
        FUN_000348e2();
        *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
      }
      if ((cVar5 == 'g') && (*(int *)(DAT_00053c7c + 0x11) < 0x1c)) {
        *(undefined4 *)(DAT_00053c7c + 0x11f) = 0x1fffff;
      }
      FUN_00034295(1);
      if ((cVar5 == '8') || (cVar5 == '\x1d')) {
        if ((0xb < *(int *)(DAT_00053c7c + 0x11)) && (*(int *)(DAT_00053c7c + 0x11) < 0x33)) {
          *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
          *(undefined4 *)(DAT_00053c7c + 0x42) = *(undefined4 *)(DAT_00053c7c + 0x11);
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          if (cVar5 == '\x1d') {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x88;
            FUN_000348e2();
          }
          else if (cVar5 == '8') {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8a;
            FUN_000348e2();
          }
        }
      }
      else if ((((cVar5 == 'g') || (*(int *)(DAT_00053c7c + 0x11f) == 0x1fffff)) &&
               (*(int *)(DAT_00053c7c + 0x11) == 0x1c)) && (1 < *(byte *)(DAT_00053c7c + 0x27))) {
        *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8d;
        *(undefined4 *)(DAT_00053c7c + 0x11f) = 0;
        FUN_000348e2();
      }
    }
  }
  else if (uVar3 < 0x8e) {
    if ((*(int *)(DAT_00053c7c + 0x17) == 0x8c) && (*(int *)(DAT_00053c7c + 0x11) == 0x67)) {
      FUN_000348e2();
      *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
    }
    if ((*(int *)(DAT_00053c7c + 0x17) != 0x8c) || (*(int *)(DAT_00053c7c + 0x11) < 0x35)) {
      if ((*(int *)(DAT_00053c7c + 0x17) == 0x8d) && (*(int *)(DAT_00053c7c + 0x11) == 0x62)) {
        FUN_000348e2();
        *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
      }
      if ((*(int *)(DAT_00053c7c + 0x17) != 0x8d) || (*(int *)(DAT_00053c7c + 0x11) < 0x2f)) {
        if (*(int *)(DAT_00053c7c + 0x3e) == 0x83) {
          FUN_000342ff(2);
        }
        else if (*(int *)(DAT_00053c7c + 0x3e) == 0x84) {
          FUN_00034295(3);
        }
        else if (*(int *)(DAT_00053c7c + 0x3e) == 0x85) {
          FUN_00034295(1);
        }
        if (((*(int *)(DAT_00053c7c + 0x42) == 0x39) && (*(int *)(DAT_00053c7c + 0x17) != 0x8c)) &&
           (*(int *)(DAT_00053c7c + 0x17) != 0x8d)) {
          *(undefined4 *)(DAT_00053c7c + 0x17) = *(undefined4 *)(DAT_00053c7c + 0x3e);
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0x39;
        }
      }
    }
  }
  else if (uVar3 < 0x98) {
    if (uVar3 == 0x97) {
      FUN_0003493e();
      if (((DAT_00063ec1 != '\0') || (DAT_00063eb4 != '\0')) &&
         (0x13 < *(int *)(DAT_00053c7c + 0x11))) {
        *(int *)(DAT_00053c7c + 0x11) = *(int *)(DAT_00053c7c + 0x11) + -1;
        *(int *)(DAT_00053c7c + 0x42) = *(int *)(DAT_00053c7c + 0x42) + 1;
      }
      if (((DAT_00063ea5 != '\0') &&
          (0x14 < (uint)(*(int *)(DAT_00053c7c + 0x11) + *(int *)(DAT_00053c7c + 0x42)))) &&
         (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
        DAT_00063364 = 0x26;
        DAT_00063368 = 0;
        *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0xa8;
        FUN_000348e2();
      }
      if (((DAT_00063ec0 != '\0') &&
          (0x14 < (uint)(*(int *)(DAT_00053c7c + 0x11) + *(int *)(DAT_00053c7c + 0x42)))) &&
         (1 < *(byte *)(DAT_00053c7c + 0x27))) {
        DAT_00063364 = 0x21;
        DAT_00063368 = 0;
        *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0xa9;
        FUN_000348e2();
      }
    }
  }
  else if (uVar3 < 0x99) {
LAB_0003358f:
    cVar5 = FUN_0003493e();
    if (cVar5 == '8') {
      *(undefined1 *)(DAT_00053c7c + 0x31) = 1;
      if (*(char *)(DAT_00053c7c + 0x27) == '\x04') {
        *(undefined4 *)(DAT_00053c7c + 0x32) = 0x2e;
      }
      else {
        *(undefined4 *)(DAT_00053c7c + 0x32) = 0x2d;
      }
    }
  }
  else if (0x9a < uVar3) {
    if (uVar3 < 0x9c) goto LAB_0003358f;
    if (((uVar3 == 0xa5) && (5 < *(int *)(DAT_00053c7c + 0x11))) && (DAT_00063ea5 == '\0')) {
      DAT_00063364 = 0x20;
      DAT_00063368 = 0;
      if ((*(int *)(DAT_00053c7c + 0x11) < 0) || (0x1e < *(int *)(DAT_00053c7c + 0x11))) {
        if ((*(int *)(DAT_00053c7c + 0x11) < 0x1f) || (0x3c < *(int *)(DAT_00053c7c + 0x11))) {
          if ((*(int *)(DAT_00053c7c + 0x11) < 0x3d) || (0x50 < *(int *)(DAT_00053c7c + 0x11))) {
            if ((0x50 < *(int *)(DAT_00053c7c + 0x11)) && (*(int *)(DAT_00053c7c + 0x11) < 0x65)) {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x2b;
            }
          }
          else {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x2a;
          }
        }
        else {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x29;
        }
      }
      else {
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0xa6;
      }
      *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
    }
  }
  if ((((((10 < *(uint *)(DAT_00053c7c + 0x17)) && (*(uint *)(DAT_00053c7c + 0x17) < 0x14)) ||
        ((0x32 < *(uint *)(DAT_00053c7c + 0x17) && (*(uint *)(DAT_00053c7c + 0x17) < 0x51)))) ||
       ((*(int *)(DAT_00053c7c + 0x17) == 0xa4 || (*(int *)(DAT_00053c7c + 0x17) == 0xa6)))) ||
      ((*(int *)(DAT_00053c7c + 0x17) == 0xa7 ||
       ((*(int *)(DAT_00053c7c + 0x17) == 0x2f ||
        ((0xaa < *(uint *)(DAT_00053c7c + 0x17) && (*(uint *)(DAT_00053c7c + 0x17) < 0xae)))))))) ||
     ((0xb4 < *(uint *)(DAT_00053c7c + 0x17) && (*(uint *)(DAT_00053c7c + 0x17) < 0xb9)))) {
    cVar5 = FUN_0003493e();
    if (((DAT_00063ec1 != '\0') || (DAT_00063eb4 != '\0')) &&
       (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
      *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
      *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
      *(undefined4 *)(DAT_00053c7c + 0x17) = 0x97;
      *(undefined4 *)(DAT_00053c7c + 0x42) = 0;
      *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
      FUN_000348e2();
    }
    if ((cVar5 == '*') || (cVar5 == '6')) {
      *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
      if ((DAT_00063ed3 == '\0') && (DAT_00063ed5 == '\0')) {
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0x83;
        *(undefined4 *)(DAT_00053c7c + 0x42) = 0;
        DAT_00062d58 = 0x83;
      }
      else {
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0x84;
        DAT_00062d58 = 0x84;
      }
      *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
      FUN_000348e2();
      *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
    }
  }
  if (((DAT_00063ed8 == '\0') && (DAT_00063ed0 == '\0')) &&
     ((DAT_00063ed3 == '\0' && ((DAT_00063ed5 == '\0' && (*(uint *)(DAT_00053c7c + 0x17) < 3)))))) {
    *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
  }
LAB_000339c5:
  if (((*(uint *)(DAT_00053c7c + 0x17) < 0x15) || (0x1e < *(uint *)(DAT_00053c7c + 0x17))) &&
     (*(int *)(DAT_00053c7c + 0x17) != 5)) {
    for (local_38 = 0; local_38 < *piVar6; local_38 = local_38 + 1) {
      local_30 = (uint)DAT_00063f50 - (uint)*(byte *)((int)piVar6 + local_38 + 4);
      local_2c = DAT_00063f50 - 1;
      if (local_30 < 0) {
        local_30 = local_30 + 10;
      }
      if (local_2c < 0) {
        local_2c = DAT_00063f50 + 9;
      }
      if (((&DAT_00063f14)[local_30] == *(char *)piVar6[local_38 + 0x1a]) &&
         ((&DAT_00063f1e)[local_2c] != '\x01')) {
        for (local_34 = 1; (int)local_34 < (int)(uint)*(byte *)((int)piVar6 + local_38 + 4);
            local_34 = local_34 + 1) {
          if (local_30 == 9) {
            local_30 = 0;
          }
          else {
            local_30 = local_30 + 1;
          }
          if (((&DAT_00063f14)[local_30] != *(char *)(piVar6[local_38 + 0x1a] + local_34)) ||
             ((int)(uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1) <
              *(int *)(&DAT_00063f28 + local_30 * 4))) break;
        }
        if ((*(byte *)((int)piVar6 + local_38 + 4) == local_34) &&
           (*(byte *)((int)piVar6 + local_38 + 0x450) <= *(byte *)(DAT_00053c7c + 0x27))) {
          if (*(short *)((int)piVar6 + local_38 * 2 + 0x388) == 0x70) {
            piVar1 = (int *)(*(int *)(DAT_00053c7c + 0x4a) + 0x52);
            *piVar1 = *piVar1 + 1;
          }
          if (((*(int *)(DAT_00053c7c + 0x4a) != 0) &&
              (*(int *)(*(int *)(DAT_00053c7c + 0x4a) + 0x52) == 5)) &&
             (*(short *)((int)piVar6 + local_38 * 2 + 0x388) == 0x70)) {
            if (DAT_00062d58 == 0x71) {
              (&DAT_00063f1e)[local_2c] = 1;
            }
            else {
              if (*(char *)(DAT_00053c7c + 0x31) == '\0') {
                (&DAT_00063f1e)[local_2c] = 1;
                *(undefined4 *)(DAT_00053c7c + 0x17) = 0x71;
                *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
                *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
                bVar2 = *(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
              }
              else {
                (&DAT_00063f1e)[local_2c] = 1;
                FUN_00031f10(DAT_00053c7c,0x71);
                *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
                bVar2 = *(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
              }
              DAT_00062d5c = (uint)bVar2;
              DAT_00062d58 = 0x71;
            }
            *(undefined4 *)(*(int *)(DAT_00053c7c + 0x4a) + 0x52) = 0;
            return;
          }
          if (((((((10 < *(uint *)(DAT_00053c7c + 0x17)) && (*(uint *)(DAT_00053c7c + 0x17) < 0x14))
                 || ((0x32 < *(uint *)(DAT_00053c7c + 0x17) &&
                     (*(uint *)(DAT_00053c7c + 0x17) < 0x51)))) ||
                ((*(int *)(DAT_00053c7c + 0x17) == 0xa4 || (*(int *)(DAT_00053c7c + 0x17) == 0xa6)))
                ) || (*(int *)(DAT_00053c7c + 0x17) == 0xa7)) ||
              (((*(int *)(DAT_00053c7c + 0x17) == 0x2f ||
                ((0xaa < *(uint *)(DAT_00053c7c + 0x17) && (*(uint *)(DAT_00053c7c + 0x17) < 0xae)))
                ) || ((0xb4 < *(uint *)(DAT_00053c7c + 0x17) &&
                      (*(uint *)(DAT_00053c7c + 0x17) < 0xb9)))))) &&
             (((*(short *)((int)piVar6 + local_38 * 2 + 0x388) == 0x65 ||
               (*(short *)((int)piVar6 + local_38 * 2 + 0x388) == 0x30)) ||
              (*(short *)((int)piVar6 + local_38 * 2 + 0x388) == 0x31)))) {
            (&DAT_00063f1e)[local_2c] = 1;
            *(uint *)(DAT_00053c7c + 0x17) = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
            *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
            DAT_00062d58 = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
            DAT_00062d5c = (uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
            return;
          }
          if ((10 < *(uint *)(DAT_00053c7c + 0x17)) && (*(uint *)(DAT_00053c7c + 0x17) < 0x15)) {
            if (*(short *)((int)piVar6 + local_38 * 2 + 0x388) != 0x99) {
              FUN_000348e2();
              return;
            }
            (&DAT_00063f1e)[local_2c] = 1;
            *(uint *)(DAT_00053c7c + 0x17) = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
            iVar4 = *(int *)(*(int *)(*(int *)(DAT_00053c7c + 0x123) + 4) +
                            *(int *)(DAT_00053c7c + 0x17) * 4);
            if (*(char *)(iVar4 + 0x69) != '\0') {
              DAT_00063364 = (uint)*(byte *)(iVar4 + 0x69);
              DAT_00063368 = (uint)*(ushort *)(iVar4 + 0x6a);
            }
            *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            DAT_00062d58 = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
            DAT_00062d5c = (uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
            return;
          }
          if (*(char *)(piVar6[local_38 + 0x7e] + (uint)*(byte *)((int)piVar6 + local_38 + 4) + -1)
              == '\0') {
            FUN_000348e2();
          }
          if (((*(int *)(DAT_00053c7c + 0x17) == 1) || (*(int *)(DAT_00053c7c + 0x17) == 2)) ||
             (*(int *)(DAT_00053c7c + 0x17) == 0x66)) {
            if (((byte)(&DAT_00062b90)[*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)] < 2) ||
               ((uint)(byte)(&DAT_00062b90)[DAT_00062d58] ==
                (byte)(&DAT_00062b90)[*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)] - 1)) {
              (&DAT_00063f1e)[local_2c] = 1;
              *(uint *)(DAT_00053c7c + 0x17) = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)
              ;
              iVar4 = *(int *)(*(int *)(*(int *)(DAT_00053c7c + 0x123) + 4) +
                              *(int *)(DAT_00053c7c + 0x17) * 4);
              if (*(char *)(iVar4 + 0x69) != '\0') {
                DAT_00063364 = (uint)*(byte *)(iVar4 + 0x69);
                DAT_00063368 = (uint)*(ushort *)(iVar4 + 0x6a);
              }
              *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
              *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
              DAT_00062d58 = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
              DAT_00062d5c = (uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
              return;
            }
          }
          else if (*(int *)(DAT_00053c7c + 0x17) == 0x70) {
            if (*(byte *)(DAT_00053c7c + 0x31) < 5) {
              if (DAT_00062d58 != 0x71) {
                (&DAT_00063f1e)[local_2c] = 1;
                FUN_00031f10(DAT_00053c7c,*(undefined2 *)((int)piVar6 + local_38 * 2 + 0x388));
                *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
                DAT_00062d58 = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
                DAT_00062d5c = (uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
                return;
              }
              FUN_000348e2();
            }
          }
          else if (((&DAT_00062ac8)[DAT_00062d58] ==
                    (&DAT_00062ac8)[*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)]) &&
                  ((uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388) !=
                   *(uint *)(DAT_00053c7c + 0x17))) {
            if ((byte)(&DAT_00062b90)[*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)] <=
                (byte)(&DAT_00062b90)[DAT_00062d58]) {
              FUN_000348e2();
              return;
            }
            if ((uint)(byte)(&DAT_00062b90)[DAT_00062d58] !=
                (byte)(&DAT_00062b90)[*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)] - 1) {
              return;
            }
            if (*(byte *)(DAT_00053c7c + 0x31) < 5) {
              (&DAT_00063f1e)[local_2c] = 1;
              FUN_00031f10(DAT_00053c7c,*(undefined2 *)((int)piVar6 + local_38 * 2 + 0x388));
              *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
              DAT_00062d58 = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
              DAT_00062d5c = (uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
              return;
            }
          }
        }
      }
    }
  }
  return;
}



// === pre_b9b7 @ 0003b9b7 ===

void FUN_0003b9b7(void)

{
  int iVar1;
  
  FUN_000418ee();
  DAT_000639d4 = DAT_000639cc;
  iVar1 = DAT_000639d4;
  while (DAT_000639d4 = iVar1, DAT_000639d4 != 0) {
    iVar1 = *(int *)(DAT_000639d4 + 0x11);
    if (DAT_000639d4 != 0) {
      FUN_0003b67e(DAT_000639d4,0);
      FUN_00041c28();
    }
  }
  return;
}



// === pre_40042 @ 00040042 ===

undefined6 FUN_00040042(void)

{
  undefined4 in_stack_00000004;
  int in_stack_00000008;
  undefined1 *in_stack_0000000c;
  
  out(0x3c8,(char)in_stack_00000004);
  in_stack_00000008 = in_stack_00000008 * 3;
  while (in_stack_00000008 != 0) {
    in_stack_00000008 = in_stack_00000008 + -1;
    out(*in_stack_0000000c,0x3c9);
    in_stack_0000000c = in_stack_0000000c + 1;
  }
  return CONCAT24(0x3c9,in_stack_00000004);
}



// === pre_48e2 @ 000348e2 ===

void FUN_000348e2(void)

{
  FUN_000418ee();
  (&DAT_00063f14)[DAT_00063f50] = 0;
  *(undefined4 *)(&DAT_00063f28 + (uint)DAT_00063f50 * 4) = DAT_00063f08;
  DAT_00063f50 = DAT_00063f50 + 1;
  if (9 < DAT_00063f50) {
    DAT_00063f50 = 0;
  }
  return;
}



// === pre_ed6f @ 0003ed6f ===

void FUN_0003ed6f(void)

{
  char *pcVar1;
  
  FUN_000418ee();
  DAT_00063e20 = DAT_00063e18;
  pcVar1 = DAT_00063e20;
  while (DAT_00063e20 = pcVar1, DAT_00063e20 != (char *)0x0) {
    FUN_0003e20f(DAT_00063e20);
    if (*DAT_00063e20 == '\0') {
      pcVar1 = *(char **)(DAT_00063e20 + 0x51);
      if (DAT_00063e20 != (char *)0x0) {
        FUN_0003e14c(DAT_00063e20,0);
        FUN_00041c28();
      }
    }
    else {
      pcVar1 = *(char **)(DAT_00063e20 + 0x51);
    }
  }
  return;
}



// === pre_801b @ 0003801b ===

void FUN_0003801b(void)

{
  undefined4 uVar1;
  
  uVar1 = FUN_000418ee();
  FUN_0003ce0b(DAT_00063354,uVar1);
  FUN_0003203d(uVar1);
  FUN_0003df96(uVar1);
  if (DAT_00053c84 != 0) {
    FUN_0001348f(DAT_00053c84,uVar1,0);
    FUN_000137c1(DAT_00053c84,uVar1);
  }
  FUN_000404c6();
  return;
}



// === pre_40660 @ 00040660 ===

void FUN_00040660(void)

{
  int iVar1;
  undefined8 uVar2;
  undefined1 local_328 [768];
  int local_28;
  int local_24;
  undefined8 local_20;
  
  uVar2 = FUN_000418ee();
  for (local_28 = 0; local_20._4_4_ = (int)((ulonglong)uVar2 >> 0x20), local_20._0_4_ = (int)uVar2,
      local_28 < local_20._4_4_; local_28 = local_28 + 1) {
    iVar1 = local_28;
    for (local_24 = 0; local_24 < 0x300; local_24 = local_24 + 1) {
      local_328[local_24] =
           (char)((int)((uint)*(byte *)((int)local_20 + local_24) * local_28) / local_20._4_4_);
      iVar1 = local_24;
    }
    local_20 = uVar2;
    FUN_00035ec2(iVar1);
    FUN_00040042();
    uVar2 = local_20;
  }
  return;
}



// === pre_81d7 @ 000381d7 ===

void FUN_000381d7(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  int *piVar2;
  undefined4 uVar3;
  int extraout_ECX;
  int iVar4;
  undefined8 uVar5;
  int in_stack_00000000;
  int local_38;
  
  uVar5 = FUN_000418ee();
  iVar4 = (int)((ulonglong)uVar5 >> 0x20);
  uVar3 = (undefined4)uVar5;
  local_38 = 0;
  uVar1 = *(undefined4 *)(DAT_00053c7c + 0x17);
  *(undefined4 *)(DAT_00053c7c + 0x17) = 2;
  *(int *)(DAT_00053c7c + 0x1b) = in_stack_00000000;
  piVar2 = *(int **)(*(int *)(*(int *)(DAT_00053c7c + 0x123) + 4) + 8);
  for (; local_38 < extraout_ECX; local_38 = local_38 + 1) {
    if (in_stack_00000000 == 0) {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) + iVar4;
    }
    else {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) - iVar4;
    }
    *(int *)(DAT_00053c7c + 9) = *(int *)(DAT_00053c7c + 9) + param_3;
    *(int *)(DAT_00053c7c + 0x11) = *(int *)(DAT_00053c7c + 0x11) + 1;
    if (*piVar2 <= *(int *)(DAT_00053c7c + 0x11)) {
      *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
    }
    FUN_00013aad(DAT_00053c70);
    if (DAT_00063e24 < (int)(0x1ea / (longlong)DAT_00053104)) {
      FUN_0003ce0b(DAT_00063354,uVar3);
      FUN_0003203d(uVar3);
      FUN_0003df96(uVar3);
      if (DAT_00053c84 != 0) {
        FUN_0001348f(DAT_00053c84,uVar3,0);
        FUN_000137c1(DAT_00053c84,uVar3);
      }
      FUN_000404c6();
    }
    do {
    } while (DAT_00063e24 < (int)(0x1ea / (longlong)DAT_00053104));
    DAT_00063e24 = DAT_00063e24 - (int)(0x1ea / (longlong)DAT_00053104);
  }
  *(undefined4 *)(DAT_00053c7c + 0x17) = uVar1;
  return;
}



// === sfx_4185e @ 0004185e ===

void FUN_0004185e(void)

{
  undefined4 uVar1;
  
  uVar1 = FUN_000418ee();
  if ((DAT_00063f90 != 0) && (DAT_00053338 != '\0')) {
    if (DAT_00063fb8 == '\x01') {
      FUN_00041340(0xd0);
    }
    FUN_00040839(uVar1);
    DAT_00063fb8 = '\x01';
  }
  return;
}



// === cd_d244 @ 0003d244 ===

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0003d244(void)

{
  int iVar1;
  int *piVar2;
  int iStack_2c;
  undefined4 local_28;
  undefined4 local_24;
  int local_20;
  
  local_20 = FUN_000418ee();
  piVar2 = &local_28;
  iStack_2c = 0x3d264;
  FUN_0003237b();
  local_28 = DAT_00063e24;
  local_24 = DAT_00063e28;
  iStack_2c = 0x3d286;
  FUN_000406f5(DAT_00063354 + 0x48,0x32);
  if (*(int *)(local_20 + 0x34c) == 0) {
    DAT_00063354 = DAT_00063350;
  }
  else {
    DAT_00063354 = *(int *)(local_20 + 0x34c);
  }
  iStack_2c = 0x3d2b6;
  FUN_00038f3d(1);
  iStack_2c = 0x3d2c0;
  FUN_0003cad3(DAT_00063354);
  if (DAT_00063398 == 0) {
    *(int *)(DAT_00053c7c + 1) = DAT_000639dc + -0x1e;
  }
  else {
    *(int *)(DAT_00053c7c + 1) = DAT_000639dc + 0x15e;
  }
  if ((&DAT_00052be0)[*(int *)(DAT_00063354 + 0x40) + (DAT_00063370 + -1) * 10] == '\x01') {
    if (DAT_00063398 == 0) {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) + 0x78;
    }
    else {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) + -0x78;
    }
  }
  *(int *)(DAT_00053c7c + 9) =
       DAT_000639e0 +
       *(int *)(&DAT_00052c3c + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28);
  *(undefined4 *)(DAT_00053c7c + 5) = 0;
  *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
  *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
  *(int *)(DAT_00053c7c + 0x1b) = DAT_00063398;
  iVar1 = DAT_00063360 / 10;
  if (DAT_00063360 % 10 == 0) {
    DAT_00063360 = DAT_00063360 + 1;
  }
  iStack_2c = 0x3d3b1;
  FUN_0003ed6f(iVar1);
  iStack_2c = 0x3d3bb;
  FUN_0003801b(DAT_000633b0);
  iStack_2c = 0x3d3dc;
  FUN_00010b1d(*(undefined4 *)
                (&DAT_00052da4 + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28));
  iStack_2c = 0x3d3ee;
  FUN_00040660(DAT_00063354 + 0x48,0x32);
  DAT_00063358 = DAT_00063354;
  DAT_00063374 = *(undefined4 *)(DAT_00053c7c + 0x2b);
  _DAT_00063384 = (uint)*(byte *)(DAT_00053c7c + 0x28);
  DAT_00063378 = (uint)*(byte *)(DAT_00053c7c + 0x27);
  DAT_0006337c = (uint)*(byte *)(DAT_00053c7c + 0x26);
  _DAT_00063380 = (uint)*(byte *)(DAT_00053c7c + 0x25);
  DAT_00063e28 = 0;
  DAT_00063e24 = 0;
  if ((&DAT_00052be0)[*(int *)(DAT_00063354 + 0x40) + (DAT_00063370 + -1) * 10] == '\0') {
    piVar2 = &iStack_2c;
    iStack_2c = DAT_00063398;
    FUN_000381d7(DAT_000633b0,1,0,0x78);
  }
  *(undefined4 *)((int)piVar2 + -4) = 0x3d4b0;
  FUN_00041e20(&DAT_00062c58,0,0x80);
  *(undefined4 *)((int)piVar2 + -4) = 0x3d4c1;
  FUN_00041e20(&DAT_00062cd8,0,0x80);
  DAT_00063e24 = local_28;
  DAT_00063e28 = local_24;
  DAT_000639bc = 0;
  return;
}



// === entdraw_3aad @ 00013aad ===

void FUN_00013aad(void)

{
  char *pcVar1;
  char *local_30;
  int local_28;
  int local_24;
  
  FUN_000418ee();
  for (local_28 = 0; local_28 < DAT_00053c68 + -1; local_28 = local_28 + 1) {
    local_30 = DAT_00053c70;
    for (local_24 = 0; local_24 < (DAT_00053c68 - local_28) + -1; local_24 = local_24 + 1) {
      if (*(int *)(local_30 + 9) < *(int *)(*(int *)(local_30 + 0x13f) + 9)) {
        local_30 = *(char **)(local_30 + 0x13f);
      }
      else {
        pcVar1 = *(char **)(local_30 + 0x13f);
        if ((pcVar1 == DAT_00053c7c) && (*(int *)(pcVar1 + 9) == *(int *)(local_30 + 9))) {
          local_30 = *(char **)(local_30 + 0x13f);
        }
        else if ((*pcVar1 == *local_30) && (*(int *)(pcVar1 + 9) == *(int *)(local_30 + 9))) {
          local_30 = *(char **)(local_30 + 0x13f);
        }
        else {
          if (*(int *)(pcVar1 + 0x13f) != 0) {
            *(char **)(*(int *)(pcVar1 + 0x13f) + 0x143) = local_30;
          }
          if (*(int *)(local_30 + 0x143) != 0) {
            *(char **)(*(int *)(local_30 + 0x143) + 0x13f) = pcVar1;
          }
          *(undefined4 *)(local_30 + 0x13f) = *(undefined4 *)(pcVar1 + 0x13f);
          *(undefined4 *)(pcVar1 + 0x143) = *(undefined4 *)(local_30 + 0x143);
          *(char **)(local_30 + 0x143) = pcVar1;
          *(char **)(pcVar1 + 0x13f) = local_30;
          if (*(int *)(pcVar1 + 0x143) == 0) {
            DAT_00053c70 = pcVar1;
          }
          if (*(int *)(local_30 + 0x13f) == 0) {
            DAT_00053c74 = local_30;
          }
        }
      }
    }
  }
  return;
}



// === boss_348f @ 0001348f ===

void FUN_0001348f(undefined4 param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  undefined8 uVar7;
  undefined1 uVar8;
  undefined1 uVar9;
  
  uVar7 = FUN_000418ee();
  uVar4 = (undefined4)((ulonglong)uVar7 >> 0x20);
  iVar1 = (int)uVar7;
  if (iVar1 == DAT_00053c7c) {
    iVar2 = (int)*(short *)(iVar1 + 0x23) / 100;
    iVar5 = (int)*(short *)(iVar1 + 0x23) % 100;
    iVar3 = (int)*(short *)(iVar1 + 0x29) / 100;
    iVar6 = (int)*(short *)(iVar1 + 0x29) % 100;
    uVar9 = (&DAT_00051c91)[iVar2];
    uVar8 = (&DAT_00051c90)[iVar2];
  }
  else {
    iVar2 = (int)*(short *)(iVar1 + 0x23) / 200;
    iVar5 = ((int)*(short *)(iVar1 + 0x23) / 2) % 100;
    iVar3 = (int)*(short *)(iVar1 + 0x29) / 200;
    iVar6 = ((int)*(short *)(iVar1 + 0x29) / 2) % 100;
    if (iVar2 < 6) {
      uVar8 = (&DAT_00051c90)[iVar2];
    }
    else {
      uVar8 = 0x95;
    }
    if (iVar2 < 5) {
      uVar9 = (&DAT_00051c91)[iVar2];
    }
    else {
      uVar9 = 0x95;
    }
  }
  if ((*(int *)(iVar1 + 0xd) == 0) || (*(int *)(iVar1 + 0x56) == 0)) {
    FUN_0003bb42(uVar4,param_3 * 0xaa + 4,3,
                 *(undefined4 *)(*(int *)(DAT_00053c8c + 4) + *(int *)(iVar1 + 0xd) * 4),
                 *(undefined4 *)(*(int *)(DAT_00053c8c + 8) + *(int *)(iVar1 + 0xd) * 4),
                 *(int *)(DAT_00053c8c + 0x38) +
                 *(int *)(*(int *)(DAT_00053c8c + 0x34) + *(int *)(iVar1 + 0xd) * 4),0,iVar2,iVar5,
                 iVar3,iVar6,uVar8,uVar9);
    FUN_0003bb42(uVar4,param_3 * 0xaa + 0x13,0x1d,**(undefined4 **)(DAT_00063e00 + 4),
                 **(undefined4 **)(DAT_00063e00 + 8),*(undefined4 *)(DAT_00063e00 + 0x38),0);
    if (iVar2 != 0) {
      FUN_0004052c();
    }
    if (iVar5 != 0) {
      FUN_0004052c();
      FUN_00040506();
    }
  }
  else {
    if ((0 < *(int *)(iVar1 + 0xd)) && (*(int *)(iVar1 + 0xd) < 10)) {
      iVar2 = (int)(CONCAT44(*(int *)(iVar1 + 0x56) >> 0x1f,*(undefined4 *)(iVar1 + 0x56)) / 3);
      iVar5 = 0x9e;
      iVar6 = 1;
    }
    FUN_0003bb42(uVar4,0x20,0xb3,*(undefined4 *)(*(int *)(DAT_00063e00 + 4) + iVar6 * 4),
                 *(undefined4 *)(*(int *)(DAT_00063e00 + 8) + iVar6 * 4),
                 *(int *)(DAT_00063e00 + 0x38) + *(int *)(iVar6 * 4 + *(int *)(DAT_00063e00 + 0x34))
                 ,0,iVar2,iVar5,iVar3,iVar6,uVar8,uVar9);
    FUN_0004052c();
  }
  return;
}



// === boss_37c1 @ 000137c1 ===

void FUN_000137c1(void)

{
  undefined8 uVar1;
  int *piVar2;
  undefined1 *puVar3;
  int local_38;
  int local_34;
  int local_30;
  byte local_2c [12];
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  FUN_00041f42(*(undefined4 *)((int)local_20 + 0x2b),local_2c,10);
  local_30 = FUN_00041f5d(local_2c);
  local_34 = 5 - local_30;
  piVar2 = &local_38;
  uVar1 = local_20;
  for (local_38 = 0; local_20._4_4_ = (undefined4)((ulonglong)uVar1 >> 0x20), local_38 < local_34;
      local_38 = local_38 + 1) {
    *(undefined4 *)((int)piVar2 + -4) = 0;
    *(int *)((int)piVar2 + -8) =
         *(int *)(DAT_00063e04 + 0x38) + *(int *)(*(int *)(DAT_00063e04 + 0x34) + 4);
    *(undefined4 *)((int)piVar2 + -0xc) = 7;
    *(undefined4 *)((int)piVar2 + -0x10) = 0x13852;
    FUN_0003bb42(local_20._4_4_,local_38 * 6 + 0x41,0x14,5);
    local_20 = uVar1;
    piVar2 = (int *)((int)piVar2 + -0xc);
    uVar1 = local_20;
  }
  local_38 = 0;
  puVar3 = (undefined1 *)piVar2;
  while( true ) {
    local_20._4_4_ = (undefined4)((ulonglong)uVar1 >> 0x20);
    local_20._0_4_ = (int)uVar1;
    if (local_30 <= local_38) break;
    *(undefined4 *)(puVar3 + -4) = 0;
    *(int *)(puVar3 + -8) =
         *(int *)(DAT_00063e04 + 0x38) +
         *(int *)((uint)local_2c[local_38] * 4 + *(int *)(DAT_00063e04 + 0x34) + -0xbc);
    *(undefined4 *)(puVar3 + -0xc) = 7;
    *(undefined4 *)(puVar3 + -0x10) = 0x138b8;
    FUN_0003bb42(local_20._4_4_,(local_38 + local_34) * 6 + 0x41,0x14,5);
    local_20 = uVar1;
    local_38 = local_38 + 1;
    puVar3 = puVar3 + -0xc;
    uVar1 = local_20;
  }
  if ((*(int *)((int)local_20 + 0x52) != 0) && (DAT_00063360 % 2 == 0)) {
    *(int *)((int)local_20 + 0x2b) = *(int *)((int)local_20 + 0x2b) + 1;
    *(int *)((int)local_20 + 0x52) = *(int *)((int)local_20 + 0x52) + -1;
  }
  if (*(int *)((int)local_20 + 0x56) != 0) {
    *(short *)((int)local_20 + 0x23) = *(short *)((int)local_20 + 0x23) + 1;
    *(int *)((int)local_20 + 0x56) = *(int *)((int)local_20 + 0x56) + -1;
  }
  return;
}



