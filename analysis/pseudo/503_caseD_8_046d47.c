// func #503  caseD_8  00046d47  size=20

void switchD_000462f7::caseD_8(void)

{
  int unaff_EBP;
  int unaff_ESI;
  int unaff_EDI;
  
  FUN_00047d24(unaff_EDI + 0x1c + unaff_EBP,unaff_ESI + 0x1c + unaff_EBP,
               unaff_EDI + 0x1c + unaff_EBP);
                    /* WARNING: Could not recover jumptable at 0x00046d58. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(unaff_EBP + 0x76))();
  return;
}
