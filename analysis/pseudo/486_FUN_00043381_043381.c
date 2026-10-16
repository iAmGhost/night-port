// func #486  FUN_00043381  00043381  size=76

undefined8 FUN_00043381(undefined4 param_1,undefined4 *param_2,undefined4 param_3)

{
  undefined4 extraout_ECX;
  undefined4 unaff_EBP;
  undefined4 unaff_ESI;
  undefined4 unaff_EDI;
  undefined2 in_ES;
  undefined2 in_DS;
  undefined2 in_FS;
  undefined2 in_GS;
  byte in_CF;
  byte in_PF;
  byte in_AF;
  byte in_ZF;
  byte in_SF;
  byte in_TF;
  byte in_IF;
  byte bVar1;
  byte in_OF;
  byte in_NT;
  undefined8 uVar2;
  
  bVar1 = 0;
  uVar2 = FUN_000433ce();
  *param_2 = (int)uVar2;
  *(ushort *)(param_2 + 9) =
       (ushort)(in_NT & 1) * 0x4000 | (ushort)(in_OF & 1) * 0x800 | (ushort)(bVar1 & 1) * 0x400 |
       (ushort)(in_IF & 1) * 0x200 | (ushort)(in_TF & 1) * 0x100 | (ushort)(in_SF & 1) * 0x80 |
       (ushort)(in_ZF & 1) * 0x40 | (ushort)(in_AF & 1) * 0x10 | (ushort)(in_PF & 1) * 4 |
       (ushort)(in_CF & 1);
  param_2[2] = extraout_ECX;
  param_2[3] = (int)((ulonglong)uVar2 >> 0x20);
  param_2[5] = unaff_ESI;
  param_2[6] = unaff_EDI;
  param_2[4] = unaff_EBP;
  *(undefined2 *)(param_2 + 7) = in_DS;
  param_2[1] = param_3;
  *(undefined2 *)((int)param_2 + 0x1e) = in_ES;
  *(undefined2 *)(param_2 + 8) = in_FS;
  *(undefined2 *)((int)param_2 + 0x22) = in_GS;
  return CONCAT44(param_2,param_1);
}
