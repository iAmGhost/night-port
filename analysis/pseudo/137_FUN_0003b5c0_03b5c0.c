// func #137  FUN_0003b5c0  0003b5c0  size=190


undefined1 * FUN_0003b5c0(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined1 *puVar1;
  undefined4 extraout_ECX;
  undefined8 uVar2;
  undefined1 in_stack_00000000;
  
  uVar2 = FUN_000418ee();
  puVar1 = (undefined1 *)uVar2;
  *(undefined4 *)(puVar1 + 1) = extraout_ECX;
  *(int *)(puVar1 + 9) = (int)((ulonglong)uVar2 >> 0x20);
  *(undefined4 *)(puVar1 + 0xd) = param_3;
  *(undefined4 *)(puVar1 + 5) = 0;
  *puVar1 = in_stack_00000000;
  if (DAT_000639cc == (undefined1 *)0x0) {
    DAT_000639cc = puVar1;
    DAT_000639d0 = puVar1;
    *(undefined4 *)(puVar1 + 0x15) = 0;
    *(undefined4 *)(puVar1 + 0x11) = 0;
  }
  else {
    *(undefined1 **)((int)DAT_000639d0 + 0x11) = puVar1;
    *(undefined1 **)(puVar1 + 0x15) = DAT_000639d0;
    *(undefined4 *)(puVar1 + 0x11) = 0;
    DAT_000639d0 = puVar1;
  }
  DAT_000639c8 = DAT_000639c8 + 1;
  return puVar1;
}


