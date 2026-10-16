// func #345  FUN_00045a44  00045a44  size=21


undefined4 FUN_00045a44(void)

{
  code *pcVar1;
  undefined4 uVar2;
  
  if (DAT_00053624 != '\0') {
    pcVar1 = (code *)swi(3);
    uVar2 = (*pcVar1)();
    return uVar2;
  }
  return 0;
}


