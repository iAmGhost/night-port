// func #468  FUN_000404db  000404db  size=42

void FUN_000404db(void)

{
  int iVar1;
  undefined2 *puVar2;
  int in_stack_00000004;
  int in_stack_00000008;
  int in_stack_0000000c;
  int in_stack_00000010;
  uint in_stack_00000014;
  undefined1 in_stack_00000018;
  
  puVar2 = (undefined2 *)
           (in_stack_00000004 + in_stack_00000010 * in_stack_00000008 + in_stack_0000000c);
  iVar1 = CONCAT22((short)(in_stack_00000014 >> 0x10),(ushort)in_stack_00000014 >> 1);
  if ((in_stack_00000014 & 1) != 0) {
    *(undefined1 *)puVar2 = in_stack_00000018;
    puVar2 = (undefined2 *)((int)puVar2 + 1);
  }
  for (; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = CONCAT11(in_stack_00000018,in_stack_00000018);
    puVar2 = puVar2 + 1;
  }
  return;
}
