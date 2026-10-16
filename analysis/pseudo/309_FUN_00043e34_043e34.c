// func #309  FUN_00043e34  00043e34  size=238


int FUN_00043e34(undefined4 *param_1,int *param_2)

{
  byte bVar1;
  int iVar2;
  undefined2 *extraout_ECX;
  undefined2 *extraout_ECX_00;
  undefined2 *extraout_ECX_01;
  undefined2 *puVar3;
  int iVar4;
  ushort in_DS;
  char local_18;
  
  if ((*(byte *)(param_1 + 4) & 0x20) == 0) {
    local_18 = '\x01';
  }
  else {
    local_18 = '\x02';
  }
  bVar1 = *(byte *)(param_1 + 4);
  if ((bVar1 & 1) != 0) {
    if ((bVar1 & 4) == 0) {
      if ((bVar1 & 8) == 0) {
        *param_2 = *param_2 + 4;
        param_2 = (int *)(uint)in_DS;
      }
      else {
        *param_2 = *param_2 + 4;
        param_2 = (int *)(uint)in_DS;
      }
    }
    else {
      *param_2 = *param_2 + 8;
    }
  }
  iVar4 = 0;
  while (param_2 = (int *)(*(code *)*param_1)(param_1,param_2),
        ((&DAT_0005185c)[(byte)((char)param_2 + 1)] & 2) != 0) {
    iVar4 = iVar4 + 1;
  }
  if ((*(byte *)(param_1 + 4) & 2) == 0) {
    iVar2 = param_1[3];
    param_1[3] = iVar2 + -1;
    puVar3 = extraout_ECX;
    if (iVar2 != 0) {
      do {
        iVar4 = iVar4 + 1;
        if ((*(byte *)(param_1 + 4) & 1) != 0) {
          if (local_18 == '\x01') {
            *(char *)puVar3 = (char)param_2;
          }
          else {
            *puVar3 = (short)param_2;
          }
        }
        param_2 = (int *)FUN_000445a6(param_1);
        puVar3 = extraout_ECX_00;
        if (param_2 == (int *)0xffffffff) goto LAB_00043efc;
      } while (((&DAT_0005185c)[(byte)((char)param_2 + 1)] & 2) == 0);
    }
    (*(code *)param_1[1])(param_2,param_1);
    puVar3 = extraout_ECX_01;
  }
  else {
    iVar4 = 0;
    puVar3 = extraout_ECX;
  }
LAB_00043efc:
  if (((*(byte *)(param_1 + 4) & 1) != 0) && (0 < iVar4)) {
    if (local_18 == '\x01') {
      *(undefined1 *)puVar3 = 0;
    }
    else {
      *puVar3 = 0;
    }
  }
  return iVar4;
}


