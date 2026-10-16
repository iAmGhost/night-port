// func #89  FUN_00034635  00034635  size=256


void FUN_00034635(void)

{
  byte bVar1;
  
  bVar1 = FUN_000418ee();
  if ((&DAT_00062c58)[bVar1] == '\x04') {
    if ((bVar1 == 0x4d) || (bVar1 == 0x4b)) {
      if (DAT_00062a94 == 0) {
        FUN_00034353(-0x4f - bVar1,DAT_00063f08);
      }
      else {
        FUN_00034353(bVar1 + 0x19,DAT_00063f08);
      }
    }
    else {
      FUN_00034353(bVar1,DAT_00063f08);
    }
    if ((&DAT_00062cd8)[bVar1] == '\0') {
      (&DAT_00062c58)[bVar1] = 0;
    }
    else {
      (&DAT_00062c58)[bVar1] = 1;
      (&DAT_00062cd8)[bVar1] = (&DAT_00062cd8)[bVar1] + -1;
    }
  }
  else {
    if ((&DAT_00062c58)[bVar1] == '\x01') {
      DAT_00062a94 = *(int *)(DAT_00053c7c + 0x1b);
    }
    (&DAT_00062c58)[bVar1] = (&DAT_00062c58)[bVar1] + '\x01';
  }
  return;
}


