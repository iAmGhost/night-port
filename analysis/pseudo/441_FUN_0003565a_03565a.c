// func #441  FUN_0003565a  0003565a  size=95

void FUN_0003565a(void)

{
  int iVar1;
  uint uVar2;
  undefined4 local_24;
  
  iVar1 = FUN_000418ee();
  uVar2 = iVar1 % 1000;
  if (iVar1 / 1000 == 0) {
    local_24 = 0x10000;
  }
  else {
    uVar2 = (iVar1 / 1000) * 0x4aa;
    local_24 = uVar2 / 0;
    uVar2 = uVar2 % 0;
  }
  FUN_000356ba(local_24,uVar2);
  return;
}
