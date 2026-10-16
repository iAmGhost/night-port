// func #300  FUN_000432b7  000432b7  size=108


undefined4 * FUN_000432b7(undefined4 *param_1,undefined4 param_2,undefined4 param_3,uint param_4)

{
  undefined4 *puVar1;
  int iVar2;
  
  puVar1 = param_1;
  if (param_4 != 0) {
    do {
      if (((uint)param_1 & 0x1f) == 0) break;
      *param_1 = param_2;
      param_1 = param_1 + 1;
      param_4 = param_4 - 1;
    } while (param_4 != 0);
    if (param_4 >> 2 != 0) {
      iVar2 = (param_4 >> 2) - 1;
      if (iVar2 != 0) {
        do {
          puVar1 = param_1;
          *puVar1 = param_2;
          puVar1[1] = param_2;
          puVar1[2] = param_2;
          puVar1[3] = param_2;
          if (iVar2 == 1) goto LAB_000432f6;
          puVar1[4] = param_2;
          puVar1[5] = param_2;
          iVar2 = iVar2 + -2;
          puVar1[6] = param_2;
          puVar1[7] = param_2;
          param_1 = puVar1 + 8;
        } while (iVar2 != 0);
        puVar1 = puVar1 + 4;
LAB_000432f6:
        param_1 = puVar1 + 4;
      }
      *param_1 = param_2;
      param_1[1] = param_2;
      param_1[2] = param_2;
      param_1[3] = param_2;
      param_1 = param_1 + 4;
    }
    param_4 = param_4 & 3;
    puVar1 = param_1;
    if (param_4 != 0) {
      *param_1 = param_2;
      puVar1 = param_1 + 1;
      if (param_4 != 1) {
        *puVar1 = param_2;
        puVar1 = param_1 + 2;
        if (param_4 != 2) {
          *puVar1 = param_2;
          puVar1 = param_1 + 3;
        }
      }
    }
  }
  return puVar1;
}


