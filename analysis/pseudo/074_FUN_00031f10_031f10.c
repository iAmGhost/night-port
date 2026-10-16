// func #74  FUN_00031f10  00031f10  size=79


void FUN_00031f10(void)

{
  int iVar1;
  undefined8 uVar2;
  
  uVar2 = FUN_000418ee();
  iVar1 = (int)uVar2;
  if (*(byte *)(iVar1 + 0x31) < 5) {
    *(int *)((uint)*(byte *)(iVar1 + 0x31) * 4 + iVar1 + 0x32) = (int)((ulonglong)uVar2 >> 0x20);
    *(char *)(iVar1 + 0x31) = *(char *)(iVar1 + 0x31) + '\x01';
  }
  return;
}


