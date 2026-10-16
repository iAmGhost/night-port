// func #475  FUN_000428e5  000428e5  size=18

void FUN_000428e5(int *param_1,undefined1 param_2)

{
  undefined1 *puVar1;
  
  puVar1 = (undefined1 *)*param_1;
  *param_1 = (int)(puVar1 + 1);
  *puVar1 = param_2;
  param_1[4] = param_1[4] + 1;
  return;
}
