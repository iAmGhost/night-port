// func #403  FUN_00049459  00049459  size=73


char * FUN_00049459(uint param_1,char *param_2,uint param_3)

{
  char cVar1;
  uint uVar2;
  char *pcVar3;
  char *pcVar4;
  char local_33 [39];
  
  pcVar3 = local_33;
  do {
    uVar2 = param_1 / param_3;
    *pcVar3 = "0123456789abcdefghijklmnopqrstuvwxyz"[param_1 % param_3];
    pcVar3 = pcVar3 + 1;
    param_1 = uVar2;
    pcVar4 = param_2;
  } while (uVar2 != 0);
  do {
    pcVar3 = pcVar3 + -1;
    cVar1 = *pcVar3;
    *pcVar4 = cVar1;
    pcVar4 = pcVar4 + 1;
  } while (cVar1 != '\0');
  return param_2;
}


