/* byod.js -- bring your own data: a game-data file the visitor downloaded, served to the port as if it
 * were on our server.  docs/464.
 *
 * WHAT THIS IS
 * ------------
 * The game's own data, brought by the visitor instead of hosted by us: the browser downloads a game-data
 * file, hands it to the page, and the page reads it locally -- so the host serves code and no game bytes.
 * A local File is no worse than a CDN for this: `file.slice()` is random access for free, so the zip is
 * indexed in place (End Of Central Directory, then the central directory) and one member is pulled out of
 * it without touching the rest.
 *
 * WHAT THE FILE MAY BE
 * --------------------
 * Either one zip, or the files picked together (`multiple`, so a cue and its image go in together).  Names
 * inside are the visitor's own: a cue states which image it describes, and that name is what is looked for.
 * The shapes, in any combination:
 *
 *   NIGHT.RES            the resource pack.  Read out of the file into the in-memory filesystem, CRC-32
 *                        checked against the zip's own central directory before a byte of it is trusted,
 *                        and reported against 0xf578a854 -- the NIGHT.RES this port was verified against.
 *                        That matters: one known repack of this game ships a NIGHT.RES that differs from
 *                        ours by 162 bytes from offset 1657 onward, which would move every resource offset
 *                        the port's analysis rests on.  A picked file states no CRC, so there the header is
 *                        checked against the container this port was built for instead.
 *   any .CUE +           a raw 2352-byte-frame disc image: sliced into per-track byte ranges, no copy, and
 *   its .IMG/.BIN/.ISO   the cue used verbatim, so the game sees the disc the file describes.  The rip's
 *                        TG_NIGHT.CUE/TG_NIGHT.IMG pair is one such pair, and it carries no NIGHT.RES: the
 *                        resource comes out of the disc's installer payload instead (src/cd/cd_res.c).
 *   a .CUE with no cue,  the port's own table of contents supplies the track geometry -- offered only for an
 *   or a bare .IMG       image the size of this disc, because guessing an image's geometry is worse than
 *                        asking for its cue.
 *   SOUND.RES +          a fan repack's cue/bin pair -- SOUND.RES is a plain CUE sheet that names MUSIC.RES
 *   MUSIC.RES            as its FILE.  Also 2352-byte frames, but the image starts at the disc's track 02
 *                        and its own cue's numbers are wrong for two tracks.  See MUSIC_ANCHORS.
 *
 * Tracks reach `cd_iso` by number through wasm_disc.c's `nklocal:` byte source, so the game's LBA
 * arithmetic and the CUE the port writes are unchanged: track 07 is still "seek to sector 0 of track 07 and
 * read", it is just answered by a slice of a local file.  Nothing above that seam knows a zip was involved.
 */
