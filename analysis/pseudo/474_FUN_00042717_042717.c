// func #474  FUN_00042717  00042717  size=103

longlong FUN_00042717(int param_1,uint param_2)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  
  puVar2 = DAT_00063ffc;
  do {
    if (puVar2 == (undefined4 *)0x0) {
      puVar2 = &DAT_00063ff8;
      do {
        puVar3 = puVar2;
        puVar2 = (undefined4 *)*puVar3;
        if (puVar2 == (undefined4 *)0x0) {
          FUN_00042a0d(4);
          return (ulonglong)param_2 << 0x20;
        }
      } while (param_1 != puVar2[1]);
      *puVar3 = *puVar2;
      *puVar2 = DAT_00063ffc;
      DAT_00063ffc = puVar2;
LAB_0004273e:
      return CONCAT44(param_2,param_1);
    }
    iVar1 = puVar2[1];
    if (param_1 == iVar1) {
      if ((*(byte *)(iVar1 + 0xc) & 3) != 0) {
        FUN_0004285c(iVar1,1);
      }
      goto LAB_0004273e;
    }
    puVar2 = (undefined4 *)*puVar2;
  } while( true );
}
