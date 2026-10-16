// func #189  FUN_00040475  00040475  size=28


void FUN_00040475(void)

{
  undefined1 *in_stack_00000004;
  undefined1 *in_stack_00000008;
  int in_stack_0000000c;
  
  do {
    *in_stack_00000004 = *in_stack_00000008;
    in_stack_00000008 = in_stack_00000008 + 1;
    in_stack_00000004 = in_stack_00000004 + -1;
    in_stack_0000000c = in_stack_0000000c + -1;
  } while (in_stack_0000000c != 0);
  return;
}


