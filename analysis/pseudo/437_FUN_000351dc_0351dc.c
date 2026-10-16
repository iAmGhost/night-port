// func #437  FUN_000351dc  000351dc  size=134

uint FUN_000351dc(void)

{
  byte *pbVar1;
  int *piVar2;
  uint local_2c;
  byte *local_28;
  
  piVar2 = (int *)FUN_000418ee();
  local_2c = (uint)(*(byte *)*piVar2 & 0x7f);
  local_28 = (byte *)*piVar2;
  while (pbVar1 = local_28 + 1, (*local_28 & 0x80) != 0) {
    local_2c = local_2c << 7 | (uint)(*pbVar1 & 0x7f);
    local_28 = pbVar1;
  }
  *piVar2 = (int)pbVar1;
  return local_2c;
}
