// func #430  FUN_00034c3e  00034c3e  size=106

undefined8 FUN_00034c3e(char *param_1)

{
  uint uVar1;
  undefined1 *puVar2;
  byte in_AF;
  byte in_TF;
  byte in_IF;
  byte bVar3;
  byte in_NT;
  byte in_AC;
  byte in_VIF;
  byte in_VIP;
  byte in_ID;
  undefined1 auStack_38 [4];
  uint uStack_34;
  undefined1 auStack_30 [16];
  
  *param_1 = *param_1 + (char)param_1;
  puVar2 = auStack_30;
  bVar3 = 0;
  uStack_34 = 0x34c55;
  FUN_00042310();
  uStack_34 = 0x34c5a;
  FUN_0004231b();
  uStack_34 = 0x34c5f;
  FUN_00034e3c();
  DAT_0006330c = DAT_0006330c + 1;
  uVar1 = DAT_0006332c - 0x10000;
  if (0xffff < (int)DAT_0006332c) {
    uStack_34 = (uint)(in_NT & 1) * 0x4000 | (uint)SBORROW4(DAT_0006332c,0x10000) * 0x800 |
                (uint)(bVar3 & 1) * 0x400 | (uint)(in_IF & 1) * 0x200 | (uint)(in_TF & 1) * 0x100 |
                (uint)((int)uVar1 < 0) * 0x80 | (uint)(uVar1 == 0) * 0x40 | (uint)(in_AF & 1) * 0x10
                | (uint)((POPCOUNT(uVar1 & 0xff) & 1U) == 0) * 4 | (uint)(DAT_0006332c < 0x10000) |
                (uint)(in_ID & 1) * 0x200000 | (uint)(in_VIP & 1) * 0x100000 |
                (uint)(in_VIF & 1) * 0x80000 | (uint)(in_AC & 1) * 0x40000;
    puVar2 = auStack_38;
    (*(code *)&SUB_00063334)();
    DAT_0006332c = DAT_0006332c - 0x10000;
  }
  DAT_0006332c = DAT_0006332c + DAT_00063318;
  *(undefined4 *)(puVar2 + -4) = 0x34c9c;
  FUN_0004231d(0x20,0x20);
  *(undefined4 *)(puVar2 + -4) = 0x34ca1;
  FUN_0004232b();
  return CONCAT44(*(undefined4 *)(puVar2 + 0x24),*(undefined4 *)(puVar2 + 0x2c));
}
