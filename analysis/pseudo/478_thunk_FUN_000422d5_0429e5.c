// func #478  thunk_FUN_000422d5  000429e5  size=4

undefined4
thunk_FUN_000422d5(undefined4 param_1,char *param_2,undefined4 param_3,undefined4 param_4,
                  undefined1 param_5)

{
  char *pcVar1;
  char cVar2;
  code *pcVar3;
  undefined2 uVar4;
  undefined4 uVar5;
  char *pcVar6;
  
  pcVar3 = (code *)swi(0x21);
  uVar4 = (*pcVar3)(0x3d01,0x420b8);
  pcVar6 = param_2;
  do {
    pcVar1 = pcVar6 + 1;
    cVar2 = *pcVar6;
    pcVar6 = pcVar1;
  } while (cVar2 != '\0');
  pcVar3 = (code *)swi(0x21);
  (*pcVar3)(0x4000,param_2,uVar4,pcVar1 + (-1 - (int)param_2));
  FUN_00044663(0,0xff);
  pcVar3 = (code *)swi(0x21);
  uVar5 = (*pcVar3)(CONCAT11(0x4c,param_5));
  return uVar5;
}
