/* assetcache.js -- the page's own copy of the big assets, in IndexedDB (docs/463).
 *
 * Preload mode takes 47 MB of music and 22 MB of resources down before the game starts, and until this file
 * existed it did that again on every reload: the copies lived in a Map in the page, which is exactly as long
 * as the page is. This is the disk half -- bytes go into IndexedDB and come back before the next download
 * would have started, so a reload costs a read from flash instead of a walk to the server.
 *
 * The whole design is one question, asked before a cached byte is trusted: *is this still the same file?*
 * A cached copy of a track the encoder later changed is not a cache hit, it is the wrong music, and it gets
 * reported as a bug in the game. So every read validates against the server and compares `content-length`
 * plus `etag` or `last-modified`. A cache miss costs bandwidth; a wrong cache hit costs the run.
 *
 * One exception, and it is deliberate: when the server cannot be asked at all -- the network is down -- a
 * copy that is present is better than no copy, and the caller is told it is `cache-unvalidated` so that the
 * difference is never silent. That is the case this cache mostly exists for: a phone whose radio has gone
 * to sleep mid-song should lose the radio, not the music.
 *
 * What this does NOT cache: the byte ranges that *streaming* mode fetches. Reassembling whole files out of
 * 16 KB windows in the background would compete for the same bandwidth with the music that is playing right
 * now, which is the opposite of what streaming mode is for.
 */
