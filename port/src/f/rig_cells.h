/* The port's state-dump table interface (rig_sink consumes it).
 *
 * The table itself is generated from the shipped Ghidra symbol database
 * (scripts/audit/gen_headers.py -> src/f/rig_table.h) and included at the end of f.c, because many of
 * the DS-claiming globals are file-scope statics there: the accessors are the only way to name them
 * from another translation unit without changing their linkage. */
#ifndef NIGHT_RIG_CELLS_H
#define NIGHT_RIG_CELLS_H
#include <stdint.h>

struct rig_cell {
    uint32_t    ds;          /* address inside NIGHT.EXE's DS group (.object2) */
    uint16_t    guest_size;  /* size the shipped symbol/accesses imply, 0 = unknown */
    uint16_t    host_size;   /* size of the port object it points at */
    uint16_t    ptr;         /* the port declares it as a pointer (host slot is 8 bytes) */
    const void *host;
    const char *name;
};

int                    rig_cell_count(void);
const struct rig_cell *rig_cell_at(int i);
#endif
