// func #304  FUN_00043958  00043958  size=33


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00043958(void)

{
  code *pcVar1;
  undefined2 uVar2;
  undefined4 extraout_EDX;
  int iVar3;
  
  iVar3 = 0;
  pcVar1 = (code *)swi(0x21);
  uVar2 = (*pcVar1)(0x2535,&DAT_000533c4,0);
  _DAT_000533c4 = (uint)DAT_0005340a;
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)(uVar2,extraout_EDX,iVar3 + 1);
  return;
}


