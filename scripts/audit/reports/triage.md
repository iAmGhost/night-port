# Static audit triage

Port source is deliberately untouched in this pass: the blind gate must run against the current build, so findings are filed with evidence and fixed afterwards.

| verdict | count |
|---|---|
| FILED-SPAN | 141 |
| FILED-RUNTIME | 107 |
| FILED-VIEW | 65 |
| FILED-CALLGRAPH-GAP | 64 |
| EXPLAINED-BY-SPAN | 41 |
| EXPLAINED-UNACCESSED | 37 |
| EXPLAINED-DEAD | 25 |
| FILED-MODEL | 24 |
| EXPLAINED-SCRATCH | 15 |
| FILED-READONLY | 12 |
| FILED-PRODUCER-CHECK | 7 |
| EXPLAINED-SAME-TABLE | 3 |
| FILED-REPR | 3 |
| EXPLAINED-RENAMED | 1 |
| EXPLAINED-NOT-MODELLED | 1 |

## C1: 80 adjudicated, 24 filed
- **FILED-MODEL** 0x639b8 — R76/W1 image=0 sites=["fn_03690b@0x36932 MOV ['[0x000639b8]', 'EAX']"]
- **FILED-MODEL** 0x53670 — R17/W1 image=0 sites=["fn_0461cc@0x46e37 XCHG ['word ptr [EBX + 0x8]', 'AX']"]
- **FILED-MODEL** 0x5185c — R15/W0 image=16843008 sites=[]
- **FILED-MODEL** 0x53668 — R15/W1 image=0 sites=["fn_0461cc@0x46e26 XCHG ['dword ptr [EBX]', 'EAX']"]
- **FILED-MODEL** 0x5366c — R15/W1 image=0 sites=["fn_0461cc@0x46e2d XCHG ['dword ptr [EBX + 0x4]', 'EAX']"]
- **FILED-MODEL** 0x63dfc — R11/W3 image=0 sites=["fn_037347@0x37467 MOV ['dword ptr [0x00063dfc]', '0x0']", "fn_0374ad@0x375d6 MOV ['dword ptr [0x00063dfc]', '0x0']"]
- **FILED-MODEL** 0x5c8c0 — R4/W1 image=0 sites=["fn_03e20f@0x3ebaf MOV ['dword ptr [EDX + 0x5c8c0]', 'EAX']"]
- **FILED-MODEL** 0x5c8c8 — R4/W1 image=0 sites=["fn_03e20f@0x3ebd5 MOV ['dword ptr [EDX + 0x5c8c8]', 'EAX']"]
- **FILED-MODEL** 0x5c8bc — R4/W1 image=0 sites=["fn_03e20f@0x3eb9c MOV ['dword ptr [EDX + 0x5c8bc]', 'EAX']"]
- **FILED-MODEL** 0x5c8c4 — R4/W1 image=0 sites=["fn_03e20f@0x3ebc2 MOV ['dword ptr [EDX + 0x5c8c4]', 'EAX']"]
- **FILED-MODEL** 0x5380c — R4/W1 image=530645248 sites=["dispatch_b@0x446a8 MOV ['byte ptr [EBX]', '0x2']"]
- **FILED-MODEL** 0x53734 — R5/W0 image=0 sites=[]
- **FILED-MODEL** 0x53758 — R5/W0 image=0 sites=[]
- **FILED-MODEL** 0x63300 — R4/W0 image=0 sites=[]
- **FILED-MODEL** 0x53388 — R3/W1 image=0 sites=["fn_0420bc@0x422b7 MOV ['[0x00053388]', 'EAX']"]
- **FILED-MODEL** 0x537dc — R3/W1 image=417734656 sites=["dispatch_a@0x44658 MOV ['byte ptr [EBX]', '0x2']"]
- **FILED-MODEL** 0x63220 — R3/W0 image=0 sites=[]
- **FILED-MODEL** 0x63328 — R2/W2 image=0 sites=["@0x34e20 MOV ['dword ptr [0x00063328]', '0x1']", "fn_034e3c@0x34e80 INC ['dword ptr [0x00063328]']"]

