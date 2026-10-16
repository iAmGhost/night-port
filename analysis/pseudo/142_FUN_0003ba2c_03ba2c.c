// func #142  FUN_0003ba2c  0003ba2c  size=90


void FUN_0003ba2c(void)

{
  int *piVar1;
  int aiStack_2c [3];
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  piVar1 = aiStack_2c + 2;
  for (aiStack_2c[2] = 0; aiStack_2c[2] < 200; aiStack_2c[2] = aiStack_2c[2] + 1) {
    *(undefined4 *)((int)piVar1 + -4) = 0;
    *(undefined4 *)((int)piVar1 + -8) = 0x3ba76;
    FUN_00034aba(0,aiStack_2c[2],0x140,(int)local_20);
    local_20 = CONCAT44(local_20._4_4_,(int)local_20 + local_20._4_4_);
    piVar1 = (int *)((int)piVar1 + -4);
  }
  return;
}


