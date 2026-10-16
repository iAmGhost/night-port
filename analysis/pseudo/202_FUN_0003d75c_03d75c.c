// func #159  FUN_0003d75c  0003d75c  size=307


void FUN_0003d75c(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  float10 fVar4;
  int local_30;
  
  iVar1 = FUN_000418ee();
  for (local_30 = 0; local_30 < DAT_00062a90; local_30 = local_30 + 1) {
    iVar3 = (&DAT_0005d0b4)[local_30 * 5];
    iVar2 = ((&DAT_0005d0b8)[local_30 * 5] - *(int *)(iVar3 * 4 + *(int *)(DAT_00063dfc + 0xc))) -
            DAT_000639dc;
    iVar3 = (((&DAT_0005d0b8)[local_30 * 5] - *(int *)(*(int *)(DAT_00063dfc + 0xc) + iVar3 * 4)) +
            *(int *)(*(int *)(DAT_00063dfc + 4) + iVar3 * 4)) - DAT_000639dc;
    fVar4 = (float10)(float)(&DAT_0005d0c4)[local_30 * 5];
    FUN_00041f7e();
    iVar2 = iVar2 + (int)ROUND(fVar4);
    fVar4 = (float10)(float)(&DAT_0005d0c4)[local_30 * 5];
    FUN_00041f7e();
    iVar3 = iVar3 + (int)ROUND(fVar4);
    if (((-1 < iVar2) && (iVar2 < 0x140)) || ((-1 < iVar3 && (iVar3 < 0x140)))) {
      (&DAT_0005d0c4)[local_30 * 5] =
           (float)(&DAT_0005d0c4)[local_30 * 5] -
           (float)iVar1 *
           *(float *)(&DAT_0005274c + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28
                     );
    }
  }
  return;
}


