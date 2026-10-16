// func #190  FUN_00040491  00040491  size=53


void FUN_00040491(void)

{
  uint uVar1;
  undefined2 *in_stack_00000004;
  int in_stack_00000008;
  uint in_stack_0000000c;
  int in_stack_00000010;
  undefined2 *in_stack_00000014;
  int in_stack_00000018;
  
  do {
    uVar1 = in_stack_0000000c >> 1;
    if ((in_stack_0000000c & 1) != 0) {
      *(undefined1 *)in_stack_00000014 = *(undefined1 *)in_stack_00000004;
      in_stack_00000004 = (undefined2 *)((int)in_stack_00000004 + 1);
      in_stack_00000014 = (undefined2 *)((int)in_stack_00000014 + 1);
    }
    for (; uVar1 != 0; uVar1 = uVar1 - 1) {
      *in_stack_00000014 = *in_stack_00000004;
      in_stack_00000004 = in_stack_00000004 + 1;
      in_stack_00000014 = in_stack_00000014 + 1;
    }
    in_stack_00000004 =
         (undefined2 *)((int)in_stack_00000004 + (in_stack_00000008 - in_stack_0000000c));
    in_stack_00000014 =
         (undefined2 *)((int)in_stack_00000014 + (in_stack_00000018 - in_stack_0000000c));
    in_stack_00000010 = in_stack_00000010 + -1;
  } while (in_stack_00000010 != 0);
  return;
}


