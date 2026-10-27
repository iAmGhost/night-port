# Static audit triage

Port source is deliberately untouched in this pass: the blind gate must run against the current build, so findings are filed with evidence and fixed afterwards.

| verdict | count |
|---|---|
| FILED-SPAN | 120 |
| FILED-RUNTIME | 107 |
| FILED-CALLGRAPH-GAP | 64 |
| FILED-VIEW | 63 |
| FILED-MODEL | 61 |
| EXPLAINED-UNACCESSED | 37 |
| EXPLAINED-DEAD | 25 |
| EXPLAINED-BY-SPAN | 19 |
| FILED-PRODUCER-CHECK | 16 |
| FILED-READONLY | 7 |
| FILED-REPR | 4 |
| EXPLAINED-SAME-TABLE | 3 |
| EXPLAINED-RENAMED | 1 |
| EXPLAINED-NOT-MODELLED | 1 |

## C1: 80 adjudicated, 61 filed
- **FILED-MODEL** 0x63354 — R236/W8 image=None sites=["fn_0374ad@0x3761f MOV ['[0x00063354]', 'EAX']", "fn_0376c8@0x377f1 MOV ['[0x00063354]', 'EAX']"]
- **FILED-MODEL** 0x53eb8 — R148/W0 image=None sites=[]
- **FILED-MODEL** 0x53eb0 — R80/W0 image=None sites=[]
- **FILED-MODEL** 0x639b8 — R76/W1 image=None sites=["fn_03690b@0x36932 MOV ['[0x000639b8]', 'EAX']"]
- **FILED-MODEL** 0x51d5c — R58/W0 image=1056964608 sites=[]
- **FILED-MODEL** 0x51c9c — R48/W0 image=4294967241 sites=[]
- **FILED-MODEL** 0x51ca0 — R42/W0 image=4294967291 sites=[]
- **FILED-MODEL** 0x53eb4 — R40/W0 image=None sites=[]
- **FILED-MODEL** 0x53f20 — R40/W0 image=None sites=[]
- **FILED-MODEL** 0x53f1c — R39/W0 image=None sites=[]
- **FILED-MODEL** 0x53c5c — R29/W1 image=None sites=["fn_0104c7@0x10530 MOV ['dword ptr [0x00053c5c]', 'EBX']"]
- **FILED-MODEL** 0x51d68 — R27/W0 image=1065353216 sites=[]
- **FILED-MODEL** 0x51ca4 — R24/W0 image=4294967256 sites=[]
- **FILED-MODEL** 0x639c4 — R22/W1 image=None sites=["fn_03690b@0x36fb5 MOV ['[0x000639c4]', 'EAX']"]
- **FILED-MODEL** 0x53ea0 — R20/W1 image=None sites=["fn_037b03@0x37ed8 MOV ['dword ptr [EDX + 0x53ea0]', 'EAX']"]
- **FILED-MODEL** 0x51ca8 — R21/W0 image=5 sites=[]
- **FILED-MODEL** 0x63e08 — R18/W1 image=None sites=["fn_03690b@0x36ffd MOV ['[0x00063e08]', 'EAX']"]
- **FILED-MODEL** 0x53eb9 — R18/W0 image=None sites=[]

