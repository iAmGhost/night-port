// func #462  FUN_0003f077  0003f077  size=231

void FUN_0003f077(char *param_1,ushort *param_2,undefined2 *param_3,undefined2 *param_4)

{
  undefined1 uVar1;
  char cVar2;
  int unaff_ESI;
  undefined1 *unaff_EDI;
  undefined2 local_70 [2];
  undefined2 local_6c;
  undefined2 local_68;
  uint *local_5c;
  uint local_54 [4];
  uint local_44;
  uint local_40;
  uint local_3c;
  uint local_38;
  undefined2 local_32;
  undefined1 local_20 [12];
  char *local_14;
  ushort *local_10;
  undefined2 *local_c;
  undefined2 *local_8;
  
  cVar2 = (char)((uint)param_4 >> 8);
  param_1[0x7c] = param_1[0x7c] + cVar2;
  *param_1 = *param_1 + (char)param_1;
  cVar2 = (char)param_1 + cVar2;
  local_14 = (char *)CONCAT31((int3)((uint)param_1 >> 8),cVar2);
  uVar1 = in((short)param_2);
  *unaff_EDI = uVar1;
  *local_14 = *local_14 - cVar2;
  *(char *)(unaff_ESI + 0x57) = *(char *)(unaff_ESI + 0x57) + (char)param_2;
  local_10 = param_2;
  local_c = param_3;
  local_8 = param_4;
  FUN_00041df2(local_20);
  FUN_00041e20(local_54,0,0x32);
  local_38 = (uint)*local_10;
  local_44 = (uint)local_10[2];
  local_3c = (uint)local_10[4];
  local_40 = (uint)local_10[6];
  local_54[0] = (uint)local_10[10];
  local_32 = *local_8;
  local_70[0] = 0x300;
  local_6c = SUB42(local_14,0);
  local_68 = 0;
  local_5c = local_54;
  FUN_00041e38(0x31,local_70,local_70,local_20);
  *local_c = (short)local_38;
  local_c[2] = (short)local_44;
  local_c[4] = (short)local_3c;
  local_c[6] = (short)local_40;
  return;
}
