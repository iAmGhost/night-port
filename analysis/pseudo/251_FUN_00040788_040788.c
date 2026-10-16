// func #199  FUN_00040788  00040788  size=112


int FUN_00040788(void)

{
  int local_40;
  uint local_3c;
  int local_38;
  undefined4 local_30;
  int local_24;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  local_3c = 0x100;
  local_38 = (int)((ulonglong)local_20 >> 0x20) + 0xf >> 4;
  FUN_00041e51(0x31,&local_3c,&local_3c);
  if (local_24 == 0) {
    *(undefined4 *)local_20 = local_30;
    local_40 = (local_3c & 0xffff) << 4;
  }
  else {
    local_40 = 0;
  }
  return local_40;
}