## C2: 126 adjudicated, 123 filed
- **FILED-SPAN** g_tab_528b4 covers g_tab_52a1c — overlap=4B shipped base constants in the inner span: ['0x52a1c']
- **FILED-SPAN** g_tab_52be0 covers g_tab_52c3c — overlap=8B shipped base constants in the inner span: ['0x52ff8', '0x52da4', '0x5302c', '0x52ffc']
- **FILED-SPAN** g_tab_52c3c covers g_tab_52da4 — overlap=400B shipped base constants in the inner span: ['0x52da4', '0x52f08', '0x52f2c']
- **FILED-SPAN** g_tab_52c3c covers g_dat_52f08 — overlap=256B shipped base constants in the inner span: ['0x52ff8', '0x52ffc', '0x52f08', '0x52f2c']
- **FILED-SPAN** g_tab_52c3c covers g_tab_52f2c — overlap=40B shipped base constants in the inner span: ['0x52f2c']
- **FILED-SPAN** g_tab_52c3c covers g_tab_52f90 — overlap=40B shipped base constants in the inner span: ['0x52f90']
- **FILED-SPAN** g_tab_52c3c covers g_dat_52ff8 — overlap=1B shipped base constants in the inner span: ['0x52ff8']
- **FILED-SPAN** g_tab_52c3c covers g_tab_52ffc — overlap=80B shipped base constants in the inner span: ['0x52ff8', '0x5302c', '0x52ffc', '0x530b4']
- **FILED-SPAN** g_tab_52da4 covers g_dat_52f08 — overlap=44B shipped base constants in the inner span: ['0x52ff8', '0x52ffc', '0x52f08', '0x52f2c']
- **FILED-SPAN** g_tab_52da4 covers g_tab_52f2c — overlap=8B shipped base constants in the inner span: ['0x52f2c']
- **FILED-SPAN** g_dat_52f08 covers g_tab_52f2c — overlap=40B shipped base constants in the inner span: ['0x52f2c']
- **FILED-SPAN** g_dat_52f08 covers g_tab_52f90 — overlap=40B shipped base constants in the inner span: ['0x52f90']
- **FILED-SPAN** g_dat_52f08 covers g_dat_52ff8 — overlap=1B shipped base constants in the inner span: ['0x52ff8']
- **FILED-SPAN** g_dat_52f08 covers g_tab_52ffc — overlap=12B shipped base constants in the inner span: ['0x52ff8', '0x5302c', '0x52ffc', '0x530b4']
- **FILED-SPAN** g_tab_52ffc covers g_dat_530b4 — overlap=4B shipped base constants in the inner span: ['0x530b4', '0x530b0', '0x530ac', '0x530b8']
- **FILED-SPAN** g_tab_52ffc covers g_dat_530b8 — overlap=4B shipped base constants in the inner span: ['0x530b4', '0x530b0', '0x530bc', '0x530b8']
- **FILED-SPAN** g_tab_52ffc covers g_dat_530bc — overlap=64B shipped base constants in the inner span: ['0x530b4', '0x530bc', '0x530b8']
- **FILED-SPAN** g_dat_537ca covers g_dat_537ca_target — overlap=1B shipped base constants in the inner span: ['0x537ca']

## C3: 31 adjudicated, 31 filed
- **FILED-RUNTIME** g_dat_530bc @ 0x530bc — image=7f0000007f0000007f0000007f000000 port_init='' guest reads=0 writes=1
- **FILED-PRODUCER-CHECK** g_tab_52f2c @ 0x52f2c — image=16000000030000000200000006000000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_dat_52f08 @ 0x52f08 — image=02000000050000000600000006000000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_tab_528b4 @ 0x528b4 — image=09000000090000000900000005000000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_tab_52be0 @ 0x52be0 — image=00000000000000000000000000000000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_tab_52ffc @ 0x52ffc — image=01000000020000000200000001000000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_tab_52b84 @ 0x52b84 — image=00000000000000000000000000000000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_rng_state @ 0x5335c — image=01000000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-RUNTIME** g_dat_530b4 @ 0x530b4 — image=64000000 port_init='' guest reads=1 writes=2
- **FILED-READONLY** g_dat_530b8 @ 0x530b8 — image=64000000 port_init='= 0' guest reads=1 writes=0, and nothing in the port ever writes 'g_dat_530b8' -> shipped bytes were dropped
- **FILED-RUNTIME** g_dat_531f8 @ 0x531f8 — image=ffff0000 port_init='= 0' guest reads=0 writes=1
- **FILED-RUNTIME** g_dat_53100 @ 0x53100 — image=03000000 port_init='' guest reads=6 writes=4
- **FILED-PRODUCER-CHECK** g_dat_53619 @ 0x53619 — image=00010000 port_init='= 0' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_dat_40010 @ 0x40010 — image=eb068b45 port_init='= 0' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_dat_40018 @ 0x40018 — image=8b45d4a3 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_dat_4001c @ 0x4001c — image=7c3c0000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_dat_40020 @ 0x40020 — image=a17c3c00 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-PRODUCER-CHECK** g_tab_5274c @ 0x5274c — image=0000003f port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- _12 FILED-RUNTIME (the state oracle decides):_ g_dat_530bc @ 0x530bc; g_dat_530b4 @ 0x530b4; g_dat_531f8 @ 0x531f8; g_dat_53100 @ 0x53100; g_dat_53784 @ 0x53784; g_dat_537a7 @ 0x537a7; g_dat_5363c @ 0x5363c; g_dat_53640 @ 0x53640; g_dat_51a10 @ 0x51a10; g_dat_53368 @ 0x53368

