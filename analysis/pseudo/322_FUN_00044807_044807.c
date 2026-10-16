// func #322  FUN_00044807  00044807  size=65


undefined4 FUN_00044807(undefined4 param_1,uint param_2,undefined1 param_3)

{
  code *pcVar1;
  int iVar2;
  undefined4 unaff_EDI;
  byte bVar3;
  
  bVar3 = (param_2 >> 0xf & 1) != 0;
  pcVar1 = (code *)swi(0x21);
  iVar2 = (*pcVar1)(CONCAT11(0x42,param_3),param_2,param_1,param_2 >> 0x10);
  if ((bVar3 & 1) != 0) {
    FUN_00045a82((iVar2 << 1 | (uint)bVar3) >> 1 & 0xffff);
    unaff_EDI = 0xffffffff;
  }
  return unaff_EDI;
}


