// func #122  FUN_00038a20  00038a20  size=192


undefined8 FUN_00038a20(void)

{
  int iVar1;
  undefined4 extraout_EDX;
  int local_20;
  
  FUN_000418ee();
  FUN_0003f15f();
  FUN_00041e20(&DAT_000633b8,0,0x300);
  FUN_00040042();
  FUN_0003628e("logo.pcx",0);
  FUN_00040660(&DAT_000636b8,0x32);
  for (local_20 = 0; local_20 < 0x400; local_20 = local_20 + 1) {
    thunk_FUN_0004564f(1);
  }
  iVar1 = FUN_000406f5(&DAT_000636b8,0x32);
  for (local_20 = 0; local_20 < 0x400; local_20 = local_20 + 1) {
    thunk_FUN_0004564f(1);
    iVar1 = local_20;
  }
  return CONCAT44(extraout_EDX,iVar1);
}


