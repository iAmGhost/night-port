// func #507  FUN_00048312  00048312  size=64

void FUN_00048312(undefined4 param_1,undefined4 param_2,int param_3,int param_4)

{
  bool bVar1;
  code *pcVar2;
  byte bVar3;
  char cVar7;
  char cVar8;
  ushort uVar9;
  uint uVar5;
  int unaff_EBP;
  int unaff_ESI;
  char in_CF;
  uint in_stack_00000000;
  int *piVar4;
  int iVar6;
  
  cVar7 = (char)((uint)param_1 >> 8);
  uVar9 = (ushort)((uint)param_1 >> 0x10);
  bVar3 = (byte)param_1;
  cVar8 = cVar7 * -0x80;
  piVar4 = (int *)CONCAT22(uVar9,CONCAT11(cVar8,bVar3));
  *(int *)(unaff_EBP + 0xb293ffa) = *(int *)(unaff_EBP + 0xb293ffa) << 1;
  *piVar4 = *piVar4 + param_4;
  *(byte *)(param_3 + -0x40047b3a) = *(byte *)(param_3 + -0x40047b3a) | (byte)param_3;
  bVar1 = 9 < (bVar3 & 0xf) || (in_stack_00000000 & 0x10) != 0;
  uVar5 = CONCAT31((int3)(((uint)uVar9 << 0x10) >> 8),bVar3 + bVar1 * -6) & 0xffffff0f;
  bVar3 = (byte)uVar5;
  cVar8 = (cVar8 - *(char *)(unaff_ESI + -1)) - bVar1;
  iVar6 = CONCAT22((short)(uVar5 >> 0x10),CONCAT11(cVar8,bVar3));
  bVar1 = 9 < bVar3 || bVar1;
  uVar5 = CONCAT31((int3)((uint)iVar6 >> 8),bVar3 + bVar1 * -6) & 0xffffff0f;
  pcVar2 = (code *)swi(3);
  (*pcVar2)(CONCAT22((short)((uint)param_2 >> 0x10),
                     CONCAT11(((char)((uint)param_2 >> 8) - cVar7) - in_CF,(char)param_2)) -
            *(int *)((int)*(undefined6 *)(unaff_ESI + 1 + unaff_EBP) + -0x1c71d8d9),
            CONCAT22((short)(uVar5 >> 0x10),CONCAT11(cVar8 - bVar1,(char)uVar5)),
            (byte)param_3 - *(char *)(iVar6 + -0x4662f595),param_4 + -1);
  return;
}
