// func #136  FUN_0003af18  0003af18  size=1702


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0003af18(void)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  int *piVar4;
  int aiStack_70 [6];
  undefined1 local_58 [16];
  int local_48;
  int local_44;
  int local_40;
  undefined1 local_3c [4];
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  int local_2c;
  int local_28;
  int local_24;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  piVar4 = aiStack_70 + 3;
  local_30 = 0;
  local_34 = 0;
  local_38 = 0;
  FUN_0003ae8e();
  FUN_0003658d();
  FUN_0003690b();
  FUN_00010c3e();
  FUN_0004297a(local_3c);
  FUN_00041ec3();
LAB_0003af69:
  do {
    *(undefined4 *)((int)piVar4 + -4) = 0x3af6e;
    FUN_00038a20();
    *(undefined4 *)((int)piVar4 + -4) = 0x3af73;
    FUN_00038ae0();
    *(undefined4 *)((int)piVar4 + -4) = 0x3af78;
    local_40 = FUN_0003a224();
    if (local_40 == 1) {
      *(undefined4 *)((int)piVar4 + -4) = 0x3af86;
      FUN_00037306();
      return;
    }
  } while (local_40 == 3);
  *(undefined4 *)((int)piVar4 + -4) = 0x3af98;
  FUN_00037b03(0);
  DAT_00063354 = DAT_0006334c;
  DAT_00063350 = DAT_0006334c;
  DAT_00063398 = (uint)*(byte *)(DAT_0006334c + 0x11);
  *(undefined4 *)((int)piVar4 + -4) = 0x3afc8;
  local_28 = FUN_00041bdc(0x147);
  local_24 = local_28;
  if (local_28 == 0) {
    local_44 = 0;
  }
  else {
    *(undefined4 *)((int)piVar4 + -4) = 0;
    *(undefined4 *)((int)piVar4 + -8) = 0;
    uVar2 = *(undefined4 *)
             (&DAT_00052c3c + *(int *)(DAT_00063354 + 0x40) * 4 + (DAT_00063370 + -1) * 0x28);
    *(undefined4 *)((int)piVar4 + -0xc) = 0x3b007;
    local_44 = FUN_00010efc(local_28,0xffffffe2,0,uVar2);
    piVar4 = (int *)((int)piVar4 + -8);
    local_2c = local_44;
  }
  DAT_00053c7c = local_44;
  DAT_00053c84 = local_44;
  if (DAT_00063398 == 0) {
    *(int *)(local_44 + 1) = DAT_000639dc + -0x1e;
  }
  else {
    *(int *)(local_44 + 1) = DAT_000639dc + 0x15e;
  }
