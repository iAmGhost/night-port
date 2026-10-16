// func #448  FUN_00035a19  00035a19  size=122

undefined4 FUN_00035a19(undefined4 param_1,undefined4 param_2,uint param_3)

{
  int iVar1;
  undefined8 uVar2;
  undefined4 local_24;
  
  uVar2 = FUN_000418ee();
  iVar1 = FUN_000358c7((uint)uVar2 & 0xff);
  if (iVar1 == 0) {
    iVar1 = FUN_000358c7((uint)((ulonglong)uVar2 >> 0x20) & 0xff);
    if (iVar1 == 0) {
      if ((-1 < (int)param_3) && (iVar1 = FUN_000358c7(param_3 & 0xff), iVar1 != 0)) {
        return 0xffffffff;
      }
      local_24 = 0;
    }
    else {
      local_24 = 0xffffffff;
    }
  }
  else {
    local_24 = 0xffffffff;
  }
  return local_24;
}
