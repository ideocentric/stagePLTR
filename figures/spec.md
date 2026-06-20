# Figure & Object SVG Standard (the contract)

The single source of truth for how every stage-plot symbol is produced, scaled,
named, and handed to the stagePLTR app. Authoring (3D → line art) is described in
[PIPELINE.md](PIPELINE.md); **this file is the contract that output must obey** so
the whole library is mutually consistent and correctly scaled.

If a rule here and a habit in a tool disagree, this file wins.

---

## 1. Coordinate & scale model

Three layers, each with one job:

| Layer | Unit | Set by | Rule |
|---|---|---|---|
| **3D render** | metres | Blender ortho camera | Fixed 2 m frame, locked 8° tilt (PIPELINE §2) |
| **Published SVG** | **millimetres** (1 user unit = 1 mm) | Blender SVG export, calibrated | Geometry is real-world true |
| **stagePLTR canvas** | pixels | the app | `defaultSize_px = footprint_mm / S` |

- **Everything is authored in millimetres.** Calibrate the Blender exporter once
  (render a known-length reference, measure the SVG, set exporter scale) so
  `1 SVG unit = 1 mm`. After that, all geometry is to scale for free.
- **`S` — the master display scale — is the only mm→pixel knob.**
  - **`S = 10` (1 px = 1 cm).** Library-wide constant.
  - A US-Letter page (816×1056 px @96 dpi) then reads as ≈ **8.16 m × 10.56 m** of
    stage. A 0.5 m-wide person → 50 px; a 1 m guitar → 100 px; a 1.5 m kit → 150 px.
  - `S` is **non-destructive**: it only sets each object's default on-canvas size.
    Change it later → regenerate `defaultSize`; placed instances can still be
    resized by hand. So this number is safe to revisit after the first real render.

**Why this guarantees scale:** because every object's `defaultSize` is *derived*
from its measured millimetre footprint by the same `S`, no object can drift out of
proportion to any other — a guitarist and a grand piano are correct relative to
each other by construction, not by eye.

---

## 2. The published SVG (one per object/variant)

What the generator emits for stagePLTR to consume.

- **Compose in the 2 m frame, then crop.** Parts (figure, hair, instrument) are
  registered in the shared 2 m frame so hands meet the instrument; the *finished*
  object is then **cropped to its own content bounding box**.
- **`viewBox` = the cropped footprint, in millimetres.** e.g. a guitarist might be
  `viewBox="0 0 520 980"` (520 mm × 980 mm). **Non-square is expected and fine**
  (stagePLTR supports non-square `defaultSize`). Do **not** force a square frame.
- **No `width`/`height` attributes** (or set them equal to the viewBox extents) —
  the app scales the viewBox into its pixel box.
- **Stroke:** single black contour, `#000000`, one standard weight. Transparent
  background (no white fill rectangle). Dark line-art reads on stagePLTR's light
  page; the palette is also light.
- **Orientation (overhead):** figure faces **−Y** = *downstage / toward the
  audience*. "Up" in the SVG (−Y in render terms, top of the viewBox) = *upstage /
  away from the audience*. Every object shares this so a stage reads coherently.
- **Pose handedness:** author **right-handed only**. Left-handed is a 2D mirror
  (§4), never a separate render.

---

## 3. Naming (deterministic)

`<kind>_<subject>_<variant...>.svg`, lowercase, hyphen-free tokens.

- Figures: `figure_<instrument>_<build>_<hair>[_left].svg`
  e.g. `figure_guitar_masc_short.svg`, `figure_keys_fem_long.svg`,
  `figure_guitar_masc_short_left.svg` (mirrored).
- Separable parts (if rendered apart for layering): `hair_<style>.svg`,
  `body_<build>_<pose>.svg`.
- Instruments/objects: `object_<name>.svg` e.g. `object_grand-piano.svg`.

Names are generated from `manifest.json`, not typed by hand, so they stay
collision-free and match the contact sheet.

---

## 4. Variation matrix (minimise renders)

Render count = **instruments × builds × hairstyles**. Nothing else multiplies:

- **Handedness** → 2D mirror at compose time
  (`transform="translate(W,0) scale(-1,1)"`), zero extra renders.
- **Ethnicity** → realised through hair variants (+ optional subtle skull morphs),
  not a separate render axis (PIPELINE §6, §8).
- **Build/gender** → re-apply the MakeHuman morph / Blender shape key, re-render.

---

## 5. Handing an object to stagePLTR

Each published object maps to one `catalog.json` entry (or is imported via the
app's object editor):

| Catalog field | From the figure pipeline |
|---|---|
| `id` | the deterministic file stem (e.g. `figure-guitar-masc-short`) |
| `name` | human label (e.g. "Guitarist") |
| `category` | **People** for musicians, **DJ** for DJ gear, else the object's group |
| `icon` | the published SVG (bytes; SVG preferred per app convention) |
| `defaultSize` | `[ viewBox_w / S , viewBox_h / S ]` — non-square allowed |
| `ports` | usually **none** for a person (it's a position marker); add a vocal mic / DI where it makes sense |
| `builtin` | `true` if shipped in the app; user-made copies are `false` |

`generate.py` (step **b**) will emit both the SVGs and a `catalog.json` fragment
with `defaultSize` already computed from each footprint and `S`, so figures drop
straight into the app at the correct, mutually-consistent scale.

---

## 6. Stable decisions (do not drift)

- `S = 10` mm/px (master display scale).
- Render frame: 2 m, ortho, 8° tilt (PIPELINE §2).
- SVG units: millimetres; cropped to content; non-square viewBox; black contour,
  transparent background.
- Orientation: faces −Y (downstage); right-handed source; mirror for left.
- Naming + `catalog.json` generated from `manifest.json`, never hand-edited.

Revisit `S` (only) after the first real test render, once a figure's on-page size
can be eyeballed against the existing instrument symbols.