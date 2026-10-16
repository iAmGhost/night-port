// func #0  FUN_00010010  00010010  size=201


int * FUN_00010010(void)

{
  int *piVar1;
  int iVar2;
  undefined8 uVar3;
  undefined4 local_2c;
  int local_24;
  
  uVar3 = FUN_000418ee();
  piVar1 = (int *)uVar3;
  FUN_000367c9((int)((ulonglong)uVar3 >> 0x20));
  FUN_0004192e(piVar1,1,4,DAT_000639b8);
  iVar2 = FUN_00041af8(*piVar1 << 2);
  piVar1[1] = iVar2;
  for (local_24 = 0; local_24 < *piVar1; local_24 = local_24 + 1) {
    iVar2 = FUN_00041bdc(0x79);
    if (iVar2 == 0) {
      local_2c = 0;
    }
    else {
      local_2c = FUN_00010181(iVar2,DAT_000639b8);
    }
    *(undefined4 *)(piVar1[1] + local_24 * 4) = local_2c;
  }
  return piVar1;
}


