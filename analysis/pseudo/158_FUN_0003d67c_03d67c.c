// func #158  FUN_0003d67c  0003d67c  size=224


void FUN_0003d67c(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined1 uVar1;
  undefined1 uVar2;
  undefined1 uVar3;
  int iVar4;
  int iVar5;
  undefined8 uVar6;
  undefined4 local_30;
  
  uVar6 = FUN_000418ee();
  iVar5 = (int)((ulonglong)uVar6 >> 0x20);
  iVar4 = (int)uVar6;
  uVar1 = *(undefined1 *)(param_3 * 3 + iVar4);
  uVar2 = *(undefined1 *)(param_3 * 3 + iVar4 + 1);
  uVar3 = *(undefined1 *)(param_3 * 3 + iVar4 + 2);
  while (local_30 = param_3 + -1, iVar5 <= local_30) {
    *(undefined1 *)(param_3 * 3 + iVar4) = *(undefined1 *)(local_30 * 3 + iVar4);
    *(undefined1 *)(param_3 * 3 + iVar4 + 1) = *(undefined1 *)(local_30 * 3 + iVar4 + 1);
    *(undefined1 *)(param_3 * 3 + iVar4 + 2) = *(undefined1 *)(local_30 * 3 + iVar4 + 2);
    param_3 = local_30;
  }
  *(undefined1 *)(iVar5 * 3 + iVar4) = uVar1;
  *(undefined1 *)(iVar5 * 3 + iVar4 + 1) = uVar2;
  *(undefined1 *)(iVar5 * 3 + iVar4 + 2) = uVar3;
  return;
}


