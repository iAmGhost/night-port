// func #242  FUN_00041f5d  00041f5d  size=25


int FUN_00041f5d(char *param_1)

{
  char cVar1;
  uint uVar2;
  
  uVar2 = 0xffffffff;
  do {
    if (uVar2 == 0) break;
    uVar2 = uVar2 - 1;
    cVar1 = *param_1;
    param_1 = param_1 + 1;
  } while (cVar1 != '\0');
  return ~uVar2 - 1;
}


