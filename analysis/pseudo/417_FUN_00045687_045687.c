// func #343  FUN_00045687  00045687  size=195


undefined8 FUN_00045687(uint *param_1,byte param_2)

{
  code *pcVar1;
  char extraout_CL;
  char extraout_CL_00;
  undefined2 extraout_CX;
  ushort extraout_DX;
  ushort extraout_DX_00;
  undefined2 extraout_DX_01;
  undefined4 in_stack_00000000;
  byte bStack_13;
  byte bStack_12;
  byte bStack_11;
  byte local_8;
  byte bStack_7;
  
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)();
  param_1[5] = (uint)(byte)(extraout_CL + 0x94);
  bStack_13 = (byte)(extraout_DX >> 8);
  param_1[4] = bStack_13 - 1;
  param_1[3] = extraout_DX & 0xff;
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)();
  bStack_11 = (byte)((ushort)extraout_CX >> 8);
  param_1[2] = (uint)bStack_11;
  bStack_12 = (byte)extraout_CX;
  param_1[1] = (uint)bStack_12;
  bStack_13 = (byte)(extraout_DX_00 >> 8);
  *param_1 = (uint)bStack_13;
  pcVar1 = (code *)swi(0x21);
  (*pcVar1)();
  local_8 = (byte)extraout_DX_01;
  if ((local_8 != param_2) && (param_1[2] != 0x17)) {
    param_1[5] = (uint)(byte)(extraout_CL_00 + 0x94);
    bStack_7 = (byte)((ushort)extraout_DX_01 >> 8);
    param_1[4] = bStack_7 - 1;
    param_1[3] = (uint)local_8;
  }
  param_1[8] = 0xffffffff;
  return CONCAT44(in_stack_00000000,(extraout_DX_00 & 0xff) * 10);
}


