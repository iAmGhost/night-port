# Static equivalence audit (Ghidra DB vs port source)

port globals with a DS address: 538 across 292 distinct DS cells; guest DS defined-data items: 1638; guest DS cells with accesses: 624

## c1_unmodelled: 343 findings
  {"ds": 343736, "reads": 148, "writes": 0, "total": 148}
  {"ds": 343728, "reads": 80, "writes": 0, "total": 80}
  {"ds": 407992, "reads": 76, "writes": 1, "total": 77}
  {"ds": 335196, "reads": 58, "writes": 0, "total": 58}
  {"ds": 335004, "reads": 48, "writes": 0, "total": 48}
  {"ds": 335008, "reads": 42, "writes": 0, "total": 42}
  {"ds": 343732, "reads": 40, "writes": 0, "total": 40}
  {"ds": 343840, "reads": 40, "writes": 0, "total": 40}
  {"ds": 343836, "reads": 39, "writes": 0, "total": 39}
  {"ds": 335208, "reads": 27, "writes": 0, "total": 27}
  {"ds": 335012, "reads": 24, "writes": 0, "total": 24}
  {"ds": 343712, "reads": 20, "writes": 1, "total": 21}
  {"ds": 335016, "reads": 21, "writes": 0, "total": 21}
  {"ds": 343737, "reads": 18, "writes": 0, "total": 18}
  {"ds": 341616, "reads": 17, "writes": 1, "total": 17}
  {"ds": 333916, "reads": 15, "writes": 0, "total": 15}
  {"ds": 341608, "reads": 15, "writes": 1, "total": 15}
  {"ds": 341612, "reads": 15, "writes": 1, "total": 15}
  {"ds": 409084, "reads": 11, "writes": 3, "total": 14}
  {"ds": 409383, "reads": 9, "writes": 0, "total": 9}
  {"ds": 408048, "reads": 4, "writes": 2, "total": 6}
  {"ds": 342044, "reads": 2, "writes": 4, "total": 6}
  {"ds": 381124, "reads": 5, "writes": 1, "total": 6}
  {"ds": 379072, "reads": 4, "writes": 1, "total": 5}
  {"ds": 379080, "reads": 4, "writes": 1, "total": 5}

