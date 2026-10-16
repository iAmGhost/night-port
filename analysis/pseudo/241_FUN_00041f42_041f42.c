// func #241  FUN_00041f42  00041f42  size=27


undefined4 FUN_00041f42(int param_1,undefined1 *param_2,int param_3,undefined4 param_4)

{
  undefined4 extraout_ECX;
  undefined1 *puVar1;
  
  puVar1 = param_2;
  if ((param_3 == 10) && (param_1 < 0)) {
    param_1 = -param_1;
    *param_2 = 0x2d;
    puVar1 = param_2 + 1;
  }
  FUN_00041ef8(param_1,puVar1,param_3,param_2,param_4);
  return extraout_ECX;
}


