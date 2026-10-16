// func #333  FUN_00044ea4  00044ea4  size=307


byte * FUN_00044ea4(undefined4 param_1,int *param_2,int param_3)

{
  byte bVar1;
  int *piVar2;
  int iVar3;
  byte *pbVar4;
  byte *pbVar5;
  
  *(undefined1 *)(param_3 + 0x17) = 0;
  *(undefined1 *)(param_3 + 0x16) = 0x20;
  pbVar4 = (byte *)FUN_00044fd7(param_1,param_3);
  *(undefined4 *)(param_3 + 4) = 0;
  if ((*pbVar4 < 0x30) || (0x39 < *pbVar4)) {
    if (*pbVar4 == 0x2a) {
      piVar2 = (int *)*param_2;
      *param_2 = (int)(piVar2 + 1);
      iVar3 = *piVar2;
      *(int *)(param_3 + 4) = iVar3;
      if (iVar3 < 0) {
        *(int *)(param_3 + 4) = -iVar3;
        *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) | 8;
      }
      pbVar4 = pbVar4 + 1;
    }
  }
  else {
    do {
      bVar1 = *pbVar4;
      pbVar4 = pbVar4 + 1;
      *(uint *)(param_3 + 4) = *(int *)(param_3 + 4) * 10 + (bVar1 - 0x30);
      if (*pbVar4 < 0x30) break;
    } while (*pbVar4 < 0x3a);
  }
  *(undefined4 *)(param_3 + 8) = 0xffffffff;
  pbVar5 = pbVar4;
  if (*pbVar4 == 0x2e) {
    pbVar5 = pbVar4 + 1;
    *(undefined4 *)(param_3 + 8) = 0;
    if (*pbVar5 == 0x2a) {
      piVar2 = (int *)*param_2;
      *param_2 = (int)(piVar2 + 1);
      iVar3 = *piVar2;
      *(int *)(param_3 + 8) = iVar3;
      if (iVar3 < 0) {
        *(undefined4 *)(param_3 + 8) = 0xffffffff;
      }
      pbVar5 = pbVar4 + 2;
    }
    else {
      for (; (0x2f < *pbVar5 && (*pbVar5 < 0x3a)); pbVar5 = pbVar5 + 1) {
        *(uint *)(param_3 + 8) = *(int *)(param_3 + 8) * 10 + (*pbVar5 - 0x30);
      }
    }
  }
  bVar1 = *pbVar5;
  pbVar4 = pbVar5 + 1;
  if (bVar1 < 0x4e) {
    if (0x45 < bVar1) {
      if (bVar1 < 0x47) {
        *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) | 0x80;
      }
      else {
        if (bVar1 != 0x4c) {
          return pbVar5;
        }
        *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) | 0x40;
      }
      return pbVar4;
    }
  }
  else {
    if (0x4e < bVar1) {
      if (bVar1 < 0x6c) {
        if (bVar1 == 0x68) {
          *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) | 0x10;
          return pbVar4;
        }
        return pbVar5;
      }
      if ((0x6c < bVar1) && (bVar1 != 0x77)) {
        return pbVar5;
      }
      *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) | 0x20;
      return pbVar5 + 1;
    }
    *(byte *)(param_3 + 0x14) = *(byte *)(param_3 + 0x14) | 0x40;
    pbVar5 = pbVar4;
  }
  return pbVar5;
}


