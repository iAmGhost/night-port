// func #379  FUN_00048585  00048585  size=102


longlong FUN_00048585(int *param_1,int *param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  undefined4 unaff_EDI;
  undefined8 uVar3;
  
  if ((((short)param_1[2] == 0) && (param_1[1] == 0)) && (*param_1 == 0)) {
    return ZEXT48(param_2) << 0x20;
  }
  if ((((short)param_2[2] == 0) && (param_2[1] == 0)) && (*param_2 == 0)) {
    *param_1 = 0;
    param_1[1] = 0;
    *(undefined2 *)(param_1 + 2) = 0;
    return 0;
  }
  iVar1 = param_1[2];
  iVar2 = param_2[2];
  uVar3 = FUN_000485eb(*param_1,param_1[1],*param_2,param_2[1],param_1,param_3,param_4);
  *(undefined8 *)param_1 = uVar3;
  *(short *)(param_1 + 2) = (short)iVar1;
  return CONCAT44(param_2,CONCAT22((short)((uint)unaff_EDI >> 0x10),(short)iVar2));
}