## C4: 4 adjudicated, 4 filed
- **FILED-REPR** g_dat_53c7c_null_warned @ 0x53c7c — port=int shipped=pointer image=None R867/W2 
- **FILED-REPR** g_dat_639ec @ 0x639ec — port=uint8_t * shipped=undefined4 image=None R26/W3 ['CMP dword ptr [EAX + 0x639ec] 0x0']
- **FILED-REPR** g_dat_53730 @ 0x53730 — port=uint32_t * shipped=pointer image=14048 R4/W0 
- **FILED-REPR** g_dat_537ca @ 0x537ca — port=uint8_t * shipped=pointer image=14247 R1/W0 

## C5: 170 adjudicated, 144 filed
- **FILED-CALLGRAPH-GAP** FUN_00041fa1 — called by 1 shipped fns (['0x44663'])
- **FILED-CALLGRAPH-GAP** date_cmp — called by 1 shipped fns (['0x49613'])
- **FILED-CALLGRAPH-GAP** date_match_recur — called by 1 shipped fns (['0x4574a'])
- **FILED-CALLGRAPH-GAP** date_recur — called by 1 shipped fns (['0x495eb'])
- **FILED-CALLGRAPH-GAP** datetime_clock_offset — called by 1 shipped fns (['0x3af18'])
- **FILED-CALLGRAPH-GAP** datetime_pack — called by 1 shipped fns (['0x4297a'])
- **FILED-CALLGRAPH-GAP** debug_break_hook — called by 2 shipped fns (['0x429c8', '0x429ea'])
- **FILED-CALLGRAPH-GAP** float_str_sci — called by 1 shipped fns (['0x45c79'])
- **FILED-CALLGRAPH-GAP** fmt_decimal — called by 1 shipped fns (['0x49e80'])
- **FILED-CALLGRAPH-GAP** fmt_int_signed — called by 1 shipped fns (['0x49e80'])
- **FILED-CALLGRAPH-GAP** fmt_u64_fixed — called by 1 shipped fns (['0x4a48f'])
- **FILED-CALLGRAPH-GAP** fn_0104c7 — called by 1 shipped fns (['0x3658d'])
- **FILED-CALLGRAPH-GAP** fn_010570 — called by 10 shipped fns (['0x105f7', '0x10664', '0x1072f'])
- **FILED-CALLGRAPH-GAP** fn_0107b9 — called by 1 shipped fns (['0x10927'])
- **FILED-CALLGRAPH-GAP** fn_010b1d — called by 9 shipped fns (['0x10d47', '0x38445', '0x38ae0'])
- **FILED-CALLGRAPH-GAP** fn_010dde — called by 6 shipped fns (['0x38445', '0x38ae0', '0x38f3d'])
- **FILED-CALLGRAPH-GAP** fn_0356ba — called by 1 shipped fns (['0x3565a'])
- **FILED-CALLGRAPH-GAP** fn_035c5c — called by 1 shipped fns (['0x1441e'])
- _80 FILED-RUNTIME (the state oracle decides):_ a3a8_dbl; abs32; act_block; ai_blk_f32; ai_blk_i32; ai_cfg_i32; ai_climb_xy; ai_fget; ai_fput; ai_rec_i32

