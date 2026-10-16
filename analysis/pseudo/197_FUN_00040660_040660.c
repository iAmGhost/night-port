// func #197  FUN_00040660  00040660  size=149


void FUN_00040660(void)

{
  int iVar1;
  undefined8 uVar2;
  undefined1 local_328 [768];
  int local_28;
  int local_24;
  undefined8 local_20;
  
  uVar2 = FUN_000418ee();
  for (local_28 = 0; local_20._4_4_ = (int)((ulonglong)uVar2 >> 0x20), local_20._0_4_ = (int)uVar2,
      local_28 < local_20._4_4_; local_28 = local_28 + 1) {
    iVar1 = local_28;
    for (local_24 = 0; local_24 < 0x300; local_24 = local_24 + 1) {
      local_328[local_24] =
           (char)((int)((uint)*(byte *)((int)local_20 + local_24) * local_28) / local_20._4_4_);
      iVar1 = local_24;
    }
    local_20 = uVar2;
    FUN_00035ec2(iVar1);
    FUN_00040042();
    uVar2 = local_20;
  }
  return;
}


