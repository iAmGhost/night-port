"""Detect STATUS 'pending' entries that are actually data (misidentified by Ghidra):
prints pending rows whose first bytes are mostly printable ASCII."""
import re, string
data = open('NIGHT/NIGHT.EXE','rb').read()
D = 0x2600
st = open('docs/12-functions/STATUS.md').read()
rows = re.findall(r'^\| \[ \] \| 0x([0-9a-f]{6}) \| (\d+) \| (\S+) \|', st, re.M)
printable = set(string.ascii_letters + string.digits + " .,:;!?()[]{}+-*/=<>&|_'\"%#@$~`^\\{}\n\r\t".encode().decode())
hits = []
for h, size, name in rows:
    va = int(h, 16); off = va - D
    if off < 0 or off + 16 > len(data): continue
    b = data[off:off+16]
    txt = b.decode('latin1')
    n = sum(1 for c in txt if c in printable)
    if n >= 14:
        hits.append((h, size, name, txt))
print("pending rows:", len(rows), " ascii-ish:", len(hits))
for h, size, name, txt in hits:
    print("  0x%s size %-5s %-24s %r" % (h, size, name, txt))
