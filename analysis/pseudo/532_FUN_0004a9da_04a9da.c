// func #532  FUN_0004a9da  0004a9da  size=110

undefined8
FUN_0004a9da(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,int param_5
            ,uint param_6,int *param_7)

{
  int iVar1;
  
  iVar1 = 0;
  if (((param_6 & 0x7fffffff) != 0) || (param_5 != 0)) {
    iVar1 = ((int)(param_6 >> 0x10 & 0x7ff0) >> 4) + -0x3fe;
  }
  *param_7 = iVar1;
  return CONCAT44(param_2,param_1);
}
