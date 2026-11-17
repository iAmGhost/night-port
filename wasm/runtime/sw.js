/* sw.js -- the browser port's service worker.  docs/457.
 *
 * WHY THIS IS A SMALL FILE
 * ------------------------
 * It exists for one thing: from a home-screen icon the page starts as an installed app, full screen, with a
 * shell that is already on the device. Everything the port loads after that -- the 22 MB resource pack and
 * the CD tracks it Range-reads 150 KB at a time -- is the game's data, and this worker deliberately does not
 * touch it. `wasm/src/wasm_disc.c` asks for byte windows inside 40-87 MB files; a worker that answered those
 * from a cache would either miss (the window has moved) or, worse, answer one that belongs to a different
 * second of the same track. So requests that are not on the allowlist below go to the network untouched --
 * `return` with no `respondWith`, which is the SW equivalent of not intercepting.
 *
 * NO CACHE-FIRST, ON PURPOSE. The port's test loop is `make check`: rebuild night.wasm, reload the page,
 * read counters out of memory. A worker that served the shell from cache would make that loop silently
 * measure the previous build -- which is the bug this repo has called the most confusing one in a browser
 * build (see the Cache-Control line in scripts/serve.py). So every request is network-first and the cache is
 * only what answers when the network is not there at all. Locally that costs a round trip to 127.0.0.1; on a
 * phone it buys an app that at least opens.
 */
'use strict';

/* One version, one cache. Bump V on any shipped change: it is what retires an old worker's files instead of
 * leaving them installed forever. */
const V = 'v27';   /* a container's CRC is a note, not a condition: no member is refused for its CRC */
const CACHE = 'night-shell-' + V;

/* The app shell: the files that make the page and the engine, and nothing that changes with the data. */
const SHELL = ['index.html', 'shell.js', 'resample.js', 'assetcache.js', 'byod.js', 'night.js', 'night.wasm',
               'manifest.webmanifest', 'icon-192.png', 'icon-512.png', 'icon-maskable-512.png'];

self.addEventListener('install', (e) => {
  e.waitUntil((async () => {
    const c = await caches.open(CACHE);
    /* `ignoreSearch` so a start URL carrying ?pwa=1 does not miss the entry it is asking for. Individual
     * failures are kept out of the way of the rest: a half-cached shell still opens. */
    await Promise.allSettled(SHELL.map((p) => c.add(new Request(p, { cache: 'no-cache' }))));
    self.skipWaiting();
  })());
});

self.addEventListener('activate', (e) => {
  e.waitUntil((async () => {
    for (const k of await caches.keys()) if (k !== CACHE) await caches.delete(k);
    await self.clients.claim();
  })());
});

self.addEventListener('fetch', (e) => {
  const req = e.request;
  if (req.method !== 'GET') return;
  const u = new URL(req.url);
  if (u.origin !== self.location.origin) return;
  /* Only the shell, by exact path. /data/cd/trackNN.bin (Range), /data/audio/trackNN.mp3 and its
   * tracks.tsv (both Range too), data/NIGHT.RES.gz, data/TG_NIGHT.CUE and any query the page puts on a URL
   * are the game's own traffic and pass straight through. */
  const path = u.pathname.replace(/^.*\//, '/');
  if (!SHELL.includes(path.slice(1))) return;
  e.respondWith((async () => {
    try {
      const live = await fetch(req);                       // network-first: the build you just made wins
      const c = await caches.open(CACHE);
      if (live && live.ok && !req.url.includes('?')) c.put(req, live.clone());
      return live;
    } catch (err) {
      const hit = await caches.match(req, { ignoreSearch: true });
      if (hit) return hit;
      throw err;                                            // nothing to fall back on: let it fail honestly
    }
  })());
});
