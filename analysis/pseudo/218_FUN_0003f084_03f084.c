// func #?  FUN_0003f084  03f084  (recovered)

void FUN_0003f084(undefined4 param_1,ushort *param_2,undefined2 *param_3,undefined2 *param_4)

{
  undefined2 local_70 [2];
  undefined2 local_6c;
  undefined2 local_68;
  uint *local_5c;
  uint local_54 [4];
  uint local_44;
  uint local_40;
  uint local_3c;
  uint local_38;
  undefined2 local_32;
  undefined1 local_20 [12];
  undefined4 local_14;
  ushort *local_10;
  undefined2 *local_c;
  undefined2 *local_8;
  
  local_14 = param_1;
  local_10 = param_2;
  local_c = param_3;
  local_8 = param_4;
  FUN_00041df2(local_20);
  FUN_00041e20(local_54,0,0x32);
  local_38 = (uint)*local_10;
  local_44 = (uint)local_10[2];
  local_3c = (uint)local_10[4];
  local_40 = (uint)local_10[6];
  local_54[0] = (uint)local_10[10];
  local_32 = *local_8;
  local_70[0] = 0x300;
  local_6c = (undefined2)local_14;
  local_68 = 0;
  local_5c = local_54;
  FUN_00041e38(0x31,local_70,local_70,local_20);
  *local_c = (short)local_38;
  local_c[2] = (short)local_44;
  local_c[4] = (short)local_3c;
  local_c[6] = (short)local_40;
  return;
}

