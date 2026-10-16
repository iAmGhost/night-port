// func #5  FUN_000104c7  000104c7  size=120


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000104c7(void)

{
  code *pcVar1;
  ushort uVar2;
  undefined2 extraout_DX;
  undefined4 extraout_EDX;
  undefined2 local_38 [2];
  short local_34;
  ushort local_30;
  
  FUN_000418ee();
  local_38[0] = 0x1500;
  local_34 = 0;
  FUN_00041e51(0x2f,local_38,local_38);
  if (local_34 != 0) {
    _DAT_00053c58 = (uint)local_30;
  }
  pcVar1 = (code *)swi(0x31);
  uVar2 = (*pcVar1)(0x100,extraout_EDX,0x11);
  *(undefined2 *)((uint)uVar2 * 0x10) = extraout_DX;
  DAT_00053c5c = (undefined2 *)((uint)uVar2 * 0x10) + 1;
  return;
}


