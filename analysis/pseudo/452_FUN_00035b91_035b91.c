// func #452  FUN_00035b91  00035b91  size=98

void FUN_00035b91(undefined4 param_1,undefined4 param_2,byte param_3)

{
  undefined1 uVar1;
  undefined1 extraout_DL;
  undefined1 local_18;
  
  uVar1 = FUN_000418ee();
  local_18 = (byte)(((uint)param_3 * 100) / 100);
  if (0x7f < local_18) {
    local_18 = 0x7f;
  }
  FUN_00035a19(uVar1,extraout_DL,local_18);
  return;
}
