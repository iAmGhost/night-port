// func #184  FUN_0003fb73  0003fb73  size=170


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0003fb73(void)

{
  byte *pbVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  undefined2 extraout_var;
  undefined4 extraout_ECX;
  undefined4 uVar5;
  int iVar6;
  uint uVar7;
  byte bVar8;
  byte *pbVar9;
  byte *pbVar10;
  byte *pbVar11;
  undefined8 uVar12;
  
  uVar12 = FUN_000418ee();
  uVar5 = (undefined4)((ulonglong)uVar12 >> 0x20);
  DAT_00063e7c = (short *)FUN_00041af8(uVar5);
  FUN_0004192e(DAT_00063e7c,uVar5,1,(int)uVar12);
  pbVar10 = (byte *)(DAT_00051a0c + CONCAT22(extraout_var,*DAT_00063e7c * 0x140));
  pbVar9 = (byte *)(DAT_00063e7c + 2);
  _DAT_00063e78 = CONCAT22(extraout_var,DAT_00063e7c[1]);
  uVar5 = extraout_ECX;
  do {
    pbVar11 = pbVar10;
    for (bVar8 = *pbVar9; pbVar9 = pbVar9 + 1, bVar8 != 0; bVar8 = bVar8 - 1) {
      while( true ) {
        pbVar1 = pbVar9 + 1;
        bVar2 = *pbVar9;
        pbVar9 = pbVar9 + 2;
        bVar3 = *pbVar1;
        if ((char)bVar3 < '\0') break;
        pbVar11 = pbVar11 + bVar2;
        for (iVar6 = CONCAT22((short)((uint)uVar5 >> 0x10),(ushort)bVar3); iVar6 != 0;
            iVar6 = iVar6 + -1) {
          *pbVar11 = *pbVar9;
          pbVar9 = pbVar9 + 1;
          pbVar11 = pbVar11 + 1;
        }
        bVar8 = bVar8 - 1;
        uVar5 = 0;
        if (bVar8 == 0) goto LAB_0003fbfd;
      }
      bVar4 = *pbVar9;
      pbVar11 = pbVar11 + bVar2;
      for (uVar7 = (uint)(byte)-bVar3; uVar7 != 0; uVar7 = uVar7 - 1) {
        *pbVar11 = bVar4;
        pbVar11 = pbVar11 + 1;
      }
      uVar5 = 0;
    }
LAB_0003fbfd:
    pbVar10 = pbVar10 + 0x140;
    _DAT_00063e78 = _DAT_00063e78 + -1;
    if (_DAT_00063e78 == 0) {
      FUN_00041c31(DAT_00063e7c);
      return;
    }
  } while( true );
}


