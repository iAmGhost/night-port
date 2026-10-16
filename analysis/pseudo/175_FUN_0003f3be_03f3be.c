// func #175  FUN_0003f3be  0003f3be  size=82


void FUN_0003f3be(void)

{
  code *pcVar1;
  int iVar2;
  int iVar3;
  
  iVar3 = FUN_000418ee();
  iVar2 = DAT_00063e5c;
  iVar3 = iVar3 << (DAT_00063e4c & 0x1f);
  if (iVar3 != *(int *)(&DAT_00063e50 + DAT_00063e5c * 4)) {
    *(int *)(&DAT_00063e50 + DAT_00063e5c * 4) = iVar3;
    pcVar1 = (code *)swi(0x10);
    (*pcVar1)(0x4f05,iVar3,iVar2);
  }
  return;
}


