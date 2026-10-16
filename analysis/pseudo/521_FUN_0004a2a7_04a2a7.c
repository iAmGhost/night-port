// func #521  FUN_0004a2a7  0004a2a7  size=131

undefined8
FUN_0004a2a7(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,uint param_6,undefined4 *param_7)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined2 local_10;
  undefined2 uStack_e;
  undefined2 local_c;
  
  local_c = (short)param_6;
  if (((ushort)(param_6 >> 0x10) & 0x7ff0) == 0x7ff0) {
    puVar1 = param_7;
    if ((param_6 & 0x80000000) != 0) {
      puVar1 = (undefined4 *)((int)param_7 + 1);
      *(undefined1 *)param_7 = 0x2d;
    }
    uStack_e = (short)((uint)param_5 >> 0x10);
    local_10 = (short)param_5;
    if (((local_10 == 0 && uStack_e == 0) && local_c == 0) && (param_6 & 0xf0000) == 0) {
      uVar2 = 0x666e69;
    }
    else {
      uVar2 = 0x6e616e;
    }
    *puVar1 = uVar2;
    uVar2 = 1;
  }
  else {
    uVar2 = 0;
  }
  return CONCAT44(param_2,uVar2);
}
