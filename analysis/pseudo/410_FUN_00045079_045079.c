// func #337  FUN_00045079  00045079  size=97


void FUN_00045079(undefined4 param_1,char *param_2,int param_3)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  char *pcVar4;
  char *pcVar5;
  byte bVar6;
  
  bVar6 = 0;
  FUN_00041f42(param_1,param_2,0x10);
  uVar3 = 0xffffffff;
  pcVar4 = param_2;
  do {
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    cVar1 = *pcVar4;
    pcVar4 = pcVar4 + (uint)bVar6 * -2 + 1;
  } while (cVar1 != '\0');
  iVar2 = param_3 + -1;
  pcVar4 = param_2 + (~uVar3 - 1);
  pcVar5 = param_2 + iVar2;
  while (pcVar4 != param_2) {
    pcVar4 = pcVar4 + -1;
    iVar2 = iVar2 + -1;
    *pcVar5 = *pcVar4;
    pcVar5 = pcVar5 + -1;
  }
  pcVar4 = param_2 + iVar2;
  for (; -1 < iVar2; iVar2 = iVar2 + -1) {
    *pcVar4 = '0';
    pcVar4 = pcVar4 + -1;
  }
  param_2[param_3] = '\0';
  return;
}


