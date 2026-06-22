# Stage-Plot Figures — MakeHuman → Blender → SVG Line-Art Pipeline

End-to-end runbook for producing the overhead, CAD-style line-art musician icons
as standardized SVG, suitable for print and PDF stage-plot export. This is the
3D-assisted production path that feeds the parametric generator (`generate.py` /
`manifest.json` / `spec.md`) already in this repo.

---

## 0. The principle (why this stays consistent)

Every figure is rendered through **one fixed orthographic camera** at a locked
tilt. That single fact gives you, for free, the thing hand-tracing could not:
identical scale and identical projection across the entire library. Nothing can
drift wider, taller, or differently-angled than anything else, because they're
all measured by the same camera.

The work is organized so you do the expensive steps rarely:

| Axis            | Where it's handled            | Cost to add one more         |
|-----------------|-------------------------------|------------------------------|
| Instrument pose | Posed once in Blender         | One pose, reused everywhere  |
| Gender / build  | MakeHuman morph sliders       | Re-apply slider, re-render   |
| Hair / style    | Swappable hair object/collection | One render of the new hair |
| Ethnicity       | Mostly hair (see `spec.md` §6)| Usually just a hair variant  |
| Handedness      | **2D mirror in SVG** — no render | Free                      |
| Instrument art  | **Your existing scaled instrument SVGs** | Already done      |

> **Formalized contributor path (whole-character capture).** The
> `blender-flow/stagegen.py` toolkit captures the **complete posed character —
> body, hair, and instrument together — as one SVG = one object** (`spec.md` §11).
> The instrument is a real mesh in the `INSTRUMENT` collection, rigid-attached to
> a torso bone so it follows posing, and it is captured *with* the figure. This is
> the simplest, most automatable path and the one the docs below assume.
>
> The 2-D vector-composite alternative described next (render the figure, keep the
> instrument as a separate scaled SVG, composite in 2-D) remains valid as an
> advanced option when you want vector-perfect instruments reused across many
> figures — but it requires precise 2-D registration and is not the default.

In the vector-composite alternative, the instrument itself is **not** rendered
from 3D. You already built a scaled, real-world-dimension instrument SVG set —
keep using it. In 3D you only need a *proxy* instrument (a primitive sized to the
real thing) so the hands pose to the correct footprint; the proxy is hidden from
the render, and the real instrument SVG is composited back in 2D at the matching
position. This keeps instruments vector-perfect and to scale, and cuts your asset
purchases down to "a figure system + a few hairstyles."

---

## 1. Tools & versions

- **Blender 4.4+ / 5.0** — Grease Pencil 3 and its Line Art modifier landed in
  4.3; the line-art and DAZ add-ons you may touch list support through 5.0.
  Upgrade off 4.0.1.
