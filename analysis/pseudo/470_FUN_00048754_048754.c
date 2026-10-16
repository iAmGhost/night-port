// func #382  FUN_00048754  00048754  size=25


void FUN_00048754(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 extraout_ECX;
  undefined8 *extraout_EDX;
  undefined8 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  
  uVar5 = 0x3fff;
  uVar4 = 0x80000000;
  uVar3 = 0;
  FUN_0004791a(param_1,param_2,param_1,param_4,0,0x80000000,0x3fff,param_1,param_2,param_3);
  uVar2 = extraout_ECX;
  FUN_000487b0();
  FUN_00047d24(uVar3,extraout_EDX,extraout_EDX);
  uVar1 = FUN_00047d4e(0x5c17f0bd,0xb8aa3b29,*(undefined4 *)extraout_EDX,
                       *(undefined4 *)((int)extraout_EDX + 4),uVar5,uVar2,uVar4);
  *extraout_EDX = uVar1;
  *(undefined2 *)(extraout_EDX + 1) = 0x3fff;
  return;
}