## C6: 56 adjudicated, 19 filed
- **FILED-READONLY** 0x52c3c..0x52f3c (192 cells) — image_head=ac000000ac000000ac000000ac000000 bases=['0x524bc', '0x52580', '0x525a8', '0x52738', '0x5274c', '0x528b4'] reads=13 writes=0 port=['g_tab_52c3c', 'g_tab_52c3c', 'g_tab_52f2c', 'g_dat_52f08', 'g_tab_52da4', 'g_tab_52da4']
- **FILED-READONLY** 0x528a8..0x52b84 (183 cells) — image_head=0000003f0000003f0000003f09000000 bases=['0x520ac', '0x52170', '0x52198', '0x524bc', '0x52580', '0x525a8'] reads=3 writes=0 port=['g_tab_528b4', 'g_tab_52a1c']
- **FILED-RUNTIME** 0x531f8..0x53334 (79 cells) — reads=2 writes=1 bases=['0x52a1c', '0x52b84', '0x52be0', '0x52c3c', '0x52da4', '0x52f08'] port=['g_dat_531f8', 'g_dat_531f8']
- **FILED-RUNTIME** 0x51b64..0x51c4c (58 cells) — reads=1 writes=0 bases=['0x5144c', '0x51450', '0x51454', '0x51458', '0x5145c', '0x5145e'] port=[]
- **FILED-RUNTIME** 0x51c9c..0x51d3c (40 cells) — reads=135 writes=0 bases=['0x514ab', '0x514b3', '0x514bc', '0x514c5', '0x514ce', '0x514d7'] port=[]
- **FILED-RUNTIME** 0x51850..0x518e0 (36 cells) — reads=17 writes=0 bases=['0x5144c', '0x51450', '0x51454', '0x51458', '0x5145c', '0x5145e'] port=[]
- **FILED-RUNTIME** 0x530bc..0x53100 (17 cells) — reads=0 writes=1 bases=['0x52a1c', '0x52b84', '0x52be0', '0x52c3c', '0x52da4', '0x52f08'] port=['g_dat_530bc', 'g_dat_530bc']
- **FILED-RUNTIME** 0x5144c..0x51484 (14 cells) — reads=13 writes=0 bases=['0x5144c', '0x51450', '0x51454', '0x51458', '0x5145c', '0x5145e'] port=[]
- **FILED-RUNTIME** 0x51968..0x51994 (11 cells) — reads=2 writes=0 bases=['0x5144c', '0x51450', '0x51454', '0x51458', '0x5145c', '0x5145e'] port=[]
- **FILED-READONLY** 0x52ffc..0x5302c (12 cells) — image_head=01000000020000000200000001000000 bases=['0x528b4', '0x52a1c', '0x52b84', '0x52be0', '0x52c3c', '0x52da4'] reads=1 writes=0 port=['g_tab_52ffc']
- **FILED-RUNTIME** 0x536e0..0x536f4 (5 cells) — reads=3 writes=0 bases=['0x52f08', '0x52f2c', '0x52f90', '0x52ff8', '0x52ffc', '0x5302c'] port=[]
- **FILED-RUNTIME** 0x51c80..0x51c98 (6 cells) — reads=2 writes=0 bases=['0x51481', '0x51484', '0x5148e', '0x514ab', '0x514b3', '0x514bc'] port=[]
- **FILED-RUNTIME** 0x51c68..0x51c7c (5 cells) — reads=1 writes=0 bases=['0x5146a', '0x51470', '0x51475', '0x5147c', '0x51481', '0x51484'] port=[]
- **FILED-RUNTIME** 0x51d58..0x51d70 (6 cells) — reads=85 writes=0 bases=['0x5155a', '0x51563', '0x5156b', '0x51576', '0x5157a', '0x5157c'] port=[]
- **FILED-RUNTIME** 0x53780..0x5378c (3 cells) — reads=2 writes=3 bases=['0x52f90', '0x52ff8', '0x52ffc', '0x5302c', '0x530ac', '0x530b0'] port=['g_dat_53784', 'g_dat_53784', 'g_dat_53788', 'g_dat_53788']
- **FILED-RUNTIME** 0x50014..0x50044 (12 cells) — reads=12 writes=0 bases=['0x50004', '0x50014', '0x50018', '0x5001c', '0x50020', '0x50024'] port=[]
- **FILED-RUNTIME** 0x51728..0x51734 (3 cells) — reads=1 writes=0 bases=['0x5144c', '0x51450', '0x51454', '0x51458', '0x5145c', '0x5145e'] port=[]
- **FILED-READONLY** 0x52738..0x52778 (16 cells) — image_head=64000000c80000002c01000090010000 bases=['0x520ac', '0x52170', '0x52198', '0x524bc', '0x52580', '0x525a8'] reads=4 writes=0 port=['g_tab_5274c']
- _15 FILED-RUNTIME (the state oracle decides):_ 0x531f8..0x53334 (79 cells); 0x51b64..0x51c4c (58 cells); 0x51c9c..0x51d3c (40 cells); 0x51850..0x518e0 (36 cells); 0x530bc..0x53100 (17 cells); 0x5144c..0x51484 (14 cells); 0x51968..0x51994 (11 cells); 0x536e0..0x536f4 (5 cells); 0x51c80..0x51c98 (6 cells); 0x51c68..0x51c7c (5 cells)