## C2: 149 adjudicated, 146 filed
- **FILED-SPAN** g_tab_51b60 covers g_blk_51c24 — overlap=476B shipped base constants in the inner span: ['0x51c24', '0x51c91', '0x51c90', '0x51c68']
- **FILED-SPAN** g_tab_51b60 covers g_dat_51c98 — overlap=4B shipped base constants in the inner span: ['0x51c91', '0x51c90', '0x51c98', '0x51c9c']
- **FILED-SPAN** g_blk_51c24 covers g_dat_51c98 — overlap=4B shipped base constants in the inner span: ['0x51c91', '0x51c90', '0x51c98', '0x51c9c']
- **FILED-SPAN** g_tab_52738 covers g_tab_5274c — overlap=1004B shipped base constants in the inner span: ['0x52a1c', '0x52c3c', '0x52b84', '0x52be0']
- **FILED-SPAN** g_tab_52738 covers g_tab_528b4 — overlap=644B shipped base constants in the inner span: ['0x52a1c', '0x52b84', '0x528b4']
- **FILED-SPAN** g_tab_52738 covers g_tab_52a1c — overlap=284B shipped base constants in the inner span: ['0x52da4', '0x52a1c', '0x52c3c', '0x52f08']
- **FILED-SPAN** g_tab_5274c covers g_tab_528b4 — overlap=720B shipped base constants in the inner span: ['0x52a1c', '0x52b84', '0x528b4']
- **FILED-SPAN** g_tab_5274c covers g_tab_52a1c — overlap=544B shipped base constants in the inner span: ['0x52da4', '0x52a1c', '0x52c3c', '0x52f08']
- **FILED-SPAN** g_tab_5274c covers g_tab_52b84 — overlap=90B shipped base constants in the inner span: ['0x52b84']
- **FILED-SPAN** g_tab_5274c covers g_tab_52be0 — overlap=92B shipped base constants in the inner span: ['0x52c3c', '0x52be0']
- **FILED-SPAN** g_tab_528b4 covers g_tab_52a1c — overlap=360B shipped base constants in the inner span: ['0x52da4', '0x52a1c', '0x52c3c', '0x52f08']
- **FILED-SPAN** g_tab_52a1c covers g_tab_52b84 — overlap=90B shipped base constants in the inner span: ['0x52b84']
- **FILED-SPAN** g_tab_52a1c covers g_tab_52be0 — overlap=100B shipped base constants in the inner span: ['0x52c3c', '0x52be0']
- **FILED-SPAN** g_tab_52a1c covers g_tab_52c3c — overlap=716B shipped base constants in the inner span: ['0x52ff8', '0x52da4', '0x5302c', '0x52ffc']
- **FILED-SPAN** g_tab_52a1c covers g_tab_52da4 — overlap=356B shipped base constants in the inner span: ['0x52da4', '0x52f08', '0x52f2c']
- **FILED-SPAN** g_tab_52be0 covers g_tab_52c3c — overlap=8B shipped base constants in the inner span: ['0x52ff8', '0x52da4', '0x5302c', '0x52ffc']
- **FILED-SPAN** g_tab_52c3c covers g_tab_52da4 — overlap=400B shipped base constants in the inner span: ['0x52da4', '0x52f08', '0x52f2c']
- **FILED-SPAN** g_tab_52c3c covers g_dat_52f08 — overlap=256B shipped base constants in the inner span: ['0x52ff8', '0x52ffc', '0x52f08', '0x52f2c']

