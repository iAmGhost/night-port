// func #166  FUN_0003df96  0003df96  size=265


void FUN_0003df96(void)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int extraout_EDX;
  int *piVar6;
  float10 fVar7;
  int local_2c;
  int local_28;
  int local_24;
  undefined4 local_20;
  
  local_20 = FUN_000418ee();
  piVar6 = &local_2c;
  for (local_24 = 0; local_24 < DAT_00062a90; local_24 = local_24 + 1) {
    local_28 = (&DAT_0005d0b4)[local_24 * 5];
    *(uint *)((int)piVar6 + -4) = (uint)*(byte *)(&DAT_0005d0c0 + local_24 * 5);
    *(int *)((int)piVar6 + -8) =
         *(int *)(DAT_00063dfc + 0x38) + *(int *)(*(int *)(DAT_00063dfc + 0x34) + local_28 * 4);
    *(undefined4 *)((int)piVar6 + -0xc) = *(undefined4 *)(*(int *)(DAT_00063dfc + 8) + local_28 * 4)
    ;
    uVar1 = *(undefined4 *)(*(int *)(DAT_00063dfc + 4) + local_28 * 4);
    iVar2 = (&DAT_0005d0bc)[local_24 * 5];
    iVar3 = *(int *)(*(int *)(DAT_00063dfc + 0x10) + local_28 * 4);
    iVar4 = (&DAT_0005d0b8)[local_24 * 5];
    iVar5 = *(int *)(*(int *)(DAT_00063dfc + 0xc) + local_28 * 4);
    fVar7 = (float10)(float)(&DAT_0005d0c4)[local_24 * 5];
    *(undefined4 *)((int)piVar6 + -0x10) = 0x3e083;
    FUN_00041f7e(local_24 * 0x14,(iVar4 - iVar5) - DAT_000639dc,(iVar2 - iVar3) - DAT_000639e0,uVar1
                );
    local_2c = (int)ROUND(fVar7);
    *(undefined4 *)((int)piVar6 + -0x10) = 0x3e091;
    FUN_0003bb42(local_20,extraout_EDX + local_2c);
    piVar6 = (int *)((int)piVar6 + -0xc);
  }
  return;
}


