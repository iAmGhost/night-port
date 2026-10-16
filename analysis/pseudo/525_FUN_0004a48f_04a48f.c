// func #525  FUN_0004a48f  0004a48f  size=485

undefined8
FUN_0004a48f(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,int param_5
            ,uint param_6,int param_7,undefined4 *param_8,undefined4 *param_9,int param_10,
            char *param_11)

{
  char cVar1;
  undefined4 uVar2;
  undefined4 extraout_ECX;
  int *extraout_ECX_00;
  undefined4 *extraout_ECX_01;
  int *extraout_ECX_02;
  int *extraout_ECX_03;
  int *piVar3;
  int iVar4;
  int extraout_EDX;
  int *piVar5;
  char *pcVar6;
  float10 in_ST1;
  undefined8 uVar7;
  int iStack_88;
  uint uStack_84;
  int iStack_80;
  undefined2 *puStack_7c;
  int iStack_78;
  uint uStack_74;
  int iStack_70;
  undefined2 *puStack_6c;
  int iStack_68;
  uint uStack_64;
  undefined1 *puStack_60;
  int iStack_5c;
  uint uStack_58;
  undefined4 uStack_54;
  int iStack_50;
  uint uStack_4c;
  char *pcStack_48;
  char local_44 [18];
  undefined2 uStack_32;
  undefined2 local_30;
  short sStack_2e;
  undefined2 local_2c;
  undefined2 uStack_2a;
  undefined1 local_28 [8];
  int local_20;
  char local_1c;
  
  pcStack_48 = param_11;
  uStack_4c = param_6;
  *param_9 = 0;
  piVar5 = &iStack_50;
  iStack_50 = param_5;
  *param_8 = 0;
  uStack_54 = 0x4a4c0;
  uVar7 = FUN_0004a2a7();
  uVar2 = (undefined4)((ulonglong)uVar7 >> 0x20);
  if ((int)uVar7 == 0) {
    local_30 = 0;
    sStack_2e = 0;
    local_2c = 0;
    uStack_2a = 0;
    if (((param_6 & 0x7fffffff) != 0) || (param_5 != 0)) {
      if ((double)CONCAT44(param_6,param_5) < 0.0) {
        *param_9 = 0xffffffff;
        param_6 = param_6 ^ 0x80000000;
      }
      uStack_58 = param_6;
      iStack_5c = param_5;
      puStack_60 = (undefined1 *)0x4a51c;
      uStack_54 = extraout_ECX;
      FUN_0004a9da();
      iVar4 = *extraout_ECX_00;
      if (iVar4 < 0) {
        iVar4 = -iVar4;
      }
      iVar4 = (iVar4 * 3 + 5) / 10;
      if (*extraout_ECX_00 < 0) {
        iVar4 = -iVar4;
      }
      *extraout_ECX_00 = iVar4;
      if (param_10 == 0x46) {
        param_7 = param_7 + iVar4;
      }
      piVar3 = extraout_ECX_00;
      piVar5 = &iStack_5c;
      if (-1 < param_7) {
        if (0x10 < param_7) {
          param_7 = 0x10;
        }
        local_1c = '\0';
        puStack_60 = local_28;
        uStack_64 = param_6;
        iStack_68 = param_5;
        puStack_6c = (undefined2 *)0x4a57a;
        FUN_0004a7fa();
        if (((float10)0 == in_ST1) && (extraout_EDX < param_7)) {
          local_1c = '\x01';
          param_7 = extraout_EDX;
        }
        puStack_6c = &local_30;
        uStack_74 = param_6;
        iStack_78 = param_5;
        puStack_7c = (undefined2 *)0x4a5a2;
        iStack_70 = param_7;
        iVar4 = FUN_0004a367(*extraout_ECX_01);
        piVar3 = extraout_ECX_02;
        piVar5 = &iStack_78;
        if (iVar4 != 0) {
          *extraout_ECX_02 = *extraout_ECX_02 + iVar4;
          if ((param_10 == 0x46) || (local_1c != '\0')) {
            param_7 = param_7 + iVar4;
          }
          if (param_7 < 1) {
            param_7 = 1;
          }
          else if (0x10 < param_7) {
            param_7 = 0x10;
          }
          puStack_7c = &local_30;
          uStack_84 = param_6;
          iStack_88 = param_5;
          iStack_80 = param_7;
          iVar4 = FUN_0004a367(*extraout_ECX_02);
          piVar3 = extraout_ECX_03;
          piVar5 = &iStack_88;
          if (0 < iVar4) {
            *extraout_ECX_03 = *extraout_ECX_03 + 1;
            piVar5 = &iStack_88;
          }
        }
      }
      local_20 = (int)sStack_2e;
      if ((int)(CONCAT22(local_30,uStack_32) | CONCAT22(sStack_2e,local_30) |
                CONCAT22(local_2c,sStack_2e) | CONCAT22(uStack_2a,local_2c)) >> 0x10 == 0) {
        *param_9 = 0;
        *piVar3 = 0;
      }
    }
    pcVar6 = local_44;
    *(undefined4 *)((int)piVar5 + -4) = 0x4a649;
    FUN_0004a8b9(&local_30,local_44);
    *(char **)((int)piVar5 + -4) = param_11;
    do {
      cVar1 = *pcVar6;
      *param_11 = cVar1;
      if (cVar1 == '\0') break;
      cVar1 = pcVar6[1];
      pcVar6 = pcVar6 + 2;
      param_11[1] = cVar1;
      param_11 = param_11 + 2;
    } while (cVar1 != '\0');
    uVar2 = *(undefined4 *)((int)piVar5 + -4);
  }
  return CONCAT44(param_2,uVar2);
}
