// func #350  FUN_00045bb7  00045bb7  size=194


void FUN_00045bb7(uint param_1,undefined4 param_2)

{
  code *pcVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  uint extraout_EDX;
  uint uVar5;
  undefined1 *puVar6;
  uint *puVar7;
  uint *puVar8;
  uint uVar9;
  undefined2 in_DS;
  byte bVar10;
  undefined8 uVar11;
  uint local_20;
  undefined1 auStack_18 [4];
  
  puVar6 = &stack0xffffffe4;
  puVar8 = &local_20;
  local_20 = param_1;
  if (DAT_00053398 <= param_1) {
    if (DAT_000533b2 == 0) {
      uVar9 = param_1 + 0xf >> 4;
      if (uVar9 == 0) {
        uVar9 = 0xfffffff;
      }
      pcVar1 = (code *)swi(0x21);
      uVar11 = (*pcVar1)(CONCAT11(0xed,(char)in_DS),param_2,in_DS);
      uVar5 = (uint)((ulonglong)uVar11 >> 0x20);
      iVar2 = (int)uVar11 << 0x1f;
      bVar10 = 0;
      if (iVar2 != 0) {
        pcVar1 = (code *)swi(0x21);
        (*pcVar1)(CONCAT22((short)((uint)iVar2 >> 0x10),0x4a00),uVar5,uVar9);
        puVar6 = auStack_18;
        puVar8 = (uint *)auStack_18;
        uVar5 = extraout_EDX;
        if ((bVar10 & 1) != 0) goto LAB_00045c1e;
      }
      bVar10 = false;
      puVar7 = (uint *)puVar6;
    }
    else {
      uVar9 = param_1 + 0xfff >> 0xc;
      if (uVar9 == 0) {
        uVar9 = 0xfffff;
      }
      uVar5 = (uint)DAT_000533b2;
      bVar10 = DAT_000533b2 == 0;
      puVar7 = &local_20;
      if ((DAT_000533b2 == 1) && (bVar10 = 1 < DAT_000533b3, puVar7 = &local_20, DAT_000533b3 == 1))
      {
        uVar9 = uVar9 << 8;
        bVar10 = false;
        puVar7 = &local_20;
      }
    }
    pcVar1 = (code *)swi(0x21);
    (*pcVar1)(CONCAT11(0x4a,(char)in_DS),uVar5,uVar9);
    puVar8 = (uint *)((int)puVar7 + 4);
    if ((bVar10 & 1) == 0) {
      uVar4 = DAT_00053384;
      DAT_00053384 = *(undefined4 *)((int)puVar7 + 4);
      goto LAB_00045c6f;
    }
  }
LAB_00045c1e:
  *(undefined4 *)((int)puVar8 + -4) = 0x45c23;
  puVar3 = (undefined4 *)FUN_00045a6a();
  *puVar3 = 5;
  uVar4 = 0xffffffff;
LAB_00045c6f:
  FUN_00045bb1(uVar4);
  return;
}


