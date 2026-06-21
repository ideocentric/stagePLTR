# Figure & Object SVG Standard (the contract)

The single source of truth for how every stage-plot symbol is authored, scaled,
named, generated, and handed to the stagePLTR app. 3D authoring is in
[PIPELINE.md](PIPELINE.md); the 2D parts compositor is `generate.py`. **This file
is the contract both obey.** If a rule here disagrees with a habit in a tool,
this file wins.

---

## 1. Canvas, coordinates, scale

- **Canvas:** `viewBox="0 0 200 200"`. The figure is centred at **(100, 100)**.
- **Real scale:** **200 units = 2 m**, i.e. **1 unit = 10 mm (1 cm)**. This matches
  the Blender 2 m render frame (PIPELINE §2), so 2D-authored and 3D-rendered parts
  share one scale.
- **Facing:** the figure faces **north (−y / up)** = the direction the performer
  faces. A small nose mark on the head encodes orientation. Author no baked
  rotation/translation — stagePLTR rotates/positions the placed icon.
- **stagePLTR mapping (the key identity):** the app's master display scale is
  **`S = 10` mm/px**. With 1 unit = 10 mm and `S` = 10, **1 SVG unit → 1 px**.
  So an object's on-canvas `defaultSize` in pixels equals its footprint in units
  (see §6). Change `S` later → regenerate; it's non-destructive.

## 2. Line convention (the "CAD look")
Strokes come from two CSS classes injected into every output by the generator —
**parts carry no inline stroke styling**, so weight lives in one place
(`style_def` in `manifest.json`):
- `.ln`  — `fill:none` (open line work)
- `.lnf` — `fill:#fff` (closed shapes that must occlude layers beneath them)
- stroke `#111`, width `1.6`, round joins/caps.

Verified: Qt's `QSvgRenderer` (stagePLTR's icon path) honours these class
selectors, so generated SVGs drop in with no flattening.

## 3. Layers (z, bottom → top)
`layer_order` in the manifest: `torso → instrument → arms → head → hair →
accessory`. Each becomes `<g id="<layer>">` in the output (toggle/recolour by id).
Use `.lnf` for shapes that must hide what's beneath (head over arms, hair over
head); `.ln` for detail strokes.

## 4. Registration / anchors (200-unit frame)
- Head centre ≈ (100, 61); shoulder line ≈ y 92; waist ≈ y 140.
- Instruments authored **right-handed** as canonical, placed relative to the torso.
- Arms authored to meet the canonical instrument's grip points.

## 5. Handedness = mirror, never a second drawing
Left-handed variants horizontally mirror the `mirror_layers` (`instrument`,
`arms`) about centre: `transform="translate(200,0) scale(-1,1)"`.

## 6. Footprint & size (proper scale, zero-dependency)
Rather than computing an SVG bounding box, each object **declares its footprint**
as a crop rectangle in units, under `output.instruments.<instrument>` in the
manifest: `"footprint_units": [x, y, w, h]`.

The generator (step b) then, per object:
- sets the output **`viewBox` to that rect** (tight to the figure, not the whole
  2 m frame), and
- emits `defaultSize = [w, h]` px (1 unit → 1 px, §1).

This keeps every object proportional by construction and needs no geometry engine.

## 7. Overhead visibility (where to spend art effort)
Top-down you see hair, shoulders/torso, arms, hands, instrument, accessories — not
faces or skin tone. So:
- **Ethnicity** → hair shape/texture + headwear (not colour). Hair-part overrides.
- **Gender** → torso proportion + hair + accessories.
- **Style/genre** → hair + accessories (later: stance/props).
Invest in a rich hair + accessory library; bodies/instruments are a small set.

## 8. Naming (deterministic)
- Parts: `parts/<layer>/<descriptor>.svg` (e.g. `hair/hair_mohawk.svg`).
- Outputs: `<instrument>_<gender>_<ethnicity>_<handedness>_<style>.svg`.
Generated from the manifest, never hand-typed.

## 9. Manifest rule resolution
Per layer, the generator uses the **first** rule whose `when` is a subset of the
current traits; `{}` matches anything (default); `"use": null` draws nothing.
Order specific → general.

## 10. Delivery to stagePLTR: curated built-ins + loadable packs
The full matrix is large (e.g. one instrument × 3 genders × 5 ethnicities × 2
hands × 5 genres = 150). It is **not** all shipped as flat palette entries:

- **Curated built-ins:** a small, representative set is selected (a `builtin: true`
  flag/list in the manifest) and emitted into the app's `assets/plot/` + a
  `catalog.json` fragment.
- **Object packs:** the *whole* matrix (or themed subsets) is emitted as
  **importable packs** — a directory of SVGs + an `objects.json` (same shape the
  app's user library uses, plus a top-level pack `name` and a stable `id`).
  The `id` is a **GUID** (the generator derives it `uuid5`-style from the pack
  slug, so regenerating keeps the same id); stagePLTR tracks packs by this id so
  two packs can share a display name without merging, and removal is exact.
  **"Import Object Pack…"** loads a pack's objects into the user library (forcing
  a name if the file omits one); **"Remove Object Pack…"** removes one by id.
  (Same machinery as Phase 3's per-file embed/import.)

## 11. Stable decisions (do not drift)
- 200 units = 2 m (1 unit = 1 cm); `S` = 10 mm/px → 1 unit = 1 px.
- Faces north; right-handed source; mirror for left.
- `.ln`/`.lnf` classes, `#111`, width 1.6, transparent background.
- Footprint declared per object (`footprint_units`); output viewBox = that rect;
  `defaultSize` = its `[w, h]`.
- Naming + catalog/pack output generated from `manifest.json`, never hand-edited.
- Curated built-ins ship with the app; the full matrix ships as loadable packs.