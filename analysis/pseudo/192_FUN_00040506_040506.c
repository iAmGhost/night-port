// func #192  FUN_00040506  00040506  size=38


void FUN_00040506(void)

{
  undefined1 *puVar1;
  int in_stack_00000004;
  int in_stack_00000008;
  int in_stack_0000000c;
  int in_stack_00000010;
  int in_stack_00000014;
  undefined1 in_stack_00000018;
  
  puVar1 = (undefined1 *)
           (in_stack_00000004 + in_stack_00000010 * in_stack_00000008 + in_stack_0000000c);
  do {
    *puVar1 = in_stack_00000018;
    puVar1 = puVar1 + in_stack_00000008;
    in_stack_00000014 = in_stack_00000014 + -1;
  } while (in_stack_00000014 != 0);
  return;
}


