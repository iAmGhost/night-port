// func #307  FUN_00043d79  00043d79  size=53


undefined8 FUN_00043d79(undefined4 *param_1,undefined4 param_2)

{
  undefined4 uVar1;
  undefined4 extraout_ECX;
  undefined4 extraout_ECX_00;
  undefined4 uVar2;
  
  do {
    uVar1 = (*(code *)*param_1)(param_1);
  } while (((&DAT_0005185c)[(byte)((char)uVar1 + 1)] & 2) != 0);
  uVar2 = extraout_ECX;
  if ((*(byte *)(param_1 + 4) & 2) == 0) {
    (*(code *)param_1[1])(uVar1,param_1);
    uVar2 = extraout_ECX_00;
  }
  return CONCAT44(param_2,uVar2);
}


