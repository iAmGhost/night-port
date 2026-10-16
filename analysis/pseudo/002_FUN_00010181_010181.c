// func #2  FUN_00010181  00010181  size=501


int * FUN_00010181(void)

{
  undefined1 uVar1;
  int *piVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined8 uVar5;
  
  uVar5 = FUN_000418ee();
  uVar4 = (undefined4)((ulonglong)uVar5 >> 0x20);
  piVar2 = (int *)uVar5;
  FUN_0004192e(piVar2,4,1,uVar4);
  FUN_0004192e(piVar2 + 1,2,2,uVar4);
  FUN_0004192e(piVar2 + 2,2,0xc,uVar4);
  FUN_0004192e(piVar2 + 8,4,6,uVar4);
  FUN_0004192e(piVar2 + 0xe,1,6,uVar4);
  FUN_0004192e((int)piVar2 + 0x3e,1,6,uVar4);
  FUN_0004192e(piVar2 + 0x11,1,6,uVar4);
  FUN_0004192e((int)piVar2 + 0x4a,2,6,uVar4);
  FUN_0004192e((int)piVar2 + 0x56,1,6,uVar4);
  uVar1 = FUN_00041c8f(uVar4);
  *(undefined1 *)(piVar2 + 0x17) = uVar1;
  FUN_0004192e((int)piVar2 + 0x5d,1,6,uVar4);
  FUN_0004192e((int)piVar2 + 99,1,6,uVar4);
  uVar1 = FUN_00041c8f(uVar4);
  *(undefined1 *)((int)piVar2 + 0x69) = uVar1;
  FUN_0004192e((int)piVar2 + 0x6a,1,2,uVar4);
  uVar1 = FUN_00041c8f(uVar4);
  *(undefined1 *)(piVar2 + 0x1b) = uVar1;
  uVar3 = FUN_00041af8(*piVar2 * 2);
  *(undefined4 *)((int)piVar2 + 0x6d) = uVar3;
  uVar3 = FUN_00041af8(*piVar2 << 2);
  *(undefined4 *)((int)piVar2 + 0x71) = uVar3;
  uVar3 = FUN_00041af8(*piVar2 << 2);
  *(undefined4 *)((int)piVar2 + 0x75) = uVar3;
  FUN_0004192e(*(undefined4 *)((int)piVar2 + 0x6d),2,*piVar2,uVar4);
  FUN_0004192e(*(undefined4 *)((int)piVar2 + 0x71),4,*piVar2,uVar4);
  FUN_0004192e(*(undefined4 *)((int)piVar2 + 0x75),4,*piVar2,uVar4);
  return piVar2;
}


