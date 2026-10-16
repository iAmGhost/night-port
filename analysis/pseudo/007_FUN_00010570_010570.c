// func #7  FUN_00010570  00010570  size=135


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00010570(void)

{
  undefined2 local_48 [6];
  undefined2 local_3c [2];
  ushort local_38;
  undefined2 local_34;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  if (_DAT_00053c58 != -1) {
    FUN_00041e6c(DAT_00053c5c + (int)((ulonglong)local_20 >> 0x20),(int)local_20,0xff);
    local_3c[0] = 0x1510;
    local_34 = DAT_00053c58;
    local_38 = (short)DAT_00053c5c + (short)((ulonglong)local_20 >> 0x20) & 0xf;
    local_48[0] = (undefined2)((uint)(DAT_00053c5c + local_20._4_4_) >> 4);
    FUN_000103e0(0x2f,local_3c,local_3c,local_48);
  }
  return;
}


