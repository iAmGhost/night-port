// func #198  FUN_000406f5  000406f5  size=146


void FUN_000406f5(void)

{
  int iVar1;
  undefined8 uVar2;
  undefined1 local_328 [768];
  int local_28;
  int local_24;
  int local_20;
  int local_1c;
  
  uVar2 = FUN_000418ee();
  local_28 = (int)((ulonglong)uVar2 >> 0x20);
  local_1c = local_28;
  for (; local_20 = (int)uVar2, -1 < local_28; local_28 = local_28 + -1) {
    iVar1 = (int)((ulonglong)uVar2 >> 0x20);
    for (local_24 = 0; local_24 < 0x300; local_24 = local_24 + 1) {
      local_328[local_24] =
           (char)((int)((uint)*(byte *)(local_20 + local_24) * local_28) / local_1c);
      iVar1 = local_24;
    }
    FUN_00035ec2(iVar1);
    FUN_00040042();
    uVar2 = CONCAT44(local_28,local_20);
  }
  return;
}


