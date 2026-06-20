# Asset provenance & licensing

So the license trail is auditable for an open-source release. The rule
(PIPELINE.md §11): **commit 2D derivatives and your own work; never commit
purchased/licensed 3D source assets** (meshes, textures, `.duf`/`.dsf`, `.blend`,
`.mhm`, `.fbx`). Those stay in the gitignored `.blendsrc/`.

stagePLTR's code and this SVG art are **dual-licensed** — code under the app's
license, the SVG art under a separate art license (CC-BY-4.0 or CC0) noted here.

| Asset | Source / Tool | License | In repo? |
|---|---|---|---|
| Base figures | MakeHuman 1.2 / MB-Lab | CC0 (verify current terms) | 2D SVG only |
| Hair shells | MakeHuman / own modeling | CC0 / your art | 2D SVG only |
| Instrument & object icons | your own work | (art license, e.g. CC-BY-4.0) | yes (SVG) |
| _(if ever used)_ DAZ hair/morphs | DAZ store | DAZ EULA — 2D render only | 2D SVG only, no mesh |

Notes:
- MakeHuman / MB-Lab output is permissive/CC0 — the recommended path for a clean
  public release. Keep working files out of git regardless.
- If you ever supplement with DAZ assets: DAZ's terms permit distributing your
  **2D renders** (the SVG, from which no mesh can be extracted) but **not** the
  meshes/textures. Avoid items with the **Editorial License**. Not legal advice —
  a DAZ sales-support ticket describing open-source 2D use removes ambiguity.