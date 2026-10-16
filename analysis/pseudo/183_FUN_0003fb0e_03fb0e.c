// func #183  FUN_0003fb0e  0003fb0e  size=101


void FUN_0003fb0e(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  undefined8 uVar5;
  
  uVar5 = FUN_000418ee();
  uVar2 = (undefined4)((ulonglong)uVar5 >> 0x20);
  DAT_00063e7c = (undefined4 *)FUN_00041af8(uVar2);
  FUN_0004192e(DAT_00063e7c,uVar2,1,(int)uVar5);
  puVar3 = DAT_00063e7c;
  puVar4 = DAT_00051a0c;
  for (iVar1 = 16000; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar4 = *puVar3;
    puVar3 = puVar3 + 1;
    puVar4 = puVar4 + 1;
  }
  FUN_00041c31(DAT_00063e7c);
  return;
}