(() => {
'use strict';

/* ---- crc32: the zip's own check, so a half-downloaded or re-packed file is caught before the game ---- */
const CRC_TABLE = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = (c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1);
    t[n] = c >>> 0;
  }
  return t;
})();
function crc32(buf, seed) {
  let c = (seed === undefined ? 0 : seed) ^ 0xffffffff;
  for (let i = 0; i < buf.length; i++) c = CRC_TABLE[(c ^ buf[i]) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

/* ---- the zip, read in place ----------------------------------------------------------------
 * Two reads and a bit of arithmetic.  The End Of Central Directory sits within the last 64 KiB (a comment
 * may push it back that far); it gives the central directory's offset and size, and the central directory
 * holds every member's name, local-header offset and CRC.  Local headers are read lazily, per member,
 * because their name/extra fields vary and only their total length is needed to find the payload.
 */
const bytesOf = async (file, from, to) =>
  new Uint8Array(await file.slice(from, Math.min(to, file.size)).arrayBuffer());
const u16 = (b, o) => b[o] | (b[o + 1] << 8);
const u32 = (b, o) => (b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | (b[o + 3] << 24)) >>> 0;

async function zipOpen(file) {
  const tailLen = Math.min(65557, file.size);
  const tail = await bytesOf(file, file.size - tailLen, file.size);
  let eocd = -1;
  for (let i = tail.length - 22; i >= 0; i--) if (u32(tail, i) === 0x06054b50) { eocd = i; break; }
  if (eocd < 0) throw new Error(file.name + ' is not a zip this reads (no end-of-central-directory)');
  const count = u16(tail, eocd + 10);
  const cdSize = u32(tail, eocd + 12), cdOff = u32(tail, eocd + 16);
  if (count === 0xffff || cdOff === 0xffffffff)
    throw new Error('zip64 archives are not read here -- unpack the archive and pick the file itself');
  const cd = await bytesOf(file, cdOff, cdOff + cdSize);
  const members = [];
  for (let p = 0, i = 0; i < count && p + 46 <= cd.length; i++) {
    if (u32(cd, p) !== 0x02014b50) break;
    const nlen = u16(cd, p + 28), elen = u16(cd, p + 30), clen = u16(cd, p + 32);
    const name = new TextDecoder('utf-8', { fatal: false }).decode(cd.subarray(p + 46, p + 46 + nlen));
    members.push({ zip: null, name, base: name.split('/').pop().toUpperCase(), dir: name.endsWith('/', 0),
                   method: u16(cd, p + 10), crc: u32(cd, p + 16), csize: u32(cd, p + 20),
                   usize: u32(cd, p + 24), lho: u32(cd, p + 42) });
    const m = members[members.length - 1];
    m.dir = name.endsWith('/');
    p += 46 + nlen + elen + clen;
  }
  const cont = { kind: 'zip', file, members,
                 byBase: (b) => members.find((x) => !x.dir && x.base === b) };
  for (const m of members) { m.cont = cont; m.src = file; }
  return cont;
}

/* The same container from files picked instead of unpacked: each is its own byte source and its own data
 * area, so there is no header to skip -- and no CRC to check, since nothing packed them.  A visitor who
 * downloaded TG_NIGHT.CUE and TG_NIGHT.IMG side by side should not have to put them back in a zip first. */
async function filesOpen(files) {
  const cont = { kind: 'files', file: files[0], members: [] };
  for (const f of files)
    cont.members.push({ cont, name: f.name, base: f.name.split('/').pop().toUpperCase(), dir: false,
                        method: 0, crc: 0, crcKnown: false, csize: f.size, usize: f.size, src: f, off: 0 });
  cont.byBase = (b) => cont.members.find((m) => !m.dir && m.base === b);
  return cont;
}

/* Where a member's bytes begin: byte 0 for a picked file, past its local header for a zip member.  Kept on
 * the member, because the disc reader asks for it on every seek. */
async function memberDataStart(m) {
  if (m.off !== undefined) return m.off;
  const h = await bytesOf(m.src, m.lho, m.lho + 30);
  if (u32(h, 0) !== 0x04034b50) throw new Error(m.name + ': bad local header');
  m.off = m.lho + 30 + u16(h, 26) + u16(h, 28);
  return m.off;
}

const INFLATE_OK = typeof DecompressionStream !== 'undefined';

/* A member read whole, deflating it if it is a deflate stream.  Checked against the zip's CRC-32 before it
 * is returned: NIGHT.RES with one wrong byte is 51 MB of wrong resource offsets. */
async function memberBytes(m, onProgress) {
  const from = await memberDataStart(m);
  const raw = await bytesOf(m.src, from, from + m.csize);
  let out;
  if (m.method === 0) out = raw;
  else if (m.method === 8) {
    if (!INFLATE_OK) throw new Error('this browser cannot inflate (no DecompressionStream)');
    const ds = new DecompressionStream('deflate-raw');
    out = new Uint8Array(await new Response(new Blob([raw]).stream().pipeThrough(ds)).arrayBuffer());
  } else throw new Error(m.name + ': zip compression method ' + m.method + ' is not read here');
  if (out.length !== m.usize)
    throw new Error(m.name + ': read ' + out.length + ' bytes, the container says ' + m.usize);
  if (!m.crcKnown) { if (onProgress) onProgress(1); return out; }   /* a picked file states no CRC */
  const c = crc32(out);
  if (c !== m.crc)
    throw new Error(m.name + ': CRC-32 ' + c.toString(16) + ', the zip says ' + m.crc.toString(16));
  if (onProgress) onProgress(1);
  return out;
}

/* The same thing, streamed into the Origin Private File System.  Deflate has no random access, so a
 * compressed member is either decompressed in one pass or not read at all -- and 546 MB must not be held
 * in a Uint8Array, so the chunks go straight to a file the disc reader can slice afterwards. */
async function memberToOpfs(m, name, onProgress) {
  const dir = await opfsDir();
  const fh = await dir.getFileHandle(name, { create: true });
  const writable = await fh.createWritable();          /* truncates: this file is ours, this run's */
  const from = await memberDataStart(m);
  const src = m.src.slice(from, from + m.csize).stream();
  const chain = m.method === 0 ? src : src.pipeThrough(new DecompressionStream('deflate-raw'));
  const reader = chain.getReader();
  let crc = 0, got = 0;
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    const chunk = value instanceof Uint8Array ? value : new Uint8Array(value);
    crc = crc32(chunk, crc);
    got += chunk.length;
    await writable.write(chunk);
    if (onProgress) onProgress(Math.min(1, got / m.usize));
  }
  await writable.close();
  if (got !== m.usize)
    throw new Error(m.name + ': wrote ' + got + ' bytes, the container says ' + m.usize);
  if (m.crcKnown && crc !== m.crc)
    throw new Error(m.name + ': CRC-32 ' + crc.toString(16) + ', the zip says ' + m.crc.toString(16));
  return await fh.getFile();
}

async function opfsDir() {
  if (typeof navigator === 'undefined' || !navigator.storage || !navigator.storage.getDirectory)
    throw new Error('this browser has no private filesystem (need Chrome 86+ or Safari 15.2+)');
  return navigator.storage.getDirectory();
}

/* ---- cue sheets ------------------------------------------------------------------------------
 * `INDEX 01` times in MM:SS:FF are 75-frame sector numbers measured from the start of the FILE the cue
 * describes.  A track's length is the gap to the next track's index, which is exactly how the original's
 * per-track files were cut; the last track runs to the end.
 */
const frames = (msf) => { const [m, s, f] = msf.split(':').map(Number); return m * 4500 + s * 75 + f; };

function cueParse(text) {
  const tracks = [];
  let file = null, cur = null;
  for (const raw of text.split(/\r?\n/)) {
    const line = raw.trim();
    let m;
    if ((m = /^FILE\s+"([^"]+)"/i.exec(line))) { file = m[1]; continue; }
    if ((m = /^TRACK\s+(\d+)\s+(\w+)/i.exec(line))) {
      if (cur) tracks.push(cur);
      cur = { track: Number(m[1]), type: m[2].toUpperCase(), index: null, pregap: 0 };
      continue;
    }
    if ((m = /^PREGAP\s+([\d:]+)/i.exec(line)) && cur) { cur.pregap = frames(m[1]); continue; }
    if ((m = /^INDEX\s+01\s+([\d:]+)/i.exec(line)) && cur) { cur.index = frames(m[1]); continue; }
  }
  if (cur) tracks.push(cur);
  tracks.sort((a, b) => a.track - b.track);
  return { file, tracks };
}

function cueLengths(tracks, endSector) {
  return tracks.map((t, i) => {
    const next = tracks.slice(i + 1).find((x) => x.index !== null);
    return { ...t, sectors: next ? next.index - t.index : Math.max(0, endSector - t.index) };
  });
}

/* The disc this port's whole analysis rests on, as sector numbers measured from the start of the image
 * (docs/96's music points are LBAs, so the game asks for these and `cd_iso` answers them).  A file that
 * brings its own TG_NIGHT.CUE overrides it; a file that brings only a fan cue -- whose numbers describe
 * the fan image, not the disc -- gets this, since the game's music cues are the disc's LBAs.
 * Sector numbers and a track order are the disc's table of contents, not its content. */
const DISC_TOC = {
  1:  { type: 'MODE1/2352', index: 0 },
  2:  { type: 'AUDIO', index: 37112,  pregap: 150 },   /* 08:14:62 -- the 2-second gap is not in the image */
  3:  { type: 'AUDIO', index: 40265 },
  4:  { type: 'AUDIO', index: 59904 },
  5:  { type: 'AUDIO', index: 77031 },
  6:  { type: 'AUDIO', index: 96388 },
  7:  { type: 'AUDIO', index: 114678 },
  8:  { type: 'AUDIO', index: 136158 },
  9:  { type: 'AUDIO', index: 160997 },
  10: { type: 'AUDIO', index: 184970 },
  11: { type: 'AUDIO', index: 212667 },
  12: { type: 'AUDIO', index: 222996 },
  13: { type: 'AUDIO', index: 240852 },
  14: { type: 'AUDIO', index: 258720 },
  15: { type: 'AUDIO', index: 268471 },
};
const msf = (s) => [Math.floor(s / 4500), Math.floor(s / 75) % 60, s % 75]
  .map((x) => String(x).padStart(2, '0')).join(':');

function discCue() {
  let t = 'FILE "TG_NIGHT.IMG" BINARY\r\n';
  for (const n of Object.keys(DISC_TOC).map(Number)) {
    const e = DISC_TOC[n];
    t += `  TRACK ${String(n).padStart(2, '0')} ${e.type}\r\n`;
    if (e.pregap) t += `    PREGAP ${msf(e.pregap)}\r\n`;
    t += `    INDEX 01 ${msf(e.index)}\r\n`;
  }
  return t;
}

/* ---- MUSIC.RES: where each track's audio actually is ----------------------------------------
 * The fan repack's SOUND.RES states an INDEX for every track, and those numbers are right for 12 of the
 * 14 audio tracks and wrong for two, measured byte-for-byte against TG_NIGHT.IMG (docs/464):
 *
 *   every track    the audio sits 560 bytes BEFORE where the cue says -- 140 sample frames early
 *   tracks 08, 09  and 4,455,768 bytes (25.26 s) AFTER it: the image carries 25.26 s this disc does not
 *                  have between tracks 07 and 08, and drops the same amount between tracks 09 and 10
 *
 * Taking the cue at its word would start track 08 with 25 seconds of foreign music and cut its ending off.
 * The numbers below are the offsets the audio is really at, measured once against the exact bytes of that
 * item -- hence the CRC guard: they are used only for a MUSIC.RES whose CRC-32 is the one measured, and
 * any other one is served by its own cue as written.
 */
const MUSIC_CRC = 0x36c386de;
const MUSIC_SHIFT = -560;
const MUSIC_EXTRA = { 8: 4455768, 9: 4455768 };

async function measureCrc(m, onProgress) {
  /* A picked file states no CRC -- and the fan corrections are guarded by one, so an unzipped repack would
   * otherwise be sliced by the wrong numbers: 560 bytes late on every track, and 25.26 s late on two of
   * them.  One pass over the image, 8 MB at a time; the same choice scripts/night-extractor makes for the
   * same reason (its `measure_crc`).  The pass is not free (~550 MB), but it happens once per file, on the
   * visitor's own disk, and it is the difference between their music and their music starting in the wrong
   * place.  Only ever a picked file: see the caller for why a compressed zip member must not be hashed. */
  if (m.crcKnown || (m.cont && m.cont.kind !== 'files')) return m.crc;
  const CH = 8 << 20;
  let crc, pos = 0;
  for (;;) {
    const buf = await bytesOf(m.src, m.off + pos, m.off + Math.min(pos + CH, m.usize));
    if (!buf.length) break;
    crc = crc === undefined ? crc32(buf) : crc32(buf, crc);
    pos += buf.length;
    if (onProgress) onProgress(pos / m.usize);
    if (pos >= m.usize) break;
  }
  if (crc === undefined) crc = 0;
  m.crc = crc; m.crcKnown = true; m.crcMeasured = true;
  return crc;
}

/* ---- the local byte source --------------------------------------------------------------------
 * wasm_disc.c answers `<base>/trackNN.bin` with a Range fetch.  `nklocal:` asks the same question and is
 * answered by a slice of a local file: { file, off, len }, keyed by the last path segment.
 */
const LOCAL = (globalThis.__nkLocal = globalThis.__nkLocal || { map: new Map() });
const trackKey = (t) => 'track' + String(t).padStart(2, '0') + '.bin';
const serveTrack = (t, entry) => LOCAL.map.set(trackKey(t), entry);
const clearLocal = () => LOCAL.map.clear();

const SECTOR = 2352;
const RES_CRC = 0xf578a854;      /* NIGHT.RES as this port was verified against */

/* Where each audio track's bytes are in an image, given the cue that claims to describe it.  Returns
 * [{ track, sectors, off, len }] -- offsets into the image, not into the zip -- so the arithmetic that the
 * MUSIC.RES corrections live in is one function, testable without a browser. */
/* Slice positions for the image, given the cue that describes it.  `fan` comes from probeImage -- whether
 * the image carries the disc's volume -- because a repack names its files whatever it likes and even copies
 * the disc's cue line for line, so only the bytes say what an image is: a fan rip begins at the disc's
 * track 02, its whole thing sits 560 bytes before where its cue says, and two tracks are further out of
 * step again.  The corrections are applied only to the bytes they were measured against. */
function planTracks(img, cueText, fan) {
  const parsed = cueParse(cueText);
  fan = !!fan;
  const shift = fan ? (img.crc === MUSIC_CRC ? MUSIC_SHIFT : 0) : 0;
  const endSector = Math.floor((img.usize || img.size) / SECTOR);
  const out = [];
  for (const t of cueLengths(parsed.tracks.filter((x) => x.index !== null), endSector)) {
    if (t.type !== 'AUDIO' || t.sectors <= 0) continue;
    const off = t.index * SECTOR + shift + (fan ? (MUSIC_EXTRA[t.track] || 0) : 0);
    if (off < 0) continue;
    out.push({ track: t.track, sectors: t.sectors, off, len: t.sectors * SECTOR });
  }
  /* A shifted start leaves a track's tail standing on the next track's first bytes (docs/466): the image
   * puts 25.26 s this disc does not have before tracks 08 and 09 and drops the same amount between 09 and
   * 10, so track 09's own length ends inside track 10.  Left alone that plays the wrong tune for 25 s; a
   * truncated track is the failure that is merely wrong.  The image's end bounds the last track of all --
   * `stage` used to skip a track that ran past it, which lost its music entirely. */
  let limit = endSector * SECTOR;
  for (let i = out.length - 1; i >= 0; i--) {
    const tr = out[i];
    if (tr.off + tr.len > limit) {
      tr.sectors = Math.max(0, Math.floor((limit - tr.off) / SECTOR));
      tr.len = tr.sectors * SECTOR;
      tr.clipped = true;
    }
    limit = tr.off;
  }
  return { fan, corrected: fan && img.crc === MUSIC_CRC, tracks: out, clipped: out.some((x) => x.clipped),
           cueAsWritten: !fan || img.crc !== MUSIC_CRC };
}

/* ---- the installer's payload, out of the original disc image ------------------------------------
 * The original CUE/IMG pair carries no NIGHT.RES.  What it carries is the installer's payload: one
 * ISO9660 file, `NIGHT/NIGHT.DAT`, whose container holds the installed files under the lower-case names
 * SETUP.EXE would have written them in.  Reading it here, instead of asking the visitor to run something
 * first, is the same job `scripts/night-extractor` does in Python (docs/455, docs/02-container.md), and
 * the byte geometry is the whole difficulty: a MODE1/2352 frame carries 2,048 bytes of user data behind a
 * 16-byte header, so mapping a file's bytes linearly onto the raw image reads everything after the first
 * sector 16 bytes early per sector travelled -- the right size, and wrong bytes. */
const UD_OFF = 16, UD_LEN = 2048;

const u32be = (b, o) => ((b[o] << 24) | (b[o + 1] << 16) | (b[o + 2] << 8) | b[o + 3]) >>> 0;

/* A MODE1/2352 file's user data with ordinary seek/read semantics over a byte source: the file starts at
 * sector `first + lba` of the image, and its byte N lives at (first + lba + N/2048)*2352 + 16 + N%2048.
 * Crossing that boundary in one place is what keeps the mistake out of every caller -- the mistake being a
 * reader that returns the right number of bytes from the wrong place, which is how the first version of
 * scripts/extract_disc.py (docs/455) produced a NIGHT.RES of exactly the right size and wrong contents. */
function isoStream(src, base, first, lba) {
  async function user(sec, n) {
    const from = base + (first + lba + sec) * SECTOR;
    const raw = new Uint8Array(await src.slice(from, from + n * SECTOR).arrayBuffer());
    const out = new Uint8Array(n * UD_LEN);
    for (let i = 0; i < n; i++)
      out.set(raw.subarray(i * SECTOR + UD_OFF, i * SECTOR + UD_OFF + UD_LEN), i * UD_LEN);
    return out;
  }
  return { user, async read(from, len) {
    const out = new Uint8Array(len);
    for (let pos = from, o = 0; o < len;) {
      const sec = Math.floor(pos / UD_LEN), rem = pos % UD_LEN;
      const n = Math.min(64, Math.floor((rem + len - o + UD_LEN - 1) / UD_LEN));
      const d = await user(sec, n);
      const take = Math.min(len - o, d.length - rem);
      if (take <= 0) break;
      out.set(d.subarray(rem, rem + take), o);
      o += take; pos += take;
    }
    return out;
  } };
}

/* The root directory record, found by the record's own shape and not by a remembered table: TG_NIGHT's
 * volume descriptor carries "CD001" one byte earlier than the book says, so offsets copied from a table
 * would read the wrong dwords and still return bytes.  A shape that cannot be a record refuses. */
function rootRecord(d, sectors) {
  for (let o = 0; o < 300 && o + 34 <= d.length; o++) {
    const L = d[o];
    if (L < 33 || L > 54) continue;
    const lba = u32(d, o + 2);
    const size = u32(d, o + 10);
    if (lba !== u32be(d, o + 6) || size !== u32be(d, o + 14)) continue;
    if (lba <= 0 || lba >= sectors || size <= 0 || size > UD_LEN * 64) continue;
    if (d[o + 32] === L - 33) return { lba, size };
  }
  return null;
}

/* One directory extent's records, stopping at the first record that cannot be one -- so a directory read
 * that runs into file data returns what it recognised instead of inventing entries. */
function dirRecords(data) {
  const out = [];
  for (let p = 0; p + 34 <= data.length;) {
    const L = data[p];
    if (L === 0) { p += 1; continue; }
    if (p + L > data.length || L < 33) break;
    const e = data.subarray(p, p + L);
    if (33 + e[32] > L) break;
    out.push({ lba: u32(e, 2), size: u32(e, 10), dir: (e[25] & 2) !== 0,
               name: latin1(e.subarray(33, 33 + e[32])).split(';')[0] });
    p += (L + 1) & ~1;
  }
  return out;
}

/* Breadth-first, case-insensitive, `.`/`..` skipped by their identifier byte and every directory extent
 * visited once -- without both guards a name that is not on the disc re-enters the root through ".." and
 * reads it 2^depth times; the Python reference hung for four minutes that way before it was interrupted. */
/* Is there an ISO9660 volume where the cue says the data track is?  That is the test that tells this disc's
 * image from a fan rip's, and it needs no name of any kind: the fan's MUSIC.RES starts where the disc's
 * audio does, so its first sector is a frame of music and nothing that could be a volume header. */
async function isoVolume(t, sectors) {
  for (const sec of [16, 17]) { const r = rootRecord(await t.user(sec, 1), sectors); if (r) return r; }
  return null;
}

async function isoFind(t, sectors, name) {
  const want = name.toUpperCase().replace(/\//g, '');
  const r = await isoVolume(t, sectors);
  if (!r) return null;
  const queue = [[r.lba, r.size]], seen = new Set();
  while (queue.length) {
    const [lba, size] = queue.shift();
    if (seen.has(lba) || lba >= sectors || size > UD_LEN * 64) continue;
    seen.add(lba);
    for (const e of dirRecords(await t.user(lba, Math.ceil(size / UD_LEN)))) {
      if (e.dir) { if (e.name !== '\0' && e.name !== '\x01') queue.push([e.lba, e.size]); continue; }
      if (e.name.toUpperCase().replace(/\//g, '') === want) return { lba: e.lba, size: e.size, name: e.name };
    }
  }
  return null;
}

/* The container's 20-byte-entry index, de-obfuscated the way the loader fn_0366a4 does it (docs/02): a
 * reverse XOR chain over the whole block -- b[i] ^= b[i-1] walking backwards -- then b[0] ^= 0x20. */
function containerDir(raw) {
  const count = u32(raw, 0);
  const b = raw.slice(4, 4 + count * 20);
  if (b.length !== count * 20) return [];
  for (let i = b.length - 1; i > 0; i--) b[i] ^= b[i - 1];
  b[0] ^= 0x20;
  const out = [];
  for (let i = 0; i < count; i++) {
    const e = b.subarray(i * 20, i * 20 + 20);
    let end = 0; while (end < 12 && e[end] !== 0) end++;
    out.push({ name: latin1(e.subarray(0, end)), off: u32(e, 16), size: u32(e, 12) });
  }
  return out;
}

const latin1 = (b) => String.fromCharCode(...b);

/* What an image turns out to be, given the cue that describes it: whether its data track carries an ISO9660
 * volume at all, and if so where NIGHT.RES sits inside the installer's payload -- the data track's geometry
 * from the cue, the payload file from the volume tree, the slot from the container's index.  Cheap, a dozen
 * frames, so `inspect` can call it before the press and say what the file will be asked to do.  It also
 * settles fan vs disc, which no name does -- and it is not the whole test: measured against TG_NIGHT.IMG,
 * the fan repack's MUSIC.RES begins with the disc's 174 header sectors, so its volume descriptors are there
 * and read correctly; what it does not carry is the disc's files (docs/466). */
async function probeImage(src, base, cueText, imgBytes) {
  const parsed = cueParse(cueText);
  const all = cueLengths(parsed.tracks.filter((x) => x.index !== null), Math.floor(imgBytes / SECTOR));
  const d = all.find((x) => x.type !== 'AUDIO' && x.sectors > 32);
  if (!d) return { volume: false, res: null };
  const t = isoStream(src, base, d.index, 0);
  if (!(await isoVolume(t, d.sectors))) return { volume: false, res: null };
  const hit = await isoFind(t, d.sectors, 'NIGHT/NIGHT.DAT') || await isoFind(t, d.sectors, 'NIGHT.DAT');
  if (!hit) return { volume: true, res: null };
  /* The index is read from a stream based at the payload file's own extent: its container's offsets are
   * relative to that, and reading them from the volume's first sector is the one mistake that gives back
   * the right size and the wrong bytes. */
  const dt = isoStream(src, base, d.index, hit.lba);
  const head = await dt.read(0, 4);
  const count = u32(head, 0);
  if (count === 0 || count > 64) return { volume: true, res: null };
  const ents = containerDir(await dt.read(0, 4 + count * 20));
  if (!ents.length || ents[0].off !== 4 + count * 20) return { volume: true, res: null };   /* the index's own consistency test */
  const e = ents.find((x) => x.name.toUpperCase() === 'NIGHT.RES');
  if (!e) return { volume: true, res: null };
  return { volume: true, res: { lba: hit.lba, first: d.index, name: hit.name, count,
                                entries: ents.map((x) => x.name), resName: e.name, off: e.off, size: e.size } };
}

/* Pull NIGHT.RES out of it.  51 MB of user data gathered 128 KB of frames at a time. */
async function readResFromImage(src, base, found, onProgress) {
  const t = isoStream(src, base, found.first, found.lba);
  const out = new Uint8Array(found.size);
  const step = 64 * UD_LEN;
  for (let off = 0; off < found.size;) {
    const len = Math.min(step, found.size - off);
    out.set(await t.read(found.off + off, len), off);
    off += len;
    if (onProgress) onProgress(off / found.size);
  }
  return out;
}

/* ---- what the visitor gave us -----------------------------------------------------------------
 * One container of either kind -- a zip indexed in place, or the files picked together -- and one set of
 * rules for what is in it.  A member carries its own byte source and its own offset, which is the only
 * difference between the two kinds: a zip member's bytes begin past its local header, a picked file's begin
 * at byte 0.
 *
 * Names are decided on the cue's terms, not by string-matching ours: a cue states which image it describes,
 * so `Eojjeonji Disc.bin` is as good a name for the disc as `TG_NIGHT.IMG`, and the names the repacks chose
 * (`SOUND.RES`, `MUSIC.RES`) are aliases in the rule rather than the rule. */
const isCue = (m) => m.base.endsWith('.CUE') || m.base === 'SOUND.RES';
const isImg = (m) => m.base.endsWith('.IMG') || m.base.endsWith('.BIN') || m.base.endsWith('.ISO') ||
                     m.base === 'MUSIC.RES';
const isRes = (m) => m.base === 'NIGHT.RES';
const DISC_SECTORS = 269303;              /* this disc's own length, from the port's table of contents */

function toFiles(input) {
  const list = (typeof FileList !== 'undefined' && input instanceof FileList) ? [...input]
             : Array.isArray(input) ? input : [input];
  return list.filter((f) => f && f.size > 0);
}

async function inspect(input) {
  const files = toFiles(input);
  if (!files.length) throw new Error('no file was picked');
  const zip = files.find((f) => f.name.toLowerCase().endsWith('.zip'));
  const cont = zip ? await zipOpen(zip) : await filesOpen(files);
  const what = cont.kind === 'zip' ? 'zip' : 'files picked';
  const notes = [];
  if (zip && files.length > 1)
    notes.push(`${files.length - 1} other file(s) ignored: ${zip.name} is the container`);

  const live = cont.members.filter((m) => !m.dir);
  const res = live.find(isRes);
  const cues = live.filter(isCue), imgs = live.filter(isImg);
  let cue = cues.find((m) => m.base === 'TG_NIGHT.CUE') || (cues.length === 1 ? cues[0] : null);
  if (!cue && cues.length > 1)
    notes.push(`${cues.length} cue sheets and none named TG_NIGHT.CUE -- cannot choose; pick one pair`);
  let img = imgs.find((m) => m.base === 'TG_NIGHT.IMG') || imgs.find((m) => m.base === 'MUSIC.RES') ||
            (imgs.length === 1 ? imgs[0] : null);
  if (!img && imgs.length > 1)
    notes.push(`${imgs.length} images and none named TG_NIGHT.IMG -- cannot choose; pick one pair`);

  /* The cue names the image it describes, and that is the only authority on the name. */
  let text = null;
  if (cue) {
    text = await cueText(cue);
    const named = (cueParse(text).file || '').split(/[\\/]/).pop().toUpperCase();
    const hit = named && live.find((m) => m.base === named);
    if (hit && isImg(hit)) img = hit;
    else if (named && !hit) notes.push(`${cue.base} names ${named}, which is not in the ${what}`);
  }

  /* An image with no cue with it: the track geometry is this port's own (docs/96's music points are LBAs),
   * so `discCue()` supplies the numbers the visitor's file does not carry.  Offered only for an image the
   * size of this disc -- guessing an image's geometry is worse than asking for its cue. */
  const ownToc = !cue && !!img && Math.abs(img.usize / SECTOR - DISC_SECTORS) <= DISC_SECTORS * 0.02;
  if (!cue && img && !ownToc)
    notes.push(`${img.base} needs its .CUE picked with it -- the image does not say where its tracks are`);
  if (cue && !img) notes.push(`${cue.base} names no image that was picked or packed with it`);

  /* The rip carries no NIGHT.RES: the disc carries the installer's payload, and looking there is what the
   * port does natively in src/cd/cd_res.c (docs/465).  Looked for here so the gate can say yes to a file
   * that holds the game before a single byte of it has been read -- and the same look says whether this is
   * the disc's own image or a fan rip's, which is the other thing that changes what the game is given. */
  const disc = img ? await probeImage(img.src, await memberDataStart(img), ownToc ? discCue() : text,
                                      img.usize) : null;
  const resDisc = disc && disc.res ? disc.res : null;
  /* Fan or disc is answered by what the image *holds*, not by whether it opens like a disc.  Measured, not
   * assumed (docs/466): the fan repack's MUSIC.RES begins with the disc's 174 header sectors, so its volume
   * descriptors are present and read correctly -- and its files are not, because the rip cut the disc's
   * contents off after the header.  Testing the volume alone would call it a disc and slice its audio with
   * the disc's numbers, which is 560 bytes late on every track and 25.26 s late on tracks 08 and 09. */
  const fan = !!img && !(disc.volume && disc.res);
  if (img && fan && img.cont && img.cont.kind === 'files' && !img.crcMeasured) {
    /* The rule that decides *how* a fan image is sliced wants its CRC.  A zip member carries one (the
     * central directory's, over its uncompressed bytes); a picked file states none, so measure it.  Measuring
     * a zip member would hash its *compressed* bytes and call the result the image -- which is exactly the
     * wrong number, and would then be trusted as the declared one. */
    await measureCrc(img);
    notes.push(`${img.base} states no CRC (a picked file), so it was measured here: ` +
               `${img.crc.toString(16)}`);
  }
  if (!res && !resDisc && img)
    notes.push(`no NIGHT.RES in ${img.base} either -- the game has nothing to load`);
  if (img && fan && !res)
    notes.push(`${img.base} is a fan rip's image, sliced by the offsets measured for it ` +
               (disc.volume ? `(it carries the disc's volume header but not its files)` +
                             ` -- and with no NIGHT.RES in the zip either, the game has nothing to load`
                             : `(no ISO9660 volume) -- and with no NIGHT.RES in the zip either, the game has `
                             + `nothing to load`));
  return { files, file: files[0], cont, res, cue, img, resDisc, ownToc, fan, notes,
           hasRes: !!(res || resDisc), hasMusic: !!img,
           summary: [res && 'NIGHT.RES', resDisc && `NIGHT.RES inside ${img.base}`,
                     img && img.base + (ownToc ? ' (with our table of contents)' : ''),
                     cue && cue.base].filter(Boolean) };
}

async function cueText(cue) { return new TextDecoder('latin1').decode(await memberBytes(cue)); }

/* Read what the file holds and put it where the port will find it.  Returns
 * { resBytes, res, tracks, music: 'none'|'disc'|'fan', note }. */
async function stage(ins, { onRes, onMusic, log = () => {} } = {}) {
  const out = { res: 0, tracks: 0, music: 'none' };

  if (ins.res) {
    out.resBytes = await memberBytes(ins.res, onRes);
    out.res = out.resBytes.length;
    if (ins.res.crcKnown && ins.res.crc !== RES_CRC)
      log(`NIGHT.RES: CRC-32 ${ins.res.crc.toString(16)}, not the ${RES_CRC.toString(16)} this port was ` +
          `verified against -- loading it anyway, but every resource offset may be wrong`);
  }

  /* The image's byte source, which the resource may live inside: an uncompressed image is read through the
   * visitor's own file, and only a compressed one is unpacked -- into the private filesystem, named by its
   * CRC, so two different files never share a staging file and a stale one never answers for a new one. */
  let src = null, base = 0;
  if (ins.img) {
    src = ins.img.src; base = await memberDataStart(ins.img);
    if (ins.img.method !== 0) {
      log(`music: unpacking ${ins.img.base} (${(ins.img.usize / 1048576).toFixed(0)} MB) to the private ` +
          `filesystem -- deflate cannot be seeked, so this happens once per file`);
      src = await memberToOpfs(ins.img,
                               `night-${ins.img.base.toLowerCase()}-` +
                               `night-${ins.img.base.toLowerCase()}-` +
                               `${(ins.img.crc || 0).toString(16)}.img`, onMusic);
      base = 0;
    }
  }

  if (!ins.res && ins.resDisc) {
    out.resBytes = await readResFromImage(src, base, ins.resDisc, onRes);
    out.res = out.resBytes.length;
    const crc = crc32(out.resBytes);
    log(`resources: ${ins.resDisc.count} files in ${ins.resDisc.name}'s installer payload, took ` +
        `${ins.resDisc.resName} (${(out.res / 1048576).toFixed(0)} MB)`);
    if (crc !== RES_CRC)
      log(`NIGHT.RES: CRC-32 ${crc.toString(16)} out of the payload, not the ${RES_CRC.toString(16)} this ` +
          `port was verified against -- loading it anyway, but every resource offset may be wrong`);
  }

  if (ins.img) {
    const text = ins.ownToc ? discCue() : await cueText(ins.cue);
    /* A fan cue describes the fan image; the game asks in the disc's LBAs, so it is used for slicing only
     * and the port's own table of contents is what the game gets. */
    const plan = planTracks(ins.img, text, ins.fan);
    out.cue = (plan.fan || ins.ownToc) ? null : text;   /* `ownToc`: the host writes these numbers itself */
    if (plan.clipped)
      log(`music: some tracks are cut short where the next track's audio begins in ${ins.img.base} -- ` +
          `its layout is not the disc's (docs/466)`);
    if (plan.fan && !plan.corrected)
      log(`music: ${ins.img.base} is not the copy the corrections were measured against (CRC-32 ` +
          `${ins.img.crc.toString(16)}${ins.img.crcMeasured ? ', measured here' : ''}) -- using its cue as ` +
          `written`);

    let served = 0, bytes = 0;
    for (const t of plan.tracks) {
      if (base + t.off + t.len > (src.size || ins.img.usize)) continue;
      serveTrack(t.track, { file: src, off: base + t.off, len: t.len });
      served++; bytes += t.len;
    }
    out.tracks = served;
    out.musicBytes = bytes;
    out.music = served ? (plan.fan ? 'fan' : 'disc') : 'none';
  }
  return out;
}

/* ---- the way in: a picker --------------------------------------------------------------------- */
globalThis.__nkByod = { inspect, stage, zipOpen, filesOpen, memberBytes, memberToOpfs, memberDataStart,
                       planTracks, cueParse, cueLengths, discCue, crc32, clearLocal, serveTrack, LOCAL,
                       SECTOR, RES_CRC, MUSIC_CRC, MUSIC_SHIFT, MUSIC_EXTRA, msf, probeImage, readResFromImage, containerDir,
                       isoVolume,
                       isoFind, cueText, frames };
})();