## C8: 61 adjudicated, 60 filed
- **FILED-VIEW** 0x639dc (g_dat_639dc) vs 0x639e0 (g_dat_639e0) — distance=0x4 n=90/77 shared_fns=['fn_012efa', 'fn_013908', 'fn_0374ad']
- **FILED-VIEW** 0x63370 (g_dat_63370) vs 0x633b0 (g_dat_633b0) — distance=0x40 n=42/43 shared_fns=['fn_03658d', 'fn_039c03', 'fn_03a70e']
- **FILED-VIEW** 0x636b8 (g_dat_636b8) vs 0x639dc (g_dat_639dc) — distance=0x324 n=49/90 shared_fns=['fn_03af18']
- **FILED-VIEW** 0x53c6c (g_dat_53c6c) vs 0x53c7c (g_dat_53c7c_null_warned) — distance=0x10 n=20/869 shared_fns=['fn_015a1f', 'fn_015b5b', 'fn_015d16']
- **FILED-VIEW** 0x53c68 (g_dat_53c68) vs 0x53c70 (g_dat_53c70) — distance=0x8 n=19/20 shared_fns=['fn_010efc', 'fn_0112fd', 'fn_013aad']
- **FILED-VIEW** 0x636b8 (g_dat_636b8) vs 0x639e0 (g_dat_639e0) — distance=0x328 n=49/77 shared_fns=[]
- **FILED-VIEW** 0x63ea4 (g_dat_63ea4) vs 0x63ec1 (g_dat_63ec1) — distance=0x1d n=21/24 shared_fns=['fn_035d5d', 'fn_035dfc', 'fn_038445']
- **FILED-VIEW** 0x633b0 (g_dat_633b0) vs 0x636b8 (g_dat_636b8) — distance=0x308 n=43/49 shared_fns=['fn_038f3d']
- **FILED-VIEW** 0x63370 (g_dat_63370) vs 0x636b8 (g_dat_636b8) — distance=0x348 n=42/49 shared_fns=['fn_03af18']
- **FILED-VIEW** 0x63ea5 (g_dat_63ea5) vs 0x63ec1 (g_dat_63ec1) — distance=0x1c n=20/24 shared_fns=['fn_03272f', 'fn_035d5d', 'fn_035dfc']
- **FILED-VIEW** 0x63ea5 (g_dat_63ea5) vs 0x63ec0 (g_dat_63ec0) — distance=0x1b n=20/19 shared_fns=['fn_03272f', 'fn_035d5d', 'fn_035dfc']
- **FILED-VIEW** 0x63ec0 (g_dat_63ec0) vs 0x63ec1 (g_dat_63ec1) — distance=0x1 n=19/24 shared_fns=['fn_03272f', 'fn_035d5d', 'fn_035dfc']
- **FILED-VIEW** 0x63ea4 (g_dat_63ea4) vs 0x63ea5 (g_dat_63ea5) — distance=0x1 n=21/20 shared_fns=['fn_035d5d', 'fn_035dfc', 'fn_038445']
- **FILED-VIEW** 0x53c68 (g_dat_53c68) vs 0x53c7c (g_dat_53c7c_null_warned) — distance=0x14 n=19/869 shared_fns=['fn_013aad', 'fn_01441e', 'fn_016fec']
- **FILED-VIEW** 0x63e89 (g_dat_63e89) vs 0x63ea4 (g_dat_63ea4) — distance=0x1b n=19/21 shared_fns=['fn_035dfc', 'fn_038cde', 'fn_038f3d']
- **FILED-VIEW** 0x63e89 (g_dat_63e89) vs 0x63ec1 (g_dat_63ec1) — distance=0x38 n=19/24 shared_fns=['fn_035dfc', 'fn_038cde', 'fn_038f3d']
- **FILED-VIEW** 0x63ea4 (g_dat_63ea4) vs 0x63ec0 (g_dat_63ec0) — distance=0x1c n=21/19 shared_fns=['fn_035d5d', 'fn_035dfc', 'fn_038445']
- **FILED-VIEW** 0x63ed3 (g_dat_63ed3) vs 0x63ed5 (g_dat_63ed5) — distance=0x2 n=16/16 shared_fns=['fn_012616', 'fn_01441e', 'fn_03272f']
