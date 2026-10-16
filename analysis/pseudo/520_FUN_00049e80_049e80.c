// func #520  FUN_00049e80  00049e80  size=547

void FUN_00049e80(char *param_1,undefined4 *param_2,int param_3,char param_4,int param_5,int param_6
                 ,undefined4 param_7,char param_8,byte param_9)

{
  uint uVar1;
  float10 fVar2;
  char *pcVar3;
  char *pcVar4;
  int iVar5;
  int extraout_ECX;
  int extraout_ECX_00;
  double *extraout_EDX;
  double *pdVar6;
  undefined1 *extraout_EDX_00;
  undefined1 *puVar7;
  undefined1 *puVar8;
  undefined8 *puVar9;
  int iVar10;
  float10 in_ST0;
  float10 in_ST1;
  undefined8 uVar11;
  undefined8 uStack_5c;
  undefined4 uStack_54;
  undefined4 uStack_50;
  char *pcStack_4c;
  char local_48 [20];
  double local_34;
  undefined8 local_2c;
  int local_24;
  int local_20;
  int local_1c;
  char *local_18;
  char local_14;
  byte local_10;
  
  pcStack_4c = local_48;
  uStack_50 = param_2[1];
  uStack_54 = *param_2;
  puVar9 = (undefined8 *)&uStack_54;
  uStack_5c._4_4_ = 0x49ea1;
  local_18 = param_1;
  local_14 = param_4;
  uVar11 = FUN_0004a2a7();
  pdVar6 = (double *)((ulonglong)uVar11 >> 0x20);
  if ((int)uVar11 == 0) {
    local_10 = param_9;
    puVar9 = (undefined8 *)&uStack_54;
    uStack_5c = (double)CONCAT44(uStack_5c._4_4_,(undefined4)uStack_5c);
    if (param_9 == 0x47) {
      local_34 = ABS(*pdVar6);
      if ((((ulonglong)local_34 & 0x7fffffff00000000) == 0) && (local_34._0_4_ == 0)) {
        param_6 = 0;
        local_10 = 0x46;
        puVar9 = (undefined8 *)&uStack_54;
        uStack_5c = (double)CONCAT44(uStack_5c._4_4_,(undefined4)uStack_5c);
      }
      else {
        fVar2 = (float10)log2((float10)local_34);
        local_34 = (double)((float10)0.3010299956639812 * fVar2);
        uStack_5c = local_34;
        FUN_0004a32b();
        FUN_00041f7e();
        local_1c = (int)ROUND(in_ST0);
        pdVar6 = extraout_EDX;
        in_ST0 = in_ST1;
        if ((local_1c < -4) || (param_3 <= local_1c)) {
          local_10 = 0x45;
          puVar9 = &uStack_5c;
        }
        else {
          if (0.0 <= local_34) {
            local_1c = local_1c + 1;
          }
          param_6 = 0;
          local_10 = 0x46;
          param_3 = param_3 - local_1c;
          puVar9 = &uStack_5c;
        }
      }
    }
    if ((local_10 == 0x45) && ((param_6 <= -param_3 || (param_3 + 2 <= param_6))))
    goto LAB_00049f5d;
    if (param_9 == 0x45) {
      if (param_6 < 1) {
        if (param_6 < 0) {
          param_3 = param_6 + param_3;
        }
      }
      else {
        param_3 = param_3 + 1;
      }
    }
    iVar10 = *(int *)pdVar6;
    uVar1 = *(uint *)((int)pdVar6 + 4);
    local_2c = *pdVar6;
    puVar8 = (undefined1 *)puVar9;
    if (((((uVar1 & 0x7fffffff) != 0) || (iVar10 != 0)) && (local_10 != 0x45)) && (param_6 != 0)) {
      *(int *)((int)puVar9 + -4) = param_6;
      *(uint *)((int)puVar9 + -8) = uVar1;
      puVar8 = (undefined1 *)((int)puVar9 + -0xc);
      *(int *)((int)puVar9 + -0xc) = iVar10;
      *(undefined4 *)((int)puVar9 + -0x10) = 0x49fbf;
      FUN_0004a3a8();
      local_2c = (double)in_ST0;
      param_3 = extraout_ECX;
    }
    *(char **)(puVar8 + -4) = local_48;
    *(uint *)(puVar8 + -8) = (uint)local_10;
    *(int **)(puVar8 + -0xc) = &local_24;
    *(int **)(puVar8 + -0x10) = &local_20;
    *(int *)(puVar8 + -0x14) = param_3;
    *(undefined4 *)(puVar8 + -0x18) = local_2c._4_4_;
    puVar9 = (undefined8 *)(puVar8 + -0x1c);
    *(int *)(puVar8 + -0x1c) = (int)local_2c;
    *(undefined4 *)(puVar8 + -0x20) = 0x49fe5;
    FUN_0004a48f(&local_20,local_18);
    pcVar3 = local_18;
    if (local_24 == 0) {
      puVar7 = extraout_EDX_00;
      if (local_14 != '\0') {
        *extraout_EDX_00 = 0x2b;
        puVar7 = extraout_EDX_00 + 1;
      }
    }
    else {
      *extraout_EDX_00 = 0x2d;
      puVar7 = extraout_EDX_00 + 1;
    }
    if (local_10 == 0x45) {
      *(undefined4 *)(puVar8 + -0x20) = 0x4a014;
      pcVar4 = (char *)FUN_00049db2(puVar7,local_48,param_6,extraout_ECX_00 - param_6);
      iVar10 = (int)pcVar4 - (int)local_18;
      pcVar3 = pcVar4;
      if ((param_8 != '\0') && (iVar10 < param_5)) {
        pcVar3 = pcVar4 + 1;
        iVar10 = iVar10 + 1;
        *pcVar4 = param_8;
      }
      if ((((ulonglong)local_2c & 0x7fffffff00000000) != 0) || ((int)local_2c != 0)) {
        local_20 = local_20 - param_6;
      }
      *(undefined4 *)(puVar8 + -0x20) = 0x4a058;
      iVar5 = FUN_00049d0b(pcVar3,local_20,param_7,param_5 - iVar10);
      iVar10 = iVar10 + iVar5;
      puVar9 = (undefined8 *)(puVar8 + -0x1c);
    }
    else if (param_5 < (int)(puVar7 + ((local_20 + 1 + extraout_ECX_00) - (int)local_18))) {
      iVar10 = param_5 + 1;
      puVar9 = (undefined8 *)(puVar8 + -0x1c);
    }
    else {
      *(undefined4 *)(puVar8 + -0x20) = 0x4a081;
      iVar10 = FUN_00049db2(puVar7,local_48,local_20);
      iVar10 = iVar10 - (int)pcVar3;
    }
  }
  else {
    pcVar3 = local_18;
    for (iVar10 = 0; local_48[iVar10] != '\0'; iVar10 = iVar10 + 1) {
      if (iVar10 < param_5) {
        *pcVar3 = local_48[iVar10];
      }
      pcVar3 = pcVar3 + 1;
    }
  }
  if (iVar10 <= param_5) {
    *(undefined4 *)((int)puVar9 + -4) = 0x4a09b;
    FUN_00049e15(local_18,iVar10,param_5);
    return;
  }
LAB_00049f5d:
  *(undefined4 *)((int)puVar9 + -4) = 0x49f6c;
  FUN_00041e20(local_18,0x2a,param_5);
  return;
}
