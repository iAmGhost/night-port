// func #375  FUN_0004811a  0004811a  size=69


void FUN_0004811a(uint param_1,undefined4 *param_2)

{
  uint uVar1;
  ushort uVar2;
  undefined2 uVar4;
  uint uVar3;
  
  uVar2 = (ushort)((int)param_1 >> 0x17) & 0xff;
  uVar1 = (int)param_1 >> 0x17 & 0xffff00ff;
  if (uVar2 != 0) {
    param_1 = param_1 << 8;
    uVar4 = (undefined2)(uVar1 >> 0x10);
    if ((char)uVar1 == -1) {
      uVar3 = CONCAT22(uVar4,0xffff);
      uVar1 = param_1 & 0x7fffffff;
      param_1 = 0;
      if (uVar1 != 0) {
        param_1 = uVar1 | 0x40000000;
      }
    }
    else {
      uVar3 = CONCAT22(uVar4,uVar2 + 0x3f80);
    }
    uVar1 = ((uint)CARRY4(uVar3,uVar3) << 0x10 | uVar3 * 2 & 0xffff) >> 1;
    param_1 = param_1 | 0x80000000;
  }
  *param_2 = 0;
  param_2[1] = param_1;
  *(short *)(param_2 + 2) = (short)uVar1;
  return;
}


