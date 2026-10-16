// func #519  FUN_00049e15  00049e15  size=106

void FUN_00049e15(char *param_1,int param_2,int param_3)

{
  char cVar1;
  char cVar2;
  char *pcVar3;
  char *pcVar4;
  int iVar5;
  
  if (param_2 != param_3) {
    cVar1 = param_1[1];
    pcVar4 = param_1 + param_3;
    pcVar3 = param_1 + param_2;
    do {
      iVar5 = param_3;
      pcVar3 = pcVar3 + -1;
      pcVar4 = pcVar4 + -1;
      *pcVar4 = *pcVar3;
      param_3 = iVar5 + -1;
    } while (pcVar3 != param_1);
    cVar2 = *param_1;
    if (cVar2 == '.') {
      param_1[iVar5 + -2] = '0';
    }
    else if (((cVar2 == '+') || (cVar2 == '-')) && (cVar1 == '.')) {
      param_1[iVar5 + -1] = '0';
      param_1[iVar5 + -2] = *param_1;
    }
    FUN_00041e20(param_1,0x20);
  }
  return;
}
