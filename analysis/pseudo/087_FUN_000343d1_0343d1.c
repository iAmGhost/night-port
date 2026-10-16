// func #87  FUN_000343d1  000343d1  size=93


void FUN_000343d1(void)

{
  byte bVar1;
  
  bVar1 = FUN_000418ee();
  if ((&DAT_00062cd8)[bVar1] == '\0') {
    (&DAT_00062c58)[bVar1] = 0;
  }
  else {
    (&DAT_00062c58)[bVar1] = 1;
    (&DAT_00062cd8)[bVar1] = (&DAT_00062cd8)[bVar1] + -1;
  }
  return;
}


