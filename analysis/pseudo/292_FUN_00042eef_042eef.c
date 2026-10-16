// func #292  FUN_00042eef  00042eef  size=211


undefined8 FUN_00042eef(uint param_1,undefined4 param_2,int param_3,undefined4 param_4)

{
  code *pcVar1;
  bool bVar2;
  undefined2 uVar3;
  undefined2 *puVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  int *piVar8;
  undefined2 extraout_CX;
  undefined2 extraout_CX_00;
  undefined2 extraout_DX;
  undefined4 extraout_EDX;
  undefined4 extraout_EDX_00;
  int *piVar9;
  uint unaff_ESI;
  undefined *unaff_EDI;
  char cVar10;
  byte bVar11;
  undefined8 uVar12;
  undefined4 uStack_8;
  int local_4;
  
  uStack_8 = param_4;
  local_4 = param_3;
  FUN_00042ccc();
  uVar3 = (undefined2)(param_1 >> 0x10);
  cVar10 = (param_1 >> 0xf & 1) != 0;
  pcVar1 = (code *)swi(0x31);
  (*pcVar1)(0x501,extraout_EDX,param_1 >> 0x10,param_1);
  piVar9 = (int *)&stack0xffffffec;
  puVar4 = (undefined2 *)0x0;
  if (cVar10 != '\x01') {
    puVar4 = (undefined2 *)CONCAT22(uVar3,extraout_CX);
    *puVar4 = (short)unaff_EDI;
    puVar4[1] = (short)unaff_ESI;
  }
  if (puVar4 == (undefined2 *)0x0) {
    if ((DAT_0006400c & 0xfff00000) != 0) {
      DAT_0006400c = 0xfffff;
    }
    piVar9 = (int *)&stack0xffffffec;
    if (unaff_EDI < &UNK_00010001) {
      bVar11 = 0;
      pcVar1 = (code *)swi(0x21);
      iVar5 = (*pcVar1)(0x4800,extraout_EDX_00,
                        CONCAT31((uint3)(DAT_0006400c >> 0xc),(char)(DAT_0006400c >> 4)) | 1);
      piVar9 = (int *)&stack0xfffffff0;
      bVar2 = (bVar11 & 1) != 0;
      uVar7 = (iVar5 << 1 | (uint)bVar11) >> 1;
      uVar6 = uVar7 | (uint)bVar2 << 0x1f;
      if (!bVar2) {
        bVar11 = (unaff_ESI >> 3 & 1) != 0;
        pcVar1 = (code *)swi(0x21);
        uVar12 = (*pcVar1)(CONCAT22((short)(uVar6 >> 0x10),CONCAT11(0x48,(char)uVar7)),uVar6,
                           unaff_ESI >> 4);
        bVar2 = (bVar11 & 1) != 0;
        uVar7 = ((int)uVar12 << 1 | (uint)bVar11) >> 1 | (uint)bVar2 << 0x1f;
        pcVar1 = (code *)swi(0x21);
        bVar11 = bVar2;
        iVar5 = (*pcVar1)(CONCAT11(0x49,(char)((ulonglong)uVar12 >> 0x20)),
                          (int)((ulonglong)uVar12 >> 0x20),uVar7);
        piVar9 = &uStack_8;
        if (!bVar2) {
          pcVar1 = (code *)swi(0x31);
          (*pcVar1)(CONCAT31((int3)(((uint)(ushort)((ushort)((uint)(iVar5 << 1) >> 0x11) |
                                                   (ushort)(((uint)((bVar11 & 1) != 0) << 0x1f) >>
                                                           0x10)) << 0x10) >> 8),6));
          piVar9 = &local_4;
          piVar8 = (int *)(CONCAT22(extraout_CX_00,extraout_DX) + 8);
          *(uint *)(CONCAT22(extraout_CX_00,extraout_DX) + 4) = uVar7;
          *piVar8 = local_4 + -8;
          goto LAB_00042fb9;
        }
      }
    }
    piVar8 = (int *)0x0;
  }
  else {
    piVar8 = (int *)(puVar4 + 4);
    *piVar8 = (int)(unaff_EDI + -8);
    *(undefined4 *)(puVar4 + 2) = 0;
  }
LAB_00042fb9:
  return CONCAT44(*(undefined4 *)((int)piVar9 + 0xc),piVar8);
}