## C3: 27 adjudicated, 27 filed
- **FILED-RUNTIME** g_dat_530bc @ 0x530bc — image=7f0000007f0000007f0000007f000000 port_init='' guest reads=0 writes=1
- **FILED-PRODUCER-CHECK** g_tab_52f2c @ 0x52f2c — image=16000000030000000200000006000000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-READONLY** g_tab_52a1c @ 0x52a1c — image=e8030000e8030000e8030000e8030000 port_init='' guest reads=1 writes=0, and nothing in the port ever writes 'g_tab_52a1c' -> shipped bytes were dropped
- **FILED-PRODUCER-CHECK** g_tab_52b84 @ 0x52b84 — image=00000000000000000000000000000000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-READONLY** kXlat_5302c @ 0x5302c — image=00000000000000000000000000000000 port_init='= { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x' guest reads=1 writes=0, and nothing in the port ever writes 'kXlat_5302c' -> shipped bytes were dropped
- **FILED-PRODUCER-CHECK** g_rng_state @ 0x5335c — image=01000000 port_init='= 0' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-RUNTIME** g_dat_530b4 @ 0x530b4 — image=64000000 port_init='= 0' guest reads=1 writes=2
- **FILED-READONLY** g_dat_530b8 @ 0x530b8 — image=64000000 port_init='= 0' guest reads=1 writes=0, and nothing in the port ever writes 'g_dat_530b8' -> shipped bytes were dropped
- **FILED-RUNTIME** g_dat_531f8 @ 0x531f8 — image=ffff0000 port_init='= 0' guest reads=0 writes=1
- **FILED-RUNTIME** g_dat_63e64 @ 0x63e64 — image=00000000 port_init='= 0x140' guest reads=6 writes=1
- **FILED-PRODUCER-CHECK** g_dat_53619 @ 0x53619 — image=00010000 port_init='= 0' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-RUNTIME** g_dat_530ac @ 0x530ac — image=20030000 port_init='= 0' guest reads=7 writes=1
- **FILED-RUNTIME** g_dat_633b0 @ 0x633b0 — image=00000000 port_init='= SCREEN_BASE' guest reads=42 writes=1
- **FILED-RUNTIME** g_dat_633b4 @ 0x633b4 — image=00000000 port_init='= SCREEN_BASE + SCREEN_SIZE' guest reads=8 writes=1
- **FILED-RUNTIME** g_dat_51c98 @ 0x51c98 — image=01000000 port_init='= 0' guest reads=1 writes=4
- **FILED-PRODUCER-CHECK** g_dat_536dc @ 0x536dc — image=14000000 port_init='' but the port writes this global itself -> compare its producer against the shipped bytes
- **FILED-RUNTIME** g_dat_53780 @ 0x53780 — image=100e0000 port_init='= 0' guest reads=1 writes=1
- **FILED-RUNTIME** g_dat_53784 @ 0x53784 — image=01000000 port_init='= 0' guest reads=0 writes=2
- _15 FILED-RUNTIME (the state oracle decides):_ g_dat_530bc @ 0x530bc; g_dat_530b4 @ 0x530b4; g_dat_531f8 @ 0x531f8; g_dat_63e64 @ 0x63e64; g_dat_530ac @ 0x530ac; g_dat_633b0 @ 0x633b0; g_dat_633b4 @ 0x633b4; g_dat_51c98 @ 0x51c98; g_dat_53780 @ 0x53780; g_dat_53784 @ 0x53784

## C4: 3 adjudicated, 3 filed
- **FILED-REPR** g_dat_639ec @ 0x639ec — port=uint8_t * shipped=undefined4 image=0 R26/W3 ['CMP dword ptr [EAX + 0x639ec] 0x0']
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
- _80 FILED-RUNTIME (the state oracle decides):_ a3a8_dbl; abs32; act_block; act_trace_counts; act_trace_targets; act_trace_targets_all; ai_blk_f32; ai_blk_i32; ai_cfg_i32; ai_climb_xy

