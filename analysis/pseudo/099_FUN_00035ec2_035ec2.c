// func #99  FUN_00035ec2  00035ec2  size=89


void FUN_00035ec2(void)

{
  byte bVar1;
  
  FUN_000418ee();
  do {
    bVar1 = FUN_0004238c(0x3da);
  } while ((bVar1 & 9) == 9);
  do {
    bVar1 = FUN_0004238c(0x3da);
  } while ((bVar1 & 9) != 9);
  return;
}