## c2_aliasing: 149 findings
  {"kind": "port-span-overlap", "a": "g_tab_51b60", "a_span": ["0x51b60", "0x51f60"], "b": "g_blk_51c24", "b_span": ["0x51c24", "0x51e00"], "overlap": 476}
  {"kind": "port-span-overlap", "a": "g_tab_51b60", "a_span": ["0x51b60", "0x51f60"], "b": "g_dat_51c98", "b_span": ["0x51c98", "0x51c9c"], "overlap": 4}
  {"kind": "port-span-overlap", "a": "g_blk_51c24", "a_span": ["0x51c24", "0x51e00"], "b": "g_dat_51c98", "b_span": ["0x51c98", "0x51c9c"], "overlap": 4}
  {"kind": "port-span-overlap", "a": "g_tab_52738", "a_span": ["0x52738", "0x52b38"], "b": "g_tab_5274c", "b_span": ["0x5274c", "0x52c3c"], "overlap": 1004}
  {"kind": "port-span-overlap", "a": "g_tab_52738", "a_span": ["0x52738", "0x52b38"], "b": "g_tab_528b4", "b_span": ["0x528b4", "0x52b84"], "overlap": 644}
  {"kind": "port-span-overlap", "a": "g_tab_52738", "a_span": ["0x52738", "0x52b38"], "b": "g_tab_52a1c", "b_span": ["0x52a1c", "0x52f08"], "overlap": 284}
  {"kind": "port-span-overlap", "a": "g_tab_5274c", "a_span": ["0x5274c", "0x52c3c"], "b": "g_tab_528b4", "b_span": ["0x528b4", "0x52b84"], "overlap": 720}
  {"kind": "port-span-overlap", "a": "g_tab_5274c", "a_span": ["0x5274c", "0x52c3c"], "b": "g_tab_52a1c", "b_span": ["0x52a1c", "0x52f08"], "overlap": 544}
  {"kind": "port-span-overlap", "a": "g_tab_5274c", "a_span": ["0x5274c", "0x52c3c"], "b": "g_tab_52b84", "b_span": ["0x52b84", "0x52bde"], "overlap": 90}
  {"kind": "port-span-overlap", "a": "g_tab_5274c", "a_span": ["0x5274c", "0x52c3c"], "b": "g_tab_52be0", "b_span": ["0x52be0", "0x52c44"], "overlap": 92}
  {"kind": "port-span-overlap", "a": "g_tab_528b4", "a_span": ["0x528b4", "0x52b84"], "b": "g_tab_52a1c", "b_span": ["0x52a1c", "0x52f08"], "overlap": 360}
  {"kind": "port-span-overlap", "a": "g_tab_52a1c", "a_span": ["0x52a1c", "0x52f08"], "b": "g_tab_52b84", "b_span": ["0x52b84", "0x52bde"], "overlap": 90}
  {"kind": "port-span-overlap", "a": "g_tab_52a1c", "a_span": ["0x52a1c", "0x52f08"], "b": "g_tab_52be0", "b_span": ["0x52be0", "0x52c44"], "overlap": 100}
  {"kind": "port-span-overlap", "a": "g_tab_52a1c", "a_span": ["0x52a1c", "0x52f08"], "b": "g_tab_52c3c", "b_span": ["0x52c3c", "0x5304c"], "overlap": 716}
  {"kind": "port-span-overlap", "a": "g_tab_52a1c", "a_span": ["0x52a1c", "0x52f08"], "b": "g_tab_52da4", "b_span": ["0x52da4", "0x52f34"], "overlap": 356}
  {"kind": "port-span-overlap", "a": "g_tab_52be0", "a_span": ["0x52be0", "0x52c44"], "b": "g_tab_52c3c", "b_span": ["0x52c3c", "0x5304c"], "overlap": 8}
  {"kind": "port-span-overlap", "a": "g_tab_52c3c", "a_span": ["0x52c3c", "0x5304c"], "b": "g_tab_52da4", "b_span": ["0x52da4", "0x52f34"], "overlap": 400}
  {"kind": "port-span-overlap", "a": "g_tab_52c3c", "a_span": ["0x52c3c", "0x5304c"], "b": "g_dat_52f08", "b_span": ["0x52f08", "0x53008"], "overlap": 256}
  {"kind": "port-span-overlap", "a": "g_tab_52c3c", "a_span": ["0x52c3c", "0x5304c"], "b": "g_tab_52f2c", "b_span": ["0x52f2c", "0x52f54"], "overlap": 40}
  {"kind": "port-span-overlap", "a": "g_tab_52c3c", "a_span": ["0x52c3c", "0x5304c"], "b": "g_tab_52f90", "b_span": ["0x52f90", "0x52fb8"], "overlap": 40}
  {"kind": "port-span-overlap", "a": "g_tab_52c3c", "a_span": ["0x52c3c", "0x5304c"], "b": "g_dat_52fb4", "b_span": ["0x52fb4", "0x52fb8"], "overlap": 4}
  {"kind": "port-span-overlap", "a": "g_tab_52c3c", "a_span": ["0x52c3c", "0x5304c"], "b": "g_dat_52ff8", "b_span": ["0x52ff8", "0x52ff9"], "overlap": 1}
  {"kind": "port-span-overlap", "a": "g_tab_52c3c", "a_span": ["0x52c3c", "0x5304c"], "b": "g_tab_52ffc", "b_span": ["0x52ffc", "0x53100"], "overlap": 80}
  {"kind": "port-span-overlap", "a": "g_tab_52c3c", "a_span": ["0x52c3c", "0x5304c"], "b": "kXlat_5302c", "b_span": ["0x5302c", "0x5312c"], "overlap": 32}
  {"kind": "port-span-overlap", "a": "g_tab_52da4", "a_span": ["0x52da4", "0x52f34"], "b": "g_dat_52f08", "b_span": ["0x52f08", "0x53008"], "overlap": 44}

