# figures/ — stage-plot figure & object production

Produces the standardized, correctly-scaled SVG line-art symbols (musicians,
objects) that stagePLTR ships as curated built-ins or that you load as packs.

- **[spec.md](spec.md)** — the contract: scale, line convention, layers,
  footprint→size, naming, and the stagePLTR (built-ins + packs) mapping. Read first.
- **[PIPELINE.md](PIPELINE.md)** — the optional 3D authoring runbook (MakeHuman →
  Blender → SVG line art) for higher-fidelity parts.
- **manifest.json** — axes + declarative part-selection rules + the `output` block
  (scale, per-instrument footprint, curated `builtin` list).
- **generate.py** — the compositor (Python 3 stdlib only): stamps every axis combo
  by layering `parts/`, mirrors for left-handed, writes `out/` + `index.html`.

## Quickstart
```bash
python3 generate.py --dry-run                 # list the matrix, write nothing
python3 generate.py                           # build everything into out/
python3 generate.py --filter style=punk       # build a subset
python3 generate.py --filter gender=female --filter handedness=left
open out/index.html                           # contact sheet

# App artifacts (step b) — written under dist/, gitignored:
python3 generate.py --emit builtins           # curated built-ins + catalog.json fragment
python3 generate.py --emit packs              # whole matrix as one importable pack
python3 generate.py --emit packs --pack-by style   # one pack per genre
python3 generate.py --emit all --pack-by style     # both
```

## Layout
```
manifest.json  spec.md  PIPELINE.md  generate.py  ASSETS.md
parts/   base/ head/ hair/ arms/ instruments/ accessories/   # SVG part library
out/     generated SVGs + index.html            (gitignored)
dist/    --emit output: builtins/ + packs/      (gitignored)
.blendsrc/   LOCAL ONLY: .blend/.mhm/.fbx       (gitignored)
.venv/       LOCAL ONLY: Python environment     (gitignored)
```

## Scale, in one line
Author in the 200-unit frame where **200 units = 2 m (1 unit = 1 cm)**; each
object declares a `footprint_units` rect, and with the app's `S = 10` mm/px that
becomes a pixel-exact, mutually-proportional `defaultSize`. See spec.md §1, §6.

## Status / roadmap
- **Now:** consolidated generator + placeholder parts; output renders correctly
  through stagePLTR's `QSvgRenderer` (CSS classes verified). **Step b done:**
  `--emit` consumes `footprint_units` to set each output's `viewBox` and
  `defaultSize`, and writes a `catalog.json` fragment for the curated built-ins
  plus app-shaped **object packs** (`objects.json` + SVGs) under `dist/`.
- **Next:** stagePLTR gains **Import Object Pack…** to load a pack into the user
  library (builds on Phase 3).
- **Then:** the placeholder `parts/` get replaced with properly-scaled art
  (hand-authored or via the Blender pipeline), and the curated built-ins get
  emitted into `assets/plot/` + merged into the shipping catalog.

The Python tooling uses an isolated in-repo venv (`python3 -m venv .venv`); the
generator itself is stdlib-only, so `requirements.txt` is empty until a step adds
a real dependency.