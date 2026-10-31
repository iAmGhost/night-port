# 274 — Canonical-run dispatch census for item 2b, and the self-correction on docs/273 §1's counters

## 1. Canonical run, full drive, room actually entered

`NIGHT_ACTTRACE=1 ./port/test/night --script scripts/rig/drives/ep1_kill3.txt --frames 9000` ✓ (the canonical
`ep1_kill3` 512-event drive ✓, i.e. the same drive as `/tmp/rig/rb8` ✓):

```
per-room key-state wipe witness:  #1        (fn_03a829's 0x3aa04-0x3aa30 group ran once -> the stage really was entered)
fn_0347e3   = 3,082,379 entries      armed_ticks = 1,900,967  (62 % of ticks see an armed slot)
case_hits   = 312                    (switch reached case 0x1d/0x2a/0x2c/0x36/0x38)
fn_034353   = 122                    (ring posts, matches the dump-side cursor advances of ~110 in rb8)
gate g_dat_52ff8 = 1    slot[0x1c]=0  slot[0x1d]=0  slot[0x38]=0
```

122 posts in 150 s against 3.08 M pump entries ✓ is the starvation, and it agrees with the independent dump-side
measurement (cursor advances on 7 % of sample pairs ✓ docs/272 §2 ✓) — three separate instruments, same magnitude ✓

## 2. Self-correction: `fn_034735` / `fn_034635` counters are inconsistent, do not quote them

The same witness printed `fn_034735=0 fn_034635=0` ✓, which **cannot** be right: `fn_034353`'s only call sites in the
port are inside `fn_034735` (`f.c:8857`) and `fn_034635` (`f.c:9759/9760/9762`) ✓, both single definitions
(`f.c:8854`, `f.c:9753` ✓) called only from `fn_0347e3` (`f.c:8882/8884` ✓) — so 122 posts require at least 122 entries
of those two functions, and `case_hits = 312` says the case bodies did run ✓ A counter that disagrees with the call
graph it is measuring is a broken counter ✓ **Withdraw the `= 0` readings and with them docs/273 §1's "chain-key cases
reached 4 times / fn_034735 never ran" claim** ✓ (docs/273 §1's drive was also the wrong one — `ep1_chainhold.txt` uses
only the first 26 events of `ep1_kill3` and never entered a room ✓ the wipe witness printed nothing ✓ — so that census
was not a gameplay measurement at all ✓)

Fix before quoting anything dispatch-shaped: put a counter **at each post site** (`8857`, `9759`, `9760`, `9762`) so the
122 posts are attributed per caller ✓ rather than inferring caller counts from entry counters ✓ Then re-run ✓

## 3. What stands from docs/273 (unchanged, static evidence)

All 19 code sites naming DS `0x62c58` belong to nine shipped functions — `fn_032464`, `fn_0343d1`, `fn_034635`,
`fn_034735`, `fn_0347e3`, `fn_039c03`, `fn_03a829`, `fn_03d244`, `fn_03fcbd` — and the port models every one
(`f.c:4214`, `8345-8346`, `9753-9780`, `8854-8862`, `8878-8884`, `26903`, `23569`, `26982`, `app/keymap.c:65/69`) ✓; the
once-per-room wipe is positioned identically (shipped `0x3aa04 call 0x348e2` / `0x3aa10 mov eax,0x12c58` / `0x3aa15 call
0x41e20`, immediately before the room `for(;;)` ✓ == `f.c:23568-23570` ✓) ✓ And the wipe ran **exactly once** in the
canonical run ✓ — so the wipe is not what erases the chain-key state ✓ That leaves the make stream
(`fn_03fcbd`'s INT 09 arming: `0x3fe33` first make arms the slot ✓, `0x3fe26` re-make raises `g_dat_62cd8` ✓, and the
port's `keymap.c:63` gates all arming behind `held == 0` ✓) as the live candidate ✓ Next measurement after the
post-site counters: count `night_key_apply` makes per second in the port against the shipped's post rate of ~1.08 cursor
advances/s ✓ (docs/272 §2 ✓) ✓
