// func #415  FUN_00049b84  00049b84  size=242


char * FUN_00049b84(char *param_1,int param_2)

{
  char *pcVar1;
  undefined4 *extraout_ECX;
  undefined4 *extraout_ECX_00;
  undefined4 *extraout_ECX_01;
  undefined4 *extraout_ECX_02;
  undefined4 *extraout_ECX_03;
  undefined4 *extraout_ECX_04;
  undefined4 *puVar2;
  int iVar3;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  int local_10;
  
  iVar3 = -1;
  if (*param_1 == 'J') {
    iVar3 = 1;
    param_1 = param_1 + 1;
  }
  if (*param_1 == 'M') {
    param_1 = param_1 + 1;
    iVar3 = 0;
  }
  *(int *)(param_2 + 0x20) = iVar3;
  pcVar1 = (char *)FUN_00049a48(param_1,&local_10);
  puVar2 = extraout_ECX;
  if (iVar3 == 0) {
    extraout_ECX[4] = local_10 + -1;
    if (*pcVar1 == '.') {
      pcVar1 = (char *)FUN_00049a48(pcVar1 + 1,&local_10);
      extraout_ECX_00[3] = local_10;
      puVar2 = extraout_ECX_00;
      if (*pcVar1 == '.') {
        pcVar1 = (char *)FUN_00049a48(pcVar1 + 1,&local_10);
        extraout_ECX_01[6] = local_10;
        puVar2 = extraout_ECX_01;
      }
    }
    puVar2[7] = 0;
  }
  else {
    extraout_ECX[7] = local_10;
  }
  local_14 = 2;
  local_1c = 0;
  local_18 = 0;
  if (*pcVar1 == '/') {
    pcVar1 = (char *)FUN_00049a48(pcVar1 + 1,&local_14);
    puVar2 = extraout_ECX_02;
    if (*pcVar1 == ':') {
      pcVar1 = (char *)FUN_00049a48(pcVar1 + 1,&local_18);
      puVar2 = extraout_ECX_03;
      if (*pcVar1 == ':') {
        pcVar1 = (char *)FUN_00049a48(pcVar1 + 1,&local_1c);
        puVar2 = extraout_ECX_04;
      }
    }
  }
  *puVar2 = local_1c;
  puVar2[1] = local_18;
  puVar2[2] = local_14;
  return pcVar1;
}


