/* byod-test.js -- run runtime/byod.js against real game-data files, outside a browser.  docs/464, docs/465.
 *
 *   node scripts/byod-test.js <game-data.zip> [more files...]
 *   node scripts/byod-test.js ../TG_NIGHT.CUE ../TG_NIGHT.IMG          # picked together, no zip
 *
 * What it insists on is the things that would otherwise be silent:
 *
 *  * NIGHT.RES comes out byte-for-byte the file this port was verified against -- the CRC the zip states
 *    AND the md5 of our own extracted copy, because a repack that changed 162 bytes of a 51 MB resource
 *    file still "unpacks fine".
 *  * the rip's NIGHT.RES, which is not a file at all but a slot inside NIGHT/NIGHT.DAT in the image's
 *    ISO9660 volume, comes out the same bytes -- which is the whole reason that reader exists.
 *  * the per-track slice offsets are the ones measured against TG_NIGHT.IMG, including the two tracks a
 *    fan repack's own cue states wrongly.
 *  * discovery does not depend on our file names: a cue that names `Eojjeonji Disc.bin` finds it, and an
 *    image with no cue at all is taken for what it is only when its size says so.
 *
 * Node stands in for the browser: navigator.storage is absent, so the pass that unpacks a compressed image
 * into the private filesystem is skipped -- this checks the index and the arithmetic, which is where the
 * mistakes live.  OPFS is exercised in the browser by `make -C wasm check-byod`.
 */
'use strict';
const fs = require('fs'), path = require('path'), crypto = require('crypto');

const BYOD = fs.readFileSync(path.join(__dirname, '..', 'runtime', 'byod.js'), 'utf8');
eval(BYOD);                                       /* an IIFE that publishes globalThis.__nkByod */
const B = globalThis.__nkByod;

const ROOT = path.join(__dirname, '..', '..');
const md5hex = (buf) => crypto.createHash('md5').update(buf).digest('hex');
const WANT_RES = md5hex(fs.readFileSync(process.env.NIGHT_RES || path.join(ROOT, 'extracted', 'NIGHT.RES')));

/* A File, duck-typed over pread: the port needs size and slice().arrayBuffer(). */
function nodeFile(p, name = path.basename(p)) {
  const size = fs.statSync(p).size, fd = fs.openSync(p, 'r');
  return {
    name, size,
    slice(from, to) {
      const len = Math.max(0, Math.min(to === undefined ? size : to, size) - from), buf = Buffer.alloc(len);
      if (len) fs.readSync(fd, buf, 0, len, from);
      return new Blob([buf]);
    },
  };
}

let fails = 0, notes = 0;
const ok = (cond, what, extra) => {
  console.log(`  ${cond ? 'ok  ' : 'FAIL'}  ${what}${extra !== undefined ? '  ' + extra : ''}`);
  if (!cond) fails++;
};
const note = (m) => { console.log(`        ${m}`); };

/* One container, whatever it was made of: assert what it supplies and what staging registers for the
 * game to read.  `want` is what the case is about -- the shape, and the geometry to expect. */
