// func #149  FUN_0003c853  0003c853  size=640


void FUN_0003c853(void)

{
  undefined1 uVar1;
  int *piVar2;
  int iVar3;
  undefined4 uVar4;
  undefined8 uVar5;
  int local_30;
  
  uVar5 = FUN_000418ee();
  uVar4 = (undefined4)((ulonglong)uVar5 >> 0x20);
  piVar2 = (int *)uVar5;
  piVar2[0x10] = DAT_000639e8;
  DAT_000639e8 = DAT_000639e8 + 1;
  *(undefined4 *)((int)piVar2 + 0x26) = 1;
  uVar1 = FUN_00041c8f(uVar4);
  *(undefined1 *)(piVar2 + 4) = uVar1;
  uVar1 = FUN_00041c8f(uVar4);
  *(undefined1 *)((int)piVar2 + 0x11) = uVar1;
  FUN_0004192e(piVar2,1,4,uVar4);
  FUN_0004192e(piVar2 + 1,1,4,uVar4);
  FUN_0004192e(piVar2 + 2,1,4,uVar4);
  FUN_0004192e(piVar2 + 3,1,4,uVar4);
  FUN_0004192e((int)piVar2 + 0x12,1,4,uVar4);
  FUN_0004192e((int)piVar2 + 0x16,1,4,uVar4);
  *(int *)((int)piVar2 + 0x1a) = piVar2[2] - *piVar2;
  if ((char)piVar2[4] == '\0') {
    iVar3 = FUN_00041af8((piVar2[2] - *piVar2) * 200);
    piVar2[0xd2] = iVar3;
    FUN_0004192e(piVar2[0xd2],piVar2[2] - *piVar2,200,uVar4);
  }
  else {
    iVar3 = FUN_00041af8((piVar2[3] - piVar2[1]) * 0x140);
    piVar2[0xd2] = iVar3;
    FUN_0004192e(piVar2[0xd2],piVar2[3] - piVar2[1],0x140,uVar4);
  }
  FUN_0004192e(piVar2 + 0x12,3,0x100,uVar4);
  FUN_0004192e(piVar2 + 0xf,1,4,uVar4);
  if (piVar2[0xf] != 0) {
    iVar3 = FUN_00041af8(piVar2[0xf] * 2);
    piVar2[0x11] = iVar3;
    FUN_0004192e(piVar2[0x11],2,piVar2[0xf],uVar4);
  }
  DAT_0006335c = DAT_0006335c + 1;
  if (DAT_0006335c == DAT_000639d8) {
    piVar2[0xd3] = 0;
  }
  else {
    iVar3 = FUN_00041bdc(0x354);
    if (iVar3 == 0) {
      local_30 = 0;
    }
    else {
      local_30 = FUN_0003c6ec(iVar3);
    }
    piVar2[0xd3] = local_30;
    *(int **)(piVar2[0xd3] + 0x350) = piVar2;
    FUN_0003c853(piVar2[0xd3],uVar4);
  }
  return;
}