- **MakeHuman 1.2.x** — free, open-source, slider-driven figure generation
  (gender, ethnicity, age, body). Its generated characters have long been
  released under a permissive/CC0 license — verify current terms, but this is
  why it's the clean choice for an open project. Export to Blender via the
  **MHX2** add-on (best rig/shape-key fidelity) or **FBX** (most robust across
  versions; fine here since line art doesn't need a fancy rig).
    - Alternative: **MB-Lab** runs natively *inside* Blender (no export step).
- *(Optional)* **NijiGPen** add-on — 2D vector cleanup, boolean ops, SVG
  clipboard exchange for tidying strokes.
- *(Optional, with license care)* **Diffeomorphic** — only if you supplement
  with DAZ Genesis hair/morphs. See §10.

---

## 2. The locked projection standard (the contract)

Author this once and never change it; it is the 3D-side companion to `spec.md`.

- **Camera:** Orthographic. `ortho_scale` set to a fixed real-world frame
  (e.g. **2.0 m**), so scale is identical for every render.
- **Tilt:** Your chosen angle from vertical (start at **8°**; the sweep showed
  6–10° is where a standing figure becomes legible). Camera looks straight down
  then pitched by 8° about X so you see the top of the head, the back, and the
  arms reaching toward the instrument (flip the sign if the reveal is reversed).
- **Figure placement:** Feet at `z = 0`, centered on the world origin, facing
  **−Y** (downstage / toward the audience). "North / up" in the output =
  upstage / away from audience, matching `spec.md`.
- **Units:** Calibrate once — render a known-length reference object, measure it
  in the exported SVG, and set the exporter scale so **1 SVG unit = 10 mm**
  (matching `spec.md` §1: 200 units = 2 m). After that every export is
  automatically to scale and consistent with the 2-D parts/app frame.
- **Stroke:** Single black contour weight; normalization strips the inline stroke
  and applies `spec.md`'s `.ln`/`.lnf` classes (the generator injects the style).
- **Canvas:** Fixed square `viewBox` `0 0 200 200` (1 unit = 10 mm for the 2 m
  frame), identical for all objects, centred at (100, 100).

---

## 3. One-time Blender scene setup

1. **World:** flat white, no lighting needed (line art ignores shading).
2. **Camera:** add a camera, set `Type = Orthographic`, `Ortho Scale = 2.0`.
   Place it above the origin and parent it to an Empty at ~hip height; rotate
   the Empty `8°` about X to tilt. (Parenting to the Empty lets you re-tilt the
   whole rig from one control — reuse the camera logic from `render_sweep_blender.py`.)
3. **Collections:** create `FIGURE`, `HAIR`, and `PROXY` collections. The proxy
   instrument goes in `PROXY` and is excluded from rendering.
4. **Line Art object:** add a Grease Pencil object, give it a **Line Art**
   modifier. Set Source = `Scene` (or target the `FIGURE`/`HAIR` collections).
   Enable edge types: **Contour + Crease + Material Borders** (add Intersection
   if you want fold lines); tune the crease angle until you get clean outlines
   without interior noise. Set the GP stroke material to solid black, thickness
   to your standard weight.
    - For *separable* parts, give `FIGURE` and `HAIR` each their own Line Art GP
      object targeting their own collection, so body and hair export as separate
      SVGs that you layer in the generator.

---

## 4. Generate a base figure (MakeHuman)

1. In MakeHuman, set the figure with the **gender**, **age**, **muscle/weight**,
   and **ethnicity** sliders. These are the morph axes for your matrix; save
   each base as a `.mhm` preset (e.g. `base_fem.mhm`, `base_masc.mhm`).
2. On the **Pose/Animate → Skeleton** tab, assign a skeleton (Default or Game
   engine rig is plenty for posing).
3. Export: **Files → Export → MHX2** (or FBX). Keep feet at origin.
4. In Blender, import (`File → Import → MHX2`/FBX). Move the figure so feet sit
   at `z = 0`, centered, facing −Y. Put the mesh + armature in `FIGURE`.

Tip: body-shape changes can also be done in Blender as shape keys, so one
imported rig can be re-shaped and re-rendered without re-exporting from
MakeHuman — useful for the build variants.

---

## 5. Pose it (once per instrument)

This is the only step needing a little 3D, and it's rotation, not modeling —
about 6–10 bones. Do it once per instrument and reuse across every morph/hair.

1. Drop a **proxy instrument** into `PROXY`, sized to your real instrument's
   dimensions (a box for a keyboard, a cylinder + thin box for a guitar, etc.).
   Position it where the instrument sits relative to a standing player.
2. Enter **Pose Mode** and rotate, using your overhead reference sketches:
   `spine`/`chest` (slight forward lean), `neck` + `head` (tilt down),
   `upperarm.L/.R`, `forearm.L/.R`, `hand.L/.R` to bring the hands onto the
   proxy. Guitar: left hand to the neck, right hand over the body. Keys: both
   hands forward at key height. Drums: arms forward/down to the pad/skin.
3. Save the pose to a **pose asset / action** named per instrument
   (`pose_guitar`, `pose_keys`, `pose_drums`).
4. Note the proxy's 2D footprint position — this is where the real instrument
   SVG will composite later, so hands and instrument register exactly.

Pose right-handed only; left-handed comes from the 2D mirror.

---

## 6. Hair (the main variation signal)

From overhead, hair carries most of the gender/ethnicity/style read (see
`spec.md` §6), so invest your variety here.

- Sources: MakeHuman's built-in hair, simple modeled hair shells, or (with the
  license care in §10) DAZ hair via Diffeomorphic. For line art, low-detail
  hair shells are enough.
- Parent each hairstyle to the head bone and keep them in the `HAIR` collection.
- Render one hairstyle at a time by toggling object visibility; because the head
  pose and camera are fixed, every hair aligns automatically.

---

## 7. Render to SVG

1. Frame the figure in the locked camera; bake the Line Art modifier (or leave
   it live).
2. In **Object Mode**: `File → Export → Grease Pencil as SVG`. Set the export
   scale to your calibrated 1 unit = 1 mm, choose the active/selected GP object
   (export body and hair separately if using separable parts).
3. **Ortho caveat:** there's a historical bug where orthographic + GP SVG export
   produced an empty file. It's almost certainly fixed in 4.4+/5.0, but
   orthographic is the whole point here, so **test one frame first**. If it ever
   exports empty, switch the camera to Perspective with a very long focal length
   (e.g. 250–500 mm) pulled far back — visually near-orthographic, and it
   exports reliably.
4. Name deterministically, matching the generator's scheme, e.g.
   `figure_guitar_fem_hairlong.svg`, `hair_long.svg`.

---

## 8. The variation matrix (minimize renders)

Render count is **instruments × builds × hairstyles** — *not* multiplied by
handedness or by ethnicity-as-its-own-axis:

- **Handedness:** apply a horizontal mirror in the generator
  (`transform="translate(W,0) scale(-1,1)"`, already supported in `manifest.json`
  via `mirror_layers`). Zero extra renders.
- **Ethnicity:** realized through hair (and optionally subtle MakeHuman
  skull/feature morphs). Add hair variants rather than a separate render axis.
