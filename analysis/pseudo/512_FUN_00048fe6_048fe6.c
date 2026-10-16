// func #512  FUN_00048fe6  00048fe6  size=201

undefined4 FUN_00048fe6(int param_1)

{
  code *pcVar1;
  char cVar2;
  short sVar3;
  undefined1 extraout_DL;
  undefined4 extraout_EDX;
  undefined4 *puVar4;
  undefined2 in_ES;
  undefined2 in_CS;
  byte in_CR0;
  undefined2 auStack_1c [2];
  undefined1 auStack_14 [4];
  
  pcVar1 = (code *)swi(0x2f);
  cVar2 = (*pcVar1)(0x1600);
  puVar4 = (undefined4 *)&stack0xffffffe8;
  if ((cVar2 != '\0') && (puVar4 = (undefined4 *)&stack0xffffffe8, cVar2 != -0x80)) {
    pcVar1 = (code *)swi(0x2f);
    sVar3 = (*pcVar1)(64000,0,0,0);
    puVar4 = (undefined4 *)auStack_14;
    if (sVar3 == 0x666) {
      pcVar1 = (code *)swi(0x2f);
      sVar3 = (*pcVar1)(0xfa22,0,0,0);
      puVar4 = (undefined4 *)&stack0xfffffff0;
      if (sVar3 == 0) {
        DAT_000536c9 = 1;
        pcVar1 = (code *)swi(0x2f);
        (*pcVar1)(0xfa1e,0,0,0);
        pcVar1 = (code *)swi(0x2f);
        (*pcVar1)(0xfa20,in_CS,&DAT_00064010,6);
        return 1;
      }
    }
  }
  if ((param_1 != 0) || ((short)*puVar4 != 0)) {
    cVar2 = '\0';
    *(undefined2 *)(puVar4 + -1) = in_ES;
    pcVar1 = (code *)swi(0x31);
    (*pcVar1)(0xa00);
    if (cVar2 == '\0') {
      *puVar4 = 0x4907d;
      FUN_00042361(7,extraout_EDX,&LAB_000461a8,in_CS);
      *(undefined1 *)(param_1 + 0x3e) = 1;
      *(byte *)(param_1 + 0x40) = *(byte *)(param_1 + 0x40) | 4;
      pcVar1 = (code *)swi(0x21);
      (*pcVar1)(CONCAT11(0x30,in_CR0 | 4),1);
      DAT_000536c8 = extraout_DL;
    }
  }
  return 0;
}
