# The game main and its entry thunk (0x0445c9 -> 0x03af18)

```
DOS4GW entry (0x420bc)
  -> fn_0445c9   stack reservation, DAT_00053640 = 0x8000, game main, terminate
      -> fn_03af18
           fn_03ae8e  free-memory gate (terminates below 0x17c DOS paragraphs)
           fn_03658d  master init          fn_03690b  master resource load
           fn_010c3e  disc-present gate
           prng_seed(fn_04297a result)
           menu loop:  fn_038a20, fn_038ae0, fn_03a224 -> 1 quit / 3 redraw / other start
           fn_037b03(0) stage 1, player from g_tab_52c3c[room serial + (stage-1)*10]
           cycle label (0x03b05a):
               fn_03f15f, "%ds%d.pcx" slide-show (g_tab_52f2c[stage], CD track 11)
               "stage%d.pcx" + 0xc00 delay units (the DAT_00063e89 latch cuts it)
               room loop over fn_03a829:
                   1  fn_03ace9, "%de%d.pcx" (g_tab_52f90[stage], CD track 12),
                      stage++ -> fn_0374ad -> cycle label;   stage 9 -> the ending
                   2  fn_0376c8 (CONTINUE: rebuild the player) -> room loop
                   0  stage = 1, fn_037347 -> menu
                  -1  fn_037306 + "
Thank You For Playing NIGHT
" -> return
               ending (stage 9): ten "end%04d.pcx" slides ("2end%04d.pcx" when the equipment
                      bytes are 1/2/4/4), 12000 delay units each, DAT_00063ec3 skips ahead
                      -> stage = 1, the player is removed and freed, back to the menu
```

## Slide-file naming

`fn_0428f8`'s vararg cursor walks **upward** through the pushed arguments, and the original pushes the
stage first, so the formats render `(stage, index)`: `<stage>s<n>.pcx` for the start slides and
`<stage>e<n>.pcx` for the end-of-stage slides.

## DS tables used here (image constants, extracted from NIGHT.EXE)

| Global | Contents |
|---|---|
| `g_tab_52f2c[10]` | 0x16, 3, 2, 6, 0, 0, 3, 0, 1, 3 -- start slides per stage (index 0 unused) |
| `g_tab_52f90[10]` | 0, 1, 1, 2, 0, 4, 3, 1, 1, 3 -- end slides per stage |
| `g_dat_52fb4` | 3 -- the stage-9 ending slide-show gate |
| `g_dat_5339c` / `g_dat_533a0` | the stack reservation pair written by `fn_0445c9` (0 in the image) |
| `g_tab_52c3c` | the player's action-container index per room (0xac everywhere in the image) |

## Testing the entry chain without the game fixtures

`test_fn_0445c9` drives the real thunk chain and escapes through the terminate callbacks: with
`g_dpmi_free_dos_paragraphs` below the 0x17c threshold, `fn_03ae8e` (the first callee of the main) ends
the program through `fn_0423b7`, and a `longjmp` out of `DAT_0005336c/70/74` proves the stack
reservation, `DAT_00053640 = 0x8000` and the entry into the game main. The rest of `fn_03af18`
(slides, CONTINUE, game over, ending) needs the resource-index and stage fixtures from the integration
tasks; the transcription is annotated with original addresses for that pass.
