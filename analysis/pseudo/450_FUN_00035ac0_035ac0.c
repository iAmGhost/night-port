// func #450  FUN_00035ac0  00035ac0  size=75

int FUN_00035ac0(void)

{
  int iVar1;
  int local_20;
  
  iVar1 = FUN_000418ee();
  for (local_20 = 0; local_20 < 0x10; local_20 = local_20 + 1) {
    FUN_00035a19(local_20 + 0xb0,0x78,0);
    iVar1 = local_20;
  }
  return iVar1;
}