## C6: 56 adjudicated, 19 filed
- **FILED-READONLY** 0x52c3c..0x52f3c (192 cells) — image_head=ac000000ac000000ac000000ac000000 bases=['0x524bc', '0x52580', '0x525a8', '0x52738', '0x5274c', '0x528b4'] reads=13 writes=0 port=['g_tab_52c3c', 'g_tab_52c3c', 'g_tab_52f2c', 'g_dat_52f08', 'g_dat_52f08', 'g_tab_52da4', 'g_tab_52da4']
- **FILED-READONLY** 0x528a8..0x52b84 (183 cells) — image_head=0000003f0000003f0000003f09000000 bases=['0x520ac', '0x52170', '0x52198', '0x524bc', '0x52580', '0x525a8'] reads=3 writes=0 port=['g_tab_528b4', 'g_tab_528b4', 'g_tab_52a1c', 'g_tab_52a1c']
- **FILED-RUNTIME** 0x531f8..0x53334 (79 cells) — reads=2 writes=1 bases=['0x52a1c', '0x52b84', '0x52be0', '0x52c3c', '0x52da4', '0x52f08'] port=['g_dat_531f8', 'g_dat_531f8']
- **FILED-READONLY** 0x51b64..0x51c4c (58 cells) — image_head=0b0000000c0000000d0000000e000000 bases=['0x5144c', '0x51450', '0x51454', '0x51458', '0x5145c', '0x5145e'] reads=1 writes=0 port=['g_blk_51c24', 'g_blk_51c24']
- **FILED-RUNTIME** 0x51c9c..0x51d3c (40 cells) — reads=135 writes=0 bases=['0x514ab', '0x514b3', '0x514bc', '0x514c5', '0x514ce', '0x514d7'] port=[]
- **FILED-RUNTIME** 0x51850..0x518e0 (36 cells) — reads=17 writes=0 bases=['0x5144c', '0x51450', '0x51454', '0x51458', '0x5145c', '0x5145e'] port=[]
- **FILED-RUNTIME** 0x530bc..0x53100 (17 cells) — reads=0 writes=1 bases=['0x52a1c', '0x52b84', '0x52be0', '0x52c3c', '0x52da4', '0x52f08'] port=['g_dat_530bc', 'g_dat_530bc']
- **FILED-READONLY** 0x5144c..0x51484 (14 cells) — image_head=25640a00256420002564200025632000 bases=['0x5144c', '0x51450', '0x51454', '0x51458', '0x5145c', '0x5145e'] reads=13 writes=0 port=['g_dat_5147c']
- **FILED-READONLY** 0x51968..0x51994 (11 cells) — image_head=78009700b500d400f300110130014e01 bases=['0x5144c', '0x51450', '0x51454', '0x51458', '0x5145c', '0x5145e'] reads=2 writes=0 port=['g_leap_tbl']
- **FILED-READONLY** 0x52ffc..0x5302c (12 cells) — image_head=01000000020000000200000001000000 bases=['0x528b4', '0x52a1c', '0x52b84', '0x52be0', '0x52c3c', '0x52da4'] reads=1 writes=0 port=['g_tab_52ffc', 'g_tab_52ffc']
- **FILED-RUNTIME** 0x536e0..0x536f4 (5 cells) — reads=3 writes=0 bases=['0x52f08', '0x52f2c', '0x52f90', '0x52ff8', '0x52ffc', '0x5302c'] port=[]
- **FILED-RUNTIME** 0x51c80..0x51c98 (6 cells) — reads=2 writes=0 bases=['0x51481', '0x51484', '0x5148e', '0x514ab', '0x514b3', '0x514bc'] port=[]
- **FILED-RUNTIME** 0x51c68..0x51c7c (5 cells) — reads=1 writes=0 bases=['0x5146a', '0x51470', '0x51475', '0x5147c', '0x51481', '0x51484'] port=[]
- **FILED-RUNTIME** 0x51d58..0x51d70 (6 cells) — reads=85 writes=0 bases=['0x5155a', '0x51563', '0x5156b', '0x51576', '0x5157a', '0x5157c'] port=[]
- **FILED-RUNTIME** 0x53780..0x5378c (3 cells) — reads=2 writes=3 bases=['0x52f90', '0x52ff8', '0x52ffc', '0x5302c', '0x530ac', '0x530b0'] port=['g_dat_53780', 'g_dat_53780', 'g_dat_53784', 'g_dat_53784', 'g_dat_53788', 'g_dat_53788']
- **FILED-RUNTIME** 0x50014..0x50044 (12 cells) — reads=12 writes=0 bases=['0x50004', '0x50014', '0x50018', '0x5001c', '0x50020', '0x50024'] port=[]
- **FILED-RUNTIME** 0x51728..0x51734 (3 cells) — reads=1 writes=0 bases=['0x5144c', '0x51450', '0x51454', '0x51458', '0x5145c', '0x5145e'] port=[]
- **FILED-READONLY** 0x52738..0x52778 (16 cells) — image_head=64000000c80000002c01000090010000 bases=['0x520ac', '0x52170', '0x52198', '0x524bc', '0x52580', '0x525a8'] reads=4 writes=0 port=['g_tab_52738', 'g_tab_52738', 'g_tab_5274c', 'g_tab_5274c']
- _12 FILED-RUNTIME (the state oracle decides):_ 0x531f8..0x53334 (79 cells); 0x51c9c..0x51d3c (40 cells); 0x51850..0x518e0 (36 cells); 0x530bc..0x53100 (17 cells); 0x536e0..0x536f4 (5 cells); 0x51c80..0x51c98 (6 cells); 0x51c68..0x51c7c (5 cells); 0x51d58..0x51d70 (6 cells); 0x53780..0x5378c (3 cells); 0x50014..0x50044 (12 cells)

