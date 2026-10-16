// func #93  FUN_0003493e  0003493e  size=73


undefined8 FUN_0003493e(void)

{
  undefined4 extraout_EDX;
  byte local_20;
  
  FUN_000418ee();
  if (DAT_00063f50 == 0) {
    local_20 = DAT_00063f1d;
  }
  else {
    local_20 = (&DAT_00063f13)[DAT_00063f50];
  }
  return CONCAT44(extraout_EDX,(uint)local_20);
}


