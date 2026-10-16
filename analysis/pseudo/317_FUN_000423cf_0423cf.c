// func #261  FUN_000423cf  000423cf  size=22


undefined4 FUN_000423cf(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  code *pcVar1;
  undefined4 uVar2;
  undefined1 extraout_DL;
  
  (*(code *)PTR_FUN_00053370)(param_1,param_1,param_3,param_4,param_2);
  (*(code *)PTR_FUN_00053374)();
  FUN_00044663(0,0xff);
  pcVar1 = (code *)swi(0x21);
  uVar2 = (*pcVar1)(CONCAT11(0x4c,extraout_DL));
  return uVar2;
}