## C8: 61 adjudicated, 60 filed
- **FILED-VIEW** 0x639dc (g_dat_639dc) vs 0x639e0 (g_dat_639e0) — distance=0x4 n=90/77 shared_fns=['fn_012efa', 'fn_013908', 'fn_0374ad']
- **FILED-VIEW** 0x63354 (g_dat_63354) vs 0x63370 (g_dat_63370) — distance=0x1c n=244/42 shared_fns=['fn_010d47', 'fn_012616', 'fn_0374ad']
- **FILED-VIEW** 0x63370 (g_dat_63370) vs 0x633b0 (g_dat_633b0) — distance=0x40 n=42/43 shared_fns=['fn_03658d', 'fn_039c03', 'fn_03a70e']
- **FILED-VIEW** 0x63354 (g_dat_63354) vs 0x633b0 (g_dat_633b0) — distance=0x5c n=244/43 shared_fns=['fn_038445', 'fn_039c03', 'fn_03a829']
- **FILED-VIEW** 0x63354 (g_dat_63354) vs 0x636b8 (g_dat_636b8) — distance=0x364 n=244/49 shared_fns=['fn_03af18']
- **FILED-VIEW** 0x636b8 (g_dat_636b8) vs 0x639dc (g_dat_639dc) — distance=0x324 n=49/90 shared_fns=['fn_03af18']
- **FILED-VIEW** 0x53c6c (g_dat_53c6c) vs 0x53c7c (g_dat_53c7c_null_warned,g_player_ent) — distance=0x10 n=20/869 shared_fns=['fn_015a1f', 'fn_015b5b', 'fn_015d16']
- **FILED-VIEW** 0x53c68 (g_dat_53c68) vs 0x53c70 (g_dat_53c70) — distance=0x8 n=19/20 shared_fns=['fn_010efc', 'fn_0112fd', 'fn_013aad']
- **FILED-VIEW** 0x636b8 (g_dat_636b8) vs 0x639e0 (g_dat_639e0) — distance=0x328 n=49/77 shared_fns=[]
- **FILED-VIEW** 0x63ea4 (g_dat_63ea4) vs 0x63ec1 (g_dat_63ec1) — distance=0x1d n=21/24 shared_fns=['fn_035d5d', 'fn_035dfc', 'fn_038445']
- **FILED-VIEW** 0x633b0 (g_dat_633b0) vs 0x636b8 (g_dat_636b8) — distance=0x308 n=43/49 shared_fns=['fn_038f3d']
- **FILED-VIEW** 0x63370 (g_dat_63370) vs 0x636b8 (g_dat_636b8) — distance=0x348 n=42/49 shared_fns=['fn_03af18']
- **FILED-VIEW** 0x63ea5 (g_dat_63ea5) vs 0x63ec1 (g_dat_63ec1) — distance=0x1c n=20/24 shared_fns=['fn_03272f', 'fn_035d5d', 'fn_035dfc']
- **FILED-VIEW** 0x63ea5 (g_dat_63ea5) vs 0x63ec0 (g_dat_63ec0) — distance=0x1b n=20/19 shared_fns=['fn_03272f', 'fn_035d5d', 'fn_035dfc']
- **FILED-VIEW** 0x63ec0 (g_dat_63ec0) vs 0x63ec1 (g_dat_63ec1) — distance=0x1 n=19/24 shared_fns=['fn_03272f', 'fn_035d5d', 'fn_035dfc']
- **FILED-VIEW** 0x63ea4 (g_dat_63ea4) vs 0x63ea5 (g_dat_63ea5) — distance=0x1 n=21/20 shared_fns=['fn_035d5d', 'fn_035dfc', 'fn_038445']
- **FILED-VIEW** 0x53c68 (g_dat_53c68) vs 0x53c7c (g_dat_53c7c_null_warned,g_player_ent) — distance=0x14 n=19/869 shared_fns=['fn_013aad', 'fn_01441e', 'fn_016fec']
- **FILED-VIEW** 0x63e89 (g_dat_63e89) vs 0x63ea4 (g_dat_63ea4) — distance=0x1b n=19/21 shared_fns=['fn_035dfc', 'fn_038cde', 'fn_038f3d']
