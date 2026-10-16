// func #391  FUN_00048f4c  00048f4c  size=82


void FUN_00048f4c(undefined4 param_1,undefined4 param_2,int param_3,undefined8 *param_4)

{
  undefined8 uVar1;
  int local_10;
  
  uVar1 = *param_4;
  local_10 = param_3;
  do {
    uVar1 = FUN_00047d4e((int)uVar1,(int)((ulonglong)uVar1 >> 0x20),param_1,param_2);
    uVar1 = FUN_0004794a((int)uVar1,(int)((ulonglong)uVar1 >> 0x20),
                         *(undefined4 *)((int)param_4 + 10),*(undefined4 *)((int)param_4 + 0xe));
    local_10 = local_10 + -1;
    param_4 = (undefined8 *)((int)param_4 + 10);
  } while (local_10 != 0);
  return;
}


