// func #188  FUN_00040042  00040042  size=31


undefined6 FUN_00040042(void)

{
  undefined4 in_stack_00000004;
  int in_stack_00000008;
  undefined1 *in_stack_0000000c;
  
  out(0x3c8,(char)in_stack_00000004);
  in_stack_00000008 = in_stack_00000008 * 3;
  while (in_stack_00000008 != 0) {
    in_stack_00000008 = in_stack_00000008 + -1;
    out(*in_stack_0000000c,0x3c9);
    in_stack_0000000c = in_stack_0000000c + 1;
  }
  return CONCAT24(0x3c9,in_stack_00000004);
}


