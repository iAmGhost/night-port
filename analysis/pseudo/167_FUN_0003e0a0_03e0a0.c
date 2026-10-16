// func #167  FUN_0003e0a0  0003e0a0  size=172


undefined1 * FUN_0003e0a0(void)

{
  undefined1 uVar1;
  undefined1 *puVar2;
  undefined4 uVar3;
  undefined8 uVar4;
  
  uVar4 = FUN_000418ee();
  uVar3 = (undefined4)((ulonglong)uVar4 >> 0x20);
  puVar2 = (undefined1 *)uVar4;
  if (DAT_00063e18 == (undefined1 *)0x0) {
    DAT_00063e18 = puVar2;
    DAT_00063e1c = puVar2;
    *(undefined4 *)(puVar2 + 0x55) = 0;
    *(undefined4 *)(puVar2 + 0x51) = 0;
  }
  else {
    *(undefined1 **)((int)DAT_00063e1c + 0x51) = puVar2;
    *(undefined1 **)(puVar2 + 0x55) = DAT_00063e1c;
    *(undefined4 *)(puVar2 + 0x51) = 0;
    DAT_00063e1c = puVar2;
  }
  uVar1 = FUN_00041c8f(uVar3);
  *puVar2 = uVar1;
  FUN_0004192e(puVar2 + 1,4,0x14,uVar3);
  return puVar2;
}


