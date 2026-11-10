# 420 — the first A/B with the disc gate: `DISC_MIN_WANT=27` was shipped, silently ran as 12, and a real 27 reads *less*

docs/419 built a reproducible gate for the disc read path. This is the deferred item it was waiting for — the
`DISC_MIN_WANT` clamp — measured rather than argued, and it turned out to be a configuration that lied.

## What the tree actually did

- `pocket/src/night/of_cdda.c` clamped the setting with `#define CD_MIN_WANT_MAX 12`.
- `scripts/pocket/mk_dist.py` shipped `DISC_MIN_WANT=27`.

So every generated `night.ini` on every card announced 27 while the code ran 12. Nothing was wrong
*behaviourally* — 12 is the shipping shape — but the file a user reads said something the core did not do, and
the deferred question ("lift the clamp?") had been framed as if the clamp were an arbitrary leftover.

## The A/B (docs/419's `--diff`, same route, same seed, 2000 frames, warp to stage 1)

Built with the clamp lifted to `CD_BATCH_MAX` (27) and `NIGHT_DISC_MIN_WANT=27`, then compared sector-granular
request sets against the shipping run:

```
A (min_want 12): 236 requests -> 2808 sectors
B (min_want 27): 233 requests -> 2772 sectors
only in A: 36    only in B: 0    shared: 2772      (every shared request byte-identical)
   A only: TRACK03 off=4925088 n=2352 ... 36 sectors, contiguous, at the tail of the track window
```

The interesting half is `only in A`. Fewer, larger operations was the expected win — but 36 sectors that the
12-policy read were **never read at all** by the 27-policy. The reason is the shape of the policy: `min_want`
is how much allowance to *wait for* before a fill read, so a bigger `min_want` does not make the pump read
more aggressively — when only a few sectors are due it **defers**. That is docs/407's starvation seen from the
read side ("my own deferral starved the ring"), arriving here as sectors at the end of a window nobody fed.

Decision, now with evidence attached at the clamp site: **the ceiling stays 12**, `DISC_MIN_WANT` above it is
refused on purpose, and the shipped ini key says 12 with the reason in its comment. The boot already prints the
effective value (`cdda policy: min_want=…`, docs/406 §5), so a refused value is visible rather than silent.

## The trap that ate my first attempt at this experiment

`make` in `pocket/src/night` builds `app.elf` (the RISC-V core). `app_pc` — the desktop shim this measurement
needed — is built by `make test`. I ran `make`, re-ran the shim, and got a policy line still saying 12, and a
diff of exactly zero against the baseline: a beautifully clean null result produced entirely by a stale binary.
docs/386's rule ("`make test` must build both halves, a passing shim is not evidence the core compiles") has a
converse that bit here: **a rebuilt core is not evidence the shim was rebuilt.** The fix in practice is that any
shim measurement starts from `make app_pc` (or `make test`), not `make`.

## What would reopen this

The case against 27 rests on a 2000-frame run, where the unread 36 sectors sit at the tail and are never
audible. What would make it a real problem — and what to measure first if anyone revisits — is a long run where
the read cursor falls behind the voice cursor for a sustained stretch: `docs/408`'s failure mode (a looping DMA
voice reading into unwritten ring) is the one that would make a deferred read audible. The instruments for that
already exist: `audio voice live=… dpos=… behind=… ahead=…` in the card log, next to the audit's request set.
