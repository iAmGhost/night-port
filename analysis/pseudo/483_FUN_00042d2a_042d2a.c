// func #483  FUN_00042d2a  00042d2a  size=258

uint * FUN_00042d2a(int *param_1,int param_2)

{
  undefined2 uVar1;
  uint uVar2;
  code *pcVar3;
  uint3 uVar4;
  undefined2 *puVar5;
  uint *puVar6;
  undefined2 extraout_CX;
  int *extraout_EDX;
  int iVar7;
  int iVar8;
  int extraout_EDX_00;
  int *piVar9;
  undefined2 uVar10;
  uint uVar11;
  char cVar12;
  
  FUN_00042ccc();
  piVar9 = DAT_00053350;
  while( true ) {
    if (piVar9 == (int *)0x0) {
      return (uint *)0x0;
    }
    if (param_1 < piVar9) {
      return (uint *)0x0;
    }
    if ((piVar9 + 0xb == param_1) && (piVar9[6] == 1)) break;
    piVar9 = (int *)piVar9[2];
  }
  FUN_00042c8d(piVar9);
  if (extraout_EDX[-1] != 0) {
    return (uint *)0x0;
  }
  iVar7 = *extraout_EDX + (param_2 - (*param_1 + -4)) + 0x100b;
  uVar4 = (uint3)((uint)iVar7 >> 8);
  iVar8 = (uint)(uVar4 & 0xfffff0) << 8;
  uVar10 = (undefined2)((uint)iVar7 >> 0x10);
  cVar12 = (uVar4 >> 7 & 1) != 0;
  iVar7 = extraout_EDX[-2];
  uVar1 = *(undefined2 *)((int)extraout_EDX + -6);
  pcVar3 = (code *)swi(0x31);
  (*pcVar3)(CONCAT31((int3)(CONCAT22((short)((uint)(extraout_EDX + -2) >> 0x10),0x500) >> 8),3),
            iVar8,uVar4 >> 8,iVar8);
  puVar5 = (undefined2 *)0x0;
  if (cVar12 != '\x01') {
    puVar5 = (undefined2 *)CONCAT22(uVar10,extraout_CX);
    *puVar5 = (short)iVar7;
    puVar5[1] = uVar1;
  }
  if (puVar5 != (undefined2 *)0x0) {
    piVar9 = (int *)(puVar5 + 4);
    *(undefined4 *)(puVar5 + 2) = 0;
    *piVar9 = extraout_EDX_00 + -0xc;
    puVar6 = (uint *)FUN_00042e2d(piVar9);
    piVar9[6] = 1;
    uVar11 = param_2 + 1U & 0xfffffffe;
    uVar2 = *puVar6;
    if (uVar2 - uVar11 < 0xc) {
      *(byte *)puVar6 = (byte)*puVar6 | 1;
      return puVar6;
    }
    *puVar6 = uVar11 | 1;
    *(uint *)((int)puVar6 + uVar11) = uVar2 - uVar11 | 1;
    piVar9[5] = -1;
    piVar9[6] = piVar9[6] + 1;
    FUN_00041c31((uint *)((int)puVar6 + uVar11) + 1);
    return puVar6;
  }
  return (uint *)0x0;
}
