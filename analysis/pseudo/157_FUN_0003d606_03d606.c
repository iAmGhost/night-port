// func #157  FUN_0003d606  0003d606  size=118


void FUN_0003d606(void)

{
  int iVar1;
  undefined4 local_24;
  
  iVar1 = FUN_000418ee();
  for (local_24 = 0; local_24 < *(int *)(iVar1 + 0x3c); local_24 = local_24 + 1) {
    FUN_0003d67c(iVar1 + 0x48,*(undefined1 *)(*(int *)(iVar1 + 0x44) + local_24 * 2),
                 *(undefined1 *)(local_24 * 2 + *(int *)(iVar1 + 0x44) + 1));
  }
  return;
}


