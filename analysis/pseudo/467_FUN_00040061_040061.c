// func #467  FUN_00040061  00040061  size=32

undefined4 FUN_00040061(void)

{
  undefined1 uVar1;
  undefined4 in_stack_00000004;
  int in_stack_00000008;
  undefined1 *in_stack_0000000c;
  
  out(0x3c7,(char)in_stack_00000004);
  in_stack_00000008 = in_stack_00000008 * 3;
  while (in_stack_00000008 != 0) {
    in_stack_00000008 = in_stack_00000008 + -1;
    uVar1 = in(0x3c9);
    *in_stack_0000000c = uVar1;
    in_stack_0000000c = in_stack_0000000c + 1;
  }
  return in_stack_00000004;
}