## c3_image_init: 27 findings
  {"ds": 340156, "kind": "image-nonzero-port-zero", "name": "g_dat_530bc", "image_head": "7f0000007f0000007f0000007f000000", "init": ""}
  {"ds": 339756, "kind": "image-nonzero-port-zero", "name": "g_tab_52f2c", "image_head": "16000000030000000200000006000000", "init": ""}
  {"ds": 338460, "kind": "image-nonzero-port-zero", "name": "g_tab_52a1c", "image_head": "e8030000e8030000e8030000e8030000", "init": ""}
  {"ds": 338820, "kind": "image-nonzero-port-zero", "name": "g_tab_52b84", "image_head": "00000000000000000000000000000000", "init": ""}
  {"ds": 340012, "kind": "image-zero-port-nonzero", "name": "kXlat_5302c", "image_head": "00000000000000000000000000000000", "init": "= { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x"}
  {"ds": 340828, "kind": "image-nonzero-port-zero", "name": "g_rng_state", "image_head": "01000000", "init": "= 0"}
  {"ds": 340148, "kind": "image-nonzero-port-zero", "name": "g_dat_530b4", "image_head": "64000000", "init": "= 0"}
  {"ds": 340152, "kind": "image-nonzero-port-zero", "name": "g_dat_530b8", "image_head": "64000000", "init": "= 0"}
  {"ds": 340472, "kind": "image-nonzero-port-zero", "name": "g_dat_531f8", "image_head": "ffff0000", "init": "= 0"}
  {"ds": 409188, "kind": "image-zero-port-nonzero", "name": "g_dat_63e64", "image_head": "00000000", "init": "= 0x140"}
  {"ds": 341529, "kind": "image-nonzero-port-zero", "name": "g_dat_53619", "image_head": "00010000", "init": "= 0"}
  {"ds": 340140, "kind": "image-nonzero-port-zero", "name": "g_dat_530ac", "image_head": "20030000", "init": "= 0"}
  {"ds": 406448, "kind": "image-zero-port-nonzero", "name": "g_dat_633b0", "image_head": "00000000", "init": "= SCREEN_BASE"}
  {"ds": 406452, "kind": "image-zero-port-nonzero", "name": "g_dat_633b4", "image_head": "00000000", "init": "= SCREEN_BASE + SCREEN_SIZE"}
  {"ds": 335000, "kind": "image-nonzero-port-zero", "name": "g_dat_51c98", "image_head": "01000000", "init": "= 0"}
  {"ds": 341724, "kind": "image-nonzero-port-zero", "name": "g_dat_536dc", "image_head": "14000000", "init": ""}
  {"ds": 341888, "kind": "image-nonzero-port-zero", "name": "g_dat_53780", "image_head": "100e0000", "init": "= 0"}
  {"ds": 341892, "kind": "image-nonzero-port-zero", "name": "g_dat_53784", "image_head": "01000000", "init": "= 0"}
  {"ds": 341896, "kind": "image-nonzero-port-zero", "name": "g_dat_53788", "image_head": "45535400", "init": "= 0"}
  {"ds": 341927, "kind": "image-nonzero-port-zero", "name": "g_dat_537a7", "image_head": "45445400", "init": "= 0"}
  {"ds": 341884, "kind": "image-nonzero-port-zero", "name": "g_dat_5377c", "image_head": "50460000", "init": "= 0"}
  {"ds": 341564, "kind": "image-nonzero-port-zero", "name": "g_dat_5363c", "image_head": "01000000", "init": "= 0"}
  {"ds": 341568, "kind": "image-nonzero-port-zero", "name": "g_dat_53640", "image_head": "10000000", "init": "= 0"}
  {"ds": 340840, "kind": "image-nonzero-port-zero", "name": "g_dat_53368", "image_head": "2000", "init": ""}
  {"ds": 334348, "kind": "image-zero-port-nonzero", "name": "g_dat_51a0c", "image_head": "00", "init": "= g_video_ram"}

## c3b_full_span_initializers: 12 findings
  {"ds": "0x53c4c", "name": "g_dat_53c4c", "span": 1, "bad": null, "note": "span runs outside the image-initialised DS region"}
  {"ds": "0x53818", "name": "g_dat_53818", "span": 4, "bad": null, "note": "span runs outside the image-initialised DS region"}
  {"ds": "0x62d68", "name": "g_dat_62d68", "span": 4, "bad": null, "note": "span runs outside the image-initialised DS region"}
  {"ds": "0x63f14", "name": "g_dat_63f14", "span": 1, "bad": null, "note": "span runs outside the image-initialised DS region"}
  {"ds": "0x63f28", "name": "g_dat_63f28", "span": 4, "bad": null, "note": "span runs outside the image-initialised DS region"}
  {"ds": "0x62cd8", "name": "g_dat_62cd8", "span": 1, "bad": null, "note": "span runs outside the image-initialised DS region"}
  {"ds": "0x62c58", "name": "g_dat_62c58", "span": 1, "bad": null, "note": "span runs outside the image-initialised DS region"}
  {"ds": "0x639ec", "name": "g_dat_639ec", "span": 1, "bad": null, "note": "span runs outside the image-initialised DS region"}
  {"ds": "0x63f1e", "name": "g_dat_63f1e", "span": 1, "bad": null, "note": "span runs outside the image-initialised DS region"}
  {"ds": "0x53e24", "name": "g_dat_53e24", "span": 1, "bad": null, "note": "span runs outside the image-initialised DS region"}
  {"ds": "0x48a36", "name": "g_scale_48a36", "inside": "fn_048a48 (data pool in the gap before the body)", "note": "address is inside a shipped function body (code, not image data); compared bytes would be instructions"}
  {"ds": "0x48312", "name": "g_coeff_48312", "inside": "fn_048312", "note": "address is inside a shipped function body (code, not image data); compared bytes would be instructions"}

