// func #26  FUN_0001435f  0001435f  size=191


undefined4 FUN_0001435f(undefined4 param_1,undefined4 param_2,byte param_3)

{
  int iVar1;
  int iVar2;
  byte extraout_DL;
  undefined4 local_2c;
  uint local_28;
  
  iVar2 = FUN_000418ee();
  iVar1 = *(int *)(*(int *)(*(int *)(iVar2 + 0x123) + 4) + *(int *)(iVar2 + 0x17) * 4);
  local_28 = 0;
  while (((int)local_28 < 6 &&
         ((*(int *)(iVar2 + 0x11) < (int)(uint)*(ushort *)(local_28 * 4 + iVar1 + 8) ||
          ((int)(uint)*(ushort *)(local_28 * 4 + iVar1 + 10) <= *(int *)(iVar2 + 0x11)))))) {
    local_28 = local_28 + 1;
  }
  if (((uint)extraout_DL == *(uint *)(iVar2 + 0x17)) && (param_3 == local_28)) {
    local_2c = 0;
  }
  else {
    local_2c = 1;
  }
  return local_2c;
}


