// func #133  FUN_0003a829  0003a829  size=1216


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0003a829(void)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 extraout_EDX;
  undefined **ppuVar3;
  undefined *puStack_28;
  undefined4 local_24;
  int local_20;
  
  FUN_000418ee();
  ppuVar3 = (undefined **)&local_24;
  local_20 = 0;
  DAT_000639bc = 0;
  DAT_00053104 = 0x46;
  puStack_28 = (undefined *)0x3a87d;
  FUN_00010b1d(*(undefined4 *)
                (&DAT_00052da4 + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28));
  puStack_28 = (undefined *)0x3a882;
  FUN_0003b9b7();
  *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
  DAT_00063374 = *(undefined4 *)(DAT_00053c7c + 0x2b);
  DAT_00063378 = (uint)*(byte *)(DAT_00053c7c + 0x27);
  DAT_0006337c = (uint)*(byte *)(DAT_00053c7c + 0x26);
  _DAT_00063380 = (uint)*(byte *)(DAT_00053c7c + 0x25);
  _DAT_00063384 = (uint)*(byte *)(DAT_00053c7c + 0x28);
  puStack_28 = (undefined *)0x3a8f4;
  FUN_00041e20(&DAT_000633b8,0,0x300);
  puStack_28 = &DAT_000633b8;
  FUN_00040042();
  DAT_00063360 = 1;
  if ((&DAT_00052b84)[*(int *)(DAT_00063354 + 0x40) + (DAT_00063370 + -1) * 10] == '\x01') {
    if (DAT_00063398 == (undefined *)0x0) {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) + 0x78;
    }
    else {
      *(int *)(DAT_00053c7c + 1) = *(int *)(DAT_00053c7c + 1) + -0x78;
    }
  }
  puStack_28 = (undefined *)0x3a95a;
  FUN_0003ed6f();
  puStack_28 = (undefined *)0x3a964;
  FUN_0003801b(DAT_000633b0);
  puStack_28 = (undefined *)0x3a976;
  FUN_00040660(DAT_00063354 + 0x48,0x32);
  DAT_0006336c = 0;
  DAT_00063360 = 0;
  DAT_00063e28 = 0;
  DAT_00063e24 = 0;
  if ((&DAT_00052b84)[*(int *)(DAT_00063354 + 0x40) + (DAT_00063370 + -1) * 10] == '\0') {
    ppuVar3 = &puStack_28;
    puStack_28 = DAT_00063398;
    FUN_000381d7(DAT_000633b0,1,0,0x78);
  }
  DAT_0006336c = 0;
  DAT_00063360 = 0;
  DAT_00063e28 = 0;
  DAT_00063e24 = 0;
  *(undefined4 *)((int)ppuVar3 + -4) = 0x3aa09;
  FUN_000348e2();
  *(undefined4 *)((int)ppuVar3 + -4) = 0x3aa1a;
  FUN_00041e20(&DAT_00062c58,0,0x80);
  *(undefined4 *)((int)ppuVar3 + -4) = 0x3aa2b;
  FUN_00041e20(&DAT_00062cd8,0,0x80);
  while( true ) {
    if (((((local_20 == 0) && (DAT_00053c80 != 0)) && (DAT_000639e0 <= *(int *)(DAT_00053c80 + 9)))
        && ((*(int *)(DAT_00053c80 + 9) < DAT_000639e0 + 200 &&
            (DAT_000639dc + 0x32 <= *(int *)(DAT_00053c80 + 1))))) &&
       (*(int *)(DAT_00053c80 + 1) <= DAT_000639dc + 0x10e)) {
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3aa94;
      FUN_0003a70e();
      local_20 = 1;
    }
    if ((DAT_00063e89 != '\0') && (*(char *)(DAT_00053c7c + 0x30) != '\x01')) {
      do {
      } while (DAT_00063e89 != '\0');
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3aac6;
      iVar2 = FUN_00039c03();
      if (iVar2 == 0) {
        local_24 = 0xffffffff;
        goto LAB_0003acdd;
      }
    }
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3aadb;
    FUN_000322f5();
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3aae0;
    FUN_0003209c();
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3aae5;
    FUN_000347e3();
    if ((*(int *)(DAT_00053c7c + 0x17) == 0x66) || (*(int *)(DAT_00053c7c + 0x17) == 0x70)) {
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab13;
      FUN_0003272f(DAT_00062a9c);
    }
    else {
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab07;
      FUN_0003272f(DAT_00062a98);
    }
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab18;
    FUN_000320f5();
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab22;
    FUN_0003d4e4(DAT_00063354);
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab27;
    FUN_0003ed6f();
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab31;
    FUN_00038098(DAT_000633b0);
    if (DAT_00063364 != 0) {
      if (DAT_00063368 == 0) {
        uVar1 = *(undefined4 *)(&DAT_00053d08 + DAT_00063364 * 4);
        *(undefined4 *)((int)ppuVar3 + -4) = 0x3ab56;
        FUN_0004185e(uVar1);
        DAT_00063364 = 0;
      }
      else {
        DAT_00063368 = DAT_00063368 + -1;
      }
    }
    DAT_00063360 = DAT_00063360 + 1;
    if (*(int *)(DAT_00053c7c + 0x17) == 1) {
      DAT_0006339c = DAT_0006339c + 1;
    }
    else {
      DAT_0006339c = 0;
    }
    iVar2 = DAT_00063360 / 10;
    if (DAT_00063360 % 10 == 0) {
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3abae;
      iVar2 = FUN_0003d606(DAT_00063354);
    }
    if (DAT_0006336c == 1) break;
    if (DAT_0006336c == 3) {
      DAT_0006336c = 0;
      local_24 = 1;
LAB_0003acdd:
      return CONCAT44(extraout_EDX,local_24);
    }
    if (DAT_0006336c == 2) {
      *(undefined **)((int)ppuVar3 + -4) = DAT_00063398;
      *(undefined4 *)((int)ppuVar3 + -8) = 0x3ac4f;
      FUN_000381d7(DAT_000633b0,1,0,0x50);
      DAT_00063398 = (undefined *)(uint)*(byte *)(*(int *)(DAT_00063354 + 0x34c) + 0x11);
      DAT_0006336c = 0;
      *(undefined4 *)((int)ppuVar3 + -8) = 0x3ac7b;
      iVar2 = FUN_0003d244(DAT_00063354);
      ppuVar3 = (undefined **)((int)ppuVar3 + -4);
    }
    else if (DAT_0006336c == 4) {
      DAT_00063398 = (undefined *)(uint)*(byte *)(*(int *)(DAT_00063354 + 0x34c) + 0x11);
      DAT_0006336c = 0;
      *(undefined4 *)((int)ppuVar3 + -4) = 0x3acb2;
      iVar2 = FUN_0003d244(DAT_00063354);
    }
    if (DAT_00053c68 == 1) {
      *(undefined4 *)(DAT_00063354 + 0x32) = 0;
      iVar2 = DAT_00063354;
      *(undefined4 *)(DAT_00063354 + 0x36) = 0;
    }
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3acd8;
    FUN_00010d47(iVar2);
  }
  DAT_0006336c = 0;
  if (DAT_00053100 == 0) {
    DAT_00053100 = 3;
    local_24 = 0;
  }
  else {
    *(undefined4 *)((int)ppuVar3 + -4) = 0x3abcf;
    iVar2 = FUN_00038445();
    if (iVar2 == 0) {
      DAT_00053100 = 3;
      local_24 = 0;
    }
    else {
      local_24 = 2;
    }
  }
  goto LAB_0003acdd;
}


