// func #155  FUN_0003d244  0003d244  size=672


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0003d244(void)

{
  int iVar1;
  int *piVar2;
  int iStack_2c;
  undefined4 local_28;
  undefined4 local_24;
  int local_20;
  
  local_20 = FUN_000418ee();
  piVar2 = &local_28;
  iStack_2c = 0x3d264;
  FUN_0003237b();
  local_28 = DAT_00063e24;
  local_24 = DAT_00063e28;
  iStack_2c = 0x3d286;
  FUN_000406f5(DAT_00063354 + 0x48,0x32);
  if (*(int *)(local_20 + 0x34c) == 0) {
    DAT_00063354 = DAT_00063350;
  }
  else {
    DAT_00063354 = *(int *)(local_20 + 0x34c);
  }
  iStack_2c = 0x3d2b6;
  FUN_00038f3d(1);
  iStack_2c = 0x3d2c0;
  FUN_0003cad3(DAT_00063354);
  if (DAT_00063398 == 0) {
    *(int *)(DAT_00053c7c + 1) = DAT_000639dc + -0x1e;
  }
  else {
    *(int *)(DAT_00053c7c + 1) = DAT_000639dc + 0x15e;
  }
  if ((&DAT_00052be0)[*(int *)(DAT_00063354 + 0x40) + (DAT_00063370 + -1) * 10] == '\x01') {
    if (DAT_00063398 == 0) {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) + 0x78;
    }
    else {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) + -0x78;
    }
  }
  *(int *)(DAT_00053c7c + 9) =
       DAT_000639e0 +
       *(int *)(&DAT_00052c3c + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28);
  *(undefined4 *)(DAT_00053c7c + 5) = 0;
  *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
  *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
  *(int *)(DAT_00053c7c + 0x1b) = DAT_00063398;
  iVar1 = DAT_00063360 / 10;
  if (DAT_00063360 % 10 == 0) {
    DAT_00063360 = DAT_00063360 + 1;
  }
  iStack_2c = 0x3d3b1;
  FUN_0003ed6f(iVar1);
  iStack_2c = 0x3d3bb;
  FUN_0003801b(DAT_000633b0);
  iStack_2c = 0x3d3dc;
  FUN_00010b1d(*(undefined4 *)
                (&DAT_00052da4 + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28));
  iStack_2c = 0x3d3ee;
  FUN_00040660(DAT_00063354 + 0x48,0x32);
  DAT_00063358 = DAT_00063354;
  DAT_00063374 = *(undefined4 *)(DAT_00053c7c + 0x2b);
  _DAT_00063384 = (uint)*(byte *)(DAT_00053c7c + 0x28);
  DAT_00063378 = (uint)*(byte *)(DAT_00053c7c + 0x27);
  DAT_0006337c = (uint)*(byte *)(DAT_00053c7c + 0x26);
  _DAT_00063380 = (uint)*(byte *)(DAT_00053c7c + 0x25);
  DAT_00063e28 = 0;
  DAT_00063e24 = 0;
  if ((&DAT_00052be0)[*(int *)(DAT_00063354 + 0x40) + (DAT_00063370 + -1) * 10] == '\0') {
    piVar2 = &iStack_2c;
    iStack_2c = DAT_00063398;
    FUN_000381d7(DAT_000633b0,1,0,0x78);
  }
  *(undefined4 *)((int)piVar2 + -4) = 0x3d4b0;
  FUN_00041e20(&DAT_00062c58,0,0x80);
  *(undefined4 *)((int)piVar2 + -4) = 0x3d4c1;
  FUN_00041e20(&DAT_00062cd8,0,0x80);
  DAT_00063e24 = local_28;
  DAT_00063e28 = local_24;
  DAT_000639bc = 0;
  return;
}


