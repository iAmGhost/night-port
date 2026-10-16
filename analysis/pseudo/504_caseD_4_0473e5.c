// func #504  caseD_4  000473e5  size=20

void switchD_000462f7::caseD_4(void)

{
  int unaff_EBP;
  int unaff_ESI;
  int unaff_EDI;
  
  FUN_000478f0(unaff_ESI + 0x1c + unaff_EBP,unaff_EDI + 0x1c + unaff_EBP,
               unaff_ESI + 0x1c + unaff_EBP);
                    /* WARNING: Could not recover jumptable at 0x000473f6. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(unaff_EBP + 0x76))();
  return;
}
