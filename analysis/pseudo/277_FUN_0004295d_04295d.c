// func #277  FUN_0004295d  0004295d  size=29


undefined8 FUN_0004295d(undefined4 param_1,undefined4 param_2)

{
  code *pcVar1;
  uint uVar2;
  undefined4 *puVar3;
  undefined4 uStack_4;
  
  uVar2 = DAT_000533a4;
  uStack_4 = param_2;
  DAT_000533a4 = 0;
  puVar3 = &uStack_4;
  if (uVar2 == 0) {
    pcVar1 = (code *)swi(0x21);
    uVar2 = (*pcVar1)();
    uVar2 = uVar2 & 0xff;
    puVar3 = (undefined4 *)register0x00000010;
  }
  return CONCAT44(*puVar3,uVar2);
}


