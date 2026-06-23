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

## MakeHuman community asset packs (full catalog)
Source: <https://static.makehumancommunity.org/assets/assetpacks.html> — zip packs
of assets for MPFB2/MakeHuman; **MakeHuman system assets** is required. Every pack
is **CC0** or **CC-BY**; the tables below are the master credit list. **★ = used
by this project** (the per-character table records each figure's actual
obligations). For CC-BY packs with no individually-named creator, attribute the
pack itself ("MakeHuman community asset pack <name>, CC-BY-4.0").

### CC0 — no attribution required (credited as courtesy)
| Pack | Contents | Author |
|---|---|---|
| ★ MakeHuman system assets *(required)* | proxies, eyes, teeth… | — |
| System clothes materials 01 | extra clothes materials | — |
| System hair materials 01 | extra hair materials | — |
| Animal 01 | animal/furry transforms | — |
| Arms 01 | realistic arm deforms | — |
| Cheek 01 | realistic cheek deforms | — |
| Ears 01 | realistic ear deforms | — |
| Hands 01 | realistic hand deforms | — |
| Nose 01 | realistic nose deforms | — |
| ★ Poses 01 | sitting poses | — |
| ★ Poses 02 | sports poses | — |
| Skins 01 / 02 / 03 | natural female / male / non-natural skins | — |
| Bodyparts 01 | horns | — |
| Bodyparts 04 | nails | — |
| Bodyparts 05 | beards & moustaches | — |
| Dress 01 | female gowns & dresses | — |
| Equipment 01 | weapons | — |
| Eyebrows 01 | hi-res eyebrows | Mindfront |
| Eyelashes 01 | hi-res eyelashes | Mindfront |
| Glasses 01 | glasses | — |
| Gloves 01 | gloves | — |
| ★ Hair 01 | low-poly / stylized hair | — |
| Hats 01 | hats & caps | — |
| Hats 02 | helmets | — |
| ★ Masks 01 | masks | — |
| Pants 01 | pants | — |
| ★ Shirts 01 | t-shirts, sweaters, tops | — |
| Shoes 01 | shoes & boots | — |
| Skirts 01 | skirts | — |
| Suits 01 | formal suits | Margaret Toigo |
| Suits 02 | sci-fi / fantasy suits | — |
| Underwear 01 | female underwear | — |
| Underwear 04 | socks | — |
| Hair editor *(functional)* | hair editing | — |
| Visemes 01 / 02 *(functional)* | visemes | — |
| Faceunits 01 *(functional)* | face units | — |

### CC-BY — attribution required (credit the author)
| Pack | Contents | Author |
|---|---|---|
| Animal 02 | animal/furry details | JALdMIC |
| Animal 03 | animal/furry head deforms | JALdMIC |
| Animal 04 | animal/furry full-body transforms | JALdMIC |
| Bodyparts 02 | horns | _(pack — MakeHuman community)_ |
| Bodyparts 03 | tails & wings | _(pack)_ |
| Bodyparts 06 | beards & moustaches | _(pack)_ |
| Dress 02 | female gowns & dresses | Elvaerwyn |
| Dress 03 | female gowns & dresses | _(pack)_ |
| Equipment 02 | weapons | _(pack)_ |
| Equipment 03 | bags & tools | _(pack)_ |
| Glasses 02 | glasses | _(pack)_ |
| Hats 03 | hats & caps | _(pack)_ |
| Hats 04 | helmets | _(pack)_ |
| ★ Hair 02 | high-poly hair | Elvaerwyn |
| ★ Hair 03 | alpha-7 hair adaptations & misc | _(pack)_ |
| ★ Masks 02 | masks | _(pack)_ |
| Pants 02 | long-legged pants | _(pack)_ |
| Pants 03 | short-legged pants / swim trunks | _(pack)_ |
| ★ Shirts 02 | long-sleeved cardigans/sweaters/shirts | _(pack)_ |
| ★ Shirts 03 | short-sleeved shirts/tunics/tops | _(pack)_ |
| Shoes 02 | low shoes / sneakers / sandals | _(pack)_ |
| Shoes 03 | boots | _(pack)_ |
| Skirts 02 | skirts | _(pack)_ |
| Suits 03 | thematic suits | _(pack)_ |
| Underwear 02 | female underwear & bikinis | _(pack)_ |
| Underwear 03 | underwear & swimwear | _(pack)_ |

## Per-character provenance
Log which packs each shipped figure was built from, so the CC-BY credits trace to
specific icons (the catalog above is the master credit list).

| Figure | MPFB packs used | CC-BY to credit |
|---|---|---|
| _(e.g. Guitarist — Female)_ | system, Hair 02, Shirts 01 | Hair 02 (Elvaerwyn) |

## Other sources (not redistributable as 3D)
- **HumGen / DAZ:** paid EULAs permit distributing your **2D renders** (the SVG,
  no extractable mesh) but **not** the meshes/textures. Avoid DAZ items with the
  **Editorial License**. Icons OK; never commit the `.blend`/mesh.

_Not legal advice — verify current pack terms at the URL above._