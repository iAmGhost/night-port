"""docs/86 probe: is the original's title frame the title3 picture with a different per-index palette?"""
import numpy as np, PIL.Image, glob, os, sys, collections
C = PIL.Image.open('scratch/res/title3.pcx')
cidx = np.asarray(C, dtype=np.int64)                    # 640x480 indices
cpal = np.array(C.getpalette()[:768], dtype=np.int64).reshape(-1,3)
crgb = cpal[cidx.reshape(-1)].reshape(640,480,3)
def probe(p):
    O = np.asarray(PIL.Image.open(p).convert('RGB'), dtype=np.int64)
    if O.shape[:2] != (640,480): O = np.asarray(PIL.Image.open(p).convert('RGB').resize((640,480), PIL.Image.NEAREST), dtype=np.int64)
    Of = O.reshape(-1,3); Cf = crgb.reshape(-1,3); Ci = cidx.reshape(-1)
    pairs = collections.Counter(zip(Ci.tolist(), map(tuple, Of.tolist())))
    # functional? each card index -> how many distinct observed colours
    per_idx = collections.defaultdict(set)
    for (i,o),n in pairs.items(): per_idx[i].add(o)
    n_idx = len(per_idx); avg = np.mean([len(v) for v in per_idx.values()])
    top = pairs.most_common(1)[0]
    # dominant class size: sum of counts following the most common colour per index
    dom = sum(max(([n for (i2,o2),n in pairs.items() if i2==i] or [0])) for i in per_idx)
    uniqO = len(set(map(tuple, Of.tolist())))
    cardset = {tuple(c) for c in cpal if any(c)}
    share_as = np.mean([tuple(o) in cardset for o in Of.tolist()])*100
    share_sw = np.mean([(o[2],o[1],o[0]) in cardset for o in Of.tolist()])*100
    # per-channel normalized NCC
    a = Cf.astype(float); b = Of.astype(float)
    ncc = ((a-a.mean())*(b-b.mean())).mean()/(a.std()*b.std())
    print(f"{os.path.basename(p):28s} uniqO={uniqO:4d} idx_seen={n_idx:4d} colours/idx avg={avg:5.2f} "
          f"dom-class={dom/Of.shape[0]*100:5.2f}% top-pair={top[1]/Of.shape[0]*100:5.2f}% "
          f"share as={share_as:5.2f}% swap={share_sw:5.2f}% NCC={ncc:6.3f}")
for p in sys.argv[1:]: probe(p)
