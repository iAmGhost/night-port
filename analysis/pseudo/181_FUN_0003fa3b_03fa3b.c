// func #181  FUN_0003fa3b  0003fa3b  size=163


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0003fa3b(void)

{
  byte *pbVar1;
  byte bVar2;
  uint uVar3;
  undefined4 uVar4;
  byte *pbVar5;
  byte bVar6;
  byte *pbVar7;
  byte *pbVar8;
  undefined8 uVar9;
  undefined4 uVar10;
  
  uVar9 = FUN_000418ee();
  uVar4 = (undefined4)((ulonglong)uVar9 >> 0x20);
  uVar10 = 0;
  DAT_00063e7c = (byte *)FUN_00041af8(uVar4);
  FUN_0004192e(DAT_00063e7c,uVar4,1,(int)uVar9,uVar10);
  _DAT_00063e78 = 200;
  pbVar5 = DAT_00051a0c;
  pbVar7 = DAT_00063e7c;
  do {
    pbVar8 = pbVar7 + 1;
    bVar6 = *pbVar7;
    pbVar7 = pbVar5;
    for (; bVar6 != 0; bVar6 = bVar6 - 1) {
      while( true ) {
        pbVar1 = pbVar8 + 1;
        bVar2 = *pbVar8;
        uVar3 = (uint)bVar2;
        if ((char)bVar2 < '\0') break;
        pbVar8 = pbVar8 + 2;
        bVar2 = *pbVar1;
        for (; uVar3 != 0; uVar3 = uVar3 - 1) {
          *pbVar7 = bVar2;
          pbVar7 = pbVar7 + 1;
        }
        bVar6 = bVar6 - 1;
        if (bVar6 == 0) goto LAB_0003fabf;
      }
      pbVar8 = pbVar1;
      for (uVar3 = (uint)(byte)-bVar2; uVar3 != 0; uVar3 = uVar3 - 1) {
        *pbVar7 = *pbVar8;
        pbVar8 = pbVar8 + 1;
        pbVar7 = pbVar7 + 1;
      }
    }
LAB_0003fabf:
    pbVar5 = (byte *)CONCAT22((short)((uint)pbVar5 >> 0x10),(short)pbVar5 + 0x140);
    _DAT_00063e78 = _DAT_00063e78 + -1;
    pbVar7 = pbVar8;
    if (_DAT_00063e78 == 0) {
      FUN_00041c31(DAT_00063e7c);
      return;
    }
  } while( true );
}


