// func #284  FUN_00042a43  00042a43  size=119


void FUN_00042a43(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  byte bVar1;
  int extraout_EDX;
  undefined4 *puVar2;
  undefined8 uVar3;
  
  FUN_00044bb4(param_1,param_1,param_3,param_4,param_2,param_4);
  if (*(int *)(extraout_EDX + 0x14) == 0) {
    if ((*(byte *)(extraout_EDX + 0xd) & 2) == 0) {
      if ((*(byte *)(extraout_EDX + 0xd) & 4) == 0) {
        *(undefined4 *)(extraout_EDX + 0x14) = 0x1000;
      }
      else {
        *(undefined4 *)(extraout_EDX + 0x14) = 1;
      }
    }
    else {
      *(undefined4 *)(extraout_EDX + 0x14) = 0x86;
    }
  }
  uVar3 = FUN_00041af8(*(undefined4 *)(extraout_EDX + 0x14));
  puVar2 = (undefined4 *)((ulonglong)uVar3 >> 0x20);
  puVar2[2] = (int)uVar3;
  if ((int)uVar3 == 0) {
    puVar2[5] = 1;
    bVar1 = *(byte *)((int)puVar2 + 0xd) & 0xf8;
    puVar2[2] = puVar2 + 6;
    *(byte *)((int)puVar2 + 0xd) = bVar1;
    *(byte *)((int)puVar2 + 0xd) = bVar1 | 4;
  }
  else {
    *(byte *)(puVar2 + 3) = *(byte *)(puVar2 + 3) | 8;
  }
  puVar2[1] = 0;
  *puVar2 = puVar2[2];
  return;
}


