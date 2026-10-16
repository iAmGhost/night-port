// func #416  FUN_0004a24d  0004a24d  size=90


undefined8 FUN_0004a24d(char *param_1,undefined4 param_2)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int extraout_ECX;
  int *piVar5;
  char *pcVar6;
  
  if ((DAT_000537d4 != (int *)0x0) && (param_1 != (char *)0x0)) {
    uVar3 = 0xffffffff;
    pcVar6 = param_1;
    do {
      if (uVar3 == 0) break;
      uVar3 = uVar3 - 1;
      cVar1 = *pcVar6;
      pcVar6 = pcVar6 + 1;
    } while (cVar1 != '\0');
    iVar4 = ~uVar3 - 1;
    for (piVar5 = DAT_000537d4; *piVar5 != 0; piVar5 = piVar5 + 1) {
      iVar2 = FUN_0004a7a2(*piVar5,param_1,iVar4);
      if ((iVar2 == 0) && (*(char *)(extraout_ECX + iVar4) == '=')) {
        iVar4 = ~uVar3 + extraout_ECX;
        goto LAB_0004a2a0;
      }
    }
  }
  iVar4 = 0;
LAB_0004a2a0:
  return CONCAT44(param_2,iVar4);
}


