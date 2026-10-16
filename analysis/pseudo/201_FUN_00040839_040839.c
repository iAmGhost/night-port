// func #201  FUN_00040839  00040839  size=140


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00040839(void)

{
  char cVar1;
  int iVar2;
  char *local_20;
  
  local_20 = (char *)FUN_000418ee();
  _DAT_00063f8c = 0;
  while( true ) {
    cVar1 = *local_20;
    iVar2 = FUN_000408c5(local_20 + 1);
    local_20 = local_20 + 4;
    if (cVar1 == '\0') {
      return;
    }
    if (cVar1 == '\x01') break;
    if (cVar1 == '\x02') {
      FUN_00040b83(local_20,iVar2);
      return;
    }
    local_20 = local_20 + iVar2;
  }
  FUN_0004097e(local_20,iVar2);
  return;
}


