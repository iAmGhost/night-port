// func #14  FUN_00010dde  00010dde  size=55


void FUN_00010dde(void)

{
  byte bVar1;
  undefined1 auStack_1808 [2036];
  undefined4 auStack_1014 [1022];
  
  bVar1 = 0;
  FUN_000418ee();
  auStack_1014[0x3fa] = DAT_00051b50;
  auStack_1014[(uint)bVar1 * -2 + 0x3fb] = (&DAT_00051b54)[(uint)bVar1 * -2];
  auStack_1014[(uint)bVar1 * -2 + (uint)bVar1 * -2 + 0x3fc] =
       (&DAT_00051b58)[(uint)bVar1 * -2 + (uint)bVar1 * -2];
  *(undefined1 *)(auStack_1014 + (uint)bVar1 * -2 + (uint)bVar1 * -2 + 0x3fc + (uint)bVar1 * -2 + 1)
       = *(undefined1 *)(&DAT_00051b58 + (uint)bVar1 * -2 + (uint)bVar1 * -2 + (uint)bVar1 * -2 + 1)
  ;
  FUN_00010570(auStack_1014 + 0x3fa,0);
  return;
}


