# figures/ — stage-plot figure & object production

Produces the standardized, correctly-scaled SVG line-art symbols (musicians,
objects) that stagePLTR ships in its catalog or that you import via the app's
object editor.

- **[spec.md](spec.md)** — the contract: scale model, viewBox/units, naming,
  and the mapping into stagePLTR. Read this first.
- **[PIPELINE.md](PIPELINE.md)** — the 3D authoring runbook (MakeHuman → Blender
  → SVG line art). The manual/creative half.
- **manifest.json** — the variation matrix + per-object real footprints.
- **generate.py** — *(step b, not yet added)* the 2D compositor: assembles
  `parts/` per `manifest.json`, crops to footprint, emits `out/` SVGs + a
  `catalog.json` fragment with `defaultSize` derived from each footprint and the
  master scale `S`.

## Layout

```
parts/         committed 2D SVG line-art parts (figures, hair, instruments, accessories)
out/           generated SVGs + index.html contact sheet
.blendsrc/     LOCAL ONLY (gitignored): .blend / .mhm / .fbx working files
.venv/         LOCAL ONLY (gitignored): Python environment
```

## Python environment (local, in-repo)

This tooling uses an isolated in-repo virtualenv — never global or conda.

```sh
cd figures
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

`requirements.txt` is finalized when `generate.py` lands (step b); it pins only
what the 2D compositor needs.

## Scale, in one line

Author everything in millimetres (Blender exports 1 unit = 1 mm); each object's
on-canvas pixel size is `footprint_mm / S` with `S = 10` (1 px = 1 cm). That one
rule keeps the whole library mutually proportional. See spec.md §1.