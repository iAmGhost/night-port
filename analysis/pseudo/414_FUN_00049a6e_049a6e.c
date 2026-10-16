// func #414  FUN_00049a6e  00049a6e  size=278


byte * FUN_00049a6e(byte *param_1,byte *param_2,int *param_3)

{
  byte bVar1;
  bool bVar2;
  byte extraout_CL;
  uint uVar3;
  byte *pbVar4;
  byte *pbVar5;
  int local_24;
  int local_20;
  int local_1c;
  uint local_18;
  byte *local_14;
  
  pbVar4 = param_1;
  if (*param_1 == 0x3a) {
    param_1 = param_1 + 1;
    pbVar4 = param_1;
  }
  for (; ((((bVar1 = *param_1, bVar1 != 0 && (bVar1 != 0x2c)) && (bVar1 != 0x2d)) && (bVar1 != 0x2b)
          ) && ((bVar1 < 0x30 || (0x39 < bVar1)))); param_1 = param_1 + 1) {
  }
  local_18 = (int)param_1 - (int)pbVar4;
  if (0x1e < (int)local_18) {
    local_18 = 0x1e;
  }
  pbVar5 = param_2;
  for (uVar3 = local_18 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    *(undefined4 *)pbVar5 = *(undefined4 *)pbVar4;
    pbVar4 = pbVar4 + 4;
    pbVar5 = pbVar5 + 4;
  }
  for (uVar3 = local_18 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *pbVar5 = *pbVar4;
    pbVar4 = pbVar4 + 1;
    pbVar5 = pbVar5 + 1;
  }
  bVar2 = false;
  param_2[local_18] = 0;
  if (bVar1 == 0x2d) {
    bVar2 = true;
  }
  else if (bVar1 != 0x2b) goto LAB_00049afb;
  param_1 = param_1 + 1;
LAB_00049afb:
  if ((0x2f < *param_1) && (*param_1 < 0x3a)) {
    local_24 = 0;
    local_20 = 0;
    local_1c = 0;
    local_14 = param_2;
    param_1 = (byte *)FUN_00049a48(param_1,&local_1c);
    if ((*param_1 == 0x3a) &&
       (param_1 = (byte *)FUN_00049a48(param_1 + 1,&local_20), *param_1 == extraout_CL)) {
      param_1 = (byte *)FUN_00049a48(param_1 + 1,&local_24);
    }
    local_24 = local_24 + (local_20 + local_1c * 0x3c) * 0x3c;
    *param_3 = local_24;
    if (bVar2) {
      *param_3 = -local_24;
    }
  }
  return param_1;
}


