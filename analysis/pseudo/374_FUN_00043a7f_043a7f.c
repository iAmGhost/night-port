// func #305  FUN_00043a7f  00043a7f  size=545


int FUN_00043a7f(undefined4 *param_1,byte *param_2,undefined4 param_3)

{
  byte *pbVar1;
  int iVar2;
  uint uVar3;
  undefined4 uVar4;
  uint uVar5;
  undefined4 uVar6;
  int iVar7;
  byte *local_18;
  int local_14;
  
  iVar7 = 0;
  local_14 = 0;
  *(byte *)(param_1 + 4) = *(byte *)(param_1 + 4) & 0xfd;
  local_18 = param_2;
  do {
    pbVar1 = local_18 + 1;
    uVar5 = (uint)*local_18;
    if (uVar5 == 0) goto LAB_00043c80;
    if (((&DAT_0005185c)[(byte)(*local_18 + 1)] & 2) != 0) {
      local_18 = pbVar1;
      iVar2 = FUN_00043d79(param_1);
      iVar7 = iVar7 + iVar2;
      goto LAB_00043c4c;
    }
    if (uVar5 == 0x25) {
      local_18 = pbVar1;
      local_18 = (byte *)FUN_00043ca0(pbVar1,param_1);
      uVar5 = (uint)*local_18;
      if (uVar5 != 0) {
        local_18 = local_18 + 1;
      }
      if (uVar5 < 0x65) {
        if (0x57 < uVar5) {
          if (uVar5 < 0x59) goto LAB_00043beb;
          if (uVar5 < 99) {
            if (uVar5 != 0x5b) goto LAB_00043c4c;
            iVar2 = FUN_00043fc3(param_1,param_3,&local_18);
          }
          else {
            if (99 < uVar5) {
              uVar6 = 10;
              goto LAB_00043bba;
            }
            iVar2 = FUN_00043dae(param_1,param_3);
          }
          goto LAB_00043c1e;
        }
        if (uVar5 < 0x45) {
          if (uVar5 == 0x25) {
            uVar3 = (*(code *)*param_1)(param_1,local_18);
            if (uVar3 != uVar5) goto LAB_00043ae6;
            goto LAB_00043c4b;
          }
        }
        else if ((uVar5 < 0x46) || (uVar5 == 0x47)) goto LAB_00043bf2;
      }
      else if (uVar5 < 0x68) {
LAB_00043bf2:
        iVar2 = FUN_000440a9(param_1,param_3);
LAB_00043c1e:
        if (iVar2 < 1) goto LAB_00043c80;
        iVar7 = iVar7 + iVar2;
        if ((*(byte *)(param_1 + 4) & 1) != 0) {
          local_14 = local_14 + 1;
        }
      }
      else if (uVar5 < 0x70) {
        if (uVar5 < 0x6e) {
          if (uVar5 == 0x69) {
            uVar4 = 1;
            uVar6 = 0;
            goto LAB_00043bc3;
          }
        }
        else {
          if (0x6e < uVar5) {
            uVar6 = 8;
            goto LAB_00043bba;
          }
          FUN_00043f22(param_1,param_3,iVar7);
        }
      }
      else {
        if (uVar5 < 0x71) {
LAB_00043beb:
          uVar6 = 0x10;
LAB_00043bba:
          uVar4 = 1;
LAB_00043bc3:
          iVar2 = FUN_00044387(param_1,param_3,uVar6,uVar4);
          goto LAB_00043c1e;
        }
        if (uVar5 < 0x75) {
          if (uVar5 == 0x73) {
            iVar2 = FUN_00043e34(param_1,param_3);
            goto LAB_00043c1e;
          }
        }
        else {
          if (uVar5 < 0x76) {
            uVar6 = 10;
            uVar4 = 0;
            goto LAB_00043bc3;
          }
          if (uVar5 == 0x78) goto LAB_00043beb;
        }
      }
    }
    else {
      local_18 = pbVar1;
      uVar3 = (*(code *)*param_1)(param_1);
      if (uVar3 != uVar5) {
LAB_00043ae6:
        if ((*(byte *)(param_1 + 4) & 2) == 0) {
          (*(code *)param_1[1])(uVar3,param_1);
        }
        goto LAB_00043c80;
      }
LAB_00043c4b:
      iVar7 = iVar7 + 1;
    }
LAB_00043c4c:
  } while ((*(byte *)(param_1 + 4) & 2) == 0);
  if (*local_18 == 0x25) {
    local_18 = local_18 + 1;
    local_18 = (byte *)FUN_00043ca0(local_18,param_1);
    if (*local_18 == 0x6e) {
      FUN_00043f22(param_1,param_3,iVar7);
    }
  }
LAB_00043c80:
  if ((local_14 == 0) && ((*(byte *)(param_1 + 4) & 2) != 0)) {
    local_14 = -1;
  }
  return local_14;
}


