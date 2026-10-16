// func #332  FUN_00044c23  00044c23  size=641


undefined4 FUN_00044c23(undefined4 param_1,char *param_2,int *param_3,code *param_4)

{
  int iVar1;
  char cVar2;
  undefined6 *puVar3;
  undefined1 *puVar4;
  char *pcVar5;
  char *extraout_ECX;
  undefined4 *puVar6;
  undefined2 *puVar7;
  bool bVar8;
  undefined1 auStack_5c [40];
  undefined4 local_34;
  int local_30;
  int local_2c;
  int local_28;
  undefined4 local_24;
  byte local_20;
  char local_1f;
  char local_1e;
  char local_1d [5];
  code *local_18;
  undefined1 local_14 [4];
  
  local_14[0] = 0;
  local_20 = 0;
  local_24 = 0;
  local_34 = param_1;
  local_18 = param_4;
  do {
    while( true ) {
      while( true ) {
        while( true ) {
          cVar2 = *param_2;
          if (cVar2 == '\0') {
            return local_24;
          }
          param_2 = param_2 + 1;
          if (cVar2 == '%') break;
          (*local_18)(&local_34,cVar2);
        }
        param_2 = (char *)FUN_00044ea4(param_2,param_3,&local_34);
        local_1f = *param_2;
        param_2 = param_2 + 1;
        if (local_1f == '\0') {
          return local_24;
        }
        if (local_1f == 'n') break;
        puVar4 = (undefined1 *)FUN_000451bc(auStack_5c,param_3,&local_34,local_14);
        if (((local_20 & 8) == 0) && (local_1e == ' ')) {
          while (local_30 = local_30 + -1, -1 < local_30) {
            (*local_18)(&local_34,local_1e);
          }
        }
        pcVar5 = local_1d;
        while (*pcVar5 != '\0') {
          (*local_18)(&local_34,*pcVar5);
          pcVar5 = extraout_ECX;
        }
        while (iVar1 = local_28 + -1, local_28 != 0) {
          local_28 = iVar1;
          (*local_18)(&local_34,0x30);
        }
        local_28 = iVar1;
        if (((local_20 & 8) == 0) && (local_1e != ' ')) {
          while (local_30 = local_30 + -1, -1 < local_30) {
            (*local_18)(&local_34,local_1e);
          }
        }
        if ((local_1f == 's') || (local_1f == 'S')) {
          if ((local_20 & 0x20) == 0) {
            while (iVar1 = local_2c + -1, bVar8 = local_2c != 0, local_2c = iVar1, bVar8) {
              (*local_18)(&local_34,*puVar4);
              puVar4 = puVar4 + 1;
            }
          }
          else {
            while (iVar1 = local_2c + -1, bVar8 = local_2c != 0, local_2c = iVar1, bVar8) {
              (*local_18)(&local_34,*puVar4);
              puVar4 = puVar4 + 2;
            }
          }
        }
        else {
          while (iVar1 = local_2c + -1, bVar8 = local_2c != 0, local_2c = iVar1, bVar8) {
            (*local_18)(&local_34,*puVar4);
            puVar4 = puVar4 + 1;
          }
        }
        if (((local_20 & 8) != 0) && (0 < local_30)) {
          while (iVar1 = local_30 + -1, bVar8 = local_30 != 0, local_30 = iVar1, bVar8) {
            (*local_18)(&local_34,0x20);
          }
        }
      }
      if ((local_20 & 0x20) == 0) break;
      if ((local_20 & 0x80) == 0) {
        if ((local_20 & 0x40) == 0) {
          puVar6 = (undefined4 *)*param_3;
          *param_3 = (int)(puVar6 + 1);
          puVar6 = (undefined4 *)*puVar6;
        }
        else {
          puVar6 = (undefined4 *)*param_3;
          *param_3 = (int)(puVar6 + 1);
          puVar6 = (undefined4 *)*puVar6;
        }
LAB_00044ca0:
        *puVar6 = local_24;
      }
      else {
        puVar3 = (undefined6 *)*param_3;
        *param_3 = (int)(puVar3 + 1);
        puVar6 = (undefined4 *)*puVar3;
LAB_00044c89:
        *puVar6 = local_24;
      }
    }
    if ((local_20 & 0x10) == 0) {
      if ((local_20 & 0x80) == 0) {
        if ((local_20 & 0x40) == 0) {
          puVar6 = (undefined4 *)*param_3;
          *param_3 = (int)(puVar6 + 1);
          puVar6 = (undefined4 *)*puVar6;
        }
        else {
          puVar6 = (undefined4 *)*param_3;
          *param_3 = (int)(puVar6 + 1);
          puVar6 = (undefined4 *)*puVar6;
        }
        goto LAB_00044ca0;
      }
      puVar3 = (undefined6 *)*param_3;
      *param_3 = (int)(puVar3 + 1);
      puVar6 = (undefined4 *)*puVar3;
      goto LAB_00044c89;
    }
    if ((local_20 & 0x80) == 0) {
      if ((local_20 & 0x40) == 0) {
        puVar6 = (undefined4 *)*param_3;
        *param_3 = (int)(puVar6 + 1);
        puVar7 = (undefined2 *)*puVar6;
      }
      else {
        puVar6 = (undefined4 *)*param_3;
        *param_3 = (int)(puVar6 + 1);
        puVar7 = (undefined2 *)*puVar6;
      }
      *puVar7 = (short)local_24;
    }
    else {
      puVar3 = (undefined6 *)*param_3;
      *param_3 = (int)(puVar3 + 1);
      *(undefined2 *)*puVar3 = (short)local_24;
    }
  } while( true );
}


