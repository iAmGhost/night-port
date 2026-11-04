/* C port of FUN_0004794a (L0, 363B) — 128-bit fixed-point add + exponent
 * normalize. Transcribed from the Ghidra decompile with register-ABI names:
 *   param_1=EAX, param_2=EDX, param_3=EBX, param_4=ECX, ESI=exponent (32-bit
 *   holding two 16-bit exponent fields). Returns CONCAT44(uVar12,uVar2) =
 *   (EDX<<32)|EAX as uint64_t.
 *
 * The CONCAT/CARRY Ghidra idioms are expanded. The one Ghidra "ESI reassign"
 * in the early path (CONCAT22(...)) is a NO-OP: disasm 0x047952 ADD SI,SI
 * followed by 0x04796a RCR SI,1 restores SI to its original value (verified by
 * hand: RCR-through-carry of a doubled 16-bit value == identity). So ESI is
 * unchanged there.
 *
 * Verified against scratch/fp_04794a_ref.py (a mini x86 emulator of the 149
 * instructions) over random inputs. */
#include <stdint.h>

static inline uint64_t cc44(uint32_t hi, uint32_t lo) { return ((uint64_t)hi << 32) | lo; }
static inline uint64_t cc22(uint16_t hi, uint16_t lo) { return ((uint64_t)hi << 16) | lo; }
static inline uint64_t cc12(uint8_t hi, uint16_t lo) { return ((uint64_t)hi << 16) | lo; }
static inline uint64_t cc11(uint8_t hi, uint8_t lo) { return ((uint64_t)hi << 8) | lo; }
static inline uint64_t cc31(uint32_t hi3, uint8_t lo) { return ((uint64_t)(hi3 & 0xffffffu) << 8) | lo; }
static inline uint64_t cc14(uint8_t hi, uint32_t lo) { return ((uint64_t)hi << 32) | lo; }
#define CARRY4(a, b) (((uint64_t)(a) + (uint64_t)(b)) > 0xffffffffULL)
#define CARRY2(a, b) (((uint32_t)(a) + (uint32_t)(b)) > 0xffffULL)

