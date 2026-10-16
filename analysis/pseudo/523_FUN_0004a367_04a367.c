// func #523  FUN_0004a367  0004a367  size=64

void FUN_0004a367(int param_1)

{
  undefined4 extraout_ECX;
  undefined4 uVar1;
  undefined4 *puVar2;
  float10 in_ST0;
  undefined4 uStack00000004;
  undefined4 uStack00000008;
  int in_stack_0000000c;
  undefined4 in_stack_00000010;
  undefined4 uStack_20;
  undefined4 uStack_1c;
  int iStack_18;
  
  puVar2 = (undefined4 *)&stack0xffffffec;
  uVar1 = in_stack_00000010;
  if (param_1 != in_stack_0000000c) {
    iStack_18 = in_stack_0000000c - param_1;
    uStack_1c = uStack00000008;
    puVar2 = &uStack_20;
    uStack_20 = uStack00000004;
    FUN_0004a3a8();
    _uStack00000004 = (double)in_ST0;
  }
  *(undefined4 *)((int)puVar2 + -4) = 0x4a397;
  FUN_0004a84e(&stack0x00000004,uVar1);
  *(undefined4 *)((int)puVar2 + -4) = 0x4a3a0;
  FUN_0004a814(extraout_ECX,uVar1);
  return;
}
