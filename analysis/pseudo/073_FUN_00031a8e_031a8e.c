// func #73  FUN_00031a8e  00031a8e  size=991


void FUN_00031a8e(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  
  iVar1 = FUN_000418ee();
  if ((*(int *)(iVar1 + 0x17) == 2) &&
     (*(int *)(iVar1 + 0x103) = *(int *)(iVar1 + 0x103) + 1, *(int *)(iVar1 + 0x103) == 8)) {
    *(undefined4 *)(iVar1 + 0x103) = 0;
    iVar2 = FUN_0001519f(iVar1);
    if ((iVar2 == *(int *)(iVar1 + 0x107)) &&
       (iVar2 = FUN_000151d5(iVar1), iVar2 == *(int *)(iVar1 + 0x10b))) {
      *(undefined4 *)(iVar1 + 0x17) = 1;
      *(undefined4 *)(iVar1 + 0x1f) = 0;
      *(undefined4 *)(iVar1 + 0xaf) = 1;
      *(undefined4 *)(iVar1 + 199) = 1;
    }
    else {
      uVar3 = FUN_0001519f(iVar1);
      *(undefined4 *)(iVar1 + 0x107) = uVar3;
      uVar3 = FUN_000151d5(iVar1);
      *(undefined4 *)(iVar1 + 0x10b) = uVar3;
    }
  }
  iVar2 = FUN_0001519f(DAT_00053c7c);
  iVar4 = FUN_000151d5(DAT_00053c7c);
  if (*(int *)(iVar1 + 0x17) == 3) {
    if (((*(int *)(iVar1 + 0x6b) == 0) && (iVar5 = FUN_0001519f(iVar1), iVar5 < iVar2 + 200)) &&
       (iVar5 = FUN_0001519f(iVar1), iVar2 + -200 < iVar5)) {
      *(int *)(iVar1 + 0x6b) = *(int *)(iVar1 + 0x6b) + 1;
    }
    else if (*(int *)(iVar1 + 0x6b) != 0) {
      *(int *)(iVar1 + 0x6b) = *(int *)(iVar1 + 0x6b) + 1;
      if (*(int *)(&DAT_00053eac + *(int *)(iVar1 + 0xd) * 0x88) < 0x1f) {
        if (999 < *(int *)(iVar1 + 0x6b)) {
          *(undefined4 *)(iVar1 + 0x17) = 6;
          *(undefined4 *)(iVar1 + 0x11) = 0;
        }
      }
      else if (99 < *(int *)(iVar1 + 0x6b)) {
        *(undefined4 *)(iVar1 + 0x17) = 6;
        *(undefined4 *)(iVar1 + 0x11) = 0;
      }
      iVar5 = FUN_0001519f(iVar1);
      if (((iVar2 < iVar5 + 0x50) && (iVar5 = FUN_0001519f(iVar1), iVar5 + -0x50 < iVar2)) &&
         ((iVar5 = FUN_000151d5(iVar1), iVar4 < iVar5 + 0x28 &&
          (iVar5 = FUN_000151d5(iVar1), iVar5 + -0x28 < iVar4)))) {
        *(undefined4 *)(iVar1 + 0x17) = 6;
        *(undefined4 *)(iVar1 + 0x11) = 0;
      }
      iVar4 = FUN_0001519f(iVar1);
      if ((iVar2 < iVar4 + 10) && (iVar4 = FUN_0001519f(iVar1), iVar4 + -10 < iVar2)) {
        *(undefined4 *)(iVar1 + 0x17) = 6;
        *(undefined4 *)(iVar1 + 0x11) = 0;
      }
    }
  }
  if (iVar1 != DAT_00053c7c) {
    if ((0xb4 < *(uint *)(DAT_00053c7c + 0x17)) && (*(uint *)(DAT_00053c7c + 0x17) < 0xba)) {
      *(undefined4 *)(DAT_00053c7c + 0x4a) = 0;
    }
    switch(*(undefined4 *)(&DAT_00053eac + *(int *)(iVar1 + 0xd) * 0x88)) {
    case 0:
      FUN_00017f13(iVar1);
      break;
    case 1:
      FUN_00018e88(iVar1);
      break;
    case 2:
      FUN_00019e48(iVar1);
      break;
    case 3:
      FUN_0001aec8(iVar1);
      break;
    case 4:
      FUN_0001bea8(iVar1);
      break;
    case 5:
      FUN_0001cf64(iVar1,2);
      break;
    case 6:
      FUN_0001df8b(iVar1);
      break;
    case 7:
      FUN_0001f064(iVar1);
      break;
    case 8:
      FUN_00020170(iVar1);
      break;
    case 9:
      FUN_0001cf64(iVar1,0);
      break;
    case 10:
      FUN_0001cf64(iVar1,1);
      break;
    case 0xb:
      FUN_0002119c(iVar1);
      break;
    default:
      break;
    case 0x15:
      FUN_000221b8(iVar1);
      break;
    case 0x1f:
      FUN_0002319c(iVar1,2);
      break;
    case 0x20:
      FUN_00024bf9(iVar1);
      break;
    case 0x21:
      FUN_000265ac(iVar1,0);
      break;
    case 0x22:
      FUN_00027bec(iVar1,0);
      break;
    case 0x23:
      FUN_00029709(iVar1,0);
      break;
    case 0x24:
      FUN_0002adf5(iVar1,0);
      break;
    case 0x25:
      FUN_0002c838(iVar1,0);
      break;
    case 0x26:
      FUN_0002e2cc(iVar1,0);
      break;
    case 0x27:
      FUN_0002fe1e(iVar1,0);
    }
  }
  return;
}