uint64_t fn_04794a(uint32_t eax, uint32_t edx, uint32_t ebx, uint32_t ecx, uint32_t esi)
{
    uint32_t uVar1, uVar2, bVar3, uVar5, uVar7, uVar10, uVar11, uVar12, uVar13, uVar14;
    uint16_t sVar4, sVar6, uVar9u;
    uint8_t bVar9;
    uint32_t uVar14i;
    int32_t iVar8;
    int bVar15, bVar16;

    // Block A: EAX==0 && EDX==0
    if (eax == 0 && edx == 0) {
        uint16_t u5 = (uint16_t)esi;
        if ((uint16_t)(u5 * 2) == 0) {
            return cc44(ecx, ebx);   // {EBX, ECX}
        }
        // else: ADD SI,SI + RCR SI,1 is a no-op; ESI unchanged.
    }
    if (ecx == 0 && ebx == 0 && ((esi & 0x7fff0000) == 0)) {
        return cc44(edx, eax);       // {EAX, EDX}
    }
    uVar7 = (uint32_t)((int32_t)esi >> 16) & 0x80007fff;
    sVar4 = (int16_t)(uVar7 >> 16) + (int16_t)((((int32_t)(int16_t)esi) >> 15) & 0x8000);
    uVar14 = (uint32_t)((((int32_t)((uint32_t)(esi << 16) | (esi >> 16))) >> 16) & 0x7fff);
    uVar5 = (uint16_t)((uint32_t)((int32_t)esi >> 16)) & 0x7fff;
    uVar13 = (uint16_t)uVar14;
    sVar6 = (int16_t)((uint16_t)uVar5 - (uint16_t)uVar13);
    iVar8 = (int32_t)cc22((uint16_t)sVar4, (uint16_t)sVar6);
    uVar10 = eax;
    uVar11 = edx;
    if (sVar6 != 0) {
        if ((uint16_t)uVar5 < (uint16_t)uVar13) {
            iVar8 = (int32_t)cc22((uint16_t)sVar4, (uint16_t)(-(int16_t)sVar6));
            uVar10 = ebx;
            uVar11 = ecx;
            ebx = eax;
            uVar7 = uVar14;
            ecx = edx;
        }
        if (0x40 < (uint16_t)(uint32_t)iVar8) {
            return cc44(ecx, ebx);
        }
    }
    uVar5 = (uint16_t)(uint32_t)iVar8 & 0xff;
    if (iVar8 < 0) {
        uVar5 = (uint16_t)cc11(0xff, (uint8_t)(uint8_t)(int8_t)iVar8);
        bVar15 = (ebx != 0) ? 1 : 0;
        ebx = -ebx;
        ecx = -(uint32_t)bVar15 - ecx;
    }
    uVar13 = (uint16_t)uVar7;
    uVar14 = 0;
    bVar3 = (uint8_t)uVar5;
    if (bVar3 != 0) {
        if (0x1f < bVar3) {
            uVar14 = (uint32_t)((uVar10 != 0) ? 1 : 0);
            uVar10 = uVar11;
            if (bVar3 == 0x40) {
                uVar14 = uVar14 | uVar11;
                uVar10 = 0;
            }
            uVar11 = 0;
        }
        uVar14 = uVar14 | (0U >> (bVar3 & 0x1f)) | (uVar10 << (0x20 - (bVar3 & 0x1f)));
        uVar10 = (uVar10 >> (bVar3 & 0x1f)) | (uVar11 << (0x20 - (bVar3 & 0x1f)));
        uVar11 = (uVar11 >> (bVar3 & 0x1f)) | (0 << (0x20 - (bVar3 & 0x1f)));
    }
    uVar2 = uVar10 + ebx;
    uVar12 = uVar11 + ecx + (uint32_t)CARRY4(uVar10, ebx);
    bVar9 = (uint8_t)(((int8_t)((uVar5 >> 8) & 0xff)) +
            (CARRY4(uVar11, ecx) || CARRY4(uVar11 + ecx, (uint32_t)CARRY4(uVar10, ebx))));
    if ((int8_t)bVar9 < 0) {
        if (bVar3 == 0x40) {
            uVar10 = (uint32_t)((uVar14 & 0x7fffffff) != 0);
            bVar15 = CARRY4(uVar2, uVar10);
            uVar2 = uVar2 + uVar10;
            uVar12 = uVar12 + (uint32_t)bVar15;
        }
        bVar15 = (uVar2 != 0) ? 1 : 0;
        uVar2 = -uVar2;
        uVar12 = -(uint32_t)bVar15 - uVar12;
        bVar9 = 0;
    }
    if (((uint32_t)cc31((uint32_t)((int32_t)uVar2 >> 8) & 0xffffff, (uint8_t)((uint8_t)uVar2 | bVar9)) == 0 && uVar12 == 0) ||
        ((uVar7 = uVar7 & 0xffff), uVar13 == 0))
        goto lab_00047a9f;
    uVar10 = uVar2;
    if (bVar9 == 0) {
        bVar15 = (int32_t)uVar14 < 0;
        uVar14 = (uVar14 & 0x7fffffff) | ((uint32_t)bVar15 << 0x1f);
        do {
            sVar6 = (int16_t)((uint16_t)uVar7) - 1;
            uVar7 = (uint32_t)(uint16_t)sVar6;
            if (sVar6 == 0) goto lab_00047a9f;
            uVar10 = uVar2 * 2 + (uint32_t)bVar15;
            uVar11 = (uint32_t)(CARRY4(uVar2, uVar2) || CARRY4(uVar2 * 2, (uint32_t)bVar15));
            bVar16 = CARRY4(uVar12, uVar12);
            uVar1 = uVar12 * 2;
            bVar15 = bVar16 || CARRY4(uVar1, uVar11);
            uVar12 = uVar1 + uVar11;
            uVar2 = uVar10;
        } while (!bVar16 && !CARRY4(uVar1, uVar11));
    }
    if (uVar13 != 0x7ffe) {
        uVar11 = (uint32_t)(((uint64_t)uVar12 | 0x100000000ULL) >> 1);
        uVar2 = (uint32_t)(cc14((uint8_t)((uVar12 & 1) != 0), uVar10) >> 1);
        uVar12 = uVar11;
        if ((uVar10 & 1) == 0) goto lab_00047a9f;
        bVar15 = CARRY4(uVar14, uVar14);
        if ((uVar14 & 0x7fffffff) == 0) {
            bVar15 = (uVar10 & 2) != 0;
            uVar2 = (uVar2 & 0xfffffffe) | (uint32_t)bVar15;
        }
        bVar16 = CARRY4(uVar2, (uint32_t)bVar15);
        uVar2 = uVar2 + (uint32_t)bVar15;
        uVar14 = (uint32_t)bVar16;
        uVar10 = uVar11 + uVar14;
        uVar12 = uVar10;
        if (!CARRY4(uVar11, uVar14)) goto lab_00047a9f;
        uVar12 = (uint32_t)(cc14(1, uVar10) >> 1);
        uVar2 = (uint32_t)(cc14((uint8_t)((uVar10 & 1) != 0), uVar2) >> 1);
        if (uVar13 != 0x7ffd) goto lab_00047a9f;
    }
    uVar2 = 0;
    uVar12 = 0x80000000;
lab_00047a9f:
    return cc44(uVar12, uVar2);
}
