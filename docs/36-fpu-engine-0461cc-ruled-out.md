
## Closing evidence: neither address appears anywhere in the image bytes

Beyond the absent call/xref edges, a raw scan of `NIGHT.EXE` for any little-endian encoding of the two
addresses finds nothing at all:

```
imm 0x0461cc hits: [] count 0      # pattern cc 61 04
imm 0x0473bf hits: [] count 0      # pattern bf 73 04
imm 0x046208 hits: [] count 0      # pattern 08 62 04 (interior, would catch a shifted table entry)
```

So no instruction, table, or relocation anywhere in the file carries a pointer to either region. A region
that nothing in the binary can name has no caller by construction: the port cannot be missing behaviour by
not implementing it, which is what the `[-]` marking records.
