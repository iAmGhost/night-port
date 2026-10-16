// func #79  FUN_00032158  00032158  size=413


void FUN_00032158(void)

{
  undefined4 uVar1;
  int iVar2;
  int local_38;
  int local_20;
  
  FUN_000418ee();
  local_20 = DAT_00053c70;
  if (DAT_00053c6c == 1) {
    for (local_38 = 0; local_38 < DAT_00053c68; local_38 = local_38 + 1) {
      if ((((local_20 != DAT_00053c7c) && (*(int *)(local_20 + 0x117) == 0)) &&
          (*(int *)(local_20 + 0x1f) == 4)) && (*(int *)(local_20 + 0xaf) != 6)) {
        uVar1 = FUN_0001519f(DAT_00053c7c);
        *(undefined4 *)(local_20 + 0xb3) = uVar1;
        uVar1 = FUN_000151d5(DAT_00053c7c);
        *(undefined4 *)(local_20 + 0xb7) = uVar1;
        if (*(int *)(DAT_00053c7c + 0x1b) == 0) {
          *(int *)(local_20 + 0xb3) = *(int *)(local_20 + 0xb3) + -0x2d;
          *(int *)(local_20 + 0xb7) = *(int *)(local_20 + 0xb7) + -5;
          iVar2 = FUN_00041e9f();
          *(int *)(local_20 + 0xb3) = *(int *)(local_20 + 0xb3) + iVar2 % 10;
          iVar2 = FUN_00041e9f();
          *(int *)(local_20 + 0xb7) = *(int *)(local_20 + 0xb7) + iVar2 % 10;
        }
        else {
          *(int *)(local_20 + 0xb3) = *(int *)(local_20 + 0xb3) + 0x2d;
          *(int *)(local_20 + 0xb7) = *(int *)(local_20 + 0xb7) + -5;
          iVar2 = FUN_00041e9f();
          *(int *)(local_20 + 0xb3) = *(int *)(local_20 + 0xb3) - iVar2 % 10;
          iVar2 = FUN_00041e9f();
          *(int *)(local_20 + 0xb7) = *(int *)(local_20 + 0xb7) + iVar2 % 10;
        }
        *(undefined4 *)(local_20 + 0xaf) = 0;
        *(undefined4 *)(local_20 + 0x1f) = 6;
      }
      local_20 = *(int *)(local_20 + 0x13f);
    }
    DAT_00053c6c = 0;
  }
  return;
}