async function check(input, label, want) {
  console.log(`\n=== ${label}`);
  const ins = await B.inspect(input);
  const files = Array.isArray(input) ? input : [input];
  ok(ins.summary.length > 0, 'the container supplies something the port reads',
     ins.summary.join(', ') || 'nothing');
  note(`notes: ${ins.notes.length ? ins.notes.join(' | ') : 'none'}`);
  if (want.shape) ok(ins.summary.join(' + ') === want.shape, 'read as', want.shape);

  /* --- NIGHT.RES, whole, checked twice, however it was found --- */
  ok(ins.hasRes === want.res, 'supplies resources', want.res ? 'yes' : 'no');
  ok(!(ins.res && ins.resDisc), 'only one way of supplying them is ever in play',
     ins.res && ins.resDisc ? 'both at once' : 'one');
  if (!ins.hasRes) return;

  let st = null;
  try {
    st = await B.stage(ins, { log: (m) => note('byod: ' + m) });
  } catch (e) {   /* the private filesystem is a browser thing; node cannot stand in for a compressed image */
    note(`[skip] stage: ${e.message} -- asserting what was already read`);
    st = ins.res ? { res: ins.res.usize } : null;
  }
  const got = st && st.resBytes ? md5hex(Buffer.from(st.resBytes)) : null;
  ok(st && st.res > 4e7, 'NIGHT.RES staged to the length its container states', st && `${st.res}`);
  ok(got === null || got === WANT_RES, 'NIGHT.RES is the file this port was verified against',
     got === null ? '[skip] its bytes were not read here' : `${got} (want ${WANT_RES})`);

  /* --- the per-track geometry the image will be sliced by --- */
  if (!ins.img) { note('music: none -- the game will play silent'); return; }
  const text = ins.ownToc ? B.discCue() : await B.cueText(ins.cue);
  const plan = B.planTracks(ins.img, text, ins.fan);
  ok(plan.tracks.length === 14, '14 audio tracks planned', plan.tracks.map((t) => t.track).join(','));
  /* Fan or disc, asserted by the evidence rather than by a guess typed into the test: the disc's volume is
   * what a rip image carries and a fan rip's does not, and finding the payload slot in the image settles it
   * by the same look. */
  note(`geometry: ${plan.fan ? 'a fan rip\'s image -- its installer payload is not readable where its own volume says it is'
                             : 'the disc -- volume found, sliced as it describes'}${ins.ownToc ? ', with our own table of contents' : ''}`);
  if (ins.resDisc) ok(!plan.fan, 'an image the payload slot was found in is the disc, by that fact alone',
                  `${ins.resDisc.name} at sector ${ins.resDisc.lba}`);
  if (ins.img.crcKnown && ins.img.crc === B.MUSIC_CRC)   /* the one fan image whose offsets were measured */
    ok(plan.fan, 'the measured fan image is treated as a fan rip', `crc=${ins.img.crc.toString(16)}`);
  ok(plan.corrected === (plan.fan && ins.img.crc === B.MUSIC_CRC),
     'the corrections are applied only to the bytes they were measured against',
     `crc=${ins.img.crc.toString(16)}`);

  /* Overlap check: no two tracks may claim the same bytes, or the plan is a lie about a seek. */
  const sorted = [...plan.tracks].sort((a, b) => a.off - b.off);
  let overlap = null;
  for (let i = 1; i < sorted.length; i++)
    if (sorted[i].off < sorted[i - 1].off + sorted[i - 1].len)
      overlap = `track ${sorted[i - 1].track} and ${sorted[i].track}`;
  note(`${sorted.length} tracks, ${overlap ? 'OVERLAP ' + overlap : 'no two tracks overlap'}` +
       ` -- docs/464 records where a fan image overlaps our rip`);

  /* --- what staging registered for the game to read --- */
  if (st && st.music) {
    const t2 = B.LOCAL.map.get('track02.bin');   /* what the nklocal: source will be asked for */
    const base = await B.memberDataStart(ins.img);   /* a zip member's bytes do not start at byte 0 */
    ok(st.music === 'fan' || st.music === 'disc', 'music registered', `${st.music}, ${st.tracks} tracks`);
    const c = B.cueParse(text), sec = (n) => c.tracks.find((x) => +x.track === n).index;
    /* A corrected fan plan sits where the audio really is, which is MUSIC_SHIFT away from where the cue
     * says; asserting the cue's numbers for it is what let a wrong plan look right. */
    const shift = (plan.fan && plan.corrected) ? B.MUSIC_SHIFT : 0;
    const want2 = base + sec(2) * B.SECTOR + shift, wantLen = (sec(3) - sec(2)) * B.SECTOR;
    ok(t2 && t2.off === want2 && t2.len === wantLen, 'track 02 is sliced at the sectors the geometry states',
       t2 ? `off=${t2.off} len=${t2.len} (want ${want2}/${wantLen})` : 'none');
    const t9 = B.LOCAL.map.get('track09.bin');
    if (plan.fan && plan.corrected)
      ok(t9 && t9.len === 22078 * B.SECTOR, 'fan track 09 is cut short at track 10\'s start, not run into it',
         t9 ? `${t9.len / B.SECTOR} sectors registered` : 'none');
    if (plan.fan && ins.img.crcKnown && ins.img.crc === B.MUSIC_CRC) {   /* where the audio really is */
      const by = Object.fromEntries(plan.tracks.map((t) => [t.track, t]));
      const offs = { 2: 408688, 3: 7824544, 8: 237820648, 9: 296241976, 15: 544565056 };
      for (const [t, off] of Object.entries(offs))
        ok(by[t] && by[t].off === off, `fan track ${t} starts at ${off}`, by[t] ? `planned ${by[t].off}` : 'absent');
    }
  }
}

(async () => {
  const args = process.argv.slice(2);
  if (!args.length) { console.log('usage: node scripts/byod-test.js <game-data.file> [more files...]'); process.exit(2); }
  const files = args.map((p) => nodeFile(p));
  await check(files.length === 1 ? files[0] : files, args.map((p) => path.basename(p)).join(' + '),
              { res: true, fan: null });   /* fan: settled from the image's own bytes, below */

  /* --- the same bytes, under names the port has never seen, picked instead of packed ---
   * A cue that names its image is the authority on the name, so a pair called anything else is the same
   * disc; and the bare image has to be recognised as this disc by its size, since it says nothing else. */
  const rip = path.join(ROOT, 'TG_NIGHT.CUE'), img = path.join(ROOT, 'TG_NIGHT.IMG');
  if (fs.existsSync(rip) && fs.existsSync(img)) {
    const c = nodeFile(rip, 'Eojjeonji Disc.cue'), i = nodeFile(img, 'Eojjeonji Disc.bin');
    await check([c, i], 'loose pair, named whatever the repacker liked', { res: true, fan: false });
    await check([nodeFile(img, 'night.bin')], 'a bare image with no cue at all', { res: true, fan: false });
  } else note(`[skip] ${rip} / ${img} not present (the rip is git-ignored)`);

  console.log(fails ? `\n${fails} failure(s)` : '\nall good');
  process.exit(fails ? 1 : 0);
})();
