# Requirements — Authoring New Figure Icons

Everything needed to produce a new overhead line-art musician icon for the
stage-plot library, using Blender + MPFB2 and the `figures/blender-flow/stagegen.py`
toolkit. This is the contributor-facing setup; it complements `../PIPELINE.md`
(the design rationale) and `../spec.md` §11 (the capture→object contract), and
feeds `../generate.py` (assembly + pack emit).

Each finished character is captured as **one complete SVG** (body, hair, and
instrument together). `stagegen.py` only exports the raw Grease-Pencil SVG plus a
small metadata sidecar; `../generate.py --ingest` then recolours it to the spec
ink, crops to the figure's content, derives its real-world size, mirrors for
left-handed, and emits an importable object pack (`spec.md` §11).

---

## 1. Software

| Tool        | Version            | Notes                                            |
|-------------|--------------------|--------------------------------------------------|
| Blender     | **4.3+** (4.4/5.0) | Grease Pencil 3 + Line Art; macOS/Apple Silicon OK |
| MPFB2       | latest extension   | Install via Preferences → Extensions → search "MPFB" |
| MakeHuman   | not required       | Only if you want its asset downloader; 1.2.0 is the last macOS build and is enough |

MPFB2 runs *inside* Blender, so the platform is whatever Blender supports —
which is why this path works cleanly on current macOS where the standalone
MakeHuman app does not.

Optional: **NijiGPen** add-on for 2D stroke cleanup/boolean ops on the exported
Grease Pencil.

## 2. MPFB asset packs

Install in MPFB via *apply assets → library settings → load pack from zip file*,
then restart Blender. Get them from the asset-packs page.

Required:
- **makehuman_system_assets** — CC0 base pack (eyes, teeth, skins, base hair and
  clothes). MPFB needs this to function.

Recommended for line-art icons (everything else is largely invisible overhead):
- **Hair 01 / 02 / 03** — the primary gender/ethnicity/style signal from above.
- **Shirts 01–03, Masks 01–02** — shoulder-level silhouette cues for genre.
- **Poses 01 (sitting) / 02 (sports)** — optional starting points before you
  save your own musician poses.

Skip for this use (face detail doesn't read overhead): Eyebrows, Eyelashes,
Visemes, Faceunits.

License: system assets are CC0; other packs are mostly third-party with varying
licenses. Record the license of each hairstyle you actually use in `ASSETS.md`.

## 3. The standardized stage

Run `stagegen.py` with `MODE = "build"` to construct it (or open the shipped
template `.blend`). It sets, from one config block:
- Metric units, **1 Blender unit = 1 metre**.
- Orthographic camera, `ortho_scale = 2.0 m` (so the 2 m frame is exactly 200
  spec units), tilted **8° from vertical** (the locked angle), parented to a
  `TiltPivot` empty so the whole rig re-tilts from one control.
- White world, square render (1024²).
- Render collections: `FIGURE`, `HAIR`, `INSTRUMENT`, `PROXY`, `LINEART`.
- A `StageLineArt` Grease Pencil object with a Line Art modifier
  (contour + crease + material + intersection edges).

Calibrate scale **once**: render a known-length object, confirm in the SVG
exporter that **1 unit maps to 10 mm** (matching `spec.md` §1), and lock that
exporter scale. After that every export is automatically to scale and to each
other. Capture then writes the raw SVG + a metadata sidecar; `generate.py
--ingest` derives each object's real size from its own content, so scale is
consistent automatically.

## 4. Instruments follow the figure (the "wardrobe" behaviour, done right)

Goal: when a contributor reposes the guitarist, the guitar moves with them. The
correct binding for a **rigid** prop is *not* MPFB clothes (MHCLO) — clothes are
surface-fitted and deform with the body, which would warp a guitar. Instead,
attach the instrument **rigidly to a torso bone**:

1. Model or import the instrument (extrude from your existing scaled SVG, or a
   primitive sized to real dimensions). Place it in the `INSTRUMENT` collection.
2. Position it in playing location relative to a standing figure.
3. Run `attach_instrument("guitar", "Human", "spine03")` (or call it from
   capture). It rigid-parents the instrument to the bone, preserving placement,
   so it follows the torso when posed and never deforms. Then pose the hands
   *onto* the instrument — exactly how a strapped guitar behaves.

Verify your rig's torso bone name and set `ANCHOR_BONE` accordingly (the
function prints the available bones if the name is wrong).

If you specifically want the instrument to appear inside the MPFB asset library,
you *can* author it as an MHCLO clothes asset, but weight its whole mesh to a
single bone so it stays rigid; the bone-parent approach above is simpler and
more reliable for a one-off rigid prop.

## 5. Author a new icon (step by step)

1. **Create the character** — MPFB → *new human → from scratch*; set gender,
   age, build, ethnicity with sliders. Save as a human preset for reuse.
2. **Dress + hair** — add a hairstyle (and a shoulder-level top/mask if the
   genre needs it) from the asset library.
3. **Pose** — apply a saved musician pose, or pose ~6–10 bones (spine, neck,
   head, upper arms, forearms, hands) to the playing posture, then save it with
   *create assets → MakePose* so it's reusable across characters.
4. **Attach the instrument** — §4. Pose the hands onto it.
5. **Name + isolate** — put the character's body + hair + instrument in a
   collection named for the combo (e.g. `CHAR_guitar_fem_hairlong`), and add an
   entry to `SUBJECTS` in `stagegen.py`.
6. **Capture** — set `MODE = "capture"` and run; for each subject it bakes Line
   Art and exports the raw SVG, then writes a `<name>.json` metadata sidecar
   (display name, category, `mirror_for_left`). All SVG processing happens later
   in `generate.py --ingest`.
7. **Assemble** — run the generator's pack emit over the captured objects
   (`python3 generate.py --emit packs`) to produce the importable `objects.json`
   pack; handedness is mirrored in 2D there (no re-render). Load the result in
   stagePLTR via **Import Object Pack…**.

## 6. Automated capture

`stagegen.py` `MODE = "capture"` loops over `SUBJECTS`, and for each: isolates
its collection from the render/Line Art, attaches the listed instrument, bakes
Line Art, exports the raw SVG, and writes a metadata sidecar. Each subject is
**one complete character → one object** (sizing/cropping happen in `--ingest`).
You capture every character you want to ship, but **never** a left-handed twin —
handedness is the free 2-D mirror in `generate.py`. (Reuse pays off in authoring,
not rendering: a saved MPFB human preset + saved pose let you spin up a new
gender/ethnicity/style variant quickly, then capture it.)

## 7. Troubleshooting

- **Empty SVG / orthographic export:** there's a historical bug where GP SVG
  export produced an empty file under an orthographic camera. Render-test one
  frame first. If empty, switch the camera to Perspective with a very long focal
  length (250–500 mm) pulled far back — visually near-orthographic, exports
  reliably.
- **GP operator names:** the create / bake / export Grease Pencil operators in
  `stagegen.py` are flagged as version-sensitive. If one logs a failure, open
  the matching menu item once (Add > Grease Pencil; File > Export > Grease
  Pencil as SVG) to read the exact operator name for your build and update the
  function. The script prints the fallback for each.
- **Wrong bone:** if `attach_instrument` reports the bone is missing, it lists
  the rig's bones — pick the torso one and set `ANCHOR_BONE`.
- **Scale drift:** never change `ORTHO_SCALE` or units mid-project; all
  consistency derives from those staying fixed.
