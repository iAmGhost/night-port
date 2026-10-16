// func #90  FUN_00034735  00034735  size=174


void FUN_00034735(void)

{
  byte bVar1;
  
  bVar1 = FUN_000418ee();
  if ((&DAT_00062c58)[bVar1] == '\x04') {
    FUN_00034353(bVar1,DAT_00063f08);
    if ((&DAT_00062cd8)[bVar1] == '\0') {
      (&DAT_00062c58)[bVar1] = 0;
    }
    else {
      (&DAT_00062c58)[bVar1] = 1;
      (&DAT_00062cd8)[bVar1] = (&DAT_00062cd8)[bVar1] + -1;
    }
  }
  else if ((byte)(&DAT_00062c58)[bVar1] < 4) {
    (&DAT_00062c58)[bVar1] = (&DAT_00062c58)[bVar1] + '\x01';
    FUN_0003442e(bVar1);
  }
  return;
}


