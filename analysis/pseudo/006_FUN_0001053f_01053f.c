// func #6  FUN_0001053f  0001053f  size=49


void FUN_0001053f(void)

{
  code *pcVar1;
  undefined2 extraout_var;
  
  FUN_000418ee();
  pcVar1 = (code *)swi(0x31);
  (*pcVar1)(0x101,CONCAT22(extraout_var,*(undefined2 *)(DAT_00053c5c + -2)));
  return;
}


