// func #208  FUN_00040f4f  00040f4f  size=100


int FUN_00040f4f(void)

{
  int iVar1;
  int local_20;
  
  iVar1 = FUN_000418ee();
  local_20 = 0;
  while( true ) {
    if (7 < local_20) {
      return 0;
    }
    FUN_0004123d(iVar1);
    iVar1 = FUN_000412a3();
    if (iVar1 == 0xaa) break;
    DAT_00053334 = DAT_00053334 + 0x10;
    iVar1 = local_20;
    local_20 = local_20 + 1;
  }
  return DAT_00053334;
}


