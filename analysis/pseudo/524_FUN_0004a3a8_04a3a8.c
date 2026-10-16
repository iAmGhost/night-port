// func #524  FUN_0004a3a8  0004a3a8  size=230

undefined8 FUN_0004a3a8(undefined4 param_1,undefined4 param_2)

{
  short *psVar1;
  int iVar2;
  bool bVar3;
  int in_stack_0000000c;
  
  psVar1 = &DAT_000519bc;
  if (in_stack_0000000c < 0) {
    iVar2 = -in_stack_0000000c;
    bVar3 = in_stack_0000000c == -0x134;
    in_stack_0000000c = iVar2;
    if (bVar3 || iVar2 < 0x134) goto LAB_0004a447;
  }
  else {
    iVar2 = in_stack_0000000c;
    if (in_stack_0000000c < 0x135) goto LAB_0004a447;
  }
  in_stack_0000000c = iVar2 + -0xd8;
LAB_0004a447:
  while( true ) {
    if (*psVar1 <= in_stack_0000000c) {
      in_stack_0000000c = in_stack_0000000c - *psVar1;
    }
    if (in_stack_0000000c == 0) break;
    if (*psVar1 != 1) {
      psVar1 = psVar1 + 5;
    }
  }
  return CONCAT44(param_2,param_1);
}
