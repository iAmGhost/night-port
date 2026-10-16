// func #318  FUN_00044618  00044618  size=75


void FUN_00044618(byte param_1)

{
  byte bVar1;
  char *pcVar2;
  char *pcVar3;
  
  while( true ) {
    pcVar2 = &DAT_0005380c;
    bVar1 = param_1;
    for (pcVar3 = &DAT_000537dc; pcVar3 < &DAT_0005380c; pcVar3 = pcVar3 + 6) {
      if ((*pcVar3 != '\x02') && ((byte)pcVar3[1] <= bVar1)) {
        bVar1 = pcVar3[1];
        pcVar2 = pcVar3;
      }
    }
    if (pcVar2 == &DAT_0005380c) break;
    if (*(code **)(pcVar2 + 2) != (code *)0x0) {
      (**(code **)(pcVar2 + 2))();
    }
    *pcVar2 = '\x02';
  }
  return;
}