LAB_0003b05a:
  local_48 = 0;
  *(undefined4 *)((int)piVar4 + -4) = 0x3b066;
  iVar3 = FUN_0003f15f();
  if (iVar3 == 1) {
    if (*(int *)(&DAT_00052f2c + DAT_00063370 * 4) != 0) {
      *(undefined4 *)((int)piVar4 + -4) = 0x3b08e;
      FUN_00010b1d(0xb);
      *(undefined4 *)((int)piVar4 + -4) = 0x3b09f;
      FUN_00041e20(&DAT_000636b8,0);
      *(undefined **)((int)piVar4 + -4) = &DAT_000636b8;
      *(undefined4 *)((int)piVar4 + -8) = 0x100;
      *(undefined4 *)((int)piVar4 + -0xc) = 0;
      *(undefined4 *)((int)piVar4 + -0x10) = 0x3b0b3;
      FUN_00040042();
      do {
        *(int *)((int)piVar4 + -4) = local_48 + 1;
        *(int *)((int)piVar4 + -8) = DAT_00063370;
        *(char **)((int)piVar4 + -0xc) = "%ds%d.pcx";
        *(undefined1 **)((int)piVar4 + -0x10) = local_58;
        *(undefined4 *)((int)piVar4 + -0x14) = 0x3b0d0;
        FUN_000428f8();
        *(undefined4 *)((int)piVar4 + -4) = 0x3b0dd;
        FUN_0003628e(local_58,0);
        *(undefined4 *)((int)piVar4 + -4) = 0x3b0ec;
        FUN_00040660(&DAT_000636b8,0x19);
        *(undefined4 *)((int)piVar4 + -4) = 0x3b0f1;
        iVar3 = FUN_00035dfc();
        if (iVar3 == 1) {
          *(undefined4 *)((int)piVar4 + -4) = 0x3b105;
          FUN_000406f5(&DAT_000636b8,0x19);
          break;
        }
        *(undefined4 *)((int)piVar4 + -4) = 0x3b116;
        FUN_000406f5(&DAT_000636b8,0x19);
        local_48 = local_48 + 1;
      } while (local_48 < *(int *)(&DAT_00052f2c + DAT_00063370 * 4));
    }
    *(undefined4 *)((int)piVar4 + -4) = 0x3b135;
    FUN_00010dde();
    *(int *)((int)piVar4 + -4) = DAT_00063370;
    *(char **)((int)piVar4 + -8) = "stage%d.pcx";
    *(undefined1 **)((int)piVar4 + -0xc) = local_58;
    *(undefined4 *)((int)piVar4 + -0x10) = 0x3b14a;
    FUN_000428f8();
    *(undefined4 *)((int)piVar4 + -4) = 0x3b157;
    FUN_0003628e(local_58,0);
    *(undefined4 *)((int)piVar4 + -4) = 0x3b166;
    FUN_00040660(&DAT_000636b8,0x32);
    for (local_40 = 0; local_40 < 0xc00; local_40 = local_40 + 1) {
      *(undefined4 *)((int)piVar4 + -4) = 0x3b188;
      thunk_FUN_0004564f(1);
      if (DAT_00063e89 != '\0') goto LAB_0003b191;
    }
    goto LAB_0003b19e;
  }
  goto LAB_0003b1ad;
LAB_0003b191:
  do {
  } while (DAT_00063e89 != '\0');
LAB_0003b19e:
  *(undefined4 *)((int)piVar4 + -4) = 0x3b1ad;
  FUN_000406f5(&DAT_000636b8,0x32);
