// func #331  thunk_FUN_00049415  00044c1e  size=5  [thunk]


longlong thunk_FUN_00049415(undefined4 param_1)

{
  code *pcVar1;
  int iVar2;
  undefined4 uVar3;
  byte in_CF;
  uint in_stack_00000000;
  
  pcVar1 = (code *)swi(0x21);
  iVar2 = (*pcVar1)(CONCAT11(0x41,(char)param_1),param_1);
  if ((in_CF & 1) != 0) {
    uVar3 = FUN_00045a82((iVar2 << 1 | (uint)in_CF) >> 1 & 0xffff);
    return CONCAT44(in_stack_00000000,uVar3);
  }
  return (ulonglong)in_stack_00000000 << 0x20;
}


