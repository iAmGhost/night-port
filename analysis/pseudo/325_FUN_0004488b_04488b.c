// func #325  FUN_0004488b  0004488b  size=453


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0004488b(void)

{
  code *pcVar1;
  bool bVar2;
  int iVar3;
  uint uVar4;
  uint *puVar5;
  uint *puVar6;
  uint uVar7;
  byte bVar8;
  ulonglong uVar9;
  undefined8 uVar10;
  char *in_stack_00000004;
  byte in_stack_00000008;
  byte bStack0000000c;
  byte local_20 [4];
  byte abStack_1c [4];
  uint local_18;
  
  for (; *in_stack_00000004 == ' '; in_stack_00000004 = in_stack_00000004 + 1) {
  }
  local_20[0] = in_stack_00000008 & 0x83;
  bVar8 = 0;
  uVar7 = 0xffffffff;
  pcVar1 = (code *)swi(0x21);
  iVar3 = (*pcVar1)(CONCAT11(0x3d,local_20[0] | bStack0000000c),in_stack_00000004);
  bVar2 = (bVar8 & 1) != 0;
  uVar4 = (iVar3 << 1 | (uint)bVar8) >> 1;
  local_18 = uVar4 | (uint)bVar2 << 0x1f;
  if (!bVar2) {
    uVar7 = uVar4 & 0xffff;
  }
  puVar5 = (uint *)abStack_1c;
  if (((_bStack0000000c & 3) == 0) || (puVar5 = (uint *)abStack_1c, uVar7 == 0xffffffff)) {
LAB_0004494c:
    puVar6 = puVar5;
    if (uVar7 != 0xffffffff) {
LAB_000449da:
      *(byte *)((int)puVar6 + -0xffffffff00000004) = 0xe1;
      *(byte *)((int)puVar6 + -0xffffffff00000003) = 0x49;
      *(byte *)((int)puVar6 + -0xffffffff00000002) = 4;
      *(byte *)((int)puVar6 + -0xffffffff00000001) = 0;
      uVar4 = FUN_000492d0(uVar7);
      *(byte *)((int)puVar6 + -0xffffffff00000004) = 0xec;
      *(byte *)((int)puVar6 + -0xffffffff00000003) = 0x49;
      *(byte *)((int)puVar6 + -0xffffffff00000002) = 4;
      *(byte *)((int)puVar6 + -0xffffffff00000001) = 0;
      uVar10 = FUN_000492b2(uVar7,uVar4 & 0xffffff3c);
      uVar4 = (uint)((ulonglong)uVar10 >> 0x20);
      if ((int)uVar10 != 0) {
        uVar4 = uVar4 | 0x2000;
      }
      *(byte *)puVar6 = *(byte *)puVar6 & 0x7f;
      if (*puVar6 == 2) {
        uVar4 = uVar4 | 3;
      }
      if (*puVar6 == 0) {
        uVar4 = uVar4 | 1;
      }
      if (*puVar6 == 1) {
        uVar4 = uVar4 | 2;
      }
      if ((*(byte *)((int)puVar6 + 0x28) & 0x10) != 0) {
        uVar4 = uVar4 | 0x80;
      }
      if ((*(byte *)((int)puVar6 + 0x29) & 3) == 0) {
        if (DAT_00053619 == 0x200) goto LAB_00044a3c;
      }
      else if ((*(byte *)((int)puVar6 + 0x29) & 2) != 0) {
LAB_00044a3c:
        uVar4 = uVar4 | 0x40;
      }
      *(byte *)((int)puVar6 + -0xffffffff00000004) = 0x45;
      *(byte *)((int)puVar6 + -0xffffffff00000003) = 0x4a;
      *(byte *)((int)puVar6 + -0xffffffff00000002) = 4;
      *(byte *)((int)puVar6 + -0xffffffff00000001) = 0;
      FUN_00049325(uVar7,uVar4);
      goto LAB_00044a47;
    }
    if (((*(byte *)((int)puVar5 + 0x28) & 0x20) != 0) && (*(short *)((int)puVar5 + 4) == 2)) {
      uVar4 = *(uint *)((int)puVar5 + 0x30);
      if (uVar4 == 0) {
        uVar4 = 0x180;
      }
      bVar8 = 0;
      pcVar1 = (code *)swi(0x21);
      uVar10 = (*pcVar1)(CONCAT22((short)((uVar4 ^ _DAT_000536d8) >> 0x10),
                                  CONCAT11(0x3c,(char)(uVar4 ^ _DAT_000536d8))),in_stack_00000004);
      puVar6 = (uint *)((int)puVar5 + 4);
      bVar2 = (bVar8 & 1) != 0;
      uVar4 = ((int)uVar10 << 1 | (uint)bVar8) >> 1;
      uVar7 = uVar4 | (uint)bVar2 << 0x1f;
      *(uint *)((int)puVar5 + 8) = uVar7;
      bVar8 = 0;
      if (!bVar2) {
        pcVar1 = (code *)swi(0x21);
        iVar3 = (*pcVar1)(CONCAT22((short)(uVar7 >> 0x10),CONCAT11(0x3e,(char)uVar4)),
                          (int)((ulonglong)uVar10 >> 0x20),*(undefined4 *)((int)puVar5 + 8));
        puVar6 = (uint *)((int)puVar5 + 8);
        bVar2 = (bVar8 & 1) != 0;
        uVar4 = (iVar3 << 1 | (uint)bVar8) >> 1 | (uint)bVar2 << 0x1f;
        *(uint *)((int)puVar5 + 0xc) = uVar4;
        if (!bVar2) {
          bVar8 = 0;
          pcVar1 = (code *)swi(0x21);
          iVar3 = (*pcVar1)(CONCAT22((short)(uVar4 >> 0x10),
                                     CONCAT11(0x3d,*(byte *)((int)puVar5 + 8) |
                                                   *(byte *)((int)puVar5 + 0x34))),in_stack_00000004
                           );
          puVar6 = (uint *)((int)puVar5 + 0xc);
          bVar2 = (bVar8 & 1) != 0;
          *(uint *)((int)puVar5 + 0x10) = (iVar3 << 1 | (uint)bVar8) >> 1 | (uint)bVar2 << 0x1f;
          if (!bVar2) {
            uVar7 = (uint)*(ushort *)((int)puVar5 + 0x10);
            puVar6 = (uint *)((int)puVar5 + 0xc);
            goto LAB_000449da;
          }
        }
      }
    }
  }
  else {
    uVar9 = FUN_000492b2(uVar7);
    puVar5 = (uint *)abStack_1c;
    if ((int)uVar9 != 0) goto LAB_0004494c;
    if ((_bStack0000000c & 0x400) != 0) {
      pcVar1 = (code *)swi(0x21);
      (*pcVar1)(0x3e00,(int)(uVar9 >> 0x20),uVar7);
      puVar6 = &local_18;
      abStack_1c[0] = 10;
      abStack_1c[1] = 0x49;
      abStack_1c[2] = 4;
      abStack_1c[3] = 0;
      FUN_00042a0d(7);
      uVar7 = 0xffffffff;
      goto LAB_00044a47;
    }
    puVar5 = (uint *)abStack_1c;
    if ((uVar9 & 0x400000000000) == 0) goto LAB_0004494c;
    bVar8 = 0;
    pcVar1 = (code *)swi(0x21);
    uVar10 = (*pcVar1)(0x4000,&stack0xffffffec,uVar7,0);
    puVar5 = &local_18;
    uVar4 = (int)uVar10 << 1;
    bVar2 = (bVar8 & 1) != 0;
    if (!bVar2) goto LAB_0004494c;
    pcVar1 = (code *)swi(0x21);
    (*pcVar1)(CONCAT22((ushort)(uVar4 >> 0x11) | (ushort)(((uint)bVar2 << 0x1f) >> 0x10),
                       CONCAT11(0x3e,(char)((uVar4 | bVar8) >> 1))),(int)((ulonglong)uVar10 >> 0x20)
              ,uVar7);
    puVar6 = (uint *)&stack0xffffffec;
  }
  *(undefined4 *)((int)puVar6 + -4) = 0x44947;
  uVar7 = FUN_00045a82(*(undefined2 *)((int)puVar6 + 4));
LAB_00044a47:
  return CONCAT44(*(undefined4 *)((int)puVar6 + 0x14),uVar7);
}


