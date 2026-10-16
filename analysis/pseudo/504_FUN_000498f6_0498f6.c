// func #411  FUN_000498f6  000498f6  size=277


int FUN_000498f6(int param_1,uint param_2,int param_3,undefined4 *param_4)

{
  ulonglong uVar1;
  uint uVar2;
  int extraout_ECX;
  int extraout_ECX_00;
  int iVar3;
  uint uVar4;
  undefined *puVar5;
  uint uVar6;
  undefined8 uVar7;
  
  if ((param_2 < 0xa8c0) && (0 < param_3)) {
    param_2 = (param_2 + 0x15180) - param_3;
    uVar6 = (param_1 + param_2 / 0x15180) - 1;
  }
  else {
    param_2 = param_2 - param_3;
    uVar6 = param_1 + param_2 / 0x15180;
  }
  param_4[2] = (int)(((ulonglong)param_2 % 0x15180) / 0xe10);
  uVar1 = ((ulonglong)param_2 % 0x15180) % 0xe10;
  param_4[1] = (int)(uVar1 / 0x3c);
  *param_4 = (int)(uVar1 % 0x3c);
  uVar2 = uVar6 / 0x16e;
  uVar4 = uVar6 + uVar2 * -0x16d;
  if (uVar2 != 0) {
    uVar4 = uVar4 - (uVar2 - 1 >> 2);
  }
  uVar7 = FUN_000494cb(uVar2 + 0x76c);
  iVar3 = (int)((ulonglong)uVar7 >> 0x20);
  uVar2 = (int)uVar7 + 0x16d;
  for (; uVar2 <= uVar4; uVar4 = uVar4 - uVar2) {
    iVar3 = iVar3 + 1;
  }
  puVar5 = &DAT_00051960;
  *(uint *)(extraout_ECX + 0x1c) = uVar4;
  *(int *)(extraout_ECX + 0x14) = iVar3;
  iVar3 = FUN_000494cb(iVar3 + 0x76c);
  if (iVar3 != 0) {
    puVar5 = &DAT_0005197a;
  }
  uVar2 = uVar4 / 0x1f;
  if ((uint)(*(int *)(puVar5 + uVar2 * 2) >> 0x10) <= uVar4) {
    uVar2 = uVar2 + 1;
  }
  *(uint *)(extraout_ECX_00 + 0x10) = uVar2;
  *(uint *)(extraout_ECX_00 + 0xc) = (uVar4 - (int)*(short *)(puVar5 + uVar2 * 2)) + 1;
  *(uint *)(extraout_ECX_00 + 0x18) = (uVar6 + 1) % 7;
  return extraout_ECX_00;
}


