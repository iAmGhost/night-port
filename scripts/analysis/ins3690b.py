voc = ['"%d.voc"' % i for i in range(1, 101)]
rows = []
for i in range(0, 100, 5):
    rows.append('    ' + ', '.join(voc[i:i+5]) + (',' if i + 5 < 100 else ''))
voc_src = '\n'.join(rows)

code = '''
/* The master loader's destination globals (host pointers; NULL when the load failed). */
uint8_t *g_dat_62aa0, *g_dat_62aa4, *g_dat_62aa8, *g_dat_62aac, *g_dat_62ab0;
uint8_t *g_dat_62ab4, *g_dat_62ab8, *g_dat_62abc, *g_dat_62ac0, *g_dat_62ac4;
uint8_t *g_dat_62a98, *g_dat_62a9c;
uint32_t *g_dat_53c28, *g_dat_53c2c, *g_dat_53c30, *g_dat_53818;
uint32_t *g_dat_63e08, *g_dat_63e0c;

/* DAT_000525a8: the 100 sound-slot name pointers. The image table is exactly the strings
 * "1.voc" .. "100.voc" (verified by dumping 0x0525a8), so the port expands it as literals. */
static const char *const kVocNames[100] = {
@VOC@
};

/* DAT_00053d0c: sound effect buffers (host pointers into the heap arena; NULL when the allocation
 * failed, exactly as the original stores the raw heap address or 0). */
uint8_t *g_dat_53d0c[100];
/* DAT_00053c94: the 6 x 5 experience table parsed out of exptab.dat (row stride 0x14 bytes). */
uint32_t g_dat_53c94[30];

/* The loader body repeats one guard pattern ~24 times (allocate a STRUCT-sized block, load it, and
 * store NULL when the allocation failed). The three helpers below are that triplicated pattern; the
 * block sizes are the struct sizes, not data sizes: 0x4b4 for the fn_032464 key tables, 8 for the
 * fn_010010 action containers and 0x3c for the fn_03d8d4 sprite structs. */
static uint8_t *key_block(const char *name)
{
    uint32_t p = heap_malloc(0x4b4);                   /* 0x041bdc wrapper (size 0 -> 1) */
    if (p == 0) return 0;
    return fn_032464((uint8_t *)g_heap_base + p, name);
}
static uint32_t *act_block(const char *name)
{
    uint32_t p = heap_malloc(8);
    if (p == 0) return 0;
    fn_010010((uint8_t *)g_heap_base + p, name);
    return (uint32_t *)((uint8_t *)g_heap_base + p);
}
static uint32_t *spr_block(const char *name)
{
    uint32_t p = heap_malloc(0x3c);
    if (p == 0) return 0;
    return fn_03d8d4((uint32_t *)((uint8_t *)g_heap_base + p), name);
}

/* fn_03690b  orig: 0x03690b (L13, 2555 B) -- THE master data loader: opens NIGHT.RES and turns it
 * into the game's runtime tables. Fully linear; every load is guarded by its allocation.
 * See docs/12-functions/03690b-master-data-loader.md for the field-level walkthrough.
 * The sprite/action name arguments of the last group come from pointer cells in the image
 * (0x51d88 "gun.spr", 0x520ac "mark.spr", 0x524bc "mark.act", 0x52170 "dust.spr",
 * 0x52580 "dust.act"); the port passes the literals. */
uint32_t fn_03690b(void)
{
    (void)canary_id(0, 0x1c8);                                        /* 0x3690b-910 */

    g_639b8 = fn_04270d((const uint8_t *)"night.res", "rb");           /* 0x36915-.. */
    if (g_639b8 == 0) {
        /* The original does NOT return here; it falls through into fn_0366a4 with a NULL reader. */
        fn_036537((uint8_t *)"NIGHT.RES File Not Found!!!\\n");         /* 0x36941 */
    }
    fn_0366a4((uint8_t *)g_639b8);                                    /* the file index */

    /* key-binding tables (0x032464 blocks) */
    g_dat_62aa0 = key_block("gun.key");
    g_dat_62aa4 = key_block("gun1.key");
    g_dat_62aa8 = key_block("gun2.key");
    g_dat_62aac = key_block("gun3.key");
    g_dat_62ab0 = key_block("gun4.key");
    g_dat_62ab4 = key_block("catch.key");
    g_dat_62ab8 = key_block("catch1.key");
    g_dat_62abc = key_block("catch2.key");
    g_dat_62ac0 = key_block("catch3.key");
    g_dat_62ac4 = key_block("catch4.key");

    /* action containers (0x010010 blocks) */
    g_dat_53c28 = act_block("gun.act");
    g_dat_53c2c = act_block("gun1.act");
    g_dat_53c30 = act_block("gun2.act");

    /* the "currently selected" indirections */
    g_dat_62a98 = g_dat_62aa0;
    g_dat_62a9c = g_dat_62ab4;

    g_dat_63e0c = spr_block("pause.spr");

    /* the 100 sound slots: size from the index table, buffer of size*2, then the payload */
    for (uint32_t i = 0; i < 100; i = i + 1) {
        int32_t size = fn_036626(kVocNames[i]);
        uint32_t off = heap_malloc((uint32_t)size * 2u);
        g_dat_53d0c[i] = (off == 0) ? 0 : (uint8_t *)g_heap_base + off;
        fn_0417f3(kVocNames[i], g_dat_53d0c[i]);                       /* skips the 0x1a header */
    }

    /* HUD / sprite tables (0x03d8d4 structs) and their action containers */
    g_dat_639c0 = spr_block("system.spr");
    g_dat_639c4 = (struct blit_ctl *)spr_block("system2.spr");
    g_dat_63e08 = spr_block("over.spr");
    g_dat_53c8c = spr_block("face.spr");
    g_dat_63e00 = spr_block("gage.spr");
    g_dat_63e04 = (uint8_t *)spr_block("exp.spr");
    g_dat_639ec = spr_block("gun.spr");
    g_dat_53818 = g_dat_53c28;                                        /* active action table */
    g_dat_63d10 = spr_block("mark.spr");
    g_dat_53b3c = act_block("mark.act");
    g_dat_63dd4 = spr_block("dust.spr");
    g_dat_53c00 = act_block("dust.act");

    /* exptab.dat: 6 rows of 5 numbers, then the row's newline */
    fn_0367c9("exptab.dat");
    for (uint32_t i = 0; i < 6; i = i + 1) {
        for (uint32_t j = 0; j < 5; j = j + 1) {
            uintptr_t slot = (uintptr_t)&g_dat_53c94[i * 5u + j];
            fn_042023((uint8_t *)g_639b8, "%d ", &slot);
        }
        uintptr_t unused = 0;                                         /* no slot for the literal */
        fn_042023((uint8_t *)g_639b8, "\\n", &unused);
    }

    g_dat_63358 = 0;                                                  /* 0x36..: DAT_00063358 = 0 */
    return 1;
}

''' .replace('@VOC@', voc_src)

