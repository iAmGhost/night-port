// func #495  FUN_0004595b  0004595b  size=232

int FUN_0004595b(char *param_1,int param_2,undefined4 *param_3)

{
  char cVar1;
  char *pcVar2;
  char *pcVar3;
  int iVar4;
  int iVar5;
  char *pcVar6;
  char *local_18;
  
  iVar4 = 0;
  do {
    while( true ) {
      for (; (cVar1 = *param_1, cVar1 == ' ' || (cVar1 == '\t')); param_1 = param_1 + 1) {
      }
      pcVar3 = param_1;
      iVar5 = iVar4;
      if (cVar1 == '\0') goto LAB_00045a34;
      pcVar6 = param_1;
      if (cVar1 == '\"') {
        local_18 = param_1 + 1;
        pcVar3 = local_18;
        pcVar6 = local_18;
        while ((cVar1 = *pcVar3, cVar1 != '\"' && (cVar1 != '\0'))) {
          pcVar2 = pcVar3;
          if ((cVar1 == '\\') && ((pcVar3[1] == '\"' || (pcVar3[1] == '\\')))) {
            pcVar2 = pcVar3 + 1;
          }
          pcVar3 = pcVar2 + 1;
          if (param_2 != 0) {
            *pcVar6 = *pcVar2;
            pcVar6 = pcVar6 + 1;
          }
        }
      }
      else {
        while (((cVar1 = *pcVar3, local_18 = param_1, cVar1 != '\0' && (cVar1 != ' ')) &&
               (cVar1 != '\t'))) {
          pcVar2 = pcVar3;
          if ((cVar1 == '\\') && (pcVar3[1] == '\"')) {
            pcVar2 = pcVar3 + 1;
          }
          pcVar3 = pcVar2 + 1;
          if (param_2 != 0) {
            *pcVar6 = *pcVar2;
            pcVar6 = pcVar6 + 1;
          }
        }
      }
      param_1 = pcVar3 + 1;
      iVar5 = iVar4 + 1;
      if (param_2 == 0) break;
      *(char **)(param_2 + iVar4 * 4) = local_18;
      if (*pcVar3 == '\0') {
        *pcVar6 = '\0';
        goto LAB_00045a34;
      }
      *pcVar6 = '\0';
      iVar4 = iVar5;
    }
    iVar4 = iVar5;
  } while (*pcVar3 != '\0');
LAB_00045a34:
  *param_3 = pcVar3;
  return iVar5;
}
