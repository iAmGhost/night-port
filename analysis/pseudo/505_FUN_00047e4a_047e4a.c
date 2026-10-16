// func #505  FUN_00047e4a  00047e4a  size=4

int FUN_00047e4a(uint *param_1)

{
  byte bVar1;
  uint uVar2;
  int iVar3;
  byte bVar4;
  ushort uVar5;
  short sVar6;
  uint uVar7;
  bool bVar8;
  
  uVar5 = (ushort)param_1[2] & 0x7fff;
  sVar6 = uVar5 + 0xc002;
  if (uVar5 < 0x3ffe) {
    return 0;
  }
  if (sVar6 < 0x21) {
    uVar7 = CONCAT11(*(byte *)((int)param_1 + 9) >> 1 | 0xa0,0xa0) & 0xffffff3f;
    bVar4 = (byte)sVar6;
    if ((char)bVar4 <= (char)uVar7) {
      uVar2 = param_1[1];
      bVar1 = (byte)(CONCAT11(CARRY4(*param_1,*param_1),(char)(uVar7 >> 8)) >> 1);
      if (bVar4 == 0x20) {
        bVar8 = CARRY1(bVar1,bVar1);
        uVar7 = uVar2;
      }
      else {
        uVar7 = 0 << (bVar4 & 0x1f) | uVar2 >> 0x20 - (bVar4 & 0x1f);
        uVar2 = uVar2 << (bVar4 & 0x1f);
        bVar8 = CARRY4(uVar2,uVar2);
      }
      bVar4 = (byte)(CONCAT11(bVar8,0xff) >> 1) & bVar1 * '\x02';
      iVar3 = uVar7 + CARRY1(bVar4,bVar4);
      if (CARRY1(bVar4 * '\x02',bVar4 * '\x02')) {
        iVar3 = -iVar3;
      }
      return iVar3;
    }
  }
  return -0x80000000;
}
