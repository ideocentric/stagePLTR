# Workflow — Authoring a Figure Icon, Step by Step

The concrete checklist for producing a new overhead line-art musician icon with
Blender + MPFB2 and `stagegen.py`. Background and rationale live in
[REQUIREMENTS.md](REQUIREMENTS.md), [../PIPELINE.md](../PIPELINE.md), and the
output contract in [../spec.md](../spec.md) §11. Steps are marked **one-time** or
**per-character** so you know what repeats.

## Where files live (read first)

Keep the **entire Blender project inside `figures/.blendsrc/`** — it is git-ignored
(so are all `*.blend`/`*.mhm`/`*.fbx`), which keeps working files and licensed MPFB
assets out of the public repo per `PIPELINE.md` §11.

```
figures/.blendsrc/
├── stageplot.blend         # your template + characters (save here)
└── svg_out/                # captures land here (OUT_DIR = "//svg_out/")
```

Because `OUT_DIR = "//svg_out/"` is relative to the `.blend`, saving the `.blend`
in `figures/.blendsrc/` puts the captures in `figures/.blendsrc/svg_out/` — all
ignored. The committed artifacts are only the *normalized SVGs we choose to ship*
(a generated pack), never the `.blend` or raw `svg_out`.

---

## Part A — Install (one-time)

1. Install **Blender 4.3+** (4.4 / 5.0 fine; macOS / Apple Silicon OK).
2. Install **MPFB2**: Preferences ▸ Extensions ▸ search "MPFB" ▸ install, restart.
3. In MPFB, load asset packs (*apply assets ▸ library settings ▸ load pack from
   zip*), then restart:
   - **makehuman_system_assets** — CC0, required for MPFB to function.
   - **Hair 01 / 02 / 03** — the primary overhead gender/ethnicity/style signal.
   - a **Shirts** and/or **Masks** pack — shoulder-level silhouette cues.
4. *(Optional)* **NijiGPen** add-on for 2-D stroke cleanup.
5. Record the license of each hairstyle/asset you actually use in
   [../ASSETS.md](../ASSETS.md).

## Part B — Build the stage (one-time per .blend)

6. Scripting workspace ▸ Open ▸ `figures/blender-flow/stagegen.py`.
7. Confirm `MODE = "build"` (bottom of the file) ▸ **Run Script** (▶ / Alt+P).
8. **Check the console:**
   - If `ensure_lineart_object` logs a fallback → **Add ▸ Grease Pencil ▸ Scene
     Line Art**, rename it `StageLineArt`, re-run build. (Note the operator name.)
   - Look down the camera (Numpad 0). If you see the *underside* of the head, set
     `TILT_DEG = -8.0` and re-run.
9. **Calibrate scale once:** render a known-length object and confirm in the GP
   SVG export that **1 unit = 10 mm** (`spec.md` §1). Lock that exporter setting.
10. **Save** the file as your template in `figures/.blendsrc/stageplot.blend`.

## Part C — Author a character (per-character)

11. **Create human:** MPFB ▸ *new human ▸ from scratch*; set gender / age / build /
    ethnicity sliders. Save as an MPFB human preset for reuse.
12. **Hair + wardrobe:** add a hairstyle (and a shoulder-level top / mask if the
    genre needs it).
13. **Pose:** pose ~6–10 bones (spine, neck, head, upper arms, forearms, hands)
    into the playing posture; save via *create assets ▸ MakePose* for reuse. Pose
    **right-handed only** — left is the free 2-D mirror later.
14. **Add the instrument:** model/import it into the `INSTRUMENT` collection, place
    it in playing position. Verify your rig's torso bone name and set `ANCHOR_BONE`
    (the script prints the rig's bones if the name is wrong). From the console run
    `attach_instrument("guitar", "Human", "spine03")`, then pose the **hands onto**
    the instrument.
15. **Isolate:** put that character's body + hair + instrument in one collection
    named for the combo, e.g. `CHAR_guitar_fem_punk`.

## Part D — Capture (per batch)

16. In `stagegen.py`, list the character(s) in `SUBJECTS`:
    ```python
    SUBJECTS = [
        dict(name="guitar_fem_punk", collection="CHAR_guitar_fem_punk",
             instrument="guitar", armature="Human", bone=ANCHOR_BONE),
    ]
    ```
17. Add optional metadata to each `SUBJECTS` entry — `display="Guitarist —
    Female, Punk"`, `category="People"`, `mirror_for_left=True` — so it travels to
    the pack.
18. Set `MODE = "capture"` ▸ Run Script. Per subject it isolates → attaches →
    exports the **raw** Grease-Pencil SVG → writes a `<name>.json` metadata
    sidecar, into `svg_out/`. (No bake needed on 5.1.) Watch the console for any
    export fallback message (note the operator name).

## Part E — Assemble + load into the app (per batch)

19. Build the importable pack from the captures:
    ```bash
    cd figures
    python3 generate.py --ingest .blendsrc/svg_out --pack-name "My Figures"
    ```
    → `dist/packs/my-figures/objects.json` + the cropped object SVGs.
20. In stagePLTR: **File ▸ Import Object Pack…** → pick that `objects.json` → the
    figures appear in the **People** palette section. Add a left-handed mirror for
    any object by setting `"mirror_for_left": true` in its sidecar before step 19.

---

## First-pass shortcut

For the very first run, do **A → B → one character in C → capture one subject in
D → ingest in E**. The capture format and ingest are already validated against
Blender 5.1, so this should produce a usable object. If anything trips:

- paste any **console fallback message** from the GP create (step 8) or SVG
  export (step 18) — these are the only version-sensitive operators left, and
- if the imported figure looks wrong (size/orientation), send the raw `svg_out`
  SVG and we'll adjust `generate.py --ingest`.