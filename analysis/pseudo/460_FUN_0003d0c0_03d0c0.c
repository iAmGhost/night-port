// func #460  FUN_0003d0c0  0003d0c0  size=387

void FUN_0003d0c0(void)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_000418ee();
  if (DAT_000639e4 == '\0') {
    FUN_0003ba2c((piVar1[0xd2] + DAT_000639dc) - *piVar1,*(undefined4 *)((int)piVar1 + 0x1a));
  }
  else if (DAT_000639e0 < piVar1[1]) {
    FUN_0003ba2c(piVar1[0xd2],*(undefined4 *)((int)piVar1 + 0x1a));
    FUN_0003ba2c(((*(int *)(piVar1[0xd4] + 0x348) + DAT_000639dc) - *(int *)piVar1[0xd4]) +
                 (200 - (piVar1[1] - DAT_000639e0)) * *(int *)(piVar1[0xd4] + 0x1a),
                 *(undefined4 *)(piVar1[0xd4] + 0x1a));
  }
  else if (piVar1[3] < DAT_000639e0 + 200) {
    FUN_0003ba2c(piVar1[0xd2] + (DAT_000639e0 - piVar1[1]) * 0x140,
                 *(undefined4 *)((int)piVar1 + 0x1a));
    FUN_0003ba2c(*(int *)(piVar1[0xd3] + 0x348) + (DAT_000639dc - *(int *)piVar1[0xd3]),
                 *(undefined4 *)(piVar1[0xd3] + 0x1a));
  }
  else {
    FUN_0003ba2c(piVar1[0xd2] + (DAT_000639e0 - piVar1[1]) * *(int *)((int)piVar1 + 0x1a),
                 *(undefined4 *)((int)piVar1 + 0x1a));
  }
  *(undefined4 *)((int)piVar1 + 0x22) = 0;
  *(undefined4 *)((int)piVar1 + 0x1e) = 0;
  return;
}
