# CD / entry-chain unlock plan (working notes)

Generated from analysis/callgraph.tsv + the STATUS.md tier tables (520/528 done).

| root | reachable | pending closures |
|---|---|---|
| 0x010927 | 51 | none |
| 0x010c3e | 87 | 0x010c3e (126 B) |
| 0x010dde | 48 | none |
| 0x010b1d | 48 | none |
| 0x3af18 | 1 | 0x3af18 (0 B) |
| 0x445c9 | 1 | 0x445c9 (0 B) |

Read: the CD/MSCDEX drive library itself is already ported (0x010927 / 0x010dde / 0x010b1d closures are clean);
what is left in that chain is 0x010c3e (the CD-readiness wait loop, which needs the ISO directory model in the
sys CD backend: it waits for DAT_00053c60 == 0xf and the NIGHT.RES entry length 0xc51) and then the entry chain
0x03af18 -> 0x0445c9.
