// func #210  FUN_000411d9  000411d9  size=100


undefined8 FUN_000411d9(void)

{
  int iVar1;
  uint uVar2;
  undefined4 extraout_EDX;
  undefined4 local_28;
  
  FUN_000418ee();
  iVar1 = FUN_00041340(0xe1);
  if (iVar1 == 0) {
    local_28 = 0;
  }
  else {
    iVar1 = FUN_000412a3();
    uVar2 = FUN_000412a3();
    DAT_00063f88 = uVar2 | iVar1 << 8;
    local_28 = DAT_00063f88;
  }
  return CONCAT44(extraout_EDX,local_28);
}