p = 'src/f/f.c'
s = open(p).read()
anchor = '/* 0x036626: case-insensitive lookup of s in the 0x5d884 table (bound g_dat_633a0),'
assert s.count(anchor) == 1
# place the loader just before the lookup helper it uses
s = s.replace(anchor, code.lstrip('\n') + anchor, 1)
open(p, 'w').write(s)

h = '''extern uint8_t *g_dat_62aa0;   /* DAT_00062aa0: "gun.key" text table (fn_032464 block) */
extern uint8_t *g_dat_62aa4;   /* DAT_00062aa4: "gun1.key" */
extern uint8_t *g_dat_62aa8;   /* DAT_00062aa8: "gun2.key" */
extern uint8_t *g_dat_62aac;   /* DAT_00062aac: "gun3.key" */
extern uint8_t *g_dat_62ab0;   /* DAT_00062ab0: "gun4.key" */
extern uint8_t *g_dat_62ab4;   /* DAT_00062ab4: "catch.key" */
extern uint8_t *g_dat_62ab8;   /* DAT_00062ab8: "catch1.key" */
extern uint8_t *g_dat_62abc;   /* DAT_00062abc: "catch2.key" */
extern uint8_t *g_dat_62ac0;   /* DAT_00062ac0: "catch3.key" */
extern uint8_t *g_dat_62ac4;   /* DAT_00062ac4: "catch4.key" */
extern uint8_t *g_dat_62a98;   /* DAT_00062a98: active key table (alias of g_dat_62aa0) */
extern uint8_t *g_dat_62a9c;   /* DAT_00062a9c: active catch table (alias of g_dat_62ab4) */
extern uint32_t *g_dat_53c28;  /* DAT_00053c28: "gun.act" record container (fn_010010) */
extern uint32_t *g_dat_53c2c;  /* DAT_00053c2c: "gun1.act" */
extern uint32_t *g_dat_53c30;  /* DAT_00053c30: "gun2.act" */
extern uint32_t *g_dat_53818;  /* DAT_00053818: active action table (alias of g_dat_53c28) */
extern uint32_t *g_dat_63e08;  /* DAT_00063e08: "over.spr" struct (fn_03d8d4) */
extern uint32_t *g_dat_63e0c;  /* DAT_00063e0c: "pause.spr" struct (fn_03d8d4) */
extern uint8_t  *g_dat_53d0c[100]; /* DAT_00053d0c: 100 sound-slot buffers ("1.voc".."100.voc") */
extern uint32_t  g_dat_53c94[30];  /* DAT_00053c94: 6x5 experience table from exptab.dat */
uint32_t fn_03690b(void);          /* 0x03690b: master data loader (NIGHT.RES -> all runtime tables) */
'''
t = open('src/f/f.h').read()
mark = 'extern uint32_t *g_dat_639c0;  /* 0x639c0: struct host-ptr (destructed by fn_03d999) */'
assert t.count(mark) == 1
t = t.replace(mark, h + mark, 1)
open('src/f/f.h', 'w').write(t)
print('inserted')
