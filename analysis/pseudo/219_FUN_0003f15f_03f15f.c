// func #174  FUN_0003f15f  0003f15f  size=607


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0003f15f(void)

{
  code *pcVar1;
  ushort uVar2;
  undefined2 extraout_var;
  int iVar3;
  undefined2 extraout_var_00;
  undefined2 extraout_DX;
  undefined2 extraout_DX_00;
  undefined4 extraout_EDX;
  undefined4 extraout_EDX_00;
  undefined2 extraout_var_01;
  undefined2 extraout_var_02;
  undefined2 uVar4;
  undefined4 extraout_EDX_01;
  undefined4 local_50;
  undefined2 local_4c;
  undefined4 local_48;
  char *pcStack_44;
  uint local_3c;
  undefined2 local_34 [6];
  undefined4 local_28;
  undefined2 *local_24;
  undefined2 *local_20;
  
  FUN_000418ee();
  FUN_00041df2(local_34);
  pcVar1 = (code *)swi(0x31);
  uVar2 = (*pcVar1)(CONCAT22(extraout_var,0x100),extraout_EDX_00,0x21);
  *(undefined2 *)((uint)uVar2 * 0x10) = extraout_DX;
  local_24 = (undefined2 *)((uint)uVar2 * 0x10) + 1;
  local_34[0] = (undefined2)((uint)local_24 >> 4);
  local_3c = (uint)local_24 & 0xf;
  local_50 = (char *)0x3f1ce;
  FUN_000103e0(0x10,&local_50,&local_50,local_34);
  local_50 = (char *)0x3f1e0;
  iVar3 = FUN_000429a8(local_24,&DAT_00051728,4);
  uVar4 = extraout_var_01;
  if (iVar3 != 0) {
    local_50 = (char *)0x3f1f6;
    iVar3 = FUN_000429a8(local_24,&DAT_0005172d,4);
    uVar4 = extraout_var_02;
    if (iVar3 != 0) {
      local_50 = "\nYour Vga Card Not Support VesaMode!!!\n";
      FUN_00042394();
      local_28 = 0;
      goto LAB_0003f3b2;
    }
  }
  pcVar1 = (code *)swi(0x31);
  (*pcVar1)(0x101,CONCAT22(uVar4,local_24[-1]));
  pcVar1 = (code *)swi(0x31);
  uVar2 = (*pcVar1)(CONCAT22(extraout_var_00,0x100),extraout_EDX_01,0x11);
  *(undefined2 *)((uint)uVar2 * 0x10) = extraout_DX_00;
  local_20 = (undefined2 *)((uint)uVar2 * 0x10) + 1;
  local_50._0_2_ = 0x4f01;
  local_34[0] = (undefined2)((uint)local_20 >> 4);
  local_3c = (uint)local_20 & 0xf;
  local_48 = 0x3f27a;
  FUN_000103e0(0x10,&local_50,&local_50,local_34);
  if ((*(byte *)(local_20 + 1) & 3) == 3) {
    _DAT_00063e58 = 0;
  }
  if ((*(byte *)(local_20 + 1) & 5) == 5) {
    DAT_00063e5c = 0;
  }
  if ((*(byte *)((int)local_20 + 3) & 3) == 3) {
    _DAT_00063e58 = 1;
  }
  if ((*(byte *)((int)local_20 + 3) & 5) == 5) {
    DAT_00063e5c = 1;
  }
  _DAT_00063e4c = 0;
  while( true ) {
    iVar3 = 0x40 >> (DAT_00063e4c & 0x1f);
    if ((iVar3 == (short)local_20[2]) || (0xd < _DAT_00063e4c)) break;
    _DAT_00063e4c = _DAT_00063e4c + 1;
  }
  _DAT_00063e60 = (int)(short)local_20[8];
  DAT_00063e64 = (int)(short)local_20[9];
  DAT_00063e68 = (int)(short)local_20[10];
  DAT_000531f4 = (short)local_20[3] * 0x400;
  _DAT_000531f8 = DAT_000531f4 + -1;
  pcVar1 = (code *)swi(0x31);
  (*pcVar1)(CONCAT22((short)((uint)_DAT_000531f8 >> 0x10),0x101),
            CONCAT22((short)((uint)iVar3 >> 0x10),local_20[-1]));
  local_50 = (char *)CONCAT22(local_50._2_2_,0x4f02);
  local_4c = 0x101;
  pcStack_44 = (char *)0x3f38a;
  FUN_00041e51(0x10,&local_50,&local_50);
  if (local_50._1_1_ == '\x01') {
    pcStack_44 = "\nMode Setting Error!!!\n";
    local_48 = 0x3f39f;
    FUN_00042394();
    local_28 = 0;
  }
  else {
    local_28 = 1;
  }
LAB_0003f3b2:
  return CONCAT44(extraout_EDX,local_28);
}


