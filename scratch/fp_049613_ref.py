#!/usr/bin/env python3
# Python reference for fn_049613 (0x049613, 236 insn) + its transitive callees.
# Mirrors the C port in port/src/f/f.c line-for-line. Used to generate the
# differential vectors in test_f1.c::test_fn_049613.
#
# Globals (module-level, set by the harness) mirror the C externs:
#   G1 @0x53734, G2 @0x53758 (adjacent struct dec_datetime), FLAG @*[0x537ca],
#   tz_sec (=g_dat_5377c), tz_range (=g_dat_53780). No-TZ-config tests use 0.
#
# struct dec_datetime fields (9 dwords): sec0 min4 hour8 day0c month10
#   year14 dow18 doy1c mode20.

def i32(x):
    x &= 0xFFFFFFFF
    return x - 0x100000000 if x & 0x80000000 else x

def is_leap(year, ctx):
    # C: ((year&3)==0) && ((int)year%100!=0 || (int)year%400==0), return (ctx<<32)|leap
    y = i32(year)
    leap = 1 if ((year & 3) == 0 and (y % 100 != 0 or y % 400 == 0)) else 0
    return (i32(ctx) << 32) | leap   # we only ever use the low 32 bits below

def leap_bit(yr, ctx):
    return i32(is_leap(yr & 0xFFFFFFFF, ctx) & 0xFFFFFFFF)

# --- tables (same as f.c) ---
NONLEAP = [0,31,59,90,120,151,181,212,243,273,304,334,365]   # g_nonleap_tbl[13]
LEAP    = [0,31,60,91,121,152,182,213,244,274,305,335,366]    # g_leap_tbl[13]
# 13th slot of fn_0498f6's LOCAL tables reads the out-of-table byte (0x51978/0x51998):
NL8F6 = [0,31,59,90,120,151,181,212,243,273,304,334,365]       # 0x51978 = 365
LP8F6 = [0,31,60,91,121,152,182,213,244,274,305,335,0]          # 0x51998 = 0
# raw int16 array base 0x51958 (index i -> 0x51958+2i) used by fn_049613
T = [0,0,0,0, 0,0,31,59,90,120,151,181,212,243,273,304,334,365,
     0,31,60,91,121,152,182,213,244,274,305,335,366]

# module globals (set by harness)
G1 = None; G2 = None; FLAG = 0
TZ_SEC = 0; TZ_RANGE = 0

SEC,MIN,HR,DAY,MON,YEAR,DOW,DOY,MODE = 0,1,2,3,4,5,6,7,8

def field(s, k): return i32(s[k])
def setfield(s, k, v): s[k] = i32(v)

def fxnum(d):
    # a struct dec_datetime reinterpreted as fxnum {lo,hi,exp} on the BASE pointer:
    # lo=sec@0, hi=min@4, exp=hour@8. fn_049613 passes the dec_datetime base to
    # fx_less, so these are the TIME fields (hour/min/sec), NOT year/dow/doy.
    return (field(d, HR), field(d, MIN), field(d, SEC))

def fx_less(a, b):
    ea,ha,la = fxnum(a); eb,hb,lb = fxnum(b)
    if ea < eb: return 1
    if ea > eb: return 0
    if ha < hb: return 1
    if ha > hb: return 0
    if la < lb: return 1
    return 0

def idiv7(v):
    # x86 IDIV by 7 truncates toward zero; operands here are non-negative so
    # this equals v//7.
    q = abs(v) // 7
    return q if v >= 0 else -q

