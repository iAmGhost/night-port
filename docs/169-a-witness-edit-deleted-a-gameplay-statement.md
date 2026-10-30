# 169 — A witness edit deleted a gameplay statement, and the suite went unrun for eleven commits

## 1. What was broken

The closeout sweep ran `make -C port test` after a long stretch of witness-only edits, and it failed:

```
[FAIL] state 0x66 with the attack key releases both halves of the pair and resets their holds
```

The cause is a real gameplay statement that had been deleted from the port.  In `fn_03272f`'s pair-break arm
(guest `0x00032779-0x0003280e`: `player[+0x17] == 0x66` and the slot byte is `'*'` or `'6'`), commit `865338d`
(docs/167 §5-§8) replaced

```c
        ((struct sa32 *)((pair + 0x17)))->v = 1;  ((struct sa32 *)((pair + 0x11)))->v = 0;
```

with a witness whose trailing comments ran onto the same lines:

```c
        if (getenv("NIGHT_STTRACE") && pair == g_player_ent)          /* docs/167 §5: the chain matcher's
            fprintf(stderr, "chainreset player state=%d ac=%n",       /* reset site, the first
                    ...                                                /* attack states */
```

The comment that began on the `if` line swallowed the two stores that followed it.  Net effect in the port:
breaking a `0x66` pair with the attack key stopped resetting the **partner's** state (`+0x17 -> 1`) and action
counter (`+0x11 -> 0`), while still clearing the partner's `+0x4a` link and `+0x50` hold -- i.e. a half-released
pair, which is the same shape as the playtest complaint "player stuck when hit mid-kick".  Restored in this
commit; `make -C port test` and `make -C port selftest` are ALL PASS again.

## 2. This is the same defect class as docs/166 §9 and docs/167 §8 -- but worse

Three prior occurrences of the witness trap produced *silent witnesses* (a `fprintf` inside a swallowed block,
caught by `strings(1)` finding zero copies of the format string).  This occurrence produced **deleted game
logic**: the swallowed text was not a witness, it was the code.

`strings` cannot catch this class.  The only detector is the fixture suite -- which is exactly what was not run.

## 3. How long it sat

`865338d` is eleven commits back.  The commits after it (`626e582`, `d3d63a3`, `5e6f307`, `e186972`, `93435d4`,
`1dbdc3c`, `e94c734`, `48ca4fb`, `a6259d1`, `61b82b6`, `2ee12ba`) all touched `port/src/` or ran oracle routes, and
none of their messages records a `make -C port test` run.  Earlier commits in this goal habitually ended with
"make -C port test ALL PASS"; that habit lapsed exactly during the stretch where the damage was done.

## 4. The rig did not see it

The oracle never touches this arm: `0x66` (a pair state) with a `'*'`/`'6'` slot byte never occurs on the drives
we run -- the same coverage hole docs/167 §15 recorded for `fn_031f10`'s deferred queue (`fn_031f10` called zero
times, `+0x3e`/`+0x42` never non-zero).  A rig that cannot reach a state cannot regress-test it, so the fixture
suite is not redundant with the differential rig: it covers the paths the drives never reach.

## 5. Standing rules filed

- **Run `make -C port test` before committing any edit under `port/src/`.**  Witness-only edits are not exempt:
  this regression came from a witness-only edit.
- **Comments go above the statement, never after it,** in any block where a witness is inserted next to real
  code.  (docs/166 §9 and docs/167 §8 already say this for witnesses; it now applies to any adjacent code.)
- When a witness insertion shows up in a diff as a `-` line that is *not* a comment or a `fprintf`, treat it as a
  suspected regression and re-run the suite for that specific function.  The line that gave this away in the
  audit was `git show <commit> -- port/src/f/f.c | grep '^-'` printing a gameplay store.
- Rig coverage gaps are filed as gaps (docs/167 §15: the pair/deferred chain machinery is unexercised by every
  drive we have).  A drive that enters `0x66`/`0x70` and then takes the attack key is a named next drive.
