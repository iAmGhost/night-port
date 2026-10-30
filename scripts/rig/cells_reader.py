"""Framing-accurate reader for a port rig dump's cells.bin.

docs/167 §19: every ad-hoc analysis of cells.bin in this cluster read the stream as `record_bytes`-sized
records starting at byte 0.  The real framing is a 16-byte file header, then per record a 40-byte header
(t_ms, frame, audio_bytes, screen_hash, tick, n -- packed, no padding) and then the cell payload.  Drifting by
those 56 bytes turns valid data into ASCII fragments and float bits and produced a false "the port's globals are
corrupted" finding.  Any script that reads cells.bin must go through this reader.

Usage:
    from cells_reader import CellStream
    cs = CellStream('/tmp/rig/q4')
    for i in range(cs.samples):
        cs.tick(i); cs.t_ms(i); cs.val(i, 0x62d58); cs.vals(i, 0x52f08, 12)
"""
import json
import os
import struct

FILE_HDR = 16
REC_HDR = 40
REC_HDR_FMT = '<qqqQII'


class CellStream:
    def __init__(self, dump_dir):
        self.dir = dump_dir
        self.meta = json.load(open(os.path.join(dump_dir, 'meta.json')))
        self.rb = self.meta['record_bytes']
        self.stride = REC_HDR + self.rb
        data = open(os.path.join(dump_dir, 'cells.bin'), 'rb').read()
        if len(data) < FILE_HDR:
            raise ValueError('cells.bin is shorter than its file header')
        self.data = data[FILE_HDR:]
        self.samples = len(self.data) // self.stride
        self.tail = len(self.data) - self.samples * self.stride
        if self.tail:
            raise ValueError(
                f'cells.bin payload ({len(self.data)} B) is not a whole number of '
                f'{self.stride}-byte records; {self.tail} bytes left over')
        self.cells = {c['ds']: c for c in self.meta['cells']}
        self.by_name = {c['name']: c for c in self.meta['cells']}

    def _off(self, i, c):
        return i * self.stride + REC_HDR + c['offset']

    def header(self, i):
        t, frame, audio, shash, tick, n = struct.unpack_from(REC_HDR_FMT, self.data, i * self.stride)
        return dict(t_ms=t, frame=frame, audio_bytes=audio, screen_hash=shash, tick=tick, n=n)

    def tick(self, i):
        return self.header(i)['tick']

    def t_ms(self, i):
        return self.header(i)['t_ms']

    def val(self, i, ds, width=None):
        c = self.cells[ds]
        w = width or c['host_size']
        o = self._off(i, c)
        return int.from_bytes(self.data[o:o + w], 'little')

    def signed(self, i, ds, width):
        v = self.val(i, ds, width)
        return v - (1 << (8 * width)) if v >= (1 << (8 * width - 1)) else v

    def vals(self, i, ds, n=None):
        c = self.cells[ds]
        n = n or c['host_size']
        o = self._off(i, c)
        return self.data[o:o + n]

    def has(self, ds):
        return ds in self.cells

    def records(self):
        p = os.path.join(self.dir, 'records.jsonl')
        if not os.path.exists(p):
            return None
        return [json.loads(l) for l in open(p, errors='replace')]