def fp_0498f6(base_day, seconds, delta, out):
    sec = 0; day_total = 0
    if (i32(seconds & 0xFFFFFFFF) < 0xa8c0) and (delta > 0):
        sec = i32(seconds + 0x15180 - delta)
        day_total = i32(base_day + (sec // 0x15180) - 1)
    else:
        sec = i32(seconds - delta)
        day_total = i32(base_day + (sec // 0x15180))
    s = sec % 0x15180
    out[HR] = s // 0xe10
    rem = s % 0xe10
    out[MIN] = rem // 0x3c
    out[SEC] = rem % 0x3c
    year_est = day_total // 365
    day_in_year = day_total - year_est * 365
    if year_est != 0:
        day_in_year -= (year_est - 1) // 4
    r1 = is_leap(year_est + 0x76c, year_est)
    year_len = (r1 & 0xFFFFFFFF) + 0x16d
    year = i32(r1 >> 32)
    while day_in_year >= year_len:
        day_in_year -= year_len
        year = i32(year + 1)
    out[DOY] = day_in_year
    out[YEAR] = year
    r2 = is_leap(year + 0x76c, 0)
    tbl = LP8F6 if (r2 & 0xFFFFFFFF) else NL8F6
    m = day_in_year // 0x1f
    if (tbl[m+1] & 0xFFFF) <= (day_in_year & 0xFFFF):
        m += 1
    out[MON] = m
    out[DAY] = day_in_year - tbl[m] + 1
    out[DOW] = (day_total + 1) % 7
    return out

def fp_04574a(d):
    r_raw = d[MON] % 12
    r = r_raw; q = d[MON] // 12; year = d[YEAR]
    if year < 0xf4fb7ea1: return -1
    year = i32(year + q)
    while r < 0:
        year = i32(year - 1); r = i32(r + 12)
    if year < 0: return -1
    leap = leap_bit(year + 0x76c, r_raw)
    tbl = LEAP if leap else NONLEAP
    absbase = year * 365 + (year + 3) // 4
    EDI = i32(d[DAY] + tbl[r] + absbase - 1)
    if year != 0: EDI = i32(EDI - 1)
    total_sec = i32(d[SEC] + d[MIN]*60 + d[HR]*3600)
    while total_sec < 0:
        EDI = i32(EDI - 1); total_sec = i32(total_sec + 0x15180)
    fp_0498f6(EDI, total_sec & 0xFFFFFFFF, 0, d)
    # fn_049a35(0,0,0): no-TZ-config -> no-op
    total_sec = i32(total_sec + TZ_SEC)
    if d[MODE] < 0:
        fp_049613(d)
    if d[MODE] > 0:
        total_sec = i32(total_sec - TZ_RANGE)
    while total_sec < 0:
        EDI = i32(EDI - 1); total_sec = i32(total_sec + 0x15180)
    if EDI < 0x63de: return -1
    if EDI != 0x63de:
        total_sec = i32(total_sec + (EDI - 0x63df) * 0x15180)
        return total_sec
    g = TZ_SEC
    total_sec = i32(total_sec - 0x15180)
    if g <= 0: return -1
    if total_sec >= 0: return total_sec
    return -1

def fp_049506(d, ref_year):
    mode = d[MODE]
    if mode == 1: return i32(d[DOY] - 1)
    if mode != 0: return d[DOY]
    month = d[MON]
    leap = leap_bit(ref_year + 0x76c, ref_year)
    if leap:
        days_in_month = LEAP[month+1] - LEAP[month]
    else:
        days_in_month = NONLEAP[month+1] - NONLEAP[month]
    temp = [0]*9
    temp[DAY] = 1; temp[MON] = month; temp[YEAR] = ref_year; temp[MODE] = 0
    fp_04574a(temp)
    dow_diff = (d[DOW] - temp[DOW] + 7) % 7
    day = d[DAY]
    if day == 5:
        cmpv = dow_diff + 29
        dayval = 4 if cmpv <= days_in_month else 3
    else:
        dayval = day - 1
    return i32(temp[DOY] + dow_diff + dayval * 7)

def fp_0495eb(x, y, ref_year):
    rx = fp_049506(x, ref_year); ry = fp_049506(y, ref_year)
    return 1 if rx > ry else 0

def fp_049613(d):
    # Mirrors int32_t fn_049613(struct dec_datetime *d) in f.c exactly.
    d = d[:]          # copy; C operates in place
    EDI = 0
    # 0x4961e-29: flag gate
    if FLAG == 0:
        d[MODE] = EDI
        return EDI
    # 0x4962f-5f: order the references
    cmp = fp_0495eb(G1, G2, field(d, YEAR))
    if cmp != 0:
        primary, secondary = G2, G1
    else:
        primary, secondary = G1, G2
    m = field(d, MON)
    leapbit = leap_bit(field(d, YEAR) + 0x76c, m)
    dtbl = LEAP if leapbit else NONLEAP
    daycnt = i32(dtbl[m+1] - dtbl[m])          # [ESP+0xc]
    match = 0                                  # [ESP+0x14]
    pmode = field(primary, MODE)
    if pmode != 0:                             # 0x496aa JNZ -> 0x49763
        EDX = field(primary, DOY)
        if pmode == 1:                         # 0x49766 CMP EAX,1
            if leap_bit(field(d, YEAR) + 0x76c, pmode):
                if EDX > 90: EDX = i32(EDX + 1)   # 0x51962>>16 = NONLEAP[3] = 90
            EDX = i32(EDX - 1)                 # 0x49789
        ddoy = field(d, DOY)
        if EDX <= ddoy:                        # 0x4978f JG -> skip
            EDI = 1                            # 0x49791
            if EDX == ddoy: match = 1          # 0x49796 JNZ; else 0x49798
    else:                                      # 0x496b0-6c7
        pmonth = field(primary, MON)
        if m == pmonth:                        # week-of-month match
            rem1 = idiv7(field(d, DOW) + 7 - field(primary, DOW))
            a0 = field(d, DAY) - rem1
            rem2 = idiv7(field(d, DOW) + 6 - field(primary, DOW))
            b8 = field(d, DAY) - 1 - rem2
            pd = field(primary, DAY)
            if pd == 5:                        # 0x49712
                v = daycnt - 7
                if v < a0:
                    EDI = 1
                    if v >= b8: match = 1
            else:                              # 0x4973c
                v = (pd - 1) * 7 + 1
                if v <= a0:
                    EDI = 1
                    if v > b8: match = 1
        elif m > pmonth:
            EDI = 1                            # 0x496b7
    if match != 0:                             # 0x4979c
        EDI = 1 if (fx_less(d, primary) == 0) else 0
    if EDI != 0:                               # 0x497b6 JNZ -> pass-2
        smode = field(secondary, MODE)
        ebxf = 0
        if smode != 0:                         # 0x497d2 JNZ -> 0x49861
            EDX = field(secondary, DOY)
            if smode == 1:                     # 0x49864 CMP EAX,1
                if leap_bit(field(d, YEAR) + 0x76c, smode):
                    if EDX > 90: EDX = i32(EDX + 1)
                EDX = i32(EDX - 1)
            ddoy = field(d, DOY)
            if EDX <= ddoy:                    # 0x4988d JG -> EDI unchanged
                EDI = 0
                if EDX == ddoy: ebxf = 1
        else:                                  # 0x497d8-7e6
            smonth = field(secondary, MON)
            if m == smonth:                    # pass-2 week-of-month
                rem1b = idiv7(field(d, DOW) + 7 - field(secondary, DOW))
                a4 = field(d, DAY) - rem1b
                rem2b = idiv7(field(d, DOW) + 6 - field(secondary, DOW))
                b4 = field(d, DAY) - 1 - rem2b
                EDI = 0
                sd = field(secondary, DAY)
                if sd == 5:                    # 0x49829
                    v = daycnt - 7
                    if v >= a4: EDI = 1
                    elif b4 <= v: ebxf = 1
                else:                          # 0x4984a
                    v = (sd - 1) * 7 + 1
                    if v > a4: EDI = 1
                    elif b4 < v: ebxf = 1
            elif m > smonth:
                EDI = 0
        if ebxf != 0:                          # 0x4989a
            EDI = fx_less(d, secondary)
        if cmp != 0:                           # 0x498a9
            EDI = i32(cmp - EDI)
    else:                                      # 0x497b8: EDI == 0
        if cmp == 0:
            d[MODE] = EDI
            return EDI
        EDI = cmp
    d[MODE] = EDI
    return EDI

if __name__ == "__main__":
    import sys, json
    G1 = [0,0,0,1,0,0,0,0,0]; G2 = [0,0,0,1,0,0,0,0,0]; FLAG = 0
    out = fp_049613([0,0,0,3,0,2000,0,0,-1])
    print(json.dumps({"mode": out[MODE], "ret": out[MODE]}))