## c4_representation: 3 findings
  {"ds": 408044, "why": "port-declares-pointer-guest-looks-numeric", "score": 2, "port_name": "g_dat_639ec", "port_type": "uint8_t *", "guest_type": "undefined4", "image_u32": 0, "small_imm_stores": [], "indexed_reads": ["CMP dword ptr [EAX + 0x639ec] 0x0"], "n_reads": 26, "n_writes": 3}
  {"ds": 341808, "why": "port-declares-pointer-guest-looks-numeric", "score": 2, "port_name": "g_dat_53730", "port_type": "uint32_t *", "guest_type": "pointer", "image_u32": 14048, "small_imm_stores": [], "indexed_reads": [], "n_reads": 4, "n_writes": 0}
  {"ds": 341962, "why": "port-declares-pointer-guest-looks-numeric", "score": 2, "port_name": "g_dat_537ca", "port_type": "uint8_t *", "guest_type": "pointer", "image_u32": 14247, "small_imm_stores": [], "indexed_reads": [], "n_reads": 1, "n_writes": 0}

## c6_image_data_without_symbol: 56 findings
  {"ds_start": "0x52c3c", "ds_end": "0x52f3c", "cells": 192, "image_head": "ac000000ac000000ac000000ac000000", "immediates_pointing_nearby": 100, "distinct_base_constants": ["0x524bc", "0x52580", "0x525a8", "0x52738", "0x5274c", "0x528b4"], "port_models_anything_here": ["g_tab_52c3c", "g_tab_52c3c", "
  {"ds_start": "0x528a8", "ds_end": "0x52b84", "cells": 183, "image_head": "0000003f0000003f0000003f09000000", "immediates_pointing_nearby": 58, "distinct_base_constants": ["0x520ac", "0x52170", "0x52198", "0x524bc", "0x52580", "0x525a8"], "port_models_anything_here": ["g_tab_528b4", "g_tab_528b4", "g
  {"ds_start": "0x531f8", "ds_end": "0x53334", "cells": 79, "image_head": "ffff000000000a00e5000000eb000000", "immediates_pointing_nearby": 107, "distinct_base_constants": ["0x52a1c", "0x52b84", "0x52be0", "0x52c3c", "0x52da4", "0x52f08"], "port_models_anything_here": ["g_dat_531f8", "g_dat_531f8"]}
  {"ds_start": "0x51b64", "ds_end": "0x51c4c", "cells": 58, "image_head": "0b0000000c0000000d0000000e000000", "immediates_pointing_nearby": 134, "distinct_base_constants": ["0x5144c", "0x51450", "0x51454", "0x51458", "0x5145c", "0x5145e"], "port_models_anything_here": ["g_blk_51c24", "g_blk_51c24"]}
  {"ds_start": "0x51c9c", "ds_end": "0x51d3c", "cells": 40, "image_head": "c9fffffffbffffffd8ffffff05000000", "immediates_pointing_nearby": 120, "distinct_base_constants": ["0x514ab", "0x514b3", "0x514bc", "0x514c5", "0x514ce", "0x514d7"], "port_models_anything_here": []}
  {"ds_start": "0x51850", "ds_end": "0x518e0", "cells": 36, "image_head": "0000f0bf010204081020408000010101", "immediates_pointing_nearby": 132, "distinct_base_constants": ["0x5144c", "0x51450", "0x51454", "0x51458", "0x5145c", "0x5145e"], "port_models_anything_here": []}
  {"ds_start": "0x530bc", "ds_end": "0x53100", "cells": 17, "image_head": "7f0000007f0000007f0000007f000000", "immediates_pointing_nearby": 107, "distinct_base_constants": ["0x52a1c", "0x52b84", "0x52be0", "0x52c3c", "0x52da4", "0x52f08"], "port_models_anything_here": ["g_dat_530bc", "g_dat_530bc"]}
  {"ds_start": "0x5144c", "ds_end": "0x51484", "cells": 14, "image_head": "25640a00256420002564200025632000", "immediates_pointing_nearby": 121, "distinct_base_constants": ["0x5144c", "0x51450", "0x51454", "0x51458", "0x5145c", "0x5145e"], "port_models_anything_here": ["g_dat_5147c"]}
  {"ds_start": "0x537e4", "ds_end": "0x5380c", "cells": 10, "image_head": "0237030000019c1f03000002bd390300", "immediates_pointing_nearby": 160, "distinct_base_constants": ["0x52ff8", "0x52ffc", "0x5302c", "0x530ac", "0x530b0", "0x530b4"], "port_models_anything_here": []}
  {"ds_start": "0x51968", "ds_end": "0x51994", "cells": 11, "image_head": "78009700b500d400f300110130014e01", "immediates_pointing_nearby": 133, "distinct_base_constants": ["0x5144c", "0x51450", "0x51454", "0x51458", "0x5145c", "0x5145e"], "port_models_anything_here": ["g_leap_tbl"]}
  {"ds_start": "0x52ffc", "ds_end": "0x5302c", "cells": 12, "image_head": "01000000020000000200000001000000", "immediates_pointing_nearby": 108, "distinct_base_constants": ["0x528b4", "0x52a1c", "0x52b84", "0x52be0", "0x52c3c", "0x52da4"], "port_models_anything_here": ["g_tab_52ffc", "g_tab_52ffc"]}
  {"ds_start": "0x519d0", "ds_end": "0x519f0", "cells": 8, "image_head": "3600cec9148887e1244b1b000284fee4", "immediates_pointing_nearby": 134, "distinct_base_constants": ["0x5144c", "0x51450", "0x51454", "0x51458", "0x5145c", "0x5145e"], "port_models_anything_here": []}
  {"ds_start": "0x536e0", "ds_end": "0x536f4", "cells": 5, "image_head": "01000000020000000200000003000000", "immediates_pointing_nearby": 161, "distinct_base_constants": ["0x52f08", "0x52f2c", "0x52f90", "0x52ff8", "0x52ffc", "0x5302c"], "port_models_anything_here": []}
  {"ds_start": "0x51c80", "ds_end": "0x51c98", "cells": 6, "image_head": "5700000058000000590000005a000000", "immediates_pointing_nearby": 122, "distinct_base_constants": ["0x51481", "0x51484", "0x5148e", "0x514ab", "0x514b3", "0x514bc"], "port_models_anything_here": []}
  {"ds_start": "0x51c68", "ds_end": "0x51c7c", "cells": 5, "image_head": "5e000000520000005300000054000000", "immediates_pointing_nearby": 126, "distinct_base_constants": ["0x5146a", "0x51470", "0x51475", "0x5147c", "0x51481", "0x51484"], "port_models_anything_here": []}
  {"ds_start": "0x51d58", "ds_end": "0x51d70", "cells": 6, "image_head": "c30000000000003f0000403fae47613f", "immediates_pointing_nearby": 102, "distinct_base_constants": ["0x5155a", "0x51563", "0x5156b", "0x51576", "0x5157a", "0x5157c"], "port_models_anything_here": []}
  {"ds_start": "0x519f4", "ds_end": "0x51a04", "cells": 4, "image_head": "84d797410400000000000088c3400100", "immediates_pointing_nearby": 134, "distinct_base_constants": ["0x5144c", "0x51450", "0x51454", "0x51458", "0x5145c", "0x5145e"], "port_models_anything_here": []}
  {"ds_start": "0x52fa4", "ds_end": "0x52fb8", "cells": 5, "image_head": "04000000030000000100000001000000", "immediates_pointing_nearby": 101, "distinct_base_constants": ["0x528b4", "0x52a1c", "0x52b84", "0x52be0", "0x52c3c", "0x52da4"], "port_models_anything_here": ["g_dat_52fb4", "g_dat_52fb4"]}
  {"ds_start": "0x51d74", "ds_end": "0x51d88", "cells": 5, "image_head": "0000e03f000000400000204000004040", "immediates_pointing_nearby": 100, "distinct_base_constants": ["0x51576", "0x5157a", "0x5157c", "0x51588", "0x51592", "0x5159e"], "port_models_anything_here": []}
  {"ds_start": "0x52780", "ds_end": "0x527ec", "cells": 27, "image_head": "0000003f0000003f0000003f0000003f", "immediates_pointing_nearby": 17, "distinct_base_constants": ["0x520ac", "0x52170", "0x52198", "0x524bc", "0x52580", "0x525a8"], "port_models_anything_here": []}
  {"ds_start": "0x53780", "ds_end": "0x5378c", "cells": 3, "image_head": "100e00000100000045535400", "immediates_pointing_nearby": 161, "distinct_base_constants": ["0x52f90", "0x52ff8", "0x52ffc", "0x5302c", "0x530ac", "0x530b0"], "port_models_anything_here": ["g_dat_53780", "g_dat_53780", "g_dat_5378
  {"ds_start": "0x53628", "ds_end": "0x53634", "cells": 3, "image_head": "000901010b06040505050203", "immediates_pointing_nearby": 146, "distinct_base_constants": ["0x52f08", "0x52f2c", "0x52f90", "0x52ff8", "0x52ffc", "0x5302c"], "port_models_anything_here": []}
  {"ds_start": "0x53078", "ds_end": "0x53088", "cells": 4, "image_head": "6c00000039674d2a0048006800002c00", "immediates_pointing_nearby": 108, "distinct_base_constants": ["0x528b4", "0x52a1c", "0x52b84", "0x52be0", "0x52c3c", "0x52da4"], "port_models_anything_here": []}
  {"ds_start": "0x53098", "ds_end": "0x530a8", "cells": 4, "image_head": "4b00000039674d2a0048006800002c00", "immediates_pointing_nearby": 108, "distinct_base_constants": ["0x528b4", "0x52a1c", "0x52b84", "0x52be0", "0x52c3c", "0x52da4"], "port_models_anything_here": []}
  {"ds_start": "0x53110", "ds_end": "0x53120", "cells": 4, "image_head": "d3000000e4000000e5000000d9000000", "immediates_pointing_nearby": 107, "distinct_base_constants": ["0x52a1c", "0x52b84", "0x52be0", "0x52c3c", "0x52da4", "0x52f08"], "port_models_anything_here": []}

## c7_ds_table_bases: 543 findings
  {"ds": "0x53c7c", "n_sites": 869, "fns": ["fn_01194c", "fn_012616", "fn_01348f", "fn_013aad", "fn_013c4d", "fn_01441e"]}
  {"ds": "0x63354", "n_sites": 244, "fns": ["fn_010d47", "fn_012616", "fn_01586b", "fn_016a60", "fn_016b8f", "fn_01730b"]}
  {"ds": "0x53eb8", "n_sites": 148, "fns": ["fn_015a1f", "fn_015b5b", "fn_015d16", "fn_015ed1", "fn_01603e", "fn_0161f9"]}
  {"ds": "0x639dc", "n_sites": 90, "fns": ["fn_012efa", "fn_013908", "fn_01519f", "fn_015295", "fn_01545e", "fn_0374ad"]}
  {"ds": "0x53eb0", "n_sites": 80, "fns": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8", "fn_01cf64"]}
  {"ds": "0x639b8", "n_sites": 77, "fns": ["fn_010010", "fn_032464", "fn_035f1b", "fn_036041", "fn_036190", "fn_03628e"]}
  {"ds": "0x639e0", "n_sites": 77, "fns": ["fn_012efa", "fn_013908", "fn_0151d5", "fn_01586b", "fn_02319c", "fn_024bf9"]}
  {"ds": "0x51d5c", "n_sites": 60, "fns": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8", "fn_01cf64"]}
  {"ds": "0x51c9c", "n_sites": 52, "fns": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8", "fn_01df8b"]}
  {"ds": "0x636b8", "n_sites": 49, "fns": ["fn_035f1b", "fn_036041", "fn_03628e", "fn_036417", "fn_038a20", "fn_038cde"]}
  {"ds": "0x51ca0", "n_sites": 46, "fns": ["fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8", "fn_01df8b", "fn_01f064"]}
  {"ds": "0x633b0", "n_sites": 43, "fns": ["fn_03658d", "fn_0366a4", "fn_038445", "fn_038f3d", "fn_039c03", "fn_03a70e"]}
  {"ds": "0x63370", "n_sites": 42, "fns": ["fn_010d47", "fn_010efc", "fn_012616", "fn_03658d", "fn_0374ad", "fn_0376c8"]}
  {"ds": "0x53eb4", "n_sites": 40, "fns": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8", "fn_01cf64"]}
  {"ds": "0x53f20", "n_sites": 40, "fns": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8", "fn_01cf64"]}
  {"ds": "0x53f1c", "n_sites": 39, "fns": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8", "fn_01cf64"]}
  {"ds": "0x639ec", "n_sites": 35, "fns": ["fn_010efc", "fn_013c4d", "fn_01441e", "fn_03690b", "fn_037347", "fn_0374ad"]}
  {"ds": "0x53c5c", "n_sites": 30, "fns": ["fn_0104c7", "fn_010570", "fn_0105f7", "fn_01072f", "fn_0107b9", "fn_010927"]}
  {"ds": "0x63f98", "n_sites": 30, "fns": ["fn_04097e", "fn_040b83"]}
  {"ds": "0x51d68", "n_sites": 27, "fns": ["fn_02319c", "fn_024bf9", "fn_0265ac", "fn_027bec", "fn_029709", "fn_02adf5"]}
  {"ds": "0x63360", "n_sites": 27, "fns": ["fn_012616", "fn_012efa", "fn_0137c1", "fn_03272f", "fn_03a829", "fn_03b718"]}
  {"ds": "0x63f50", "n_sites": 27, "fns": ["fn_03272f", "fn_034353", "fn_03442e", "fn_0348e2", "fn_03493e", "fn_03feb8"]}
  {"ds": "0x51ca4", "n_sites": 26, "fns": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8", "fn_01df8b"]}
  {"ds": "0x63e24", "n_sites": 24, "fns": ["fn_010d47", "fn_038098", "fn_0381d7", "fn_039c03", "fn_03a5f7", "fn_03a70e"]}
  {"ds": "0x63ec1", "n_sites": 24, "fns": ["fn_03272f", "fn_035d5d", "fn_035dfc", "fn_038445", "fn_038cde", "fn_038f3d"]}

## c8_base_pair_aliasing: 5466 findings
  {"base_a": "0x53eb0", "base_b": "0x53eb8", "distance": "0x8", "n_a": 80, "n_b": 148, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8"], "port_name_a": [], "port_name_b": [], "score": 140}
  {"base_a": "0x639dc", "base_b": "0x639e0", "distance": "0x4", "n_a": 90, "n_b": 77, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_012efa", "fn_013908", "fn_0374ad", "fn_0376c8"], "port_name_a": ["g_dat_639dc"], "port_name_b": ["g_dat_639e0"], "score": 119}
  {"base_a": "0x53eb0", "base_b": "0x53eb4", "distance": "0x4", "n_a": 80, "n_b": 40, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8"], "port_name_a": [], "port_name_b": [], "score": 100}
  {"base_a": "0x53eb4", "base_b": "0x53eb8", "distance": "0x4", "n_a": 40, "n_b": 148, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8"], "port_name_a": [], "port_name_b": [], "score": 100}
  {"base_a": "0x53f1c", "base_b": "0x53f20", "distance": "0x4", "n_a": 39, "n_b": 40, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8"], "port_name_a": [], "port_name_b": [], "score": 99}
  {"base_a": "0x51c9c", "base_b": "0x51ca0", "distance": "0x4", "n_a": 52, "n_b": 46, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8"], "port_name_a": [], "port_name_b": [], "score": 88}
  {"base_a": "0x53ea0", "base_b": "0x53eb0", "distance": "0x10", "n_a": 21, "n_b": 80, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8"], "port_name_a": [], "port_name_b": [], "score": 78}
  {"base_a": "0x53ea0", "base_b": "0x53eb4", "distance": "0x14", "n_a": 21, "n_b": 40, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8"], "port_name_a": [], "port_name_b": [], "score": 78}
  {"base_a": "0x53ea0", "base_b": "0x53eb8", "distance": "0x18", "n_a": 21, "n_b": 148, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8"], "port_name_a": [], "port_name_b": [], "score": 78}
  {"base_a": "0x639b8", "base_b": "0x639dc", "distance": "0x24", "n_a": 77, "n_b": 90, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": [], "port_name_a": [], "port_name_b": ["g_dat_639dc"], "score": 77}
  {"base_a": "0x639b8", "base_b": "0x639e0", "distance": "0x28", "n_a": 77, "n_b": 77, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": [], "port_name_a": [], "port_name_b": ["g_dat_639e0"], "score": 77}
  {"base_a": "0x63354", "base_b": "0x63370", "distance": "0x1c", "n_a": 244, "n_b": 42, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_010d47", "fn_012616", "fn_0374ad", "fn_0376c8"], "port_name_a": ["g_dat_63354"], "port_name_b": ["g_dat_63370"], "score": 75}
  {"base_a": "0x51c9c", "base_b": "0x51ca4", "distance": "0x8", "n_a": 52, "n_b": 26, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_017f13", "fn_018e88", "fn_019e48", "fn_01aec8"], "port_name_a": [], "port_name_b": [], "score": 74}
  {"base_a": "0x51ca0", "base_b": "0x51ca4", "distance": "0x4", "n_a": 46, "n_b": 26, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8"], "port_name_a": [], "port_name_b": [], "score": 68}
  {"base_a": "0x51c9c", "base_b": "0x51ca8", "distance": "0xc", "n_a": 52, "n_b": 23, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8"], "port_name_a": [], "port_name_b": [], "score": 65}
  {"base_a": "0x51ca0", "base_b": "0x51ca8", "distance": "0x8", "n_a": 46, "n_b": 23, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8"], "port_name_a": [], "port_name_b": [], "score": 65}
  {"base_a": "0x51ca4", "base_b": "0x51ca8", "distance": "0x4", "n_a": 26, "n_b": 23, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_018e88", "fn_019e48", "fn_01aec8", "fn_01bea8"], "port_name_a": [], "port_name_b": [], "score": 65}
  {"base_a": "0x63370", "base_b": "0x633b0", "distance": "0x40", "n_a": 42, "n_b": 43, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_03658d", "fn_039c03", "fn_03a70e", "fn_03a829"], "port_name_a": ["g_dat_63370"], "port_name_b": ["g_dat_633b0"], "score": 57}
  {"base_a": "0x63354", "base_b": "0x633b0", "distance": "0x5c", "n_a": 244, "n_b": 43, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_038445", "fn_039c03", "fn_03a829", "fn_03d244"], "port_name_a": ["g_dat_63354"], "port_name_b": ["g_dat_633b0"], "score": 55}
  {"base_a": "0x51d5c", "base_b": "0x51d68", "distance": "0xc", "n_a": 60, "n_b": 27, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_02319c", "fn_024bf9", "fn_0265ac", "fn_027bec"], "port_name_a": [], "port_name_b": [], "score": 54}
  {"base_a": "0x53eb8", "base_b": "0x53eb9", "distance": "0x1", "n_a": 148, "n_b": 18, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_015b5b", "fn_015d16", "fn_01603e", "fn_0161f9"], "port_name_a": [], "port_name_b": [], "score": 54}
  {"base_a": "0x63354", "base_b": "0x636b8", "distance": "0x364", "n_a": 244, "n_b": 49, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_03af18"], "port_name_a": ["g_dat_63354"], "port_name_b": ["g_dat_636b8"], "score": 52}
  {"base_a": "0x636b8", "base_b": "0x639dc", "distance": "0x324", "n_a": 49, "n_b": 90, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_03af18"], "port_name_a": ["g_dat_636b8"], "port_name_b": ["g_dat_639dc"], "score": 52}
  {"base_a": "0x53c6c", "base_b": "0x53c7c", "distance": "0x10", "n_a": 20, "n_b": 869, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_015a1f", "fn_015b5b", "fn_015d16", "fn_015ed1"], "port_name_a": ["g_dat_53c6c"], "port_name_b": ["g_dat_53c7c_null_warned", "g_player
  {"base_a": "0x53c68", "base_b": "0x53c70", "distance": "0x8", "n_a": 19, "n_b": 20, "strides_a": [], "strides_b": [], "same_stride": false, "shared_functions": ["fn_010efc", "fn_0112fd", "fn_013aad", "fn_01441e"], "port_name_a": ["g_dat_53c68"], "port_name_b": ["g_dat_53c70"], "score": 49}

## c5_dead_sides: guest-without-port=90 port-never-called=184
  guest_functi FUN_00041fa1
  guest_functi FUN_00042b7d
  guest_functi _entry
  guest_functi calibrate_clock
  guest_functi caseD_0
  guest_functi caseD_11
  guest_functi caseD_2
  guest_functi caseD_3
  guest_functi caseD_4
  guest_functi caseD_8
  guest_functi case_field_0x28
  guest_functi date_cmp
  guest_functi date_match_recur
  guest_functi date_norm
  guest_functi date_recur
  guest_functi datetime_clock_offset
  guest_functi datetime_pack
  guest_functi debug_break_hook
  guest_functi delay_units
  guest_functi float_str_sci
  port_functio a3a8_dbl
  port_functio abs32
  port_functio act_block
  port_functio act_trace_counts
  port_functio act_trace_targets
  port_functio act_trace_targets_all
  port_functio ai_blk_f32
  port_functio ai_blk_i32
  port_functio ai_cfg_i32
  port_functio ai_climb_xy
  port_functio ai_fget
  port_functio ai_fput
  port_functio ai_rec_i32
  port_functio ai_script_byte
  port_functio blit_stats_dump
  port_functio blit_stats_on
  port_functio blk50_f32
  port_functio blk50_f64
  port_functio blt_rows
  port_functio board_in
