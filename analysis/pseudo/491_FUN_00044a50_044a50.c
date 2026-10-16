// func #491  FUN_00044a50  00044a50  size=100

undefined8 FUN_00044a50(undefined4 param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  code *pcVar1;
  bool bVar2;
  uint uVar3;
  undefined2 extraout_var;
  undefined2 extraout_var_00;
  undefined4 uVar4;
  undefined4 extraout_EDX;
  int extraout_EDX_00;
  int extraout_EDX_01;
  uint *puVar5;
  uint *puVar6;
  byte bVar7;
  byte bVar8;
  undefined1 auStack_14 [4];
  uint local_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  puVar6 = (uint *)auStack_14;
  local_c = param_2;
  uStack_8 = param_4;
  uStack_4 = param_3;
  uVar3 = FUN_000492d0();
  uVar3 = uVar3 | 0x40;
  FUN_00049325(param_1,uVar3);
  bVar7 = 0;
  if ((uVar3 & 0x2000) == 0) {
LAB_00044aab:
    uVar4 = 0;
  }
  else {
    pcVar1 = (code *)swi(0x21);
    (*pcVar1)(CONCAT22(extraout_var,0x4400),extraout_EDX,param_1);
    puVar5 = (uint *)&stack0xfffffff0;
    bVar2 = (bVar7 & 1) != 0;
    if (!bVar2) {
      bVar8 = 0;
      pcVar1 = (code *)swi(0x21);
      (*pcVar1)(CONCAT22(extraout_var_00,0x4401),
                CONCAT22((ushort)((uint)(extraout_EDX_00 << 1) >> 0x11) |
                         (ushort)(((uint)bVar2 << 0x1f) >> 0x10),
                         (ushort)(byte)((byte)((extraout_EDX_00 << 1 | (uint)bVar7) >> 1) | 0x20)),
                param_1);
      puVar6 = &local_c;
      puVar5 = &local_c;
      bVar2 = (bVar8 & 1) != 0;
      local_c = (extraout_EDX_01 << 1 | (uint)bVar8) >> 1 | (uint)bVar2 << 0x1f;
      if (!bVar2) goto LAB_00044aab;
    }
    *(undefined4 *)((int)puVar5 + -4) = 0x44a8e;
    uVar4 = FUN_00045a82(*(undefined2 *)puVar5);
    puVar6 = puVar5;
  }
  return CONCAT44(*(undefined4 *)((int)puVar6 + 8),uVar4);
}
