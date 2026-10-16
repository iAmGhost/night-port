// func #358  caseD_3  000473bf  size=38


void switchD_0004738b::caseD_3(void)

{
  undefined2 *unaff_EBP;
  
  *unaff_EBP = 0x33f;
  unaff_EBP[2] = 0;
  unaff_EBP[4] = 0xffff;
  *(code **)(unaff_EBP + 0x3b) = FUN_000461cc;
  FUN_000461cc();
  return;
}


