// func #111  FUN_0003690b  0003690b  size=2555


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0003690b(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 extraout_EDX;
  undefined4 local_1bc;
  undefined4 local_1b8;
  undefined4 local_1b4;
  undefined4 local_1b0;
  undefined4 local_1ac;
  undefined4 local_1a8;
  undefined4 local_1a4;
  undefined4 local_1a0;
  undefined4 local_19c;
  undefined4 local_198;
  undefined4 local_194;
  undefined4 local_190;
  undefined4 local_18c;
  undefined4 local_188;
  undefined4 local_184;
  undefined4 local_180;
  undefined4 local_17c;
  undefined4 local_178;
  undefined4 local_174;
  undefined4 local_170;
  undefined4 local_16c;
  undefined4 local_168;
  undefined4 local_164;
  undefined4 local_160;
  undefined4 local_15c;
  int local_154;
  int local_14c;
  
  FUN_000418ee();
  DAT_000639b8 = FUN_0004270d("night.res",&DAT_00051481);
  if (DAT_000639b8 == 0) {
    FUN_00036537("NIGHT.RES File Not Found!!!\n");
  }
  FUN_000366a4(DAT_000639b8);
  iVar1 = FUN_00041bdc(0x4b4);
  if (iVar1 == 0) {
    local_190 = 0;
  }
  else {
    local_190 = FUN_00032464(iVar1,"gun.key");
  }
  DAT_00062aa0 = local_190;
  iVar1 = FUN_00041bdc(0x4b4);
  if (iVar1 == 0) {
    local_18c = 0;
  }
  else {
    local_18c = FUN_00032464(iVar1,"gun1.key");
  }
  _DAT_00062aa4 = local_18c;
  iVar1 = FUN_00041bdc(0x4b4);
  if (iVar1 == 0) {
    local_188 = 0;
  }
  else {
    local_188 = FUN_00032464(iVar1,"gun2.key");
  }
  _DAT_00062aa8 = local_188;
  iVar1 = FUN_00041bdc(0x4b4);
  if (iVar1 == 0) {
    local_184 = 0;
  }
  else {
    local_184 = FUN_00032464(iVar1,"gun3.key");
  }
  _DAT_00062aac = local_184;
  iVar1 = FUN_00041bdc(0x4b4);
  if (iVar1 == 0) {
    local_180 = 0;
  }
  else {
    local_180 = FUN_00032464(iVar1,"gun4.key");
  }
  _DAT_00062ab0 = local_180;
  iVar1 = FUN_00041bdc(0x4b4);
  if (iVar1 == 0) {
    local_17c = 0;
  }
  else {
    local_17c = FUN_00032464(iVar1,"catch.key");
  }
  DAT_00062ab4 = local_17c;
  iVar1 = FUN_00041bdc(0x4b4);
  if (iVar1 == 0) {
    local_178 = 0;
  }
  else {
    local_178 = FUN_00032464(iVar1,"catch1.key");
  }
  _DAT_00062ab8 = local_178;
  iVar1 = FUN_00041bdc(0x4b4);
  if (iVar1 == 0) {
    local_174 = 0;
  }
  else {
    local_174 = FUN_00032464(iVar1,"catch2.key");
  }
  _DAT_00062abc = local_174;
  iVar1 = FUN_00041bdc(0x4b4);
  if (iVar1 == 0) {
    local_170 = 0;
  }
  else {
    local_170 = FUN_00032464(iVar1,"catch3.key");
  }
  _DAT_00062ac0 = local_170;
  iVar1 = FUN_00041bdc(0x4b4);
  if (iVar1 == 0) {
    local_16c = 0;
  }
  else {
    local_16c = FUN_00032464(iVar1,"catch4.key");
  }
  _DAT_00062ac4 = local_16c;
  iVar1 = FUN_00041bdc(8);
  if (iVar1 == 0) {
    local_168 = 0;
  }
  else {
    local_168 = FUN_00010010(iVar1,"gun.act");
  }
  DAT_00053c28 = local_168;
  iVar1 = FUN_00041bdc(8);
  if (iVar1 == 0) {
    local_164 = 0;
  }
  else {
    local_164 = FUN_00010010(iVar1,"gun1.act");
  }
  _DAT_00053c2c = local_164;
  iVar1 = FUN_00041bdc(8);
  if (iVar1 == 0) {
    local_160 = 0;
  }
  else {
    local_160 = FUN_00010010(iVar1,"gun2.act");
  }
  _DAT_00053c30 = local_160;
  DAT_00062a98 = DAT_00062aa0;
  DAT_00062a9c = DAT_00062ab4;
  iVar1 = FUN_00041bdc(0x3c);
  if (iVar1 == 0) {
    local_15c = 0;
  }
  else {
    local_15c = FUN_0003d8d4(iVar1,"pause.spr");
  }
  DAT_00063e0c = local_15c;
  for (local_154 = 0; local_154 < 100; local_154 = local_154 + 1) {
    iVar1 = FUN_00036626((&PTR_s_1_voc_000525a8)[local_154]);
    uVar2 = FUN_00041af8(iVar1 * 2);
    *(undefined4 *)(&DAT_00053d0c + local_154 * 4) = uVar2;
    FUN_000417f3((&PTR_s_1_voc_000525a8)[local_154],*(undefined4 *)(&DAT_00053d0c + local_154 * 4));
  }
  iVar1 = FUN_00041bdc(0x3c);
  if (iVar1 == 0) {
    local_1bc = 0;
  }
  else {
    local_1bc = FUN_0003d8d4(iVar1,"system.spr");
  }
  DAT_000639c0 = local_1bc;
  iVar1 = FUN_00041bdc(0x3c);
  if (iVar1 == 0) {
    local_1b8 = 0;
  }
  else {
    local_1b8 = FUN_0003d8d4(iVar1,"system2.spr");
  }
  DAT_000639c4 = local_1b8;
  iVar1 = FUN_00041bdc(0x3c);
  if (iVar1 == 0) {
    local_1b4 = 0;
  }
  else {
    local_1b4 = FUN_0003d8d4(iVar1,"over.spr");
  }
  DAT_00063e08 = local_1b4;
  iVar1 = FUN_00041bdc(0x3c);
  if (iVar1 == 0) {
    local_1b0 = 0;
  }
  else {
    local_1b0 = FUN_0003d8d4(iVar1,"face.spr");
  }
  DAT_00053c8c = local_1b0;
  iVar1 = FUN_00041bdc(0x3c);
  if (iVar1 == 0) {
    local_1ac = 0;
  }
  else {
    local_1ac = FUN_0003d8d4(iVar1,"gage.spr");
  }
  DAT_00063e00 = local_1ac;
  iVar1 = FUN_00041bdc(0x3c);
  if (iVar1 == 0) {
    local_1a8 = 0;
  }
  else {
    local_1a8 = FUN_0003d8d4(iVar1,"exp.spr");
  }
  DAT_00063e04 = local_1a8;
  iVar1 = FUN_00041bdc(0x3c);
  if (iVar1 == 0) {
    local_1a4 = 0;
  }
  else {
    local_1a4 = FUN_0003d8d4(iVar1,PTR_s_gun_spr_00051d88);
  }
  _DAT_000639ec = local_1a4;
  DAT_00053818 = DAT_00053c28;
  iVar1 = FUN_00041bdc(0x3c);
  if (iVar1 == 0) {
    local_1a0 = 0;
  }
  else {
    local_1a0 = FUN_0003d8d4(iVar1,PTR_s_mark_spr_000520ac);
  }
  _DAT_00063d10 = local_1a0;
  iVar1 = FUN_00041bdc(8);
  if (iVar1 == 0) {
    local_19c = 0;
  }
  else {
    local_19c = FUN_00010010(iVar1,PTR_s_mark_act_000524bc);
  }
  _DAT_00053b3c = local_19c;
  iVar1 = FUN_00041bdc(0x3c);
  if (iVar1 == 0) {
    local_198 = 0;
  }
  else {
    local_198 = FUN_0003d8d4(iVar1,PTR_s_dust_spr_00052170);
  }
  _DAT_00063dd4 = local_198;
  iVar1 = FUN_00041bdc(8);
  if (iVar1 == 0) {
    local_194 = 0;
  }
  else {
    local_194 = FUN_00010010(iVar1,PTR_s_dust_act_00052580);
  }
  _DAT_00053c00 = local_194;
  FUN_000367c9("exptab.dat");
  for (local_154 = 0; local_154 < 6; local_154 = local_154 + 1) {
    for (local_14c = 0; local_14c < 5; local_14c = local_14c + 1) {
      FUN_00042023();
    }
    FUN_00042023();
  }
  DAT_00063358 = 0;
  return CONCAT44(extraout_EDX,1);
}


