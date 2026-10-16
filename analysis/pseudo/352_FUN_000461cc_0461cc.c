// func #352  FUN_000461cc  000461cc  size=2998


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000461cc(undefined *param_1,undefined4 param_2,undefined4 *param_3,undefined4 param_4,
                 undefined4 param_5,undefined2 param_6)

{
  byte *pbVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  undefined2 uVar6;
  undefined2 uVar7;
  char cVar8;
  byte bVar14;
  undefined2 uVar9;
  short sVar10;
  undefined4 uVar11;
  uint uVar12;
  undefined *puVar13;
  short extraout_CX;
  ushort uVar15;
  undefined2 uVar16;
  ushort uVar17;
  uint uVar18;
  undefined4 *puVar19;
  undefined2 in_FS;
  undefined2 in_GS;
  byte *in_stack_00000038;
  undefined2 in_stack_0000003c;
  
code_r0x000461cc:
  bVar14 = *in_stack_00000038;
  uVar9 = (undefined2)param_5;
  uVar6 = param_5._2_2_;
  for (uVar11 = CONCAT31((int3)((uint)param_1 >> 8),bVar14);
      (cVar8 = (char)uVar11, bVar14 < 0x9c &&
      ((((((uVar7 = uVar9, uVar16 = uVar6, bVar14 == 0x9b ||
           (uVar7 = param_6, uVar16 = param_6, cVar8 == '&')) ||
          (uVar7 = in_stack_0000003c, uVar16 = in_stack_0000003c, cVar8 == '.')) ||
         ((uVar7 = param_5._2_2_, uVar16 = param_5._2_2_, cVar8 == '6' ||
          (uVar7 = (undefined2)param_5, uVar16 = (undefined2)param_5, cVar8 == '>')))) ||
        ((uVar7 = in_FS, uVar16 = in_FS, cVar8 == 'd' ||
         ((uVar7 = in_GS, uVar16 = in_GS, cVar8 == 'e' ||
          (uVar7 = uVar9, uVar16 = uVar6, cVar8 == 'f')))))) || (cVar8 == 'g'))));
      uVar11 = CONCAT31((int3)((uint)uVar11 >> 8),bVar14)) {
    uVar9 = uVar7;
    in_stack_00000038 = in_stack_00000038 + 1;
    bVar14 = *in_stack_00000038;
    uVar6 = uVar16;
  }
  uVar18 = CONCAT31((int3)((uint)param_3 >> 8),cVar8) & 0xfffffff8;
  if ((char)uVar18 != -0x28) {
    return;
  }
  pbVar1 = in_stack_00000038 + 1;
  in_stack_00000038 = in_stack_00000038 + 2;
  uVar12 = CONCAT31((int3)(CONCAT22((short)((uint)uVar11 >> 0x10),CONCAT11(*pbVar1,cVar8)) >> 8),
                    cVar8) & 0xffffff07;
  bVar14 = (byte)(uVar12 >> 8);
  if (bVar14 < 0xc0) {
    uVar18 = CONCAT11(bVar14 >> 3,bVar14) & 0x1807;
    uVar9 = (*(code *)(&PTR_LAB_00045e78)[(byte)((byte)uVar18 | (byte)(uVar18 >> 8))])();
    uVar18 = (uint)(byte)((byte)((ushort)uVar9 >> 8) & 0x38 | (byte)uVar9);
    FUN_00042310();
                    /* WARNING: Could not recover jumptable at 0x0004624a. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (*(code *)(&PTR_LAB_00045f38)[uVar18])();
    return;
  }
  param_1 = (undefined *)FUN_00042310(uVar12,uVar6,uVar18,uVar9);
  bVar14 = (byte)((uint)param_1 >> 8);
  uVar18 = (_DAT_00053650 & 0x3800) >> 0xb;
  param_3 = (undefined4 *)(bVar14 & 7);
  puVar19 = param_3;
  if ((short)param_1 == -0x7ff) {
    puVar19 = (undefined4 *)0x1;
  }
  iVar4 = uVar18 * 10;
  iVar5 = ((uint)((int)puVar19 + uVar18) & 7) * 10;
                    /* WARNING (jumptable): Sanity check requires truncation of jumptable */
                    /* WARNING: Could not find normalized switch variable to match jumptable */
  switch(bVar14 & 0x38 | (byte)param_1) {
  case 0:
    FUN_000478f0(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5,&DAT_00053668 + iVar4);
    (*DAT_000536c2)();
    return;
  case 1:
    uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
    _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
    _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
    *(undefined4 *)(&DAT_00053668 + uVar15) = *(undefined4 *)(&DAT_00053668 + iVar5);
    *(undefined4 *)(&DAT_0005366c + uVar15) = *(undefined4 *)(&DAT_0005366c + iVar5);
    *(undefined2 *)(&DAT_00053670 + uVar15) = *(undefined2 *)(&DAT_00053670 + iVar5);
    FUN_000461cc();
    return;
  case 4:
    FUN_000478f0(&DAT_00053668 + iVar5,&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    (*DAT_000536c2)();
    return;
  case 5:
  case 7:
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar5) |
                   *(ushort *)(&DAT_00045e22 + iVar5);
    FUN_000461cc();
    return;
  case 6:
    FUN_000478f0(&DAT_00053668 + iVar5,&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                   *(ushort *)(&DAT_00045e22 + iVar4);
    _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
    _DAT_00053650 =
         CONCAT22(_UNK_00053652,
                  DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
    (*DAT_000536c2)();
    return;
  case 8:
    FUN_00047d24(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5,&DAT_00053668 + iVar4);
    (*DAT_000536c2)();
    return;
  case 9:
  case 0xd:
  case 0xf:
    LOCK();
    uVar11 = *(undefined4 *)(&DAT_00053668 + iVar4);
    *(undefined4 *)(&DAT_00053668 + iVar4) = *(undefined4 *)(&DAT_00053668 + iVar5);
    UNLOCK();
    *(undefined4 *)(&DAT_00053668 + iVar5) = uVar11;
    LOCK();
    uVar11 = *(undefined4 *)(&DAT_0005366c + iVar4);
    *(undefined4 *)(&DAT_0005366c + iVar4) = *(undefined4 *)(&DAT_0005366c + iVar5);
    UNLOCK();
    *(undefined4 *)(&DAT_0005366c + iVar5) = uVar11;
    LOCK();
    uVar9 = *(undefined2 *)(&DAT_00053670 + iVar4);
    *(undefined2 *)(&DAT_00053670 + iVar4) = *(undefined2 *)(&DAT_00053670 + iVar5);
    UNLOCK();
    *(undefined2 *)(&DAT_00053670 + iVar5) = uVar9;
    FUN_000461cc();
    return;
  case 0xc:
    FUN_00047d24(&DAT_00053668 + iVar5,&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    (*DAT_000536c2)();
    return;
  case 0xe:
    FUN_00047d24(&DAT_00053668 + iVar5,&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                   *(ushort *)(&DAT_00045e22 + iVar4);
    _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
    _DAT_00053650 =
         CONCAT22(_UNK_00053652,
                  DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
    (*DAT_000536c2)();
    return;
  case 0x10:
  case 0x14:
    FUN_00047ab6(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    FUN_000463c0();
    return;
  case 0x11:
    switch(param_3) {
    case (undefined4 *)0x0:
      goto code_r0x000461cc;
    }
    break;
  case 0x15:
  case 0x17:
    *(undefined4 *)(&DAT_00053668 + iVar5) = *(undefined4 *)(&DAT_00053668 + iVar4);
    *(undefined4 *)(&DAT_0005366c + iVar5) = *(undefined4 *)(&DAT_0005366c + iVar4);
    *(undefined2 *)(&DAT_00053670 + iVar5) = *(undefined2 *)(&DAT_00053670 + iVar4);
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar5);
    FUN_000461cc();
    return;
  case 0x18:
  case 0x1c:
    FUN_00047ab6(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    FUN_0004636e();
    return;
  case 0x19:
  case 0x1d:
  case 0x1f:
    *(undefined4 *)(&DAT_00053668 + iVar5) = *(undefined4 *)(&DAT_00053668 + iVar4);
    *(undefined4 *)(&DAT_0005366c + iVar5) = *(undefined4 *)(&DAT_0005366c + iVar4);
    *(undefined2 *)(&DAT_00053670 + iVar5) = *(undefined2 *)(&DAT_00053670 + iVar4);
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar5) &
                   *(ushort *)(&DAT_00045e1a + iVar4) | *(ushort *)(&DAT_00045e22 + iVar4);
    _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
    _DAT_00053650 =
         CONCAT22(_UNK_00053652,
                  DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
    FUN_000461cc();
    return;
  case 0x1e:
    switch(param_3) {
    case (undefined4 *)0x1:
      DAT_000536b8 = *(undefined4 *)(&DAT_00053668 + iVar4);
      DAT_000536bc = *(undefined4 *)(&DAT_0005366c + iVar4);
      DAT_000536c0 = *(undefined2 *)(&DAT_00053670 + iVar4);
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                     *(ushort *)(&DAT_00045e22 + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 =
           CONCAT22(_UNK_00053652,
                    DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
      FUN_00047ab6(&DAT_000536b8,&DAT_00053668 + *(ushort *)(&DAT_00045dca + iVar4));
      FUN_0004636e();
      return;
    }
    break;
  case 0x20:
    FUN_000478e2(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5,&DAT_00053668 + iVar4);
    (*DAT_000536c2)();
    return;
  case 0x21:
    switch(param_3) {
    case (undefined4 *)0x0:
      param_1 = &DAT_00053668 + iVar4;
      (&DAT_00053671)[iVar4] = (&DAT_00053671)[iVar4] ^ 0x80;
      goto code_r0x000461cc;
    case (undefined4 *)0x1:
      param_1 = &DAT_00053668 + iVar4;
      (&DAT_00053671)[iVar4] = (&DAT_00053671)[iVar4] & 0x7f;
      goto code_r0x000461cc;
    case (undefined4 *)0x4:
      DAT_000536b8 = DAT_00045dc0;
      DAT_000536bc = DAT_00045dc4;
      DAT_000536c0 = DAT_00045dc8;
      FUN_00047ab6(&DAT_00053668 + iVar4,&DAT_000536b8);
      FUN_000463c0();
      return;
    case (undefined4 *)0x5:
      param_1 = (undefined *)FUN_00048666(&DAT_00053668 + iVar4);
      goto code_r0x000461cc;
    }
    break;
  case 0x23:
    switch(param_3) {
    case (undefined4 *)0x2:
      _DAT_00053650 = _DAT_00053650 & 0xffff7f00;
      goto code_r0x000461cc;
    case (undefined4 *)0x3:
      DAT_0005364c = 0x33f;
      _DAT_00053650 = _DAT_00053650 & 0xffff0000;
      DAT_00053654 = 0xffff;
      DAT_000536c2 = FUN_000461cc;
      FUN_000461cc();
      return;
    }
    break;
  case 0x24:
    FUN_000478e2(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5,&DAT_00053668 + iVar5);
    (*DAT_000536c2)();
    return;
  case 0x25:
    FUN_00047ab6(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    FUN_000463c0();
    return;
  case 0x26:
    FUN_000478e2(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5,&DAT_00053668 + iVar5);
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                   *(ushort *)(&DAT_00045e22 + iVar4);
    _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
    _DAT_00053650 =
         CONCAT22(_UNK_00053652,
                  DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
    (*DAT_000536c2)();
    return;
  case 0x27:
    switch(param_3) {
    case (undefined4 *)0x0:
      param_1 = (undefined *)((uint)param_1 & 0xffff0000);
      goto code_r0x000461cc;
    }
    break;
  case 0x28:
    FUN_000478e2(&DAT_00053668 + iVar5,&DAT_00053668 + iVar4,&DAT_00053668 + iVar4);
    (*DAT_000536c2)();
    return;
  case 0x29:
    switch(param_3) {
    case (undefined4 *)0x0:
      uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
      param_3 = (undefined4 *)(uint)*(ushort *)(&DAT_00045e1a + uVar15);
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
      param_1 = (undefined *)0x0;
      *(undefined4 *)(&DAT_00053668 + uVar15) = 0;
      *(undefined4 *)(&DAT_0005366c + uVar15) = 0x80000000;
      *(undefined2 *)(&DAT_00053670 + uVar15) = 0x3fff;
      goto code_r0x000461cc;
    case (undefined4 *)0x1:
      uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
      param_3 = (undefined4 *)(uint)*(ushort *)(&DAT_00045e1a + uVar15);
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
      *(undefined4 *)(&DAT_00053668 + uVar15) = 0xcd1b8afe;
      *(undefined4 *)(&DAT_0005366c + uVar15) = 0xd49a784b;
      *(undefined2 *)(&DAT_00053670 + uVar15) = 0x4000;
      goto code_r0x000461cc;
    case (undefined4 *)0x2:
      uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
      param_3 = (undefined4 *)(uint)*(ushort *)(&DAT_00045e1a + uVar15);
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
      *(undefined4 *)(&DAT_00053668 + uVar15) = 0x5c17f0bc;
      *(undefined4 *)(&DAT_0005366c + uVar15) = 0xb8aa3b29;
      *(undefined2 *)(&DAT_00053670 + uVar15) = 0x3fff;
      goto code_r0x000461cc;
    case (undefined4 *)0x3:
      uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
      param_3 = (undefined4 *)(uint)*(ushort *)(&DAT_00045e1a + uVar15);
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
      *(undefined4 *)(&DAT_00053668 + uVar15) = 0x2168c235;
      *(undefined4 *)(&DAT_0005366c + uVar15) = 0xc900daa2;
      *(undefined2 *)(&DAT_00053670 + uVar15) = 0x4000;
      goto code_r0x000461cc;
    case (undefined4 *)0x4:
      uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
      param_3 = (undefined4 *)(uint)*(ushort *)(&DAT_00045e1a + uVar15);
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
      *(undefined4 *)(&DAT_00053668 + uVar15) = 0xfbcff799;
      *(undefined4 *)(&DAT_0005366c + uVar15) = 0x9a209a84;
      *(undefined2 *)(&DAT_00053670 + uVar15) = 0x3ffd;
      goto code_r0x000461cc;
    case (undefined4 *)0x5:
      uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
      param_3 = (undefined4 *)(uint)*(ushort *)(&DAT_00045e1a + uVar15);
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
      *(undefined4 *)(&DAT_00053668 + uVar15) = 0xd1cf79ac;
      *(undefined4 *)(&DAT_0005366c + uVar15) = 0xb17217f7;
      *(undefined2 *)(&DAT_00053670 + uVar15) = 0x3ffe;
      goto code_r0x000461cc;
    case (undefined4 *)0x6:
      uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
      param_1 = (undefined *)0x0;
      *(undefined4 *)(&DAT_00053668 + uVar15) = 0;
      *(undefined4 *)(&DAT_0005366c + uVar15) = 0;
      *(undefined2 *)(&DAT_00053670 + uVar15) = 0;
      param_3 = (undefined4 *)(uint)*(ushort *)(&DAT_00045e1c + uVar15);
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15) |
                     *(ushort *)(&DAT_00045e1c + uVar15);
      goto code_r0x000461cc;
    }
    break;
  case 0x2a:
    switch(param_3) {
    case (undefined4 *)0x1:
      DAT_000536b8 = *(undefined4 *)(&DAT_00053668 + iVar4);
      DAT_000536bc = *(undefined4 *)(&DAT_0005366c + iVar4);
      DAT_000536c0 = *(undefined2 *)(&DAT_00053670 + iVar4);
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                     *(ushort *)(&DAT_00045e22 + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 =
           CONCAT22(_UNK_00053652,
                    DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
      FUN_00047ab6(&DAT_000536b8,&DAT_00053668 + *(ushort *)(&DAT_00045dca + iVar4));
      FUN_0004636e();
      return;
    }
    break;
  case 0x2c:
    FUN_000478e2(&DAT_00053668 + iVar5,&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    (*DAT_000536c2)();
    return;
  case 0x2d:
    FUN_00047ab6(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    FUN_0004636e();
    return;
  case 0x2e:
    FUN_000478e2(&DAT_00053668 + iVar5,&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                   *(ushort *)(&DAT_00045e22 + iVar4);
    _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
    _DAT_00053650 =
         CONCAT22(_UNK_00053652,
                  DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
    (*DAT_000536c2)();
    return;
  case 0x30:
    FUN_00047b4f(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5,&DAT_00053668 + iVar4);
    (*DAT_000536c2)();
    return;
  case 0x31:
    switch(param_3) {
    case (undefined4 *)0x0:
      FUN_00048c8c(&DAT_00053668 + iVar4);
      (*DAT_000536c2)();
      return;
    case (undefined4 *)0x1:
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                     *(ushort *)(&DAT_00045e22 + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 =
           CONCAT22(_UNK_00053652,
                    DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
      FUN_0004876d(&DAT_00053668 + iVar4,&DAT_00053668 + *(ushort *)(&DAT_00045dca + iVar4));
      (*DAT_000536c2)();
      return;
    case (undefined4 *)0x2:
      FUN_00048c56(&DAT_00053668 + iVar4);
      uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
      param_3 = (undefined4 *)
                CONCAT22((short)((uint)param_3 >> 0x10),*(ushort *)(&DAT_00045e1a + uVar15));
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
      param_1 = (undefined *)0x0;
      *(undefined4 *)(&DAT_00053668 + uVar15) = 0;
      *(undefined4 *)(&DAT_0005366c + uVar15) = 0x80000000;
      *(undefined2 *)(&DAT_00053670 + uVar15) = 0x3fff;
      break;
    case (undefined4 *)0x3:
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                     *(ushort *)(&DAT_00045e22 + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 =
           CONCAT22(_UNK_00053652,
                    DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
      FUN_0004836c(&DAT_00053668 + *(ushort *)(&DAT_00045dca + iVar4),&DAT_00053668 + iVar4,
                   &DAT_00053668 + *(ushort *)(&DAT_00045dca + iVar4));
      (*DAT_000536c2)();
      return;
    case (undefined4 *)0x4:
      uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
      DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
      param_3 = (undefined4 *)(&DAT_00053668 + uVar15);
      *param_3 = *(undefined4 *)(&DAT_00053668 + iVar4);
      *(undefined4 *)(&DAT_0005366c + uVar15) = *(undefined4 *)(&DAT_0005366c + iVar4);
      uVar17 = *(ushort *)(&DAT_00053670 + iVar4);
      *(ushort *)(&DAT_00053670 + uVar15) = uVar17 & 0x8000 | 0x3fff;
      param_1 = (undefined *)
                FUN_00047e9f((int)(short)((uVar17 & 0x7fff) + 0xc001),&DAT_00053668 + iVar4);
      break;
    case (undefined4 *)0x5:
      uVar18 = FUN_00048585(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
      _DAT_00053650 =
           CONCAT22(_UNK_00053652,
                    CONCAT11((byte)((_DAT_00053650 & 0xb8ff) >> 8) | (&DAT_00045e6e)[uVar18 & 7],
                             (char)(_DAT_00053650 & 0xb8ff)));
      (*DAT_000536c2)();
      return;
    case (undefined4 *)0x6:
      param_3 = (undefined4 *)(uint)*(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dcc + iVar4));
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 =
           CONCAT22(_UNK_00053652,
                    DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dcc + iVar4)));
      break;
    case (undefined4 *)0x7:
      param_3 = (undefined4 *)(uint)*(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4));
      _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
      _DAT_00053650 =
           CONCAT22(_UNK_00053652,
                    DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
    }
    goto code_r0x000461cc;
  case 0x34:
    FUN_00047b4f(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5,&DAT_00053668 + iVar5);
    (*DAT_000536c2)();
    return;
  case 0x36:
    FUN_00047b4f(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5,&DAT_00053668 + iVar5);
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                   *(ushort *)(&DAT_00045e22 + iVar4);
    _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
    _DAT_00053650 =
         CONCAT22(_UNK_00053652,
                  DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
    (*DAT_000536c2)();
    return;
  case 0x38:
    FUN_00047b4f(&DAT_00053668 + iVar5,&DAT_00053668 + iVar4,&DAT_00053668 + iVar4);
    (*DAT_000536c2)();
    return;
  case 0x39:
    goto switchD_000462f7_caseD_39;
  case 0x3c:
    FUN_00047b4f(&DAT_00053668 + iVar5,&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    (*DAT_000536c2)();
    return;
  case 0x3e:
    FUN_00047b4f(&DAT_00053668 + iVar5,&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                   *(ushort *)(&DAT_00045e22 + iVar4);
    _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
    _DAT_00053650 =
         CONCAT22(_UNK_00053652,
                  DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
    (*DAT_000536c2)();
    return;
  }
  FUN_000461cc();
  return;
switchD_000462f7_caseD_39:
  switch(param_3) {
  case (undefined4 *)0x0:
    uVar18 = FUN_00048585(&DAT_00053668 + iVar4,&DAT_00053668 + iVar5);
    _DAT_00053650 =
         CONCAT22(_UNK_00053652,
                  CONCAT11((byte)((_DAT_00053650 & 0xb8ff) >> 8) | (&DAT_00045e6e)[uVar18 & 7],
                           (char)(_DAT_00053650 & 0xb8ff)));
    (*DAT_000536c2)();
    return;
  case (undefined4 *)0x1:
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + iVar4) |
                   *(ushort *)(&DAT_00045e22 + iVar4);
    _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
    _DAT_00053650 =
         CONCAT22(_UNK_00053652,
                  DAT_00053650 | *(ushort *)(&DAT_00045dce + *(ushort *)(&DAT_00045dca + iVar4)));
    FUN_00048754(&DAT_00053668 + iVar4,&DAT_00053668 + *(ushort *)(&DAT_00045dca + iVar4));
    (*DAT_000536c2)();
    return;
  case (undefined4 *)0x2:
    FUN_00048e60(&DAT_00053668 + iVar4);
    (*DAT_000536c2)();
    return;
  case (undefined4 *)0x3:
    uVar11 = *(undefined4 *)(&DAT_00053668 + iVar4);
    uVar2 = *(undefined4 *)(&DAT_0005366c + iVar4);
    uVar3 = *(undefined4 *)(&DAT_0005366e + iVar4);
    FUN_00048c3f(&DAT_00053668 + iVar4);
    uVar15 = *(ushort *)(&DAT_00045dcc + iVar4);
    _DAT_00053650 = _DAT_00053650 & 0xffffc7ff;
    _DAT_00053650 = CONCAT22(_UNK_00053652,DAT_00053650 | *(ushort *)(&DAT_00045dce + uVar15));
    DAT_00053654 = DAT_00053654 & *(ushort *)(&DAT_00045e1a + uVar15);
    param_3 = (undefined4 *)(&DAT_00053668 + uVar15);
    *(undefined4 *)(&DAT_0005366e + uVar15) = uVar3;
    *(undefined4 *)(&DAT_0005366c + uVar15) = uVar2;
    *param_3 = uVar11;
    param_1 = (undefined *)FUN_00048c49(&DAT_00053668 + uVar15);
    break;
  case (undefined4 *)0x4:
    param_1 = (undefined *)FUN_000481df(&DAT_00053668 + iVar4,DAT_0005364c & 0xc00);
    break;
  case (undefined4 *)0x5:
    puVar13 = (undefined *)FUN_00047e9f();
    param_3 = (undefined4 *)(&DAT_00053668 + iVar4);
    sVar10 = (short)puVar13;
    param_1 = (undefined *)(int)sVar10;
    if (param_1 == puVar13) {
      uVar15 = *(ushort *)(&DAT_00053670 + iVar4) & 0x8000;
      uVar17 = (*(ushort *)(&DAT_00053670 + iVar4) & 0x7fff) + sVar10;
      if ((short)uVar17 < 0) {
        bVar14 = (byte)((uint)puVar13 >> 8);
        param_1 = (undefined *)((uint)CONCAT21(sVar10 >> 0xf,bVar14) << 8);
        if ((short)((ushort)bVar14 << 8) < 0) {
          uVar17 = 0;
          uVar15 = 0;
        }
        else {
          uVar17 = 0x7fff;
        }
      }
    }
    else if (extraout_CX < 0) {
      uVar17 = 0;
      uVar15 = 0;
    }
    else {
      uVar17 = 0x7fff;
      uVar15 = *(ushort *)(&DAT_00053670 + iVar4) & 0x8000;
    }
    *(ushort *)(&DAT_00053670 + iVar4) = uVar17 | uVar15;
    break;
  case (undefined4 *)0x6:
    FUN_00048c3f(&DAT_00053668 + iVar4);
    (*DAT_000536c2)();
    return;
  case (undefined4 *)0x7:
    FUN_00048c49(&DAT_00053668 + iVar4);
    (*DAT_000536c2)();
    return;
  }
  goto code_r0x000461cc;
}


