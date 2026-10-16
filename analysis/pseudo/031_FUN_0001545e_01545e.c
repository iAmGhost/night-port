// func #31  FUN_0001545e  0001545e  size=1037


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


