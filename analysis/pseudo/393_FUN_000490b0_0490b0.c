// func #393  FUN_000490b0  000490b0  size=89


undefined4 FUN_000490b0(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  code *pcVar1;
  undefined2 extraout_var;
  undefined2 in_CS;
  uint in_CR0;
  
  if (DAT_000536c9 != '\0') {
    pcVar1 = (code *)swi(0x2f);
    (*pcVar1)(0xfa21,in_CS,0,0,param_4,param_3);
    pcVar1 = (code *)swi(0x2f);
    (*pcVar1)(CONCAT22(extraout_var,0xfa1f),0,0,0);
    return 1;
  }
  if (DAT_000536c8 != '\0') {
    *(undefined1 *)(param_1 + 0x3e) = 0;
    *(byte *)(param_1 + 0x40) = *(byte *)(param_1 + 0x40) & 0xfb;
    pcVar1 = (code *)swi(0x21);
    (*pcVar1)(CONCAT22((short)((in_CR0 & 0xfffffffb) >> 0x10),
                       CONCAT11(0x30,(char)(in_CR0 & 0xfffffffb))));
  }
  return 0;
}


