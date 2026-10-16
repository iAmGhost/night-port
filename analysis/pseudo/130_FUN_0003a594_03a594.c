// func #130  FUN_0003a594  0003a594  size=99


void FUN_0003a594(undefined4 param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined8 uVar2;
  undefined4 local_24;
  
  uVar2 = FUN_000418ee();
  iVar1 = (int)((ulonglong)uVar2 >> 0x20);
  for (local_24 = 0; local_24 < param_3; local_24 = local_24 + 1) {
    if (*(char *)(iVar1 + local_24) != -1) {
      *(undefined1 *)((int)uVar2 + local_24) = *(undefined1 *)(iVar1 + local_24);
    }
  }
  return;
}


