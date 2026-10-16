// func #12  FUN_00010c3e  00010c3e  size=127


void FUN_00010c3e(void)

{
  bool bVar1;
  
  FUN_000418ee();
  bVar1 = false;
  do {
    FUN_00010927();
    if (DAT_00053c60 == 0xf) {
      if (*(int *)(DAT_00053c64 + 0xb) - *(int *)(DAT_00053c64 + 6) == 0xc51) {
        if (bVar1) {
          FUN_000405a7();
        }
        return;
      }
    }
    bVar1 = true;
    FUN_0003f15f();
    FUN_0003628e("cdinsert.pcx",1);
    FUN_00035d5d();
  } while( true );
}


