// func #193  FUN_0004052c  0004052c  size=66


void FUN_0004052c(void)

{
  uint uVar1;
  int iVar2;
  undefined2 *puVar3;
  int in_stack_00000004;
  int in_stack_00000008;
  int in_stack_0000000c;
  uint in_stack_00000010;
  int in_stack_00000014;
  undefined1 in_stack_00000018;
  
  puVar3 = (undefined2 *)(in_stack_00000004 + in_stack_0000000c * DAT_00040010 + in_stack_00000008);
  iVar2 = DAT_00040010 - in_stack_00000010;
  do {
    uVar1 = in_stack_00000010 >> 1;
    if ((in_stack_00000010 & 1) != 0) {
      *(undefined1 *)puVar3 = in_stack_00000018;
      puVar3 = (undefined2 *)((int)puVar3 + 1);
    }
    for (; uVar1 != 0; uVar1 = uVar1 - 1) {
      *puVar3 = CONCAT11(in_stack_00000018,in_stack_00000018);
      puVar3 = puVar3 + 1;
    }
    puVar3 = (undefined2 *)((int)puVar3 + iVar2);
    in_stack_00000014 = in_stack_00000014 + -1;
  } while (in_stack_00000014 != 0);
  return;
}


