// func #312  FUN_00043fc3  00043fc3  size=230


int FUN_00043fc3(undefined4 param_1,int *param_2,undefined4 *param_3)

{
  byte bVar1;
  undefined6 *puVar2;
  uint uVar3;
  undefined4 uVar4;
  uint uVar5;
  undefined4 *extraout_ECX;
  undefined4 *extraout_ECX_00;
  undefined4 *extraout_ECX_01;
  undefined4 *puVar6;
  int iVar7;
  undefined1 *unaff_ESI;
  byte abStack_3c [32];
  uint local_1c;
  int local_18;
  
  local_1c = (uint)(*(char *)*param_3 == '^');
  if (local_1c != 0) {
    *param_3 = (char *)*param_3 + 1;
  }
  uVar4 = FUN_00043f80(*param_3,abStack_3c,param_3,param_1);
  uVar3 = local_1c;
  *param_3 = uVar4;
  bVar1 = *(byte *)(extraout_ECX + 4);
  if ((bVar1 & 1) != 0) {
    if ((bVar1 & 4) == 0) {
      if ((bVar1 & 8) == 0) {
        puVar6 = (undefined4 *)*param_2;
        *param_2 = (int)(puVar6 + 1);
        unaff_ESI = (undefined1 *)*puVar6;
      }
      else {
        puVar6 = (undefined4 *)*param_2;
        *param_2 = (int)(puVar6 + 1);
        unaff_ESI = (undefined1 *)*puVar6;
      }
    }
    else {
      puVar2 = (undefined6 *)*param_2;
      *param_2 = (int)(puVar2 + 1);
      unaff_ESI = (undefined1 *)*puVar2;
    }
  }
  iVar7 = extraout_ECX[3];
  local_18 = 0;
  puVar6 = extraout_ECX;
  do {
    if (iVar7 == 0) {
LAB_0004408b:
      if (((*(byte *)(puVar6 + 4) & 1) != 0) && (0 < local_18)) {
        *unaff_ESI = 0;
      }
      return local_18;
    }
    uVar5 = (*(code *)*puVar6)(puVar6);
    puVar6 = extraout_ECX_00;
    if ((*(byte *)(extraout_ECX_00 + 4) & 2) != 0) goto LAB_0004408b;
    if (((abStack_3c[(int)uVar5 >> 3] & (&DAT_00051854)[uVar5 & 7]) == 0) != uVar3) {
      (*(code *)extraout_ECX_00[1])(uVar5,extraout_ECX_00);
      puVar6 = extraout_ECX_01;
      goto LAB_0004408b;
    }
    local_18 = local_18 + 1;
    iVar7 = iVar7 + -1;
    if ((*(byte *)(extraout_ECX_00 + 4) & 1) != 0) {
      *unaff_ESI = (char)uVar5;
      unaff_ESI = unaff_ESI + 1;
    }
  } while( true );
}


