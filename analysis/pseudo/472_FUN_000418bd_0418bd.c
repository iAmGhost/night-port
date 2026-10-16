// func #472  FUN_000418bd  000418bd  size=20

void FUN_000418bd(int *param_1,int param_2)

{
  *(char *)param_1 = (char)*param_1 + (char)param_1;
  *(char *)(param_2 + -0x46) = *(char *)(param_2 + -0x46) + (char)param_2;
  *param_1 = (int)(*param_1 + (int)param_1);
  *(char *)param_1 = (char)*param_1 + (char)param_1;
  FUN_000429c8("undefined constructor or destructor called!");
  return;
}
