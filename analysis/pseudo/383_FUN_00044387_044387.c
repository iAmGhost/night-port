// func #314  FUN_00044387  00044387  size=504


int FUN_00044387(undefined4 *param_1,int *param_2,int param_3,int param_4)

{
  byte bVar1;
  undefined6 *puVar2;
  undefined4 *puVar3;
  int iVar4;
  int iVar5;
  int *piVar6;
  int iVar7;
  int iVar8;
  undefined8 uVar9;
  int local_20;
  int local_18;
  
  iVar7 = 0;
  iVar8 = 0;
  local_18 = 0;
  while( true ) {
    iVar4 = (*(code *)*param_1)(param_1);
    if (((&DAT_0005185c)[(byte)((char)iVar4 + 1)] & 2) == 0) break;
    iVar8 = iVar8 + 1;
  }
  if ((*(byte *)(param_1 + 4) & 2) == 0) {
    iVar5 = param_1[3];
    param_1[3] = iVar5 + -1;
    if (iVar5 == 0) {
LAB_000444f6:
      (*(code *)param_1[1])(iVar4,param_1);
    }
    else {
      local_20 = 0x2b;
      iVar5 = iVar4;
      if ((param_4 != 0) && ((iVar4 == 0x2b || (iVar4 == 0x2d)))) {
        iVar8 = iVar8 + 1;
        iVar5 = FUN_000445a6(param_1);
        local_20 = iVar4;
        if (iVar5 == -1) goto LAB_000444fd;
      }
      if (param_3 == 0) {
        if (iVar5 == 0x30) {
          iVar7 = 1;
          iVar5 = FUN_000445a6(param_1);
          if (iVar5 == -1) goto LAB_000444fd;
          if ((iVar5 == 0x78) || (iVar5 == 0x58)) {
            iVar8 = iVar8 + 2;
            iVar5 = FUN_000445a6(param_1);
            iVar7 = 0;
            if (iVar5 == -1) goto LAB_000444fd;
            param_3 = 0x10;
          }
          else {
            param_3 = 8;
          }
        }
        else {
          param_3 = 10;
        }
      }
      else if ((param_3 == 0x10) && (iVar5 == 0x30)) {
        iVar7 = 1;
        iVar5 = FUN_000445a6(param_1);
        if (iVar5 == -1) goto LAB_000444fd;
        if ((iVar5 == 0x78) || (iVar5 == 0x58)) {
          iVar8 = iVar8 + 2;
          iVar5 = FUN_000445a6(param_1);
          iVar7 = 0;
          goto LAB_00044499;
        }
      }
      do {
        uVar9 = FUN_0004457f(iVar5);
        iVar4 = (int)((ulonglong)uVar9 >> 0x20);
        if (param_3 <= (int)uVar9) {
          if ((iVar4 != 0x3a) || ((*(byte *)(param_1 + 4) & 0x80) == 0)) goto LAB_000444f6;
          goto LAB_000444cd;
        }
        iVar7 = iVar7 + 1;
        local_18 = local_18 * param_3 + (int)uVar9;
        iVar5 = FUN_000445a6(param_1);
LAB_00044499:
      } while (iVar5 != -1);
    }
  }
LAB_000444fd:
  if (local_20 == 0x2d) {
    local_18 = -local_18;
  }
  if (0 < iVar7) {
    bVar1 = *(byte *)(param_1 + 4);
    iVar7 = iVar7 + iVar8;
    if ((bVar1 & 1) != 0) {
      if ((bVar1 & 4) == 0) {
        if ((bVar1 & 8) == 0) {
          puVar3 = (undefined4 *)*param_2;
          *param_2 = (int)(puVar3 + 1);
          piVar6 = (int *)*puVar3;
        }
        else {
          puVar3 = (undefined4 *)*param_2;
          *param_2 = (int)(puVar3 + 1);
          piVar6 = (int *)*puVar3;
        }
      }
      else {
        puVar2 = (undefined6 *)*param_2;
        *param_2 = (int)(puVar2 + 1);
        piVar6 = (int *)*puVar2;
      }
      if ((*(byte *)(param_1 + 4) & 0x10) == 0) {
        *piVar6 = local_18;
      }
      else {
        *(short *)piVar6 = (short)local_18;
      }
    }
  }
  return iVar7;
LAB_000444cd:
  iVar7 = iVar7 + 1;
  iVar4 = FUN_000445a6(param_1);
  if (iVar4 == -1) goto LAB_000444fd;
  uVar9 = FUN_0004457f(iVar4,iVar4);
  iVar4 = (int)((ulonglong)uVar9 >> 0x20);
  if (param_3 <= (int)uVar9) goto LAB_000444f6;
  local_18 = local_18 * param_3 + (int)uVar9;
  goto LAB_000444cd;
}