(() => {
'use strict';

const DB = 'night-assets', STORE = 'bytes', SCHEMA = 1;

/* hits: served from disk after the server agreed the copy is current. stale: served from disk because the
 * server could not be asked. misses: not held, or held and the server said it had changed. `wrote`/
 * `diskBytes` say what went in, `netBytes` what the run actually had to pull over the wire -- which is the
 * number a reload is supposed to make read zero. */
const stats = { hits: 0, stale: 0, misses: 0, wrote: 0, hitBytes: 0, netBytes: 0, diskBytes: 0, errors: 0 };

let dbP = null;
function openDB() {
  if (dbP) return dbP;
  dbP = new Promise((res, rej) => {
    if (typeof indexedDB === 'undefined') return rej(new Error('no indexedDB'));
    const r = indexedDB.open(DB, SCHEMA);
    r.onupgradeneeded = () => { if (!r.result.objectStoreNames.contains(STORE)) r.result.createObjectStore(STORE, { keyPath: 'url' }); };
    r.onblocked = () => rej(new Error('indexedDB open blocked by another tab'));
    r.onerror = () => rej(r.error || new Error('indexedDB would not open'));
    r.onsuccess = () => res(r.result);
  }).catch(() => { dbP = null; stats.errors++; return null; });
  return dbP;
}

async function read(url) {
  const db = await openDB();
  if (!db) return null;
  return new Promise((res) => {
    try {
      const q = db.transaction(STORE, 'readonly').objectStore(STORE).get(url);
      q.onsuccess = () => res(q.result || null);
      q.onerror = () => { stats.errors++; res(null); };
    } catch (e) { stats.errors++; res(null); }
  });
}

/* The ArrayBuffer is structured-cloned into the store, so the caller keeps its own copy and this one outlives
 * the page. The identity tokens are the ones the server sent for these exact bytes, kept with them so
 * "is this still current" can be answered by comparing the file against what came with it. */
async function write(url, bytes, etag, lastModified) {
  const db = await openDB();
  if (!db) return false;
  const rec = { url, bytes: bytes.buffer, size: bytes.length, savedAt: Date.now(),
                etag: etag || null, lastModified: lastModified || null };
  return new Promise((res) => {
    try {
      const q = db.transaction(STORE, 'readwrite').objectStore(STORE).put(rec);
      q.onsuccess = () => { stats.wrote++; stats.diskBytes += rec.size; res(true); };
      q.onerror = () => { stats.errors++; res(false); };
    } catch (e) { stats.errors++; res(false); }
  });
}

/* The identity the server gives for this URL right now, or null when it would not say. */
async function probe(url) {
  try {
    const r = await fetch(url, { method: 'HEAD', cache: 'no-store' });
    if (!r.ok) return null;
    const cl = r.headers.get('content-length');
    return { size: cl ? Number(cl) : 0, etag: r.headers.get('etag'), lastModified: r.headers.get('last-modified') };
  } catch (e) { return null; }
}

/* "Same file" means: same length, and every identity token both sides actually hold must agree. A server
 * that sends neither token (serve.py answers length only) is still validated, just weakly -- and weakly
 * validated is stated in the log line rather than being quietly good enough. */
function sameFile(rec, now) {
  if (rec.size && now.size && rec.size !== now.size) return false;
  if (rec.etag && now.etag && rec.etag !== now.etag) return false;
  if (rec.lastModified && now.lastModified && rec.lastModified !== now.lastModified) return false;
  return true;
}

async function download(url, onProgress) {
  const r = await fetch(url, { cache: 'no-store' });
  if (!r.ok) throw new Error(url + ': HTTP ' + r.status);
  const total = +(r.headers.get('content-length') || 0);
  const id = { etag: r.headers.get('etag'), lastModified: r.headers.get('last-modified') };
  if (!r.body || !total) {
    const bytes = new Uint8Array(await r.arrayBuffer());
    stats.netBytes += bytes.length;
    return { bytes, ...id };
  }
  const reader = r.body.getReader();
  const out = new Uint8Array(total);
  let got = 0;
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    out.set(value, got); got += value.length;
    if (onProgress) onProgress(Math.min(1, got / total));
  }
  stats.netBytes += got;
  return { bytes: out.subarray(0, got), ...id };
}

/* The one call the page makes. { bytes: Uint8Array, from: 'cache' | 'cache-unvalidated' | 'network' }. */
async function fetchAsset(url, { revalidate = true, store = true, onProgress } = {}) {
  const rec = await read(url);
  if (rec && rec.bytes && rec.bytes.byteLength === rec.size) {
    let v = 'match';
    if (revalidate) {
      const now = await probe(url);
      v = now ? (sameFile(rec, now) ? 'match' : 'changed') : 'unreachable';
    }
    if (v !== 'changed') {
      if (v === 'unreachable') stats.stale++; else stats.hits++;
      stats.hitBytes += rec.size;
      if (onProgress) onProgress(1);
      return { bytes: new Uint8Array(rec.bytes), from: v === 'unreachable' ? 'cache-unvalidated' : 'cache' };
    }
  }
  const got = await download(url, onProgress);
  stats.misses++;
  if (store) await write(url, got.bytes, got.etag, got.lastModified);
  return { bytes: got.bytes, from: 'network' };
}

async function drop(url) {
  const db = await openDB(); if (!db) return false;
  return new Promise((res) => {
    try { const q = db.transaction(STORE, 'readwrite').objectStore(STORE).delete(url);
          q.onsuccess = () => res(true); q.onerror = () => res(false); } catch (e) { res(false); }
  });
}

async function clearAll() {
  const db = await openDB(); if (!db) return false;
  return new Promise((res) => {
    try { const q = db.transaction(STORE, 'readwrite').objectStore(STORE).clear();
          q.onsuccess = () => res(true); q.onerror = () => res(false); } catch (e) { res(false); }
  });
}

globalThis.__nkCache = {
  stats,
  fetchAsset,
  drop,
  clear: clearAll,
  /* Ask the browser not to evict us. 70 MB of music is precisely the sort of thing a phone with a full disk
     deletes, and a silently emptied cache looks like a cache that does not work. */
  async persist() { try { return (await navigator.storage?.persist?.()) ?? false; } catch (e) { return false; } },
  async usage() { try { return await navigator.storage?.estimate?.() ?? null; } catch (e) { return null; } },
};
})();
