// func #255  FUN_0004232d  0004232d  size=52


ulonglong FUN_0004232d(uint param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  code *pcVar1;
  undefined2 uVar2;
  uint uVar3;
  undefined2 in_ES;
  
  if ((DAT_000533b2 < 2) || (8 < DAT_000533b2)) {
    uVar3 = 0;
    uVar2 = CONCAT11(0x35,(char)param_1);
  }
  else {
    uVar2 = 0x2502;
    uVar3 = param_1 & 0xff;
  }
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)(uVar2,param_2,param_1,uVar3,param_4,param_3);
  return (ulonglong)CONCAT24(in_ES,param_1);
}