- **Build/gender:** re-apply the MakeHuman morph or Blender shape key on the
  posed rig, re-render.

So a guitar + keys + drums set with 2 builds and 5 hairstyles is ~30 figure
renders, not 150+ — the rest is 2D composition.

---

## 9. Composite & assemble

Feed the rendered parts into the existing generator:

1. Drop the figure SVGs into `parts/` (e.g. `parts/figures/`), and the hair
   SVGs into `parts/hair/` if you rendered them separately. Your existing
   instrument SVGs stay in `parts/instruments/`.
2. Update `manifest.json` rules so each combo selects the right figure render +
   your scaled instrument SVG, layered per `layer_order`, with the instrument
   positioned at the proxy footprint from §5.
3. Run `python3 generate.py` to stamp out the full matrix (handedness mirrored
   automatically, deterministic filenames). `out/index.html` is your contact
   sheet.

The 3D step guarantees consistent projection, scale, and anatomy; the generator
handles the cheap combinatorial assembly. Same architecture you already have,
now populated with rendered line art at your locked tilt.

---

## 10. Automation (optional, version-sensitive)

The repetitive render loop can be scripted with `bpy`: iterate builds (set shape
key / morph), iterate hair (toggle collection visibility), bake Line Art, export
SVG. The Grease Pencil 3 line-art-bake and SVG-export operators are
version-specific, so validate one manual export in your installed Blender first,
then I can harden a batch script against that exact version. The camera/tilt/loop
scaffold in `render_sweep_blender.py` is the starting point — swap its Freestyle
PNG render for GP Line Art + SVG export and wrap it in the build/hair loops.

---

## 11. License-safe repo layout

For a clean open-source release, the rule is simple: **commit 2D derivatives and
your own work; never commit purchased/licensed 3D assets.**

```
stageplot-figures/
├── LICENSE                # code license (your choice: AGPL/MIT/etc.)
├── LICENSE-assets         # art/SVG license (e.g. CC-BY-4.0 or CC0)
├── ASSETS.md              # provenance + license of every source (see below)
├── PIPELINE.md            # this file
├── spec.md  README.md  manifest.json  generate.py
├── parts/                 # committed: SVG line-art parts (2D derivatives)
│   ├── figures/  hair/  instruments/  accessories/
├── out/                   # committed or built: final SVGs + index.html
└── .blendsrc/             # LOCAL ONLY — gitignored working files
    ├── *.blend  *.mhm  *.fbx  hair meshes, proxies, DAZ assets
```

`.gitignore`:
```
# 3D working files and any licensed source assets — never published
.blendsrc/
*.blend
*.blend1
*.mhm
*.fbx
*.obj
*.duf        # DAZ scene files
*.dsf        # DAZ assets
textures/
```

`ASSETS.md` — record provenance so the license trail is auditable:
```
| Asset            | Source / Tool   | License        | In repo?            |
|------------------|-----------------|----------------|---------------------|
| Base figures     | MakeHuman 1.2   | CC0 (verify)   | 2D SVG only         |
| Hair shells      | MakeHuman / own | CC0 / your art | 2D SVG only         |
| Instrument icons | Your own work   | your choice    | yes (SVG)           |
| (if used) Hair X | DAZ store       | DAZ EULA       | 2D SVG only, no mesh |
```

Notes:
- **MakeHuman / MB-Lab path:** output is permissive/CC0, so even the meshes
  could be shared — but keeping `.blend`/working files out of git is still
  cleaner. This is the recommended path for a public OSS release.
- **DAZ supplement path:** DAZ's terms let you use, modify, and distribute your
  **2D renders** (your SVG line art is a 2D derivative from which no mesh can be
  extracted); you may **not** redistribute the meshes or textures. So commit
  only the SVGs, never the figures/textures/`.duf`. Avoid items flagged with the
  **Editorial License** (they restrict commercial 2D use). This is not legal
  advice — a quick DAZ sales-support ticket describing the open-source 2D use
  removes any ambiguity.
- Dual-license clearly: code under your code license, the SVG art under a
  separate art license (CC-BY or CC0), since they're different kinds of work.

---

## 12. Order of operations (checklist)

1. Upgrade Blender to 4.4+/5.0; install MakeHuman (+ MHX2) or MB-Lab.
2. Build the locked scene once: ortho camera, 8° tilt, white world, Line Art
   GP object(s), collections. Calibrate 1 unit = 1 mm. (§2–3)
3. Generate base figures in MakeHuman; import to Blender. (§4)
4. Pose once per instrument against a proxy; save poses. (§5)
5. Collect hairstyles into the HAIR collection. (§6)
6. Render-test one SVG to confirm ortho export works; fix scale. (§7)
7. Render the build × hair × instrument matrix to SVG. (§7–8)
8. Drop parts into `parts/`, update `manifest.json`, run `generate.py`. (§9)
9. Commit SVGs + code; keep `.blend`/source assets local per §11.