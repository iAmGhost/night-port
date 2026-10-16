// func #384  FUN_000487b0  000487b0  size=465


void FUN_000487b0(uint *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 uVar2;
  undefined2 uVar3;
  bool bVar4;
  bool bVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  undefined4 uVar8;
  
  iVar1 = (ushort)param_1[2] - 0x3ffe;
  *(undefined2 *)(param_1 + 2) = 0x3ffe;
  uVar6 = FUN_0004794a(*param_1,param_1[1],0,0x80000000,iVar1,param_3,param_4,param_2);
  bVar4 = (ushort)param_1[2] < 0x3ffe;
  bVar5 = false;
  if ((ushort)param_1[2] == 0x3ffe) {
    bVar4 = param_1[1] < 0xb504f333;
    bVar5 = false;
    if (param_1[1] == 0xb504f333) {
      bVar4 = *param_1 < 0xf9de6484;
      bVar5 = *param_1 == 0xf9de6484;
    }
  }
  if (bVar4 || bVar5) {
    *(undefined8 *)param_1 = uVar6;
    *(undefined2 *)(param_1 + 2) = 0x3ffe;
    iVar1 = iVar1 + -1;
  }
  else {
    uVar6 = FUN_0004794a((int)uVar6,(int)((ulonglong)uVar6 >> 0x20),0,0x80000000,iVar1,param_3,
                         param_4,param_2);
  }
  uVar7 = FUN_0004794a(*param_1,param_1[1],0,0x80000000);
  uVar6 = FUN_00047b79((int)uVar6,(int)((ulonglong)uVar6 >> 0x20),(int)uVar7,
                       (int)((ulonglong)uVar7 >> 0x20));
  uVar2 = (undefined4)((ulonglong)uVar6 >> 0x20);
  uVar8 = (undefined4)uVar6;
  uVar6 = FUN_00047d4e(uVar8,uVar2,uVar8,uVar2);
  *(undefined8 *)param_1 = uVar6;
  *(undefined2 *)(param_1 + 2) = 0x3ffe;
  uVar6 = FUN_00048f4c((int)uVar6,(int)((ulonglong)uVar6 >> 0x20),3,&DAT_0004872c);
  uVar3 = (undefined2)param_1[2];
  uVar7 = FUN_00048f4c(*param_1,param_1[1],2,&DAT_0004870e,0x3ffe3ffe);
  uVar6 = FUN_00047b79((int)uVar7,(int)((ulonglong)uVar7 >> 0x20),(int)uVar6,
                       (int)((ulonglong)uVar6 >> 0x20));
  uVar6 = FUN_00047d4e((int)uVar6,(int)((ulonglong)uVar6 >> 0x20),*param_1,param_1[1]);
  uVar6 = FUN_00047d4e((int)uVar6,(int)((ulonglong)uVar6 >> 0x20),uVar8,uVar2);
  uVar6 = FUN_0004794a((int)uVar6,(int)((ulonglong)uVar6 >> 0x20),uVar8,uVar2);
  uVar8 = (undefined4)uVar6;
  if (iVar1 != 0) {
    FUN_00047f0e(iVar1,param_1);
    uVar7 = FUN_00047d4e(*param_1,param_1[1],0x865435c,0xde8082e3);
    uVar6 = FUN_0004794a((int)uVar7,(int)((ulonglong)uVar7 >> 0x20),uVar8,
                         (int)((ulonglong)uVar6 >> 0x20));
    uVar8 = (undefined4)uVar6;
    uVar3 = (undefined2)param_1[2];
    uVar7 = FUN_00047d4e(*param_1,param_1[1],0,0xb1800000);
    uVar6 = FUN_0004794a((int)uVar7,(int)((ulonglong)uVar7 >> 0x20),uVar8,
                         (int)((ulonglong)uVar6 >> 0x20));
  }
  *(undefined8 *)param_1 = uVar6;
  *(undefined2 *)(param_1 + 2) = uVar3;
  return;
}


