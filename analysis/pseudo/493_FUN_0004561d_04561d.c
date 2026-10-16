// func #493  FUN_0004561d  0004561d  size=49

void FUN_0004561d(void)

{
  code *pcVar1;
  uint uVar2;
  byte extraout_DH;
  char extraout_DH_00;
  uint extraout_EDX;
  int iVar3;
  
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)();
  uVar2 = extraout_EDX >> 8 & 0xff;
  do {
    pcVar1 = (code *)swi(0x21);
    (*pcVar1)();
  } while (uVar2 == extraout_DH);
  iVar3 = 0;
  uVar2 = (uint)extraout_DH;
  do {
    pcVar1 = (code *)swi(0x21);
    (*pcVar1)();
    iVar3 = iVar3 + 1;
  } while ((char)uVar2 == extraout_DH_00);
  DAT_00064000 = iVar3;
  return;
}
