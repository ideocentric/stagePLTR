# Asset provenance & licensing

So the license trail is auditable for an open-source release. We ship **2D SVG
derivatives** (line-art icons); the 3D working files stay in the gitignored
`blendsrc/` (PIPELINE.md §11). The MakeHuman-community source assets are all
**CC0 or CC-BY** — both redistributable — so the editable sources *may* also be
committed if desired.

## Licensing structure
- **Code:** GPLv3 (`LICENSE`).
- **Our SVG art** (figure/object icons we author): a CC art license in
  `LICENSE-assets` — **CC-BY-4.0** (or CC0).
- **Third-party MPFB assets:** CC0 or CC-BY per the table below. Because CC-BY
  attribution **carries to derivatives**, every figure icon built from a CC-BY
  pack must credit that pack's creator here — even though we ship only the SVG.
  Ship the **CC-BY-4.0 license text** (`LICENSE-CC-BY-4.0.txt` or folded into
  `LICENSE-assets`) since CC-BY requires the license travel with the work; CC0
  needs no bundled text but is marked below.

## MakeHuman community asset packs
Source: <https://static.makehumancommunity.org/assets/assetpacks.html>
(licenses vary per pack — verify on the page; the "01" base packs are CC0, the
"02/03" variants are typically CC-BY by third-party creators).

| Pack | License | Creator (for CC-BY) |
|---|---|---|
| MakeHuman system assets *(required)* | CC0 | — |
| System hair / clothes materials 01 | CC0 | — |
| Skins 01 / 02 / 03 | CC0 | — |
| Poses 01 / 02 | CC0 | — |
| Hair 01 | CC0 | — |
| Hair 02 | **CC-BY** | _(fill in from pack page)_ |
| Hair 03 | **CC-BY** | _(fill in)_ |
| Shirts 01 | CC0 | — |
| Shirts 02 / 03 | **CC-BY** | _(fill in)_ |
| Masks 01 | CC0 | — |
| Masks 02 | **CC-BY** | _(fill in)_ |

## Per-character provenance
Log which packs each shipped figure was built from, so the CC-BY credits are
traceable to specific icons (the pack table above is the master credit list).

| Figure / pack | MPFB packs used | Any CC-BY? |
|---|---|---|
| _(e.g. Guitarist — Female)_ | system, Hair 02, Shirts 01 | Hair 02 → credit |

## Other sources (not redistributable as 3D)
- **HumGen / DAZ:** paid EULAs permit distributing your **2D renders** (the SVG,
  no extractable mesh) but **not** the meshes/textures. Avoid DAZ items with the
  **Editorial License**. Icons OK; never commit the `.blend`/mesh.

_Not legal advice — verify current pack terms at the URL above._