LAB_0003b1ad:
  *(undefined4 *)((int)piVar4 + -4) = 0x3b1b2;
  FUN_000405a7();
  while( true ) {
    *(undefined4 *)((int)piVar4 + -4) = 0x3b1b7;
    aiStack_70[5] = FUN_0003a829();
    *(undefined4 *)((int)piVar4 + -4) = 0x3b1bf;
    FUN_00010dde();
    if (aiStack_70[5] == 1) break;
    if (aiStack_70[5] != 2) {
      if (aiStack_70[5] != 0) {
        if (aiStack_70[5] == -1) {
          *(undefined4 *)((int)piVar4 + -4) = 0x3b597;
          FUN_00010dde();
          *(undefined4 *)((int)piVar4 + -4) = 0x3b59c;
          FUN_00037306();
          *(undefined4 *)((int)piVar4 + -4) = DAT_000633ac;
          *(undefined4 *)((int)piVar4 + -8) = DAT_000633a8;
          *(char **)((int)piVar4 + -0xc) = "\nThank You For Playing NIGHT\n";
          *(undefined4 *)((int)piVar4 + -0x10) = 0x3b5b3;
          FUN_00042394();
          return;
        }
        goto LAB_0003b05a;
      }
      DAT_00063370 = 1;
      *(undefined4 *)((int)piVar4 + -4) = 0x3b578;
      FUN_00037347();
      DAT_00063398 = 0;
      goto LAB_0003af69;
    }
    *(undefined4 *)((int)piVar4 + -4) = 0x3b55e;
    FUN_000376c8();
  }
  *(undefined4 *)((int)piVar4 + -4) = 0x3b1ce;
  FUN_0003ace9();
  if (DAT_00063370 == 9) {
    local_48 = 0;
    if (_UNK_00052fb4 == 0) goto LAB_0003b2b1;
    *(undefined4 *)((int)piVar4 + -4) = 0x3b1fc;
    iVar3 = FUN_0003f15f();
    if (iVar3 != 1) goto LAB_0003b2b1;
    *(undefined4 *)((int)piVar4 + -4) = 0x3b20f;
    FUN_00010b1d(0xc);
    *(undefined4 *)((int)piVar4 + -4) = 0x3b220;
    FUN_00041e20(&DAT_000636b8,0);
    *(undefined **)((int)piVar4 + -4) = &DAT_000636b8;
    *(undefined4 *)((int)piVar4 + -8) = 0x100;
    *(undefined4 *)((int)piVar4 + -0xc) = 0;
    *(undefined4 *)((int)piVar4 + -0x10) = 0x3b234;
    FUN_00040042();
    goto LAB_0003b237;
  }
  *(undefined4 *)((int)piVar4 + -4) = 0x3b44a;
  FUN_000406f5(DAT_00063354 + 0x48,0x32);
  local_48 = 0;
  if (*(int *)(&DAT_00052f90 + DAT_00063370 * 4) != 0) {
    *(undefined4 *)((int)piVar4 + -4) = 0x3b46b;
    iVar3 = FUN_0003f15f();
    if (iVar3 == 1) {
      *(undefined4 *)((int)piVar4 + -4) = 0x3b47e;
      FUN_00010b1d(0xc);
      *(undefined4 *)((int)piVar4 + -4) = 0x3b48f;
      FUN_00041e20(&DAT_000636b8,0);
      *(undefined **)((int)piVar4 + -4) = &DAT_000636b8;
      *(undefined4 *)((int)piVar4 + -8) = 0x100;
      *(undefined4 *)((int)piVar4 + -0xc) = 0;
      *(undefined4 *)((int)piVar4 + -0x10) = 0x3b4a3;
      FUN_00040042();
      do {
        *(int *)((int)piVar4 + -4) = local_48 + 1;
        *(int *)((int)piVar4 + -8) = DAT_00063370;
        *(char **)((int)piVar4 + -0xc) = "%de%d.pcx";
        *(undefined1 **)((int)piVar4 + -0x10) = local_58;
        *(undefined4 *)((int)piVar4 + -0x14) = 0x3b4c0;
        FUN_000428f8();
        *(undefined4 *)((int)piVar4 + -4) = 0x3b4cd;
        FUN_0003628e(local_58,0);
        *(undefined4 *)((int)piVar4 + -4) = 0x3b4dc;
        FUN_00040660(&DAT_000636b8,0x19);
        *(undefined4 *)((int)piVar4 + -4) = 0x3b4e1;
        iVar3 = FUN_00035dfc();
        if (iVar3 == 1) {
          *(undefined4 *)((int)piVar4 + -4) = 0x3b4f5;
          FUN_000406f5(&DAT_000636b8,0x19);
          break;
        }
        *(undefined4 *)((int)piVar4 + -4) = 0x3b506;
        FUN_000406f5(&DAT_000636b8,0x19);
        local_48 = local_48 + 1;
      } while (local_48 < *(int *)(&DAT_00052f90 + DAT_00063370 * 4));
    }
  }
  DAT_00063370 = DAT_00063370 + 1;
  *(undefined4 *)((int)piVar4 + -4) = 0x3b52b;
  FUN_00010dde();
  *(undefined4 *)((int)piVar4 + -4) = 0x3b530;
  FUN_000405a7();
  if (*(char *)(DAT_00053c7c + 0x30) != '\x01') {
    *(undefined4 *)((int)piVar4 + -4) = 0x3b54c;
    FUN_00038f3d(1);
  }
  *(undefined4 *)((int)piVar4 + -4) = 0x3b551;
  FUN_000374ad();
  goto LAB_0003b05a;
  while( true ) {
    *(undefined4 *)((int)piVar4 + -4) = 0x3b297;
    FUN_000406f5(&DAT_000636b8,0x19);
    local_48 = local_48 + 1;
    if (*(int *)(&DAT_00052f90 + DAT_00063370 * 4) <= local_48) break;
LAB_0003b237:
    *(int *)((int)piVar4 + -4) = local_48 + 1;
    *(int *)((int)piVar4 + -8) = DAT_00063370;
    *(char **)((int)piVar4 + -0xc) = "%de%d.pcx";
    *(undefined1 **)((int)piVar4 + -0x10) = local_58;
    *(undefined4 *)((int)piVar4 + -0x14) = 0x3b251;
    FUN_000428f8();
    *(undefined4 *)((int)piVar4 + -4) = 0x3b25e;
    FUN_0003628e(local_58,0);
    *(undefined4 *)((int)piVar4 + -4) = 0x3b26d;
    FUN_00040660(&DAT_000636b8,0x19);
    *(undefined4 *)((int)piVar4 + -4) = 0x3b272;
    iVar3 = FUN_00035dfc();
    if (iVar3 == 1) {
      *(undefined4 *)((int)piVar4 + -4) = 0x3b286;
      FUN_000406f5(&DAT_000636b8,0x19);
      break;
    }
  }
