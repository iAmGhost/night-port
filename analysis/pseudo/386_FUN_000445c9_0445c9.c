// func #317  FUN_000445c9  000445c9  size=79


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000445c9(undefined4 param_1)

{
  int iVar1;
  uint uVar2;
  undefined1 *puVar3;
  undefined8 uVar4;
  
  puVar3 = &stack0xfffffff8;
  uVar4 = FUN_0004917e(param_1,DAT_0005339c + 3U & 0xfffffffc);
  uVar2 = (uint)((ulonglong)uVar4 >> 0x20);
  if (uVar2 < (uint)uVar4) {
    iVar1 = -uVar2;
    puVar3 = &stack0xfffffff8 + iVar1;
    _DAT_000533a0 = &stack0xfffffff8 + iVar1;
  }
  else {
    _DAT_000533a0 = (undefined1 *)0x0;
  }
  _DAT_000533a0 = _DAT_000533a0 + DAT_0005339c;
  *(undefined4 *)(puVar3 + -4) = 0x445fe;
  FUN_00049187();
  *(undefined4 *)(puVar3 + -4) = 0x4460e;
  FUN_0003af18(DAT_00064090,DAT_00064094);
  *(undefined4 *)(puVar3 + -4) = 0x44613;
  FUN_000423b7();
  return;
}


