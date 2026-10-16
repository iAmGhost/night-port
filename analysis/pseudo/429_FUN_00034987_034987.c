// func #429  FUN_00034987  00034987  size=8

char FUN_00034987(char *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  char cVar1;
  
  cVar1 = (char)((uint)param_4 >> 8);
  param_1[0x24] = param_1[0x24] + cVar1;
  *param_1 = *param_1 + (char)param_1;
  return (char)param_1 + cVar1;
}