LAB_0003b2b1:
  local_48 = 0;
  *(undefined4 *)((int)piVar4 + -4) = 0x3b2bd;
  iVar3 = FUN_0003f15f();
  if (iVar3 == 1) {
    *(undefined4 *)((int)piVar4 + -4) = 0x3b2d0;
    FUN_00010b1d(10);
    *(undefined4 *)((int)piVar4 + -4) = 0x3b2e1;
    FUN_00041e20(&DAT_000636b8,0);
    *(undefined **)((int)piVar4 + -4) = &DAT_000636b8;
    *(undefined4 *)((int)piVar4 + -8) = 0x100;
    *(undefined4 *)((int)piVar4 + -0xc) = 0;
    *(undefined4 *)((int)piVar4 + -0x10) = 0x3b2f5;
    FUN_00040042();
    do {
      if ((((*(char *)(DAT_00053c7c + 0x25) == '\x01') && (*(char *)(DAT_00053c7c + 0x26) == '\x02')
           ) && (*(char *)(DAT_00053c7c + 0x27) == '\x04')) &&
         (*(char *)(DAT_00053c7c + 0x28) == '\x04')) {
        *(int *)((int)piVar4 + -4) = local_48 + 1;
        *(char **)((int)piVar4 + -8) = "2end%04d.pcx";
        *(undefined1 **)((int)piVar4 + -0xc) = local_58;
        *(undefined4 *)((int)piVar4 + -0x10) = 0x3b35a;
        FUN_000428f8();
      }
      else {
        *(int *)((int)piVar4 + -4) = local_48 + 1;
        *(char **)((int)piVar4 + -8) = "end%04d.pcx";
        *(undefined1 **)((int)piVar4 + -0xc) = local_58;
        *(undefined4 *)((int)piVar4 + -0x10) = 0x3b373;
        FUN_000428f8();
      }
      *(undefined4 *)((int)piVar4 + -4) = 0x3b380;
      FUN_0003628e(local_58,0);
      *(undefined4 *)((int)piVar4 + -4) = 0x3b38f;
      FUN_00040660(&DAT_000636b8,0x32);
      for (local_40 = 0; local_40 < 12000; local_40 = local_40 + 1) {
        *(undefined4 *)((int)piVar4 + -4) = 0x3b3b1;
        thunk_FUN_0004564f(1);
        if (DAT_00063ec3 != '\0') {
          *(undefined4 *)((int)piVar4 + -4) = 0x3b3c9;
          FUN_000406f5(&DAT_000636b8,0x32);
          break;
        }
      }
      *(undefined4 *)((int)piVar4 + -4) = 0x3b3dc;
      FUN_000406f5(&DAT_000636b8,0x32);
      iVar1 = local_48 + 1;
      iVar3 = local_48;
      local_48 = iVar1;
    } while (iVar1 < 10);
  }
  *(undefined4 *)((int)piVar4 + -4) = 0x3b3f1;
  FUN_000405a7(iVar3);
  DAT_00063370 = 1;
  aiStack_70[4] = DAT_00053c7c;
  if (DAT_00053c7c == 0) {
    aiStack_70[3] = 0;
  }
  else {
    *(undefined4 *)((int)piVar4 + -4) = 0x3b413;
    FUN_000112fd(DAT_00053c7c,0);
    *(undefined4 *)((int)piVar4 + -4) = 0x3b418;
    aiStack_70[3] = FUN_00041c28();
  }
  *(undefined4 *)((int)piVar4 + -4) = 0x3b429;
  FUN_00037347();
  DAT_00063398 = 0;
  goto LAB_0003af69;
}


