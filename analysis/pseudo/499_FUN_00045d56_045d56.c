// func #499  FUN_00045d56  00045d56  size=71

void FUN_00045d56(char *param_1,int param_2)

{
  char cVar1;
  char *pcVar2;
  
  if ((*(byte *)(param_2 + 0x14) & 1) == 0) {
    for (; ((cVar1 = *param_1, cVar1 != '\0' && (cVar1 != 'e')) && (cVar1 != 'E'));
        param_1 = param_1 + 1) {
    }
    pcVar2 = param_1 + -1;
    if ((*(char *)(param_2 + 0x15) == 'G') || (*(char *)(param_2 + 0x15) == 'g')) {
      for (; *pcVar2 == '0'; pcVar2 = pcVar2 + -1) {
      }
    }
    if (*pcVar2 != '.') {
      pcVar2 = pcVar2 + 1;
    }
    do {
      cVar1 = *param_1;
      param_1 = param_1 + 1;
      *pcVar2 = cVar1;
      pcVar2 = pcVar2 + 1;
    } while (cVar1 != '\0');
  }
  return;
}
