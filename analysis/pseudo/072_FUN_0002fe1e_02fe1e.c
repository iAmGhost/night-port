// func #72  FUN_0002fe1e  0002fe1e  size=7266


/* WARNING: Removing unreachable block (ram,0x00031a31) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0002fe1e(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int **ppiVar4;
  float10 fVar5;
  undefined8 uVar6;
  int *piStack_8c;
  int *piStack_88;
  undefined1 local_84 [4];
  int local_80;
  undefined4 local_7c;
  undefined4 local_78;
  int local_74;
  int local_70;
  int local_6c;
  int local_68;
  int local_64;
  int local_60;
  int local_5c;
  uint local_58;
  float local_54;
  int local_50;
  int local_4c;
  int local_48;
  int local_44;
  int local_40;
  int local_3c;
  int local_38;
  int local_34;
  uint local_30;
  int local_2c;
  int local_28;
  int local_24;
  undefined8 local_20;
  
  local_20 = FUN_000418ee();
  piStack_88 = (int *)0x2fe45;
  local_4c = FUN_0001519f(DAT_00053c7c);
  piStack_88 = (int *)0x2fe52;
  local_48 = FUN_000151d5(DAT_00053c7c);
  if ((char *)local_20 == (char *)DAT_00053c7c) {
    return;
  }
  if ((*(int *)((int)(char *)local_20 + 0x1f) < 10) ||
     (0x13 < *(int *)((int)(char *)local_20 + 0x1f))) {
    local_2c = *(int *)(&DAT_00053ea0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  else {
    local_2c = *(int *)(&DAT_00053f1c + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  if (*(int *)((int)(char *)local_20 + 0x6f) == 1) {
    local_2c = *(int *)(&DAT_00053f1c + *(int *)((int)(char *)local_20 + 0xd) * 0x88);
  }
  fVar5 = (float10)*(float *)(&DAT_00051d5c + local_2c * 4);
  piStack_88 = (int *)0x2fecb;
  FUN_00041f7e();
  local_44 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)((int)(char *)local_20 + 99) +
          (float10)*(float *)(&DAT_00051d5c + local_2c * 4);
  piStack_88 = (int *)0x2fee5;
  FUN_00041f7e();
  local_50 = (int)ROUND(fVar5);
  fVar5 = (float10)*(float *)((int)(char *)local_20 + 99);
  piStack_88 = (int *)0x2fef3;
  FUN_00041f7e();
  local_54 = (float)(int)ROUND(fVar5);
  if ((int)local_54 + local_44 < local_50) {
    local_44 = local_44 + 1;
  }
  *(float *)((int)(char *)local_20 + 99) =
       *(float *)(&DAT_00051d5c + local_2c * 4) + *(float *)((int)(char *)local_20 + 99);
  local_40 = 0;
  local_3c = 0;
  if ((((((*(int *)(DAT_00053c7c + 0x17) != 0x15) && (*(int *)(DAT_00053c7c + 0x17) != 0x16)) &&
        (*(int *)(DAT_00053c7c + 0x17) != 0x17)) &&
       ((*(int *)(DAT_00053c7c + 0x17) != 0x18 && (*(int *)(DAT_00053c7c + 0x17) != 0x1a)))) &&
      (*(int *)(DAT_00053c7c + 0x17) != 0x1d)) &&
     ((*(int *)(DAT_00053c7c + 0x17) != 0x1e && (*(int *)(DAT_00053c7c + 0x17) != 0x20)))) {
    piStack_88 = (int *)0x2ff98;
    local_38 = FUN_00016750((char *)local_20);
    if (local_38 != 0) {
      *(int *)((int)(char *)local_20 + 0x10f) = local_38;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 10;
    }
  }
  if ((int)*(short *)((int)(char *)local_20 + 0x23) + *(int *)((int)(char *)local_20 + 0x56) +
      (int)*(short *)((int)(char *)local_20 + 0x29) != *(int *)((int)(char *)local_20 + 0x77)) {
    *(int *)((int)(char *)local_20 + 0x7b) =
         *(int *)((int)(char *)local_20 + 0x77) -
         ((int)*(short *)((int)(char *)local_20 + 0x23) + *(int *)((int)(char *)local_20 + 0x56) +
         (int)*(short *)((int)(char *)local_20 + 0x29));
    *(int *)((int)(char *)local_20 + 0x77) =
         *(int *)((int)(char *)local_20 + 0x56) + (int)*(short *)((int)(char *)local_20 + 0x23) +
         (int)*(short *)((int)(char *)local_20 + 0x29);
  }
  if ((((*(uint *)((int)(char *)local_20 + 0x17) < 0x15) ||
       (0x18 < *(uint *)((int)(char *)local_20 + 0x17))) &&
      (*(int *)((int)(char *)local_20 + 0x17) != 0x1a)) &&
     (((*(int *)((int)(char *)local_20 + 0x17) != 0x1d &&
       (*(int *)((int)(char *)local_20 + 0x17) != 0x1e)) &&
      (*(int *)((int)(char *)local_20 + 0x17) != 0x20)))) {
    if (200 < *(int *)((int)(char *)local_20 + 0x73) -
              ((int)*(short *)((int)(char *)local_20 + 0x23) +
               *(int *)((int)(char *)local_20 + 0x56) +
              (int)*(short *)((int)(char *)local_20 + 0x29))) {
      *(int *)((int)(char *)local_20 + 0x73) =
           *(int *)((int)(char *)local_20 + 0x56) + (int)*(short *)((int)(char *)local_20 + 0x23) +
           (int)*(short *)((int)(char *)local_20 + 0x29);
      if (10 < *(int *)((int)(char *)local_20 + 0x7b)) {
        return;
      }
      *(undefined4 *)((int)(char *)local_20 + 0x17) = 0x2f;
      *(undefined4 *)((int)(char *)local_20 + 0x11) = 0;
      return;
    }
  }
  else {
    *(int *)((int)(char *)local_20 + 0x73) =
         *(int *)((int)(char *)local_20 + 0x56) + (int)*(short *)((int)(char *)local_20 + 0x23) +
         (int)*(short *)((int)(char *)local_20 + 0x29);
  }
  if ((*(int *)((int)(char *)local_20 + 0x17) != 2) && (*(int *)((int)(char *)local_20 + 0x17) != 1)
     ) {
    if (*(int *)((int)(char *)local_20 + 0x17) == 0x2e) {
      if (*(int *)((int)(char *)local_20 + 0x11) == 1) {
        piStack_88 = (int *)0x30115;
        uVar3 = FUN_0001519f((char *)local_20);
        *(undefined4 *)((int)(char *)local_20 + 0xd3) = uVar3;
        piStack_88 = (int *)0x30128;
        uVar3 = FUN_000151d5((char *)local_20);
        *(undefined4 *)((int)(char *)local_20 + 0xd7) = uVar3;
        *(int *)((int)(char *)local_20 + 0xcb) = local_4c;
        *(int *)((int)(char *)local_20 + 0xcf) = local_48;
        piStack_88 = (int *)0x30153;
        iVar2 = FUN_0001519f((char *)local_20);
        if (*(int *)((int)(char *)local_20 + 0xcb) == iVar2) {
          *(undefined4 *)((int)(char *)local_20 + 0xdb) = 0x47c35000;
        }
        else {
          piStack_88 = (int *)0x3017b;
          iVar2 = FUN_000151d5((char *)local_20);
          local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcf) - iVar2);
          piStack_88 = (int *)0x30199;
          iVar2 = FUN_0001519f((char *)local_20);
          *(float *)((int)(char *)local_20 + 0xdb) =
               local_54 / (float)(*(int *)((int)(char *)local_20 + 0xcb) - iVar2);
        }
        *(float *)((int)(char *)local_20 + 0xe3) =
             (float)*(int *)((int)(char *)local_20 + 0xcf) -
             (float)*(int *)((int)(char *)local_20 + 0xcb) *
             *(float *)((int)(char *)local_20 + 0xdb);
        return;
      }
      if (*(int *)((int)(char *)local_20 + 0x11) < 2) {
        return;
      }
      if (0x1e < *(int *)((int)(char *)local_20 + 0x11)) {
        return;
      }
      piStack_88 = (int *)0x30211;
      iVar2 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcb) -
                           *(int *)((int)(char *)local_20 + 0xd3));
      piStack_88 = (int *)0x3022a;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcf) -
                           *(int *)((int)(char *)local_20 + 0xd7));
      if (iVar1 < iVar2) {
        local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcb) -
                          *(int *)((int)(char *)local_20 + 0xd3));
        fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd3) +
                (float10)*(int *)((int)(char *)local_20 + 0x11) *
                ((float10)(int)local_54 / (float10)_DAT_0005009c);
        piStack_88 = (int *)0x30268;
        FUN_00041f7e();
        local_28 = (int)ROUND(fVar5);
        local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcb) -
                          *(int *)((int)(char *)local_20 + 0xd3));
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 0xe3) +
                (float10)*(float *)((int)(char *)local_20 + 0xdb) *
                ((float10)*(int *)((int)(char *)local_20 + 0xd3) +
                (float10)*(int *)((int)(char *)local_20 + 0x11) *
                ((float10)(int)local_54 / (float10)_DAT_0005009c));
        piStack_88 = (int *)0x302b7;
        FUN_00041f7e();
        local_24 = (int)ROUND(fVar5);
      }
      else {
        local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcf) -
                          *(int *)((int)(char *)local_20 + 0xd7));
        fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd7) +
                (float10)*(int *)((int)(char *)local_20 + 0x11) *
                ((float10)(int)local_54 / (float10)_DAT_000500a4);
        piStack_88 = (int *)0x302f5;
        FUN_00041f7e();
        local_24 = (int)ROUND(fVar5);
        if ((*(uint *)((int)(char *)local_20 + 0xdb) & 0x7fffffff) == 0) {
          local_28 = 100;
        }
        else {
          local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcf) -
                            *(int *)((int)(char *)local_20 + 0xd7));
          fVar5 = (((float10)*(int *)((int)(char *)local_20 + 0xd7) +
                   (float10)*(int *)((int)(char *)local_20 + 0x11) *
                   ((float10)(int)local_54 / (float10)_DAT_000500ac)) -
                  (float10)*(float *)((int)(char *)local_20 + 0xe3)) /
                  (float10)*(float *)((int)(char *)local_20 + 0xdb);
          piStack_88 = (int *)0x3035c;
          FUN_00041f7e();
          local_28 = (int)ROUND(fVar5);
        }
      }
      piStack_88 = (int *)0x30367;
      iVar2 = FUN_0001519f((char *)local_20);
      piStack_88 = (int *)0x30374;
      FUN_0001545e((char *)local_20,local_28 - iVar2);
      piStack_88 = (int *)0x3037c;
      iVar2 = FUN_000151d5((char *)local_20);
      piStack_88 = (int *)0x30389;
      FUN_0001586b((char *)local_20,local_24 - iVar2);
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x17) == 0x31) {
      if (*(int *)((int)(char *)local_20 + 0x11) == 1) {
        piStack_88 = (int *)0x303b0;
        uVar3 = FUN_0001519f((char *)local_20);
        *(undefined4 *)((int)(char *)local_20 + 0xd3) = uVar3;
        piStack_88 = (int *)0x303c3;
        uVar3 = FUN_000151d5((char *)local_20);
        *(undefined4 *)((int)(char *)local_20 + 0xd7) = uVar3;
        *(int *)((int)(char *)local_20 + 0xcb) = local_4c;
        *(int *)((int)(char *)local_20 + 0xcf) = local_48;
        piStack_88 = (int *)0x303ee;
        iVar2 = FUN_0001519f((char *)local_20);
        if (*(int *)((int)(char *)local_20 + 0xcb) == iVar2) {
          *(undefined4 *)((int)(char *)local_20 + 0xdb) = 0x47c35000;
        }
        else {
          piStack_88 = (int *)0x30416;
          iVar2 = FUN_000151d5((char *)local_20);
          local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcf) - iVar2);
          piStack_88 = (int *)0x30434;
          iVar2 = FUN_0001519f((char *)local_20);
          *(float *)((int)(char *)local_20 + 0xdb) =
               local_54 / (float)(*(int *)((int)(char *)local_20 + 0xcb) - iVar2);
        }
        *(float *)((int)(char *)local_20 + 0xe3) =
             (float)*(int *)((int)(char *)local_20 + 0xcf) -
             (float)*(int *)((int)(char *)local_20 + 0xcb) *
             *(float *)((int)(char *)local_20 + 0xdb);
        return;
      }
      if (*(int *)((int)(char *)local_20 + 0x11) < 2) {
        return;
      }
      if (0x1b < *(int *)((int)(char *)local_20 + 0x11)) {
        return;
      }
      piStack_88 = (int *)0x304ac;
      iVar2 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcb) -
                           *(int *)((int)(char *)local_20 + 0xd3));
      piStack_88 = (int *)0x304c5;
      iVar1 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xcf) -
                           *(int *)((int)(char *)local_20 + 0xd7));
      if (iVar1 < iVar2) {
        local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcb) -
                          *(int *)((int)(char *)local_20 + 0xd3));
        fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd3) +
                (float10)*(int *)((int)(char *)local_20 + 0x11) *
                ((float10)(int)local_54 / (float10)_DAT_000500b4);
        piStack_88 = (int *)0x30503;
        FUN_00041f7e();
        local_28 = (int)ROUND(fVar5);
        local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcb) -
                          *(int *)((int)(char *)local_20 + 0xd3));
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 0xe3) +
                (float10)*(float *)((int)(char *)local_20 + 0xdb) *
                ((float10)*(int *)((int)(char *)local_20 + 0xd3) +
                (float10)*(int *)((int)(char *)local_20 + 0x11) *
                ((float10)(int)local_54 / (float10)_DAT_000500b4));
        piStack_88 = (int *)0x30552;
        FUN_00041f7e();
        local_24 = (int)ROUND(fVar5);
      }
      else {
        local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcf) -
                          *(int *)((int)(char *)local_20 + 0xd7));
        fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd7) +
                (float10)*(int *)((int)(char *)local_20 + 0x11) *
                ((float10)(int)local_54 / (float10)_DAT_000500bc);
        piStack_88 = (int *)0x30590;
        FUN_00041f7e();
        local_24 = (int)ROUND(fVar5);
        if ((*(uint *)((int)(char *)local_20 + 0xdb) & 0x7fffffff) == 0) {
          local_28 = 100;
        }
        else {
          local_54 = (float)(*(int *)((int)(char *)local_20 + 0xcf) -
                            *(int *)((int)(char *)local_20 + 0xd7));
          fVar5 = (((float10)*(int *)((int)(char *)local_20 + 0xd7) +
                   (float10)*(int *)((int)(char *)local_20 + 0x11) *
                   ((float10)(int)local_54 / (float10)_DAT_000500c4)) -
                  (float10)*(float *)((int)(char *)local_20 + 0xe3)) /
                  (float10)*(float *)((int)(char *)local_20 + 0xdb);
          piStack_88 = (int *)0x305f7;
          FUN_00041f7e();
          local_28 = (int)ROUND(fVar5);
        }
      }
      piStack_88 = (int *)0x30602;
      iVar2 = FUN_0001519f((char *)local_20);
      piStack_88 = (int *)0x3060f;
      FUN_0001545e((char *)local_20,local_28 - iVar2);
      piStack_88 = (int *)0x30617;
      iVar2 = FUN_000151d5((char *)local_20);
      piStack_88 = (int *)0x30624;
      FUN_0001586b((char *)local_20,local_24 - iVar2);
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x17) != 0x34) {
      if (*(int *)((int)(char *)local_20 + 0x17) != 5) {
        return;
      }
      *(undefined4 *)((int)(char *)local_20 + 0x7f) = 1;
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) == 1) {
      piStack_88 = (int *)0x30647;
      uVar3 = FUN_0001519f((char *)local_20);
      *(undefined4 *)((int)(char *)local_20 + 0xd3) = uVar3;
      piStack_88 = (int *)0x3065a;
      uVar3 = FUN_000151d5((char *)local_20);
      *(undefined4 *)((int)(char *)local_20 + 0xd7) = uVar3;
      *(int *)((int)(char *)local_20 + 0xcb) = local_4c;
      *(int *)((int)(char *)local_20 + 0xcf) = local_48;
      *(float *)((int)(char *)local_20 + 0xe7) =
           (float)(*(int *)((int)(char *)local_20 + 0xcf) - *(int *)((int)(char *)local_20 + 0xd7))
           / (float)_DAT_000500cc;
      return;
    }
    if (*(int *)((int)(char *)local_20 + 0x11) < 2) {
      return;
    }
    if (0x14 < *(int *)((int)(char *)local_20 + 0x11)) {
      return;
    }
    fVar5 = (float10)*(int *)((int)(char *)local_20 + 0xd7) +
            (float10)*(int *)((int)(char *)local_20 + 0x11) *
            (float10)*(float *)((int)(char *)local_20 + 0xe7);
    piStack_88 = (int *)0x306d9;
    FUN_00041f7e();
    local_24 = (int)ROUND(fVar5);
    piStack_88 = (int *)0x306e4;
    iVar2 = FUN_000151d5((char *)local_20);
    piStack_88 = (int *)0x306f1;
    FUN_0001586b((char *)local_20,local_24 - iVar2);
    return;
  }
  if (*(int *)((int)(char *)local_20 + 0x7f) == 1) {
    piStack_88 = (int *)0x3071c;
    iVar2 = FUN_0001519f((char *)local_20);
    piStack_88 = (int *)0x30724;
    iVar2 = FUN_00041f76(iVar2 - local_4c);
    if (iVar2 < 0x46) {
      piStack_88 = (int *)0x30731;
      iVar2 = FUN_000151d5((char *)local_20);
      piStack_88 = (int *)0x30739;
      iVar2 = FUN_00041f76(iVar2 - local_48);
      if (iVar2 < 10) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xe;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 0;
      }
    }
    *(undefined4 *)((int)(char *)local_20 + 0x7f) = 0;
  }
  if (*(char *)local_20 != '\x01') {
    return;
  }
  local_58 = *(uint *)((char *)local_20 + 0x1f);
  if (8 < local_58) {
    if (local_58 < 10) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar2 = FUN_00017637((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar2 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 9;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_8c;
      }
    }
    else if (local_58 < 0xe) {
      if (local_58 < 0xb) {
        if (*(int *)((char *)local_20 + 0x10f) == 1) {
          ((char *)local_20)[0xaf] = '\n';
          ((char *)local_20)[0xb0] = '\0';
          ((char *)local_20)[0xb1] = '\0';
          ((char *)local_20)[0xb2] = '\0';
          ((char *)local_20)[0x1f] = '\v';
          ((char *)local_20)[0x20] = '\0';
          ((char *)local_20)[0x21] = '\0';
          ((char *)local_20)[0x22] = '\0';
          ((char *)local_20)[0x11b] = '\x01';
          ((char *)local_20)[0x11c] = '\0';
          ((char *)local_20)[0x11d] = '\0';
          ((char *)local_20)[0x11e] = '\0';
          *(undefined4 *)((char *)local_20 + 0xab) =
               *(undefined4 *)(&DAT_00053f20 + *(int *)((char *)local_20 + 0xd) * 0x88);
          ppiVar4 = (int **)local_84;
        }
        else {
          ppiVar4 = (int **)local_84;
          if (*(int *)((char *)local_20 + 0x10f) == 2) {
            ((char *)local_20)[0xaf] = '\n';
            ((char *)local_20)[0xb0] = '\0';
            ((char *)local_20)[0xb1] = '\0';
            ((char *)local_20)[0xb2] = '\0';
            ((char *)local_20)[0x1f] = '\v';
            ((char *)local_20)[0x20] = '\0';
            ((char *)local_20)[0x21] = '\0';
            ((char *)local_20)[0x22] = '\0';
            ((char *)local_20)[0x11b] = '\x01';
            ((char *)local_20)[0x11c] = '\0';
            ((char *)local_20)[0x11d] = '\0';
            ((char *)local_20)[0x11e] = '\0';
            *(int *)((char *)local_20 + 0xab) =
                 *(int *)(&DAT_00053f20 + *(int *)((char *)local_20 + 0xd) * 0x88) >> 1;
            ppiVar4 = (int **)local_84;
          }
        }
      }
      else if (local_58 < 0xc) {
        if (*(int *)((char *)local_20 + 0xab) < 0) {
          local_30 = (uint)(byte)(&DAT_00053eb8)
                                 [*(int *)((char *)local_20 + 0xd) * 0x88 +
                                  *(int *)((char *)local_20 + 0x113) * 10];
          if ((((&DAT_00053eb9)
                [*(int *)((char *)local_20 + 0x113) * 10 + *(int *)((char *)local_20 + 0xd) * 0x88]
                == '1') ||
              ((&DAT_00053eb9)
               [*(int *)((char *)local_20 + 0x113) * 10 + *(int *)((char *)local_20 + 0xd) * 0x88]
               == '4')) ||
             ((&DAT_00053eb9)
              [*(int *)((char *)local_20 + 0x113) * 10 + *(int *)((char *)local_20 + 0xd) * 0x88] ==
              '.')) {
            piStack_88 = (int *)0x3164b;
            iVar2 = FUN_0001519f((char *)local_20);
            piStack_88 = (int *)0x31653;
            iVar2 = FUN_00041f76(iVar2 - local_4c);
            if (iVar2 < (int)local_30) {
              if (((&DAT_00053eb8)
                   [*(int *)((int)(char *)local_20 + 0x11b) +
                    *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                    *(int *)((int)(char *)local_20 + 0x113) * 10] == '\0') ||
                 (9 < *(int *)((int)(char *)local_20 + 0x11b))) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_84;
              }
              else {
                *(uint *)((int)(char *)local_20 + 0x17) =
                     (uint)(byte)(&DAT_00053eb8)
                                 [*(int *)((int)(char *)local_20 + 0x11b) +
                                  *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                                  *(int *)((int)(char *)local_20 + 0x113) * 10];
                *(undefined4 *)((int)(char *)local_20 + 0x11) = 0;
                *(int *)((int)(char *)local_20 + 0x11b) =
                     *(int *)((int)(char *)local_20 + 0x11b) + 1;
                ppiVar4 = (int **)local_84;
              }
            }
            else {
              piStack_88 = (int *)0x316fd;
              iVar2 = FUN_00041e9f();
              if (iVar2 % 0x14 == 2) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_84;
              }
              else {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                ppiVar4 = (int **)local_84;
              }
            }
          }
          else {
            piStack_88 = (int *)0x31752;
            iVar2 = FUN_0001519f((char *)local_20);
            piStack_88 = (int *)0x3175a;
            iVar2 = FUN_00041f76(iVar2 - local_4c);
            if (iVar2 < (int)local_30) {
              piStack_88 = (int *)0x31767;
              iVar2 = FUN_000151d5((char *)local_20);
              piStack_88 = (int *)0x3176f;
              iVar2 = FUN_00041f76(iVar2 - local_48);
              if (iVar2 < 10) {
                if (((&DAT_00053eb8)
                     [*(int *)((int)(char *)local_20 + 0x11b) +
                      *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                      *(int *)((int)(char *)local_20 + 0x113) * 10] == '\0') ||
                   (9 < *(int *)((int)(char *)local_20 + 0x11b))) {
                  *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                  *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
                  ppiVar4 = (int **)local_84;
                }
                else {
                  *(uint *)((int)(char *)local_20 + 0x17) =
                       (uint)(byte)(&DAT_00053eb8)
                                   [*(int *)((int)(char *)local_20 + 0x11b) +
                                    *(int *)((int)(char *)local_20 + 0xd) * 0x88 +
                                    *(int *)((int)(char *)local_20 + 0x113) * 10];
                  *(undefined4 *)((int)(char *)local_20 + 0x11) = 0;
                  *(int *)((int)(char *)local_20 + 0x11b) =
                       *(int *)((int)(char *)local_20 + 0x11b) + 1;
                  ppiVar4 = (int **)local_84;
                }
                goto LAB_00031a36;
              }
            }
            piStack_88 = (int *)0x3181a;
            iVar2 = FUN_00041e9f();
            if (iVar2 % 0x14 == 2) {
              *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
              *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
              ppiVar4 = (int **)local_84;
            }
            else {
              *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
              *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
              ppiVar4 = (int **)local_84;
            }
          }
        }
        else {
          *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
          ((char *)local_20)[0x17] = '\x01';
          ((char *)local_20)[0x18] = '\0';
          ((char *)local_20)[0x19] = '\0';
          ((char *)local_20)[0x1a] = '\0';
          ppiVar4 = (int **)local_84;
        }
      }
      else if (local_58 < 0xd) {
        *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_84;
        if (*(int *)((char *)local_20 + 0xab) < 0) {
          ((char *)local_20)[0x17] = '2';
          ((char *)local_20)[0x18] = '\0';
          ((char *)local_20)[0x19] = '\0';
          ((char *)local_20)[0x1a] = '\0';
          ((char *)local_20)[0x11] = '\0';
          ((char *)local_20)[0x12] = '\0';
          ((char *)local_20)[0x13] = '\0';
          ((char *)local_20)[0x14] = '\0';
          ((char *)local_20)[0xaf] = '\n';
          ((char *)local_20)[0xb0] = '\0';
          ((char *)local_20)[0xb1] = '\0';
          ((char *)local_20)[0xb2] = '\0';
          ((char *)local_20)[0x1f] = '\0';
          ((char *)local_20)[0x20] = '\0';
          ((char *)local_20)[0x21] = '\0';
          ((char *)local_20)[0x22] = '\0';
          ppiVar4 = (int **)local_84;
        }
      }
      else {
        *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
        ppiVar4 = (int **)local_84;
        if (*(int *)((char *)local_20 + 0xab) < 0) {
          ((char *)local_20)[0x17] = '1';
          ((char *)local_20)[0x18] = '\0';
          ((char *)local_20)[0x19] = '\0';
          ((char *)local_20)[0x1a] = '\0';
          ((char *)local_20)[0x11] = '\0';
          ((char *)local_20)[0x12] = '\0';
          ((char *)local_20)[0x13] = '\0';
          ((char *)local_20)[0x14] = '\0';
          ((char *)local_20)[0xaf] = '\n';
          ((char *)local_20)[0xb0] = '\0';
          ((char *)local_20)[0xb1] = '\0';
          ((char *)local_20)[0xb2] = '\0';
          ((char *)local_20)[0x1f] = '\0';
          ((char *)local_20)[0x20] = '\0';
          ((char *)local_20)[0x21] = '\0';
          ((char *)local_20)[0x22] = '\0';
          ppiVar4 = (int **)local_84;
        }
      }
    }
    else if (local_58 < 0xf) {
      *(int *)((char *)local_20 + 0xab) = *(int *)((char *)local_20 + 0xab) + -1;
      ppiVar4 = (int **)local_84;
      if (*(int *)((char *)local_20 + 0xab) < 0) {
        ((char *)local_20)[0x17] = '0';
        ((char *)local_20)[0x18] = '\0';
        ((char *)local_20)[0x19] = '\0';
        ((char *)local_20)[0x1a] = '\0';
        ((char *)local_20)[0x11] = '\0';
        ((char *)local_20)[0x12] = '\0';
        ((char *)local_20)[0x13] = '\0';
        ((char *)local_20)[0x14] = '\0';
        ((char *)local_20)[0xaf] = '\n';
        ((char *)local_20)[0xb0] = '\0';
        ((char *)local_20)[0xb1] = '\0';
        ((char *)local_20)[0xb2] = '\0';
        ((char *)local_20)[0x1f] = '\0';
        ((char *)local_20)[0x20] = '\0';
        ((char *)local_20)[0x21] = '\0';
        ((char *)local_20)[0x22] = '\0';
        ppiVar4 = (int **)local_84;
      }
    }
    else if (local_58 < 0x15) {
      ppiVar4 = (int **)local_84;
      if (local_58 == 0x14) {
        piStack_88 = &local_3c;
        piStack_8c = &local_40;
        iVar2 = FUN_0001776f((char *)local_20,local_44,local_4c,local_48);
        ppiVar4 = &piStack_8c;
        if (iVar2 != 0) {
          *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x14;
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
          *(undefined4 *)((int)(char *)local_20 + 199) = 1;
          ppiVar4 = &piStack_8c;
        }
      }
    }
    else if (local_58 < 0x16) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar2 = FUN_00017927((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar2 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x15;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_8c;
      }
    }
    else if (local_58 < 0x17) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar2 = FUN_00017ca6((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar2 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0x16;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_8c;
      }
    }
    else {
      ppiVar4 = (int **)local_84;
      if (local_58 == 0x6a) {
        fVar5 = (float10)_DAT_00051d68;
        piStack_88 = (int *)0x3123f;
        FUN_00041f7e();
        local_44 = (int)ROUND(fVar5);
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 99) + (float10)_DAT_00051d68;
        piStack_88 = (int *)0x31253;
        FUN_00041f7e();
        local_54 = (float)(int)ROUND(fVar5);
        fVar5 = (float10)*(float *)((int)(char *)local_20 + 99);
        piStack_88 = (int *)0x31261;
        FUN_00041f7e();
        local_50 = (int)ROUND(fVar5);
        if (local_50 + local_44 < (int)local_54) {
          local_44 = local_44 + 1;
        }
        *(float *)((int)(char *)local_20 + 99) =
             _DAT_00051d68 + *(float *)((int)(char *)local_20 + 99);
        piStack_88 = &local_3c;
        piStack_8c = &local_40;
        iVar2 = FUN_0001730b((char *)local_20,local_44,local_4c,local_48);
        ppiVar4 = &piStack_8c;
        if (iVar2 != 0) {
          if ((((*(int *)(DAT_00053c7c + 0x17) == 0x15) || (*(int *)(DAT_00053c7c + 0x17) == 0x16))
              || (*(int *)(DAT_00053c7c + 0x17) == 0x17)) ||
             (((*(int *)(DAT_00053c7c + 0x17) == 0x18 || (*(int *)(DAT_00053c7c + 0x17) == 0x1a)) ||
              ((*(int *)(DAT_00053c7c + 0x17) == 0x1d ||
               ((*(int *)(DAT_00053c7c + 0x17) == 0x1e || (*(int *)(DAT_00053c7c + 0x17) == 0x20))))
              )))) {
            iVar2 = FUN_0001519f((char *)local_20);
            iVar2 = FUN_00041f76(iVar2 - local_4c);
            ppiVar4 = &piStack_8c;
            if (iVar2 < 0x48) {
              iVar2 = FUN_000151d5((char *)local_20);
              iVar2 = FUN_00041f76(iVar2 - local_48);
              ppiVar4 = &piStack_8c;
              if (iVar2 < 0xc) {
                *(undefined4 *)((int)(char *)local_20 + 0xaf) = 10;
                *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xd;
                *(undefined4 *)((int)(char *)local_20 + 0xab) = 0;
                ppiVar4 = &piStack_8c;
              }
            }
          }
          else {
            *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
            *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
            ppiVar4 = &piStack_8c;
          }
        }
      }
    }
    goto LAB_00031a36;
  }
  if (3 < local_58) {
    if (local_58 < 5) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      local_5c = FUN_00016fec((char *)local_20,local_44,local_4c,local_48);
      if (((((*(int *)(DAT_00053c7c + 0x17) == 0x15) || (*(int *)(DAT_00053c7c + 0x17) == 0x16)) ||
           ((*(int *)(DAT_00053c7c + 0x17) == 0x17 ||
            ((*(int *)(DAT_00053c7c + 0x17) == 0x18 || (*(int *)(DAT_00053c7c + 0x17) == 0x1a))))))
          || (*(int *)(DAT_00053c7c + 0x17) == 0x1d)) ||
         ((*(int *)(DAT_00053c7c + 0x17) == 0x1e || (*(int *)(DAT_00053c7c + 0x17) == 0x20)))) {
        local_5c = 4;
      }
      if (local_5c == 2) {
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 1;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 10;
        ppiVar4 = &piStack_8c;
      }
      else if (local_5c == 1) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
      else if (local_5c == 3) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0xc;
        *(undefined4 *)((int)(char *)local_20 + 0xab) = 0x1e;
        ppiVar4 = &piStack_8c;
      }
      else {
        ppiVar4 = &piStack_8c;
        if (local_5c == 4) {
          uVar3 = FUN_0001519f(DAT_00053c7c);
          *(undefined4 *)((int)(char *)local_20 + 0xb3) = uVar3;
          uVar3 = FUN_000151d5(DAT_00053c7c);
          *(undefined4 *)((int)(char *)local_20 + 0xb7) = uVar3;
          uVar3 = FUN_0001519f(DAT_00053c7c);
          uVar6 = FUN_0001519f((char *)local_20,uVar3);
          if ((int)((ulonglong)uVar6 >> 0x20) - (int)uVar6 < 0) {
            *(int *)((int)(char *)local_20 + 0xb3) = *(int *)((int)(char *)local_20 + 0xb3) + 0x46;
          }
          else {
            *(int *)((int)(char *)local_20 + 0xb3) = *(int *)((int)(char *)local_20 + 0xb3) + -0x46;
          }
          *(undefined4 *)((int)(char *)local_20 + 0xaf) = 0;
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x6a;
          ppiVar4 = &piStack_8c;
        }
      }
    }
    else if (local_58 < 6) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar2 = FUN_00016959((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar2 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 5;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        *(undefined4 *)((int)(char *)local_20 + 199) = 1;
        ppiVar4 = &piStack_8c;
      }
    }
    else if (local_58 < 7) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar2 = FUN_0001730b((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar2 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 6;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    else if (local_58 < 8) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar2 = FUN_000174a2((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar2 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 4;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    else {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar2 = FUN_00017530((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar2 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    goto LAB_00031a36;
  }
  if (local_58 != 0) {
    if (local_58 < 2) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar2 = FUN_00017218((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar2 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 1;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    else if (local_58 < 3) {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar2 = FUN_00016a60((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar2 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    else {
      piStack_88 = &local_3c;
      piStack_8c = &local_40;
      iVar2 = FUN_00016b8f((char *)local_20,local_44,local_4c,local_48);
      ppiVar4 = &piStack_8c;
      if (iVar2 != 0) {
        *(undefined4 *)((int)(char *)local_20 + 0xaf) = 3;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0;
        ppiVar4 = &piStack_8c;
      }
    }
    goto LAB_00031a36;
  }
  if (*(int *)((char *)local_20 + 0x8b) == 0) {
    ((char *)local_20)[0x8b] = '\x01';
    ((char *)local_20)[0x8c] = '\0';
    ((char *)local_20)[0x8d] = '\0';
    ((char *)local_20)[0x8e] = '\0';
    ppiVar4 = (int **)local_84;
    goto LAB_00031a36;
  }
  if (*(int *)((char *)local_20 + 0xaf) != 4) {
    if (*(int *)((char *)local_20 + 0xaf) == 10) {
      *(int *)((char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)((char *)local_20 + 0xd) * 0x88) >> 1;
    }
    else {
      piStack_88 = (int *)0x307e9;
      local_34 = FUN_00041e9f();
      local_34 = local_34 % 0x32;
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb4 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) + local_34;
    }
    *(undefined4 *)((int)(char *)local_20 + 0x1f) = 4;
    piStack_88 = (int *)0x30825;
    iVar2 = FUN_00041e9f();
    *(int *)((int)(char *)local_20 + 0x113) = iVar2 % 10;
  }
  if (*(int *)((int)(char *)local_20 + 0xaf) == 4) {
    if (*(int *)((int)(char *)local_20 + 0x8f) == 1) {
      *(int *)((int)(char *)local_20 + 0x97) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 3;
    }
    else {
      piStack_88 = (int *)0x3088a;
      iVar2 = FUN_0001519f((char *)local_20);
      if (iVar2 < 0) {
LAB_0003089d:
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 7;
        return;
      }
      piStack_88 = (int *)0x30896;
      iVar2 = FUN_0001519f((char *)local_20);
      if (0x13f < iVar2) goto LAB_0003089d;
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      if (*(int *)((int)(char *)local_20 + 199) == 1) {
        do {
          piStack_88 = (int *)0x308dc;
          local_80 = FUN_00041e9f();
          local_80 = local_80 % 2;
        } while (local_80 == *(int *)((int)(char *)local_20 + 0xff));
        *(int *)((int)(char *)local_20 + 0xff) = local_80;
        if ((local_80 == 0) || (local_80 == 1)) {
          *(undefined4 *)((int)(char *)local_20 + 0x6f) = 1;
        }
        else {
          *(undefined4 *)((int)(char *)local_20 + 0x6f) = 0;
        }
        local_7c = *(undefined4 *)((int)(char *)local_20 + 0xbf);
        local_78 = *(undefined4 *)((int)(char *)local_20 + 0xc3);
        if (local_80 == 0) {
          if ((uint)((int)(uint)(byte)(&DAT_00053eb8)
                                      [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                       *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) < 0x3c) {
            *(uint *)((int)(char *)local_20 + 0xbf) =
                 local_4c -
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88];
          }
          else {
            *(int *)((int)(char *)local_20 + 0xbf) =
                 (local_4c -
                 ((int)(uint)(byte)(&DAT_00053eb8)
                                   [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                    *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1)) + 1;
          }
          *(int *)((int)(char *)local_20 + 0xc3) = local_48;
        }
        else if (local_80 == 1) {
          if ((uint)((int)(uint)(byte)(&DAT_00053eb8)
                                      [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                       *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) < 0x3c) {
            *(uint *)((int)(char *)local_20 + 0xbf) =
                 local_4c +
                 (uint)(byte)(&DAT_00053eb8)
                             [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                              *(int *)((int)(char *)local_20 + 0xd) * 0x88];
          }
          else {
            *(int *)((int)(char *)local_20 + 0xbf) =
                 local_4c +
                 ((int)(uint)(byte)(&DAT_00053eb8)
                                   [*(int *)((int)(char *)local_20 + 0x113) * 10 +
                                    *(int *)((int)(char *)local_20 + 0xd) * 0x88] >> 1) + -1;
          }
          *(int *)((int)(char *)local_20 + 0xc3) = local_48;
        }
        if (*(int *)((int)(char *)local_20 + 0xbf) < 0) {
          *(undefined4 *)((int)(char *)local_20 + 0xbf) = 0x14;
        }
        else if (0x13f < *(int *)((int)(char *)local_20 + 0xbf)) {
          *(undefined4 *)((int)(char *)local_20 + 0xbf) = 300;
        }
        piStack_88 = (int *)0x30ac1;
        iVar2 = FUN_0001519f((char *)local_20);
        piStack_88 = (int *)0x30ad3;
        iVar2 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - iVar2);
        if (iVar2 < 10) {
          piStack_88 = (int *)0x30ae0;
          iVar2 = FUN_000151d5((char *)local_20);
          piStack_88 = (int *)0x30af2;
          iVar2 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - iVar2);
          if (iVar2 < 5) {
            *(undefined4 *)((int)(char *)local_20 + 0xbf) = local_7c;
            *(undefined4 *)((int)(char *)local_20 + 0xc3) = local_78;
            goto LAB_00030b20;
          }
        }
        *(undefined4 *)((int)(char *)local_20 + 199) = 0;
      }
LAB_00030b20:
      if (*(int *)((int)(char *)local_20 + 0xc3) <= *(int *)(DAT_00063354 + 0x12)) {
        *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x12) + 1;
      }
      if (*(int *)(DAT_00063354 + 0x16) <= *(int *)((int)(char *)local_20 + 0xc3)) {
        *(int *)((int)(char *)local_20 + 0xc3) = *(int *)(DAT_00063354 + 0x16) + -1;
      }
      piStack_88 = (int *)0x30b74;
      iVar2 = FUN_0001519f((char *)local_20);
      if (iVar2 < *(int *)((int)(char *)local_20 + 0xbf)) {
        local_74 = *(int *)((int)(char *)local_20 + 0xbf);
        piStack_88 = (int *)0x30b95;
        local_70 = FUN_0001519f((char *)local_20);
      }
      else {
        piStack_88 = (int *)0x30ba2;
        local_74 = FUN_0001519f((char *)local_20);
        local_70 = *(int *)((int)(char *)local_20 + 0xbf);
      }
      piStack_88 = (int *)0x30bb9;
      iVar2 = FUN_000151d5((char *)local_20);
      if (iVar2 < *(int *)((int)(char *)local_20 + 0xc3)) {
        local_6c = *(int *)((int)(char *)local_20 + 0xc3);
        piStack_88 = (int *)0x30bda;
        local_68 = FUN_000151d5((char *)local_20);
      }
      else {
        piStack_88 = (int *)0x30be7;
        local_6c = FUN_000151d5((char *)local_20);
        local_68 = *(int *)((int)(char *)local_20 + 0xc3);
      }
      piStack_88 = (int *)0x30bfe;
      local_64 = FUN_0001519f((char *)local_20);
      piStack_88 = (int *)0x30c09;
      local_60 = FUN_000151d5((char *)local_20);
      piStack_88 = (int *)0x30c1d;
      iVar2 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_64);
      if (iVar2 < 5) {
        piStack_88 = (int *)0x30c33;
        iVar2 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_60);
        if ((((5 < iVar2) && (local_48 < local_6c)) && (local_68 < local_48)) &&
           ((local_64 + -8 < local_4c && (local_4c < local_64 + 8)))) {
          *(int *)((int)(char *)local_20 + 0xcb) = local_64;
          *(undefined4 *)((int)(char *)local_20 + 0xeb) =
               *(undefined4 *)((int)(char *)local_20 + 0xcb);
          *(int *)((int)(char *)local_20 + 0xcf) = local_60;
          *(undefined4 *)((int)(char *)local_20 + 0xef) =
               *(undefined4 *)((int)(char *)local_20 + 0xcf);
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x14;
          *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
          return;
        }
      }
      piStack_88 = (int *)0x30cd0;
      iVar2 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xc3) - local_60);
      if (iVar2 < 10) {
        piStack_88 = (int *)0x30ce6;
        iVar2 = FUN_00041f76(*(int *)((int)(char *)local_20 + 0xbf) - local_64);
        if (((5 < iVar2) && (local_48 < local_60 + 8)) &&
           ((local_60 + -8 < local_48 && ((local_70 < local_4c && (local_4c < local_74)))))) {
          *(int *)((int)(char *)local_20 + 0xcb) = local_64;
          *(undefined4 *)((int)(char *)local_20 + 0xeb) =
               *(undefined4 *)((int)(char *)local_20 + 0xcb);
          *(int *)((int)(char *)local_20 + 0xcf) = local_60;
          *(undefined4 *)((int)(char *)local_20 + 0xef) =
               *(undefined4 *)((int)(char *)local_20 + 0xcf);
          if (*(int *)((int)(char *)local_20 + 0xcf) <
              *(int *)(DAT_00063354 + 0x12) +
              (*(int *)(DAT_00063354 + 0x16) - *(int *)(DAT_00063354 + 0x12) >> 1)) {
            if (*(int *)((int)(char *)local_20 + 0xcf) < *(int *)((int)(char *)local_20 + 0xc3)) {
              *(int *)((int)(char *)local_20 + 0xcb) =
                   *(int *)((int)(char *)local_20 + 0xcb) -
                   (*(int *)((int)(char *)local_20 + 0xc3) - *(int *)((int)(char *)local_20 + 0xcf))
              ;
            }
            else {
              *(int *)((int)(char *)local_20 + 0xcb) =
                   *(int *)((int)(char *)local_20 + 0xcb) +
                   (*(int *)((int)(char *)local_20 + 0xc3) - *(int *)((int)(char *)local_20 + 0xcf))
              ;
            }
          }
          else if (*(int *)((int)(char *)local_20 + 0xcf) < *(int *)((int)(char *)local_20 + 0xc3))
          {
            *(int *)((int)(char *)local_20 + 0xcb) =
                 *(int *)((int)(char *)local_20 + 0xcb) +
                 (*(int *)((int)(char *)local_20 + 0xc3) - *(int *)((int)(char *)local_20 + 0xcf));
          }
          else {
            *(int *)((int)(char *)local_20 + 0xcb) =
                 *(int *)((int)(char *)local_20 + 0xcb) -
                 (*(int *)((int)(char *)local_20 + 0xc3) - *(int *)((int)(char *)local_20 + 0xcf));
          }
          *(undefined4 *)((int)(char *)local_20 + 0x1f) = 0x15;
          *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
          return;
        }
      }
      if ((local_48 < local_6c) && (local_68 < local_48)) {
        piStack_88 = (int *)0x30e61;
        iVar2 = FUN_0001519f((char *)local_20);
        if (local_4c <= iVar2 + -8) goto LAB_00030e79;
        piStack_88 = (int *)0x30e71;
        iVar2 = FUN_0001519f((char *)local_20);
        if (iVar2 + 8 <= local_4c) goto LAB_00030e79;
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      else {
LAB_00030e79:
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      if ((((local_4c < local_74) && (local_70 < local_4c)) &&
          (*(int *)((int)(char *)local_20 + 0xc3) + -8 < local_48)) &&
         (local_48 < *(int *)((int)(char *)local_20 + 0xc3) + 8)) {
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 8;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
      else {
        *(undefined4 *)((int)(char *)local_20 + 0x1f) = 2;
        *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      }
    }
  }
  if (*(int *)((int)(char *)local_20 + 0xaf) == 2) {
    *(int *)((int)(char *)local_20 + 0xab) =
         *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
    *(undefined4 *)((int)(char *)local_20 + 0x1f) = 5;
    *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
    ppiVar4 = (int **)local_84;
  }
  else {
    ppiVar4 = (int **)local_84;
    if (*(int *)((int)(char *)local_20 + 0xaf) == 8) {
      *(int *)((int)(char *)local_20 + 0xab) =
           *(int *)(&DAT_00053eb0 + *(int *)((int)(char *)local_20 + 0xd) * 0x88) >> 1;
      *(undefined4 *)((int)(char *)local_20 + 0x1f) = 9;
      *(undefined4 *)((int)(char *)local_20 + 0x8b) = 0;
      ppiVar4 = (int **)local_84;
    }
  }
LAB_00031a36:
  if (local_40 != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x31a47;
    FUN_0001545e((char *)local_20,local_40);
  }
  if (local_3c != 0) {
    *(undefined4 *)((int)ppiVar4 + -4) = 0x31a58;
    FUN_0001586b((char *)local_20,local_3c);
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x31a60;
  iVar2 = FUN_0001519f((char *)local_20);
  if (local_4c < iVar2) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 1;
  }
  *(undefined4 *)((int)ppiVar4 + -4) = 0x31a77;
  iVar2 = FUN_0001519f((char *)local_20);
  if (iVar2 < local_4c) {
    *(undefined4 *)((int)(char *)local_20 + 0x1b) = 0;
  }
  return;
}


