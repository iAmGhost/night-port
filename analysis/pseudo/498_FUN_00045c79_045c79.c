// func #498  FUN_00045c79  00045c79  size=220

undefined8 FUN_00045c79(char *param_1,int *param_2,int param_3)

{
  byte bVar1;
  undefined4 *puVar2;
  undefined2 in_DS;
  undefined4 uStack_30;
  undefined4 uStack_2c;
  undefined4 local_28;
  uint uStack_24;
  uint uStack_20;
  undefined4 local_1c;
  undefined4 local_18;
  
  puVar2 = &local_1c;
  *(undefined4 *)(param_3 + 0xc) = 0;
  bVar1 = *(byte *)(param_3 + 0x15);
  uStack_24 = (uint)bVar1;
  uStack_2c = 0;
  if (bVar1 < 0x47) {
    puVar2 = &local_1c;
    if (bVar1 < 0x45) goto LAB_00045d4a;
    if (bVar1 < 0x46) goto LAB_00045cd3;
  }
  else {
    if (bVar1 < 0x48) {
LAB_00045cc3:
      if (*(int *)(param_3 + 8) == 0) {
        *(undefined4 *)(param_3 + 8) = 1;
      }
      uStack_24 = (uint)(byte)(bVar1 - 2);
    }
    else {
      if (0x65 < bVar1) {
        if (bVar1 < 0x67) goto LAB_00045cd8;
        puVar2 = &local_1c;
        if (bVar1 != 0x67) goto LAB_00045d4a;
        goto LAB_00045cc3;
      }
      if (bVar1 != 0x65) goto LAB_00045d4a;
    }
LAB_00045cd3:
    uStack_2c = 1;
  }
LAB_00045cd8:
  puVar2 = (undefined4 *)*param_2;
  *param_2 = (int)(puVar2 + 2);
  local_1c = *puVar2;
  local_18 = puVar2[1];
  if (*(int *)(param_3 + 8) == -1) {
    *(undefined4 *)(param_3 + 8) = 6;
  }
  uStack_20 = (uint)(*(byte *)(param_3 + 0x15) & 0x5f);
  local_28 = 3;
  uStack_30 = 0x27;
  FUN_00049e80(param_1,&local_1c,*(undefined4 *)(param_3 + 8),0);
  if (*param_1 == '*') {
    *(undefined1 *)(param_3 + 0x16) = 0x2a;
  }
  param_1[0x27] = '\0';
  for (; *param_1 == ' '; param_1 = param_1 + 1) {
  }
  FUN_00045d56(param_1,param_3);
  local_28 = CONCAT22(local_28._2_2_,in_DS);
  *(undefined4 *)(param_3 + 8) = 0xffffffff;
  puVar2 = &uStack_30;
LAB_00045d4a:
  return CONCAT44(*(undefined4 *)((int)puVar2 + 8),param_1);
}
