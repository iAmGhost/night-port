// func #354  FUN_000463c0  000463c0  size=28


void FUN_000463c0(int param_1)

{
  ushort uVar1;
  int unaff_EBP;
  
  uVar1 = *(ushort *)(unaff_EBP + 4) & 0xb8ff;
  *(ushort *)(unaff_EBP + 4) = CONCAT11((byte)(uVar1 >> 8) | (&DAT_00045e6b)[param_1],(char)uVar1);
  FUN_000461cc();
  return;
}


