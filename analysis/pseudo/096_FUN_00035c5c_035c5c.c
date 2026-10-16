// func #96  FUN_00035c5c  00035c5c  size=179


void FUN_00035c5c(void)

{
  char *pcVar1;
  char *pcVar2;
  
  FUN_000418ee();
  DAT_00053c78 = DAT_00053c70;
  pcVar1 = DAT_00053c78;
  while (DAT_00053c78 = pcVar1, DAT_00053c78 != (char *)0x0) {
    pcVar1 = *(char **)(DAT_00053c78 + 0x13f);
    if ((*DAT_00053c78 == '\x01') && (DAT_00053c78[0x30] != '\x01')) {
      DAT_00053c78[0x30] = '\x01';
      pcVar2 = DAT_00053c78;
      pcVar2[0x17] = '\x15';
      pcVar2[0x18] = '\0';
      pcVar2[0x19] = '\0';
      pcVar2[0x1a] = '\0';
      pcVar2 = DAT_00053c78;
      pcVar2[0x11] = '\0';
      pcVar2[0x12] = '\0';
      pcVar2[0x13] = '\0';
      pcVar2[0x14] = '\0';
      *(undefined4 *)(DAT_00053c78 + 0x32) =
           *(undefined4 *)(&DAT_00053ea8 + *(int *)(DAT_00053c78 + 0xd) * 0x88);
    }
  }
  return;
}


