// func #381  FUN_00048666  00048666  size=167


void FUN_00048666(int *param_1)

{
  byte bVar1;
  byte bVar3;
  ushort uVar2;
  int unaff_EBP;
  int unaff_EDI;
  bool bVar4;
  
  uVar2 = *(ushort *)(&DAT_00045e22 + unaff_EDI);
  if ((uVar2 & *(ushort *)(unaff_EBP + 8)) == uVar2) {
    bVar1 = 0x41;
    bVar3 = 0;
  }
  else {
    *(ushort *)(unaff_EBP + 8) = *(ushort *)(unaff_EBP + 8) & ~uVar2;
    bVar3 = ((short)*(ushort *)(param_1 + 2) < 0) << 1;
    uVar2 = *(ushort *)(param_1 + 2) & 0x7fff;
    if (uVar2 == 0) {
      bVar1 = 0x44;
      if (param_1[1] == 0 && *param_1 == 0) {
        *(ushort *)(unaff_EBP + 8) =
             *(ushort *)(unaff_EBP + 8) | *(ushort *)(&DAT_00045e1c + unaff_EDI);
        bVar1 = 0x40;
      }
    }
    else if (uVar2 == 0x7fff) {
      *(ushort *)(unaff_EBP + 8) =
           *(ushort *)(unaff_EBP + 8) | *(ushort *)(&DAT_00045e20 + unaff_EDI);
      bVar4 = false;
      if (*param_1 == 0) {
        bVar4 = param_1[1] == -0x80000000;
      }
      bVar1 = 1;
      if (bVar4) {
        bVar1 = 5;
      }
    }
    else {
      bVar1 = 4;
      if ((*(byte *)((int)param_1 + 7) & 0x80) == 0) {
        *(ushort *)(unaff_EBP + 8) =
             *(ushort *)(unaff_EBP + 8) | *(ushort *)(&DAT_00045e20 + unaff_EDI);
        bVar1 = 0x44;
      }
    }
  }
  uVar2 = *(ushort *)(unaff_EBP + 4) & 0xb8ff;
  *(ushort *)(unaff_EBP + 4) = CONCAT11((byte)(uVar2 >> 8) | bVar1 | bVar3,(char)uVar2);
  return;
}


