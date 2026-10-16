// func #285  FUN_00042aba  00042aba  size=31


void FUN_00042aba(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  code *pcVar1;
  uint uVar2;
  bool bVar3;
  int iVar4;
  byte in_CF;
  
  pcVar1 = (code *)swi(0x21);
  iVar4 = (*pcVar1)(CONCAT11(0x3f,(char)param_1),param_2,param_1,param_3);
  bVar3 = (in_CF & 1) != 0;
  uVar2 = (iVar4 << 1 | (uint)in_CF) >> 1;
  if (bVar3) {
    FUN_00045a82(uVar2 & 0xffff,uVar2 | (uint)bVar3 << 0x1f);
  }
  return;
}


