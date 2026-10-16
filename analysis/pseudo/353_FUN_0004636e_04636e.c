// func #353  FUN_0004636e  0004636e  size=82


void FUN_0004636e(int param_1)

{
  ushort uVar1;
  int unaff_EBP;
  int unaff_EDI;
  
  uVar1 = *(ushort *)(unaff_EBP + 4) & 0xb8ff;
  *(ushort *)(unaff_EBP + 4) = CONCAT11((byte)(uVar1 >> 8) | (&DAT_00045e6b)[param_1],(char)uVar1);
  *(ushort *)(unaff_EBP + 8) = *(ushort *)(unaff_EBP + 8) & *(ushort *)(&DAT_00045e1a + unaff_EDI);
  *(ushort *)(unaff_EBP + 8) = *(ushort *)(unaff_EBP + 8) | *(ushort *)(&DAT_00045e22 + unaff_EDI);
  uVar1 = *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + unaff_EDI));
  *(ushort *)(unaff_EBP + 4) = *(ushort *)(unaff_EBP + 4) & 0xc7ff;
  *(ushort *)(unaff_EBP + 4) = *(ushort *)(unaff_EBP + 4) | uVar1;
  FUN_000461cc();
  return;
}


