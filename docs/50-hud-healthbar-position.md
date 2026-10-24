
## Gauge panel, byte-level (this cycle)

`NIGHT_HUDDBG=1` now also dumps the `gage.spr` cell the gauge panel blits (its row-RLE stream and the bytes the
blit wrote), and `scripts/playtest_verify_spr.py` decodes that stream independently. Over the episode-1 route:
**34,932 compared bytes, 0 mismatches** across 82 gauge composites (the same run gives 15,037 entity-sprite
bytes, also 0 mismatches). So the panel's pixels are the shipped data, at the column/row the disassembly of
`0x13745-13789` prescribes (`dest = slot*0xaa + 0x13`, row `0x1d`), on top of the fill geometry measured above.
