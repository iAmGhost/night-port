/* sys/auxv.h — desktop-only shim for the app_pc build (-DOF_PC).
 *
 * Why this exists: src/sdk/of_init.c includes <sys/auxv.h> and calls
 * getauxval(AT_OF_CAPS)/getauxval(AT_OF_SVC) to pick up the kernel-supplied
 * capability and service tables.  That is right on the device (musl ships the
 * header); macOS has no <sys/auxv.h> at all, so the desktop build — which
 * compiles of_init.c too — needs a stand-in.  On the desktop there is no
 * kernel auxv vector: the SDK's own OF_PC branches (of_caps.h:284,
 * of_services.h:342) supply caps/services without the tables, so returning 0
 * for every tag is the honest answer.
 *
 * Only the PC include path (-I. in sdk.mk's ALL_PC_CFLAGS) sees this file; the
 * RISC-V build is -nostdinc and picks musl's real header from
 * pocket/src/sdk/musl/include/sys/auxv.h.
 */
#ifndef NIGHT_PC_SYS_AUXV_H
#define NIGHT_PC_SYS_AUXV_H

static inline unsigned long getauxval(unsigned long type) {
    (void)type;
    return 0UL;
}

#endif /* NIGHT_PC_SYS_AUXV_H */
