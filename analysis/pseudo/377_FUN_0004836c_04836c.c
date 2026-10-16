// func #377  FUN_0004836c  0004836c  size=182


void FUN_0004836c(int param_1,int param_2,undefined4 *param_3)

{
  short extraout_CX;
  
  if ((*(ushort *)(param_1 + 8) & 0x7fff) == 0) {
    if ((*(byte *)(param_2 + 9) & 0x80) == 0) {
      *(undefined2 *)(param_3 + 2) = 0;
      param_3[1] = 0;
      *param_3 = 0;
    }
    else {
      *(undefined2 *)(param_3 + 2) = 0x4000;
      param_3[1] = 0xc90fdaa2;
      *param_3 = 0x2168c235;
    }
  }
  else if ((*(ushort *)(param_2 + 8) & 0x7fff) == 0) {
    *(ushort *)(param_3 + 2) = *(ushort *)(param_1 + 8) & 0x8000 | 0x3fff;
    param_3[1] = 0xc90fdaa2;
    *param_3 = 0x2168c235;
  }
  else {
    FUN_00047b4f();
    FUN_00048422(param_3);
    if (extraout_CX < 0) {
      if ('\0' < *(char *)((int)param_3 + 9)) {
        FUN_0004791a(param_3);
      }
    }
    else if (*(char *)((int)param_3 + 9) < '\0') {
      FUN_0004791a(param_3);
    }
  }
  return;
}


