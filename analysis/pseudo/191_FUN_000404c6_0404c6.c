// func #191  FUN_000404c6  000404c6  size=21


void FUN_000404c6(void)

{
  undefined4 *in_stack_00000004;
  int in_stack_00000008;
  undefined4 *in_stack_0000000c;
  
  for (; in_stack_00000008 != 0; in_stack_00000008 = in_stack_00000008 + -1) {
    *in_stack_0000000c = *in_stack_00000004;
    in_stack_00000004 = in_stack_00000004 + 1;
    in_stack_0000000c = in_stack_0000000c + 1;
  }
  return;
}


