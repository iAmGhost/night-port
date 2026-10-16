// func #20  FUN_00012616  00012616  size=2276


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


