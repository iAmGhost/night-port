// func #466  FUN_0004000d  0004000d  size=1

void FUN_0004000d(char *param_1,char param_2,undefined4 param_3,char param_4)

{
  char cVar1;
  char *pcVar2;
  int unaff_EBP;
  int unaff_EDI;
  
  cVar1 = (char)param_1;
  *param_1 = *param_1 + cVar1;
  *param_1 = *param_1 + cVar1;
  *param_1 = *param_1 + cVar1;
  *param_1 = *param_1 + cVar1;
  *param_1 = *param_1 + cVar1;
  *param_1 = *param_1 + cVar1;
  *param_1 = *param_1 + cVar1;
  *param_1 = *param_1 + cVar1;
  *param_1 = *param_1 + cVar1;
  *param_1 = *param_1 + cVar1;
  *param_1 = *param_1 + cVar1;
  pcVar2 = (char *)CONCAT31((int3)((uint)param_1 >> 8),cVar1 + param_4);
  *pcVar2 = *pcVar2 + cVar1 + param_4;
  *(char *)(unaff_EDI + -0x75) = *(char *)(unaff_EDI + -0x75) + param_2;
  *(char *)(*(int *)(unaff_EBP + 9) + ((uint)pcVar2 & 0x40010) + *(int *)(unaff_EBP + 0xd)) =
       (char)*(undefined4 *)(unaff_EBP + 0x15);
  return;
}
