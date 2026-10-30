# 203 — Player-side debt/settle census on the kill-bearing route: the port's debt is positive in 60 samples, shipped's in 201

`scripts/rig/debt_trace.py` on `/tmp/rig/ents-orig` + `/tmp/rig/ents-port` (drive `ep1_kill3.txt`; anchor
`0x63e14 == 67` at shipped t=72 696 ms, port clock shifted 2 015 ms; 447 aligned samples per side):

```
ORIG player record: samples=447  debt>0: 201  debt<=0: 246  acc>0: 36  flag(+0x30)==0: 231  ==2: 216  hp_lost: 30
   transitions: HP x9  ACC_REARM x1  ARMED x1  SETTLE x1
     HP    t= 62489  debt 0 -> 1000        ACC_REARM t=62489  acc 0 -> 100
     ARMED t= 72696  debt 0 -> 1 (acc 100 -> 99)
     SETTLE t=228333 debt 6 -> 0
     HP    t=260078..265904  debt 1000 -> 999 -> 996 -> 990 ... -> 970   (eight decreases across the fight)
PORT player record: samples=447  debt>0:  60  debt<=0: 387  acc>0: 36  flag==0:  90  ==2: 357  hp_lost: 10
   transitions: HP x5  ACC_REARM x1  ARMED x1  SETTLE x1
```

Read together with docs/199 §3, this is the same divergence seen from the player's side: the shipped player spends
45 % of its live samples carrying a positive debt (`+0x23`) and its debt is then *drained* across a fight (the eight
`HP` steps at t=260-266 s, exactly the window in which its victims die carrying award 10); the port's player carries
a positive debt in 13 % of samples and sits in settle state 2 far more often (357 vs 216 samples).  Since shipped's
`fn_01441e` damage stage is gated on `debt == 0` (0x148ea-0x148f0) and the port's kind-1 victims reach the settle
block with `+0x30 == 0` and never pass the damage stage, the debt bookkeeping is the next concrete thing to pin down —
it is docs/162's open question, now measured on a route that does produce kills.
