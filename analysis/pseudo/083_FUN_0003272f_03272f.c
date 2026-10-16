// func #83  FUN_0003272f  0003272f  size=7014


void FUN_0003272f(void)

{
  int *piVar1;
  byte bVar2;
  uint uVar3;
  int iVar4;
  char cVar5;
  int *piVar6;
  int local_38;
  uint local_34;
  int local_30;
  int local_2c;
  
  piVar6 = (int *)FUN_000418ee();
  if (*(char *)(DAT_00053c7c + 0x30) == '\x01') {
    return;
  }
  if (DAT_0006339c == 500) {
    *(undefined4 *)(DAT_00053c7c + 0x17) = 0x28;
    *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
  }
  if ((*(int *)(DAT_00053c7c + 0x17) == 0x66) &&
     ((cVar5 = FUN_0003493e(), cVar5 == '*' || (cVar5 == '6')))) {
    *(undefined4 *)(*(int *)(DAT_00053c7c + 0x4a) + 0x17) = 1;
    *(undefined4 *)(*(int *)(DAT_00053c7c + 0x4a) + 0x11) = 0;
    *(undefined4 *)(*(int *)(DAT_00053c7c + 0x4a) + 0x4a) = 0;
    *(undefined2 *)(*(int *)(DAT_00053c7c + 0x4a) + 0x50) = 0;
    *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
    *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
    *(undefined4 *)(DAT_00053c7c + 0x4a) = 0;
    *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
    FUN_000348e2();
    return;
  }
  if ((*(int *)(DAT_00053c7c + 0x17) == 0x66) || (*(int *)(DAT_00053c7c + 0x17) == 0x70))
  goto LAB_000339c5;
  uVar3 = *(uint *)(DAT_00053c7c + 0x17);
  if (uVar3 < 0x47) {
    if (uVar3 < 0x15) {
      if (uVar3 < 3) {
        if (uVar3 != 0) {
LAB_0003284e:
          cVar5 = FUN_0003493e();
          if ((((DAT_00063ed8 != '\0') || (DAT_00063ed0 != '\0')) || (DAT_00063ed3 != '\0')) ||
             (DAT_00063ed5 != '\0')) {
            iVar4 = *(int *)(&DAT_00052ffc +
                            ((DAT_00063360 % 4) * 3 + (uint)*(byte *)(DAT_00053c7c + 0x25)) * 4);
            if (DAT_00063ed0 == '\0') {
              if (DAT_00063ed8 != '\0') {
                FUN_0001586b(DAT_00053c7c,iVar4);
              }
            }
            else {
              FUN_0001586b(DAT_00053c7c,-iVar4);
            }
            if (DAT_00063ed3 == '\0') {
              if (DAT_00063ed5 != '\0') {
                FUN_0001545e(DAT_00053c7c,iVar4);
                *(undefined4 *)(DAT_00053c7c + 0x1b) = 0;
              }
            }
            else {
              FUN_0001545e(DAT_00053c7c,-iVar4);
              *(undefined4 *)(DAT_00053c7c + 0x1b) = 1;
            }
            if (*(int *)(DAT_00053c7c + 0x17) != 2) {
              *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
              *(undefined4 *)(DAT_00053c7c + 0x17) = 2;
            }
          }
          if (((DAT_00063ec1 != '\0') || (DAT_00063eb4 != '\0')) &&
             (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x97;
            *(undefined4 *)(DAT_00053c7c + 0x42) = 0;
            FUN_000348e2();
          }
          if ((cVar5 == '*') || (cVar5 == '6')) {
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            if ((DAT_00063ed3 == '\0') && (DAT_00063ed5 == '\0')) {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x83;
              *(undefined4 *)(DAT_00053c7c + 0x42) = 0;
              DAT_00062d58 = 0x83;
            }
            else {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x84;
              DAT_00062d58 = 0x84;
            }
            FUN_000348e2();
          }
        }
      }
      else if (uVar3 < 4) {
        cVar5 = FUN_0003493e();
        if (((cVar5 == '8') || (cVar5 == '\x1d')) && (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0xa3;
          DAT_00062d58 = 0xa3;
          FUN_000348e2();
        }
        if (((cVar5 == '*') || (cVar5 == '6')) && (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x85;
          DAT_00062d58 = 0x85;
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          FUN_000348e2();
        }
        if (*(int *)(DAT_00053c7c + 0x1b) == 0) {
          if ((DAT_00063ed5 == '\0') &&
             ((*(int *)(DAT_00053c7c + 0x11) == 0x13 || (*(int *)(DAT_00053c7c + 0x11) == 0x2d)))) {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 4;
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          }
        }
        else if ((DAT_00063ed3 == '\0') &&
                ((*(int *)(DAT_00053c7c + 0x11) == 0x13 || (*(int *)(DAT_00053c7c + 0x11) == 0x2d)))
                ) {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 4;
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
        }
      }
      else if (uVar3 == 5) {
        cVar5 = FUN_0003493e();
        if (*(int *)(DAT_00053c7c + 0x11) == 8) {
          if ((cVar5 == '8') && (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x9c;
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
            *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
            *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
            FUN_000348e2();
          }
        }
        else if (*(int *)(DAT_00053c7c + 0x11) == 0x11) {
          *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
          FUN_000348e2();
        }
      }
    }
    else if (uVar3 < 0x16) {
      cVar5 = FUN_0003493e();
      if ((*(int *)(DAT_00053c7c + 0x11) < 0x32) && ((cVar5 == '*' || (cVar5 == '6')))) {
        if (((*(short *)(DAT_00053c7c + 0x23) != 0) || (*(short *)(DAT_00053c7c + 0x29) != 0)) &&
           (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x98;
          *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
          *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
          if ((*(short *)(DAT_00053c7c + 0x23) == 0) && (*(char *)(DAT_00053c7c + 0x30) == '\x01'))
          {
            *(undefined1 *)(DAT_00053c7c + 0x30) = 2;
          }
        }
        FUN_000348e2();
      }
      else if ((*(int *)(DAT_00053c7c + 0x11) < 0x56) && ((cVar5 == '*' || (cVar5 == '6')))) {
        *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
      }
    }
    else if (uVar3 < 0x1e) {
      if (uVar3 == 0x16) {
        cVar5 = FUN_0003493e();
        if ((*(int *)(DAT_00053c7c + 0x11) < 0x1e) && ((cVar5 == '*' || (cVar5 == '6')))) {
          if (((*(short *)(DAT_00053c7c + 0x23) != 0) || (*(short *)(DAT_00053c7c + 0x29) != 0)) &&
             (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x9b;
            *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
            *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
            if ((*(short *)(DAT_00053c7c + 0x23) == 0) && (*(char *)(DAT_00053c7c + 0x30) == '\x01')
               ) {
              *(undefined1 *)(DAT_00053c7c + 0x30) = 2;
            }
            FUN_000348e2();
          }
        }
        else if ((*(int *)(DAT_00053c7c + 0x11) < 0x44) && ((cVar5 == '*' || (cVar5 == '6')))) {
          *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
          FUN_000348e2();
        }
      }
    }
    else if (uVar3 < 0x1f) {
      cVar5 = FUN_0003493e();
      if ((((9 < *(int *)(DAT_00053c7c + 0x11)) && (*(int *)(DAT_00053c7c + 0x11) < 0x3b)) &&
          ((cVar5 == '*' || (cVar5 == '6')))) && (*(char *)(DAT_00053c7c + 0x30) != '\x01')) {
        *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
      }
    }
    else if (0x27 < uVar3) {
      if (uVar3 < 0x29) goto LAB_0003284e;
      if ((uVar3 == 0x29) &&
         (((cVar5 = FUN_0003493e(), cVar5 == '*' || (cVar5 == '6')) &&
          (*(char *)(DAT_00053c7c + 0x27) != '\0')))) {
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0x85;
        DAT_00062d58 = 0x85;
        *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
        FUN_000348e2();
      }
    }
  }
  else if (uVar3 < 0x48) {
    cVar5 = FUN_0003493e();
    if (((cVar5 == '*') || (cVar5 == '6')) && (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
      *(undefined4 *)(DAT_00053c7c + 0x17) = 0x85;
      DAT_00062d58 = 0x85;
      *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
      FUN_000348e2();
    }
  }
  else if (uVar3 < 0x88) {
    if (uVar3 < 0x84) {
      if (uVar3 == 0x83) {
        cVar5 = FUN_0003493e();
        if (*(int *)(DAT_00053c7c + 0x11) == 0x3e) {
          FUN_000348e2();
          *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
        }
        if ((cVar5 == 'g') && (*(int *)(DAT_00053c7c + 0x11) < 0x1c)) {
          *(undefined4 *)(DAT_00053c7c + 0x11f) = 0x1fffff;
        }
        FUN_000342ff(2);
        if ((cVar5 == '8') || (cVar5 == '\x1d')) {
          if ((0xb < *(int *)(DAT_00053c7c + 0x11)) && (*(int *)(DAT_00053c7c + 0x11) < 0x33)) {
            *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
            *(undefined4 *)(DAT_00053c7c + 0x42) = *(undefined4 *)(DAT_00053c7c + 0x11);
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            if (cVar5 == '\x1d') {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x88;
              FUN_000348e2();
            }
            else if (cVar5 == '8') {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x89;
              FUN_000348e2();
            }
          }
        }
        else if ((cVar5 == 'g') || (*(int *)(DAT_00053c7c + 0x11f) == 0x1fffff)) {
          if ((*(int *)(DAT_00053c7c + 0x11) == 0x1c) && (1 < *(byte *)(DAT_00053c7c + 0x27))) {
            *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
            if (*(char *)(DAT_00053c7c + 0x26) == '\x02') {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8d;
            }
            else {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8c;
            }
            *(undefined4 *)(DAT_00053c7c + 0x11f) = 0;
            FUN_000348e2();
          }
        }
        else if ((((cVar5 == 'i') && (3 < *(byte *)(DAT_00053c7c + 0x27))) &&
                 (0xb < *(int *)(DAT_00053c7c + 0x11))) && (*(int *)(DAT_00053c7c + 0x11) < 0x33)) {
          *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
          *(undefined4 *)(DAT_00053c7c + 0x42) = *(undefined4 *)(DAT_00053c7c + 0x11);
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8b;
          FUN_000348e2();
        }
      }
    }
    else if (uVar3 < 0x85) {
      cVar5 = FUN_0003493e();
      if (*(int *)(DAT_00053c7c + 0x11) == 0x3e) {
        FUN_000348e2();
        *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
      }
      if ((cVar5 == 'g') && (*(int *)(DAT_00053c7c + 0x11) < 0x1c)) {
        *(undefined4 *)(DAT_00053c7c + 0x11f) = 0x1fffff;
      }
      FUN_00034295(3);
      if ((cVar5 == '8') || (cVar5 == '\x1d')) {
        if ((0xb < *(int *)(DAT_00053c7c + 0x11)) && (*(int *)(DAT_00053c7c + 0x11) < 0x33)) {
          *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
          *(undefined4 *)(DAT_00053c7c + 0x42) = *(undefined4 *)(DAT_00053c7c + 0x11);
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          if (cVar5 == '\x1d') {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x88;
            FUN_000348e2();
          }
          else if (cVar5 == '8') {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8a;
            FUN_000348e2();
          }
        }
      }
      else if ((((cVar5 == 'g') || (*(int *)(DAT_00053c7c + 0x11f) == 0x1fffff)) &&
               (*(int *)(DAT_00053c7c + 0x11) == 0x1c)) && (1 < *(byte *)(DAT_00053c7c + 0x27))) {
        *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
        if (*(char *)(DAT_00053c7c + 0x26) == '\x02') {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8d;
        }
        else {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8c;
        }
        *(undefined4 *)(DAT_00053c7c + 0x11f) = 0;
        FUN_000348e2();
      }
    }
    else if (uVar3 == 0x85) {
      cVar5 = FUN_0003493e();
      if (*(int *)(DAT_00053c7c + 0x11) == 0x3e) {
        FUN_000348e2();
        *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
      }
      if ((cVar5 == 'g') && (*(int *)(DAT_00053c7c + 0x11) < 0x1c)) {
        *(undefined4 *)(DAT_00053c7c + 0x11f) = 0x1fffff;
      }
      FUN_00034295(1);
      if ((cVar5 == '8') || (cVar5 == '\x1d')) {
        if ((0xb < *(int *)(DAT_00053c7c + 0x11)) && (*(int *)(DAT_00053c7c + 0x11) < 0x33)) {
          *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
          *(undefined4 *)(DAT_00053c7c + 0x42) = *(undefined4 *)(DAT_00053c7c + 0x11);
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
          if (cVar5 == '\x1d') {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x88;
            FUN_000348e2();
          }
          else if (cVar5 == '8') {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8a;
            FUN_000348e2();
          }
        }
      }
      else if ((((cVar5 == 'g') || (*(int *)(DAT_00053c7c + 0x11f) == 0x1fffff)) &&
               (*(int *)(DAT_00053c7c + 0x11) == 0x1c)) && (1 < *(byte *)(DAT_00053c7c + 0x27))) {
        *(undefined4 *)(DAT_00053c7c + 0x3e) = *(undefined4 *)(DAT_00053c7c + 0x17);
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0x8d;
        *(undefined4 *)(DAT_00053c7c + 0x11f) = 0;
        FUN_000348e2();
      }
    }
  }
  else if (uVar3 < 0x8e) {
    if ((*(int *)(DAT_00053c7c + 0x17) == 0x8c) && (*(int *)(DAT_00053c7c + 0x11) == 0x67)) {
      FUN_000348e2();
      *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
    }
    if ((*(int *)(DAT_00053c7c + 0x17) != 0x8c) || (*(int *)(DAT_00053c7c + 0x11) < 0x35)) {
      if ((*(int *)(DAT_00053c7c + 0x17) == 0x8d) && (*(int *)(DAT_00053c7c + 0x11) == 0x62)) {
        FUN_000348e2();
        *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
      }
      if ((*(int *)(DAT_00053c7c + 0x17) != 0x8d) || (*(int *)(DAT_00053c7c + 0x11) < 0x2f)) {
        if (*(int *)(DAT_00053c7c + 0x3e) == 0x83) {
          FUN_000342ff(2);
        }
        else if (*(int *)(DAT_00053c7c + 0x3e) == 0x84) {
          FUN_00034295(3);
        }
        else if (*(int *)(DAT_00053c7c + 0x3e) == 0x85) {
          FUN_00034295(1);
        }
        if (((*(int *)(DAT_00053c7c + 0x42) == 0x39) && (*(int *)(DAT_00053c7c + 0x17) != 0x8c)) &&
           (*(int *)(DAT_00053c7c + 0x17) != 0x8d)) {
          *(undefined4 *)(DAT_00053c7c + 0x17) = *(undefined4 *)(DAT_00053c7c + 0x3e);
          *(undefined4 *)(DAT_00053c7c + 0x11) = 0x39;
        }
      }
    }
  }
  else if (uVar3 < 0x98) {
    if (uVar3 == 0x97) {
      FUN_0003493e();
      if (((DAT_00063ec1 != '\0') || (DAT_00063eb4 != '\0')) &&
         (0x13 < *(int *)(DAT_00053c7c + 0x11))) {
        *(int *)(DAT_00053c7c + 0x11) = *(int *)(DAT_00053c7c + 0x11) + -1;
        *(int *)(DAT_00053c7c + 0x42) = *(int *)(DAT_00053c7c + 0x42) + 1;
      }
      if (((DAT_00063ea5 != '\0') &&
          (0x14 < (uint)(*(int *)(DAT_00053c7c + 0x11) + *(int *)(DAT_00053c7c + 0x42)))) &&
         (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
        DAT_00063364 = 0x26;
        DAT_00063368 = 0;
        *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0xa8;
        FUN_000348e2();
      }
      if (((DAT_00063ec0 != '\0') &&
          (0x14 < (uint)(*(int *)(DAT_00053c7c + 0x11) + *(int *)(DAT_00053c7c + 0x42)))) &&
         (1 < *(byte *)(DAT_00053c7c + 0x27))) {
        DAT_00063364 = 0x21;
        DAT_00063368 = 0;
        *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0xa9;
        FUN_000348e2();
      }
    }
  }
  else if (uVar3 < 0x99) {
LAB_0003358f:
    cVar5 = FUN_0003493e();
    if (cVar5 == '8') {
      *(undefined1 *)(DAT_00053c7c + 0x31) = 1;
      if (*(char *)(DAT_00053c7c + 0x27) == '\x04') {
        *(undefined4 *)(DAT_00053c7c + 0x32) = 0x2e;
      }
      else {
        *(undefined4 *)(DAT_00053c7c + 0x32) = 0x2d;
      }
    }
  }
  else if (0x9a < uVar3) {
    if (uVar3 < 0x9c) goto LAB_0003358f;
    if (((uVar3 == 0xa5) && (5 < *(int *)(DAT_00053c7c + 0x11))) && (DAT_00063ea5 == '\0')) {
      DAT_00063364 = 0x20;
      DAT_00063368 = 0;
      if ((*(int *)(DAT_00053c7c + 0x11) < 0) || (0x1e < *(int *)(DAT_00053c7c + 0x11))) {
        if ((*(int *)(DAT_00053c7c + 0x11) < 0x1f) || (0x3c < *(int *)(DAT_00053c7c + 0x11))) {
          if ((*(int *)(DAT_00053c7c + 0x11) < 0x3d) || (0x50 < *(int *)(DAT_00053c7c + 0x11))) {
            if ((0x50 < *(int *)(DAT_00053c7c + 0x11)) && (*(int *)(DAT_00053c7c + 0x11) < 0x65)) {
              *(undefined4 *)(DAT_00053c7c + 0x17) = 0x2b;
            }
          }
          else {
            *(undefined4 *)(DAT_00053c7c + 0x17) = 0x2a;
          }
        }
        else {
          *(undefined4 *)(DAT_00053c7c + 0x17) = 0x29;
        }
      }
      else {
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0xa6;
      }
      *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
    }
  }
  if ((((((10 < *(uint *)(DAT_00053c7c + 0x17)) && (*(uint *)(DAT_00053c7c + 0x17) < 0x14)) ||
        ((0x32 < *(uint *)(DAT_00053c7c + 0x17) && (*(uint *)(DAT_00053c7c + 0x17) < 0x51)))) ||
       ((*(int *)(DAT_00053c7c + 0x17) == 0xa4 || (*(int *)(DAT_00053c7c + 0x17) == 0xa6)))) ||
      ((*(int *)(DAT_00053c7c + 0x17) == 0xa7 ||
       ((*(int *)(DAT_00053c7c + 0x17) == 0x2f ||
        ((0xaa < *(uint *)(DAT_00053c7c + 0x17) && (*(uint *)(DAT_00053c7c + 0x17) < 0xae)))))))) ||
     ((0xb4 < *(uint *)(DAT_00053c7c + 0x17) && (*(uint *)(DAT_00053c7c + 0x17) < 0xb9)))) {
    cVar5 = FUN_0003493e();
    if (((DAT_00063ec1 != '\0') || (DAT_00063eb4 != '\0')) &&
       (*(char *)(DAT_00053c7c + 0x27) != '\0')) {
      *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
      *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
      *(undefined4 *)(DAT_00053c7c + 0x17) = 0x97;
      *(undefined4 *)(DAT_00053c7c + 0x42) = 0;
      *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
      FUN_000348e2();
    }
    if ((cVar5 == '*') || (cVar5 == '6')) {
      *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
      if ((DAT_00063ed3 == '\0') && (DAT_00063ed5 == '\0')) {
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0x83;
        *(undefined4 *)(DAT_00053c7c + 0x42) = 0;
        DAT_00062d58 = 0x83;
      }
      else {
        *(undefined4 *)(DAT_00053c7c + 0x17) = 0x84;
        DAT_00062d58 = 0x84;
      }
      *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
      FUN_000348e2();
      *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
    }
  }
  if (((DAT_00063ed8 == '\0') && (DAT_00063ed0 == '\0')) &&
     ((DAT_00063ed3 == '\0' && ((DAT_00063ed5 == '\0' && (*(uint *)(DAT_00053c7c + 0x17) < 3)))))) {
    *(undefined4 *)(DAT_00053c7c + 0x17) = 1;
  }
LAB_000339c5:
  if (((*(uint *)(DAT_00053c7c + 0x17) < 0x15) || (0x1e < *(uint *)(DAT_00053c7c + 0x17))) &&
     (*(int *)(DAT_00053c7c + 0x17) != 5)) {
    for (local_38 = 0; local_38 < *piVar6; local_38 = local_38 + 1) {
      local_30 = (uint)DAT_00063f50 - (uint)*(byte *)((int)piVar6 + local_38 + 4);
      local_2c = DAT_00063f50 - 1;
      if (local_30 < 0) {
        local_30 = local_30 + 10;
      }
      if (local_2c < 0) {
        local_2c = DAT_00063f50 + 9;
      }
      if (((&DAT_00063f14)[local_30] == *(char *)piVar6[local_38 + 0x1a]) &&
         ((&DAT_00063f1e)[local_2c] != '\x01')) {
        for (local_34 = 1; (int)local_34 < (int)(uint)*(byte *)((int)piVar6 + local_38 + 4);
            local_34 = local_34 + 1) {
          if (local_30 == 9) {
            local_30 = 0;
          }
          else {
            local_30 = local_30 + 1;
          }
          if (((&DAT_00063f14)[local_30] != *(char *)(piVar6[local_38 + 0x1a] + local_34)) ||
             ((int)(uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1) <
              *(int *)(&DAT_00063f28 + local_30 * 4))) break;
        }
        if ((*(byte *)((int)piVar6 + local_38 + 4) == local_34) &&
           (*(byte *)((int)piVar6 + local_38 + 0x450) <= *(byte *)(DAT_00053c7c + 0x27))) {
          if (*(short *)((int)piVar6 + local_38 * 2 + 0x388) == 0x70) {
            piVar1 = (int *)(*(int *)(DAT_00053c7c + 0x4a) + 0x52);
            *piVar1 = *piVar1 + 1;
          }
          if (((*(int *)(DAT_00053c7c + 0x4a) != 0) &&
              (*(int *)(*(int *)(DAT_00053c7c + 0x4a) + 0x52) == 5)) &&
             (*(short *)((int)piVar6 + local_38 * 2 + 0x388) == 0x70)) {
            if (DAT_00062d58 == 0x71) {
              (&DAT_00063f1e)[local_2c] = 1;
            }
            else {
              if (*(char *)(DAT_00053c7c + 0x31) == '\0') {
                (&DAT_00063f1e)[local_2c] = 1;
                *(undefined4 *)(DAT_00053c7c + 0x17) = 0x71;
                *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
                *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
                bVar2 = *(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
              }
              else {
                (&DAT_00063f1e)[local_2c] = 1;
                FUN_00031f10(DAT_00053c7c,0x71);
                *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
                bVar2 = *(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
              }
              DAT_00062d5c = (uint)bVar2;
              DAT_00062d58 = 0x71;
            }
            *(undefined4 *)(*(int *)(DAT_00053c7c + 0x4a) + 0x52) = 0;
            return;
          }
          if (((((((10 < *(uint *)(DAT_00053c7c + 0x17)) && (*(uint *)(DAT_00053c7c + 0x17) < 0x14))
                 || ((0x32 < *(uint *)(DAT_00053c7c + 0x17) &&
                     (*(uint *)(DAT_00053c7c + 0x17) < 0x51)))) ||
                ((*(int *)(DAT_00053c7c + 0x17) == 0xa4 || (*(int *)(DAT_00053c7c + 0x17) == 0xa6)))
                ) || (*(int *)(DAT_00053c7c + 0x17) == 0xa7)) ||
              (((*(int *)(DAT_00053c7c + 0x17) == 0x2f ||
                ((0xaa < *(uint *)(DAT_00053c7c + 0x17) && (*(uint *)(DAT_00053c7c + 0x17) < 0xae)))
                ) || ((0xb4 < *(uint *)(DAT_00053c7c + 0x17) &&
                      (*(uint *)(DAT_00053c7c + 0x17) < 0xb9)))))) &&
             (((*(short *)((int)piVar6 + local_38 * 2 + 0x388) == 0x65 ||
               (*(short *)((int)piVar6 + local_38 * 2 + 0x388) == 0x30)) ||
              (*(short *)((int)piVar6 + local_38 * 2 + 0x388) == 0x31)))) {
            (&DAT_00063f1e)[local_2c] = 1;
            *(uint *)(DAT_00053c7c + 0x17) = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
            *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            *(undefined1 *)(DAT_00053c7c + 0x31) = 0;
            DAT_00062d58 = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
            DAT_00062d5c = (uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
            return;
          }
          if ((10 < *(uint *)(DAT_00053c7c + 0x17)) && (*(uint *)(DAT_00053c7c + 0x17) < 0x15)) {
            if (*(short *)((int)piVar6 + local_38 * 2 + 0x388) != 0x99) {
              FUN_000348e2();
              return;
            }
            (&DAT_00063f1e)[local_2c] = 1;
            *(uint *)(DAT_00053c7c + 0x17) = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
            iVar4 = *(int *)(*(int *)(*(int *)(DAT_00053c7c + 0x123) + 4) +
                            *(int *)(DAT_00053c7c + 0x17) * 4);
            if (*(char *)(iVar4 + 0x69) != '\0') {
              DAT_00063364 = (uint)*(byte *)(iVar4 + 0x69);
              DAT_00063368 = (uint)*(ushort *)(iVar4 + 0x6a);
            }
            *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
            *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
            DAT_00062d58 = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
            DAT_00062d5c = (uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
            return;
          }
          if (*(char *)(piVar6[local_38 + 0x7e] + (uint)*(byte *)((int)piVar6 + local_38 + 4) + -1)
              == '\0') {
            FUN_000348e2();
          }
          if (((*(int *)(DAT_00053c7c + 0x17) == 1) || (*(int *)(DAT_00053c7c + 0x17) == 2)) ||
             (*(int *)(DAT_00053c7c + 0x17) == 0x66)) {
            if (((byte)(&DAT_00062b90)[*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)] < 2) ||
               ((uint)(byte)(&DAT_00062b90)[DAT_00062d58] ==
                (byte)(&DAT_00062b90)[*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)] - 1)) {
              (&DAT_00063f1e)[local_2c] = 1;
              *(uint *)(DAT_00053c7c + 0x17) = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)
              ;
              iVar4 = *(int *)(*(int *)(*(int *)(DAT_00053c7c + 0x123) + 4) +
                              *(int *)(DAT_00053c7c + 0x17) * 4);
              if (*(char *)(iVar4 + 0x69) != '\0') {
                DAT_00063364 = (uint)*(byte *)(iVar4 + 0x69);
                DAT_00063368 = (uint)*(ushort *)(iVar4 + 0x6a);
              }
              *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
              *(undefined4 *)(DAT_00053c7c + 0x11) = 0;
              DAT_00062d58 = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
              DAT_00062d5c = (uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
              return;
            }
          }
          else if (*(int *)(DAT_00053c7c + 0x17) == 0x70) {
            if (*(byte *)(DAT_00053c7c + 0x31) < 5) {
              if (DAT_00062d58 != 0x71) {
                (&DAT_00063f1e)[local_2c] = 1;
                FUN_00031f10(DAT_00053c7c,*(undefined2 *)((int)piVar6 + local_38 * 2 + 0x388));
                *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
                DAT_00062d58 = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
                DAT_00062d5c = (uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
                return;
              }
              FUN_000348e2();
            }
          }
          else if (((&DAT_00062ac8)[DAT_00062d58] ==
                    (&DAT_00062ac8)[*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)]) &&
                  ((uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388) !=
                   *(uint *)(DAT_00053c7c + 0x17))) {
            if ((byte)(&DAT_00062b90)[*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)] <=
                (byte)(&DAT_00062b90)[DAT_00062d58]) {
              FUN_000348e2();
              return;
            }
            if ((uint)(byte)(&DAT_00062b90)[DAT_00062d58] !=
                (byte)(&DAT_00062b90)[*(ushort *)((int)piVar6 + local_38 * 2 + 0x388)] - 1) {
              return;
            }
            if (*(byte *)(DAT_00053c7c + 0x31) < 5) {
              (&DAT_00063f1e)[local_2c] = 1;
              FUN_00031f10(DAT_00053c7c,*(undefined2 *)((int)piVar6 + local_38 * 2 + 0x388));
              *(undefined2 *)(DAT_00053c7c + 0x50) = 0;
              DAT_00062d58 = (uint)*(ushort *)((int)piVar6 + local_38 * 2 + 0x388);
              DAT_00062d5c = (uint)*(byte *)(piVar6[local_38 + 0x7e] + local_34 + -1);
              return;
            }
          }
        }
      }
    }
  }
  return;
}


