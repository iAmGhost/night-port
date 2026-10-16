// func #518  FUN_00049db2  00049db2  size=98

char * FUN_00049db2(char *param_1,char *param_2,int param_3,int param_4)

{
  char cVar1;
  
  if ((param_4 == 0) && (param_3 < 1)) {
    *param_1 = '0';
    param_1[1] = '.';
    return param_1 + 2;
  }
  for (; (0 < param_3 && (*param_2 != '\0')); param_2 = param_2 + 1) {
    param_3 = param_3 + -1;
    *param_1 = *param_2;
    param_1 = param_1 + 1;
  }
  for (; 0 < param_3; param_3 = param_3 + -1) {
    *param_1 = '0';
    param_1 = param_1 + 1;
  }
  *param_1 = '.';
  param_1 = param_1 + 1;
  if (0 < param_4) {
    do {
      if (param_3 == 0) break;
      param_3 = param_3 + 1;
      *param_1 = '0';
      param_1 = param_1 + 1;
      param_4 = param_4 + -1;
    } while (param_4 != 0);
  }
  if (0 < param_4) {
    do {
      cVar1 = *param_2;
      if (cVar1 == '\0') break;
      param_2 = param_2 + 1;
      *param_1 = cVar1;
      param_1 = param_1 + 1;
      param_4 = param_4 + -1;
    } while (param_4 != 0);
    for (; param_4 != 0; param_4 = param_4 + -1) {
      *param_1 = '0';
      param_1 = param_1 + 1;
    }
  }
  return param_1;
}
