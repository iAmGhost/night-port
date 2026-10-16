// func #43  FUN_00016b8f  00016b8f  size=1117


/* WARNING: Removing unreachable block (ram,0x00016fd8) */

undefined4
FUN_00016b8f(undefined4 param_1,undefined4 param_2,int param_3,undefined4 param_4,int *param_5)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int extraout_ECX;
  int iVar6;
  undefined8 uVar7;
  int *in_stack_00000000;
  
  uVar7 = FUN_000418ee();
  iVar6 = (int)((ulonglong)uVar7 >> 0x20);
  iVar4 = (int)uVar7;
  *(undefined4 *)(iVar4 + 0x17) = 2;
  if (*(int *)(iVar4 + 0x8f) == 0) {
    iVar5 = *(int *)(DAT_00063354 + 0x16);
    iVar1 = *(int *)(DAT_00063354 + 0x12);
    iVar2 = *(int *)(DAT_00063354 + 0x12);
    *(undefined4 *)(iVar4 + 0x8f) = 1;
    if (extraout_ECX < iVar2 + (iVar5 - iVar1 >> 1)) {
      *(undefined4 *)(iVar4 + 0x9f) = 0xffffffff;
    }
    else {
      *(undefined4 *)(iVar4 + 0x9f) = 1;
    }
    iVar5 = FUN_0001519f(iVar4);
    if (iVar5 < param_3) {
      *(undefined4 *)(iVar4 + 0x9b) = 1;
    }
    else {
      *(undefined4 *)(iVar4 + 0x9b) = 0xffffffff;
    }
    *(undefined4 *)(iVar4 + 0x93) = 0;
    *(undefined4 *)(iVar4 + 0xa3) = 0;
    *(undefined4 *)(iVar4 + 0xa7) = 0;
  }
  uVar3 = *(uint *)(iVar4 + 0x93);
  if (uVar3 == 0) {
    uVar7 = FUN_00041f76(*(undefined4 *)(iVar4 + 0xa3),*(int *)(iVar4 + 0x11f) >> 1);
    if ((int)uVar7 < (int)((ulonglong)uVar7 >> 0x20) + 0xf) {
      if (*(int *)(iVar4 + 0x9b) < 0) {
        *in_stack_00000000 = iVar6;
        *in_stack_00000000 = -*in_stack_00000000;
      }
      else {
        *in_stack_00000000 = iVar6;
      }
      if (*(int *)(iVar4 + 0x9f) < 0) {
        *param_5 = iVar6;
      }
      else {
        *param_5 = iVar6;
        *param_5 = -*param_5;
      }
      if (*(int *)(iVar4 + 0x97) < 0) {
        return 1;
      }
      *(int *)(iVar4 + 0x97) = *(int *)(iVar4 + 0x97) + -1;
      *(int *)(iVar4 + 0xa3) = *(int *)(iVar4 + 0xa3) + *in_stack_00000000;
      *(int *)(iVar4 + 0xa7) = *(int *)(iVar4 + 0xa7) + *param_5;
    }
    else {
      *(undefined4 *)(iVar4 + 0x93) = 1;
      *(undefined4 *)(iVar4 + 0xa3) = 0;
      *(undefined4 *)(iVar4 + 0xa7) = 0;
    }
  }
  else if (uVar3 < 2) {
    iVar5 = FUN_00041f76(*(undefined4 *)(iVar4 + 0xa3));
    if (iVar5 < 0x3c) {
      if (*(int *)(iVar4 + 0x9b) < 0) {
        *in_stack_00000000 = iVar6;
        *in_stack_00000000 = -*in_stack_00000000;
      }
      else {
        *in_stack_00000000 = iVar6;
      }
      *param_5 = 0;
      *(int *)(iVar4 + 0xa3) = *(int *)(iVar4 + 0xa3) + *in_stack_00000000;
      if (*(int *)(iVar4 + 0x97) < 0) {
        return 1;
      }
      *(int *)(iVar4 + 0x97) = *(int *)(iVar4 + 0x97) + -1;
      if (*(int *)(iVar4 + 0x9f) < 0) {
        if ((((param_3 < *(int *)(iVar4 + 0xb3) + -0x28) ||
             (*(int *)(iVar4 + 0xb3) + 0x28 < param_3)) ||
            (extraout_ECX < *(int *)(iVar4 + 0xb7) + -7)) ||
           (*(int *)(iVar4 + 0xb7) + 7 < extraout_ECX)) {
          *(undefined4 *)(iVar4 + 0x93) = 1;
          *(undefined4 *)(iVar4 + 0x8f) = 0;
          return 1;
        }
      }
      else if ((0 < *(int *)(iVar4 + 0x9f)) &&
              (((param_3 < *(int *)(iVar4 + 0xb3) + -0x28 ||
                (*(int *)(iVar4 + 0xb3) + 0x28 < param_3)) ||
               ((extraout_ECX < *(int *)(iVar4 + 0xb7) + -7 ||
                (*(int *)(iVar4 + 0xb7) + 7 < extraout_ECX)))))) {
        *(undefined4 *)(iVar4 + 0x93) = 1;
        *(undefined4 *)(iVar4 + 0x8f) = 0;
        return 1;
      }
    }
    else {
      *(undefined4 *)(iVar4 + 0x93) = 2;
      *(undefined4 *)(iVar4 + 0xa3) = 0;
      *(undefined4 *)(iVar4 + 0xa7) = 0;
    }
  }
  else if (uVar3 == 2) {
    uVar7 = FUN_00041f76(*(undefined4 *)(iVar4 + 0xa3),*(int *)(iVar4 + 0x11f) >> 1);
    if ((int)((ulonglong)uVar7 >> 0x20) + 0xf <= (int)uVar7) {
      *(undefined4 *)(iVar4 + 0x93) = 1;
      *(undefined4 *)(iVar4 + 0x8f) = 0;
      return 1;
    }
    if (*(int *)(iVar4 + 0x9b) < 0) {
      *in_stack_00000000 = iVar6;
      *in_stack_00000000 = -*in_stack_00000000;
    }
    else {
      *in_stack_00000000 = iVar6;
    }
    if (*(int *)(iVar4 + 0x9f) < 0) {
      *param_5 = iVar6;
      *param_5 = -*param_5;
    }
    else {
      *param_5 = iVar6;
    }
    if (*(int *)(iVar4 + 0x97) < 0) {
      return 1;
    }
    *(int *)(iVar4 + 0x97) = *(int *)(iVar4 + 0x97) + -1;
    *(int *)(iVar4 + 0xa3) = *(int *)(iVar4 + 0xa3) + *in_stack_00000000;
  }
  return 0;
}


