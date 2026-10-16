// func #357  caseD_0  00046d32  size=21


void switchD_000462f7::caseD_0(void)

{
  int unaff_EBP;
  int unaff_ESI;
  int unaff_EDI;
  
  FUN_000478f0(unaff_EDI + 0x1c + unaff_EBP,unaff_ESI + 0x1c + unaff_EBP,
               unaff_EDI + 0x1c + unaff_EBP);
                    /* WARNING: Could not recover jumptable at 0x00046d43. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(unaff_EBP + 0x76))();
  return;
}


