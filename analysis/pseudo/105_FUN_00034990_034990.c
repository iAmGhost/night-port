// func #?  FUN_00034990  034990  (recovered)

void FUN_00034990(char *param_1)

{
  int iVar1;
  undefined4 *puVar2;
  int local_1c;
  
  *param_1 = *param_1 + (char)param_1;
  DAT_00062d60 = thunk_FUN_00041bdc(110000);
  iVar1 = DAT_00062d60;
  DAT_00062d64 = DAT_00062d60;
  for (local_1c = 0; local_1c < 200; local_1c = local_1c + 1) {
    puVar2 = (undefined4 *)FUN_00034a87(iVar1);
    *(undefined2 *)(puVar2 + 2) = 0x140;
    *puVar2 = &DAT_000a0000;
    puVar2[1] = 0;
    *(undefined4 **)(&DAT_00062d68 + local_1c * 4) = puVar2;
    iVar1 = local_1c;
  }
  return;
}

