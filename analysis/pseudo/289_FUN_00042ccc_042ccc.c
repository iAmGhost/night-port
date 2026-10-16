// func #289  FUN_00042ccc  00042ccc  size=94


undefined8 FUN_00042ccc(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  code *pcVar2;
  undefined4 extraout_ECX;
  int *piVar3;
  int *piVar4;
  undefined2 *puVar5;
  undefined2 in_ES;
  byte bVar6;
  undefined8 uVar7;
  undefined4 uStack_18;
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uVar7 = CONCAT44(DAT_00053350,param_1);
  uStack_4 = param_3;
  uStack_8 = param_4;
  uStack_c = param_2;
  puVar5 = (undefined2 *)&stack0xffffffec;
  while (piVar3 = (int *)((ulonglong)uVar7 >> 0x20), piVar3 != (int *)0x0) {
    if (*(int *)piVar3[9] + 0x2c == *piVar3) {
      piVar4 = piVar3 + -2;
      iVar1 = piVar3[2];
      *(undefined4 *)(puVar5 + -2) = 0x42cfa;
      FUN_00042c8d(piVar3,piVar3,piVar4,iVar1);
      iVar1 = piVar4[1];
      bVar6 = 0;
      if (iVar1 == 0) {
        pcVar2 = (code *)swi(0x31);
        uVar7 = (*pcVar2)(0x502);
        puVar5 = puVar5 + 2;
      }
      else {
        puVar5[-2] = in_ES;
        pcVar2 = (code *)swi(0x21);
        uVar7 = (*pcVar2)(CONCAT22((short)((uint)iVar1 >> 0x10),CONCAT11(0x49,(char)iVar1)),
                          extraout_ECX);
        uVar7 = CONCAT44((int)((ulonglong)uVar7 >> 0x20),
                         ((int)uVar7 << 1 | (uint)bVar6) >> 1 | (uint)((bVar6 & 1) != 0) << 0x1f);
        in_ES = *puVar5;
        puVar5 = puVar5 + 2;
      }
    }
    else {
      uVar7 = CONCAT44(piVar3[2],*(int *)piVar3[9] + 0x2c);
    }
  }
  return CONCAT44(*(undefined4 *)(puVar5 + 4),(int)uVar7);
}


