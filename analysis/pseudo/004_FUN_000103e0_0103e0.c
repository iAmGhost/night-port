// func #4  FUN_000103e0  000103e0  size=231


void FUN_000103e0(undefined4 param_1,undefined4 param_2,undefined2 *param_3)

{
  undefined2 *extraout_ECX;
  undefined2 local_7c [2];
  undefined2 local_78;
  undefined2 local_74;
  uint *local_68;
  uint local_60 [4];
  uint local_50;
  uint local_4c;
  uint local_48;
  uint local_44;
  undefined2 local_3e;
  undefined1 local_2c [12];
  undefined8 local_20;
  undefined2 *local_18;
  undefined2 *local_14;
  
  local_20 = FUN_000418ee();
  local_18 = param_3;
  local_14 = extraout_ECX;
  FUN_00041df2(local_2c);
  FUN_00041e20(local_60,0,0x32);
  local_44 = (uint)*local_20._4_4_;
  local_50 = (uint)local_20._4_4_[2];
  local_48 = (uint)local_20._4_4_[4];
  local_4c = (uint)local_20._4_4_[6];
  local_60[0] = (uint)local_20._4_4_[10];
  local_3e = *local_14;
  local_7c[0] = 0x300;
  local_78 = (undefined2)local_20;
  local_74 = 0;
  local_68 = local_60;
  FUN_00041e38(0x31,local_7c,local_7c,local_2c);
  *local_18 = (short)local_44;
  local_18[2] = (short)local_50;
  local_18[4] = (short)local_48;
  local_18[6] = (short)local_4c;
  return;
}


