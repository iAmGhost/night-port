// func #514  FUN_0004933a  0004933a  size=119

void FUN_0004933a(void)

{
  undefined4 *puVar1;
  undefined *puVar2;
  int extraout_ECX;
  int extraout_ECX_00;
  int extraout_ECX_01;
  int iVar3;
  
  DAT_00053451 = DAT_00053451 & 0xf8 | 4;
  puVar2 = &DAT_00053410;
  while (*(int *)(puVar2 + 0xc) != 0) {
    puVar1 = (undefined4 *)FUN_00041af8(8);
    iVar3 = extraout_ECX;
    if (puVar1 == (undefined4 *)0x0) {
      puVar1 = (undefined4 *)FUN_00041af8(8);
      iVar3 = extraout_ECX_00;
      if (puVar1 == (undefined4 *)0x0) {
        FUN_000429ea("Not enough memory to allocate file structures\r\n",1);
        iVar3 = extraout_ECX_01;
      }
    }
    puVar1[1] = iVar3;
    *puVar1 = DAT_00063ffc;
    DAT_00063ffc = puVar1;
    puVar2 = (undefined *)(iVar3 + 0x1a);
  }
  DAT_00063ff8 = *(int *)(puVar2 + 0xc);
  return;
}
