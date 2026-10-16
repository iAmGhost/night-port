// func #24  FUN_00013aad  00013aad  size=416


void FUN_00013aad(void)

{
  char *pcVar1;
  char *local_30;
  int local_28;
  int local_24;
  
  FUN_000418ee();
  for (local_28 = 0; local_28 < DAT_00053c68 + -1; local_28 = local_28 + 1) {
    local_30 = DAT_00053c70;
    for (local_24 = 0; local_24 < (DAT_00053c68 - local_28) + -1; local_24 = local_24 + 1) {
      if (*(int *)(local_30 + 9) < *(int *)(*(int *)(local_30 + 0x13f) + 9)) {
        local_30 = *(char **)(local_30 + 0x13f);
      }
      else {
        pcVar1 = *(char **)(local_30 + 0x13f);
        if ((pcVar1 == DAT_00053c7c) && (*(int *)(pcVar1 + 9) == *(int *)(local_30 + 9))) {
          local_30 = *(char **)(local_30 + 0x13f);
        }
        else if ((*pcVar1 == *local_30) && (*(int *)(pcVar1 + 9) == *(int *)(local_30 + 9))) {
          local_30 = *(char **)(local_30 + 0x13f);
        }
        else {
          if (*(int *)(pcVar1 + 0x13f) != 0) {
            *(char **)(*(int *)(pcVar1 + 0x13f) + 0x143) = local_30;
          }
          if (*(int *)(local_30 + 0x143) != 0) {
            *(char **)(*(int *)(local_30 + 0x143) + 0x13f) = pcVar1;
          }
          *(undefined4 *)(local_30 + 0x13f) = *(undefined4 *)(pcVar1 + 0x13f);
          *(undefined4 *)(pcVar1 + 0x143) = *(undefined4 *)(local_30 + 0x143);
          *(char **)(local_30 + 0x143) = pcVar1;
          *(char **)(pcVar1 + 0x13f) = local_30;
          if (*(int *)(pcVar1 + 0x143) == 0) {
            DAT_00053c70 = pcVar1;
          }
          if (*(int *)(local_30 + 0x13f) == 0) {
            DAT_00053c74 = local_30;
          }
        }
      }
    }
  }
  return;
}


