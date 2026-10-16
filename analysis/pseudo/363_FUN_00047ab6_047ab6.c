// func #363  FUN_00047ab6  00047ab6  size=153


int FUN_00047ab6(uint *param_1,uint *param_2)

{
  ushort uVar1;
  uint uVar2;
  bool bVar3;
  bool bVar4;
  
  if ((((ushort)((ushort)param_1[2] | 0x8000) == 0xffff) &&
      ((param_1[1] & 0x7fffffff) != 0 || *param_1 != 0)) ||
     (((ushort)((ushort)param_2[2] | 0x8000) == 0xffff &&
      ((param_2[1] & 0x7fffffff) != 0 || *param_2 != 0)))) {
    return 2;
  }
  uVar2 = *(uint *)((int)param_1 + 6);
  if ((int)(*(uint *)((int)param_2 + 6) ^ uVar2) < 0) {
    if ((CONCAT22((short)param_1[2],(short)param_2[2]) & 0x7fff7fff) == 0 &&
        (((*param_1 == 0 && *param_2 == 0) && param_1[1] == 0) && param_2[1] == 0)) {
      return 0;
    }
  }
  else {
    uVar1 = (ushort)param_1[2];
    bVar3 = uVar1 < (ushort)param_2[2];
    bVar4 = false;
    if (uVar1 == (ushort)param_2[2]) {
      bVar3 = param_1[1] < param_2[1];
      bVar4 = false;
      if (param_1[1] == param_2[1]) {
        bVar3 = *param_1 < *param_2;
        bVar4 = *param_1 == *param_2;
      }
    }
    if (bVar4) {
      return 0;
    }
    uVar2 = CONCAT22((short)(uVar2 >> 0x10),uVar1) ^ (uint)(CONCAT14(bVar3,param_1) >> 1);
  }
  return (uint)CARRY4(uVar2,uVar2) * -2 + 1;
}


