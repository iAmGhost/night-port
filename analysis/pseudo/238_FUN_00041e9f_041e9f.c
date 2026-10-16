// func #238  FUN_00041e9f  00041e9f  size=36


undefined8 FUN_00041e9f(undefined4 param_1,undefined4 param_2)

{
  uint *puVar1;
  uint uVar2;
  
  puVar1 = (uint *)FUN_00041e99();
  uVar2 = 0;
  if (puVar1 != (uint *)0x0) {
    uVar2 = *puVar1 * 0x41c64e6d + 0x3039;
    *puVar1 = uVar2;
    uVar2 = uVar2 >> 0x10 & 0x7fff;
  }
  return CONCAT44(param_2,uVar2);
}


