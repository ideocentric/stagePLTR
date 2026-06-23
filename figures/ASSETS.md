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
- **BlenderKit Royalty-Free** (instrument sources): 2D derivatives are
  distributable; the 3D models are not. See the BlenderKit section below.

## MakeHuman community asset packs (full catalog)
Source: <https://static.makehumancommunity.org/assets/assetpacks.html> — zip packs
of assets for MPFB2/MakeHuman; **MakeHuman system assets** is required. Every pack
is **CC0** or **CC-BY**; the tables below are the master credit list. **★ = used
by this project** (the per-character table records each figure's actual
obligations). Authors are taken from each pack's page; most packs are community
collections, so multiple contributors are credited (handles, as listed on the
pack pages).

### CC0 — no attribution required (credited as courtesy)
| Pack | Contents | Contributors |
|---|---|---|
| ★ MakeHuman system assets *(required)* | proxies, eyes, teeth… | not individually credited |
| System clothes materials 01 | clothes materials | MargaretToigo |
| System hair materials 01 | hair materials | bogdan666, culturalibre, dariush086, Joel Palmius, MTKnife, MargaretToigo |
| Animal 01 | animal/furry transforms | culturalibre, Elvaerwyn, JALdMIC, titleknown |
| Arms 01 | arm deforms | Elvaerwyn, Mindfront |
| Cheek 01 | cheek deforms | Elvaerwyn |
| Ears 01 | ear deforms | Elvaerwyn, jujube, Mindfront, RehmanPolanski |
| Hands 01 | hand deforms | jujube, Mindfront |
| Nose 01 | nose deforms | Elvaerwyn, Habanero, jujube, Mindfront |
| ★ Poses 01 | sitting poses | Anrico, callharvey3d, Elvaerwyn, milkman, Mindfront, sweetan008, wolgade, xhado84 |
| ★ Poses 02 | sports poses | callharvey3d, culturalibre, Elvaerwyn, gschabau, Henny, joachip, punkduck |
| Skins 01 | natural female skins | blindsaypatten, bobby_03, callharvey3d, CutOff3D, darthfurby, flower-angel, Nyloseth, OnlyTheGhosts, saltycowdawg, skalldyrssuppe, spreadcore, MargaretToigo |
| Skins 02 | natural male skins | jartur69, ken1138, Mindfront, OnlyTheGhosts, RehmanPolanski, MargaretToigo |
| Skins 03 | non-natural skins | bobby_03, bogdan666, callharvey3d, culturalibre, joepal, naim_abbassi, reizibarrientos, sohh, sonntag78, spreadcore, titleknown, trashhunter, xhado84 |
| Bodyparts 01 | horns | culturalibre, FreezyChan, JALdMIC |
| Bodyparts 04 | nails | grinsegold, Mindfront, MargaretToigo, wolgade |
| Bodyparts 05 | beards & moustaches | culturalibre, grinsegold, RehmanPolanski, WDG |
| Dress 01 | female gowns & dresses | Aethelraed_Unraed, Mindfront, MargaretToigo, WDG |
| Equipment 01 | weapons | culturalibre, Joel Palmius, o4saken |
| Eyebrows 01 | hi-res eyebrows | Mindfront |
| Eyelashes 01 | hi-res eyelashes | Mindfront |
| Glasses 01 | glasses | culturalibre, EWS, frankyaye, kwnet_at, spamrakuen, MargaretToigo |
| Gloves 01 | gloves | culturalibre, learning, MargaretToigo |
| ★ Hair 01 | low-poly / stylized hair | Cortu, culturalibre, Elvaerwyn, Faydaen, learning, littleright, punkduck, RehmanPolanski, sonntag78, MargaretToigo |
| Hats 01 | hats & caps | Aethelraed_Unraed, grinsegold, Joel Palmius, jujube, MargaretToigo |
| Hats 02 | helmets | culturalibre, grinsegold, javherre, MrGreaterThan |
| ★ Masks 01 | masks | culturalibre, Joel Palmius |
| Pants 01 | pants | Cortu, MargaretToigo |
| ★ Shirts 01 | t-shirts, sweaters, tops | Elvaerwyn, Joel Palmius, namuhekam, skalldyrssuppe, MargaretToigo |
| Shoes 01 | shoes & boots | Cortu, culturalibre, grinsegold, scailman, MargaretToigo |
| Skirts 01 | skirts | frankyaye, MargaretToigo |
| Suits 01 | formal suits | Margaret Toigo |
| Suits 02 | sci-fi / fantasy suits | culturalibre, Donitz, green_tomato, joachip, MatCreator, RehmanPolanski, Slayer227, TheGreatEngineer |
| Underwear 01 | female underwear | not individually credited |
| Underwear 04 | socks | Joel Palmius, MargaretToigo |
| Hair editor *(functional)* | hair editing | Tomáš Klecer |
| Visemes 01 / 02 *(functional)* | visemes | Mika Suominen |
| Faceunits 01 *(functional)* | face units | Mika Suominen |

### CC-BY — attribution required (credit the contributors)
| Pack | Contents | Contributors |
|---|---|---|
| Animal 02 | animal/furry details | JALdMIC |
| Animal 03 | animal/furry head deforms | JALdMIC |
| Animal 04 | animal/furry full-body transforms | JALdMIC |
| Bodyparts 02 | horns | Elvaerwyn, JALdMIC |
| Bodyparts 03 | tails & wings | culturalibre, Elvaerwyn |
| Bodyparts 06 | beards & moustaches | culturalibre, Elvaerwyn, grinsegold |
| Dress 02 | female gowns & dresses | Elvaerwyn |
| Dress 03 | female gowns & dresses | Mindfront, punkduck |
| Equipment 02 | weapons | culturalibre, JALdMIC, MaciekG, punkduck |
| Equipment 03 | bags & tools | culturalibre, Elvaerwyn, Mindfront, punkduck |
| Glasses 02 | glasses | Elvaerwyn, punkduck |
| Hats 03 | hats & caps | culturalibre, Elvaerwyn, MaciekG, Mindfront, punkduck |
| Hats 04 | helmets | culturalibre, DredNicolson, JALdMIC, MaciekG, Mindfront, punkduck |
| ★ Hair 02 | high-poly hair | Elvaerwyn |
| ★ Hair 03 | alpha-7 hair adaptations & misc | culturalibre, grinsegold, punkduck |
| ★ Masks 02 | masks | culturalibre, Elvaerwyn, Mathias_Gredal |
| Pants 02 | long-legged pants | Elvaerwyn, Mindfront, punkduck |
| Pants 03 | short-legged pants / swim trunks | Elvaerwyn, Mindfront, punkduck |
| ★ Shirts 02 | long-sleeved cardigans/sweaters/shirts | Elvaerwyn, EWS, janexx, Mindfront, punkduck |
| ★ Shirts 03 | short-sleeved shirts/tunics/tops | DredNicolson, Elvaerwyn, Mindfront, punkduck |
| Shoes 02 | low shoes / sneakers / sandals | callharvey3d, culturalibre, DressUpDoc, Elvaerwyn, Mindfront, punkduck |
| Shoes 03 | boots | Elvaerwyn, MaciekG, madmanny, Mindfront, punkduck, scailman |
| Skirts 02 | skirts | callharvey3d, Elvaerwyn, MTKnife, punkduck, scsouza, sonicteam998 |
| Suits 03 | thematic suits | culturalibre, Elvaerwyn, grinsegold, MaciekG, punkduck |
| Underwear 02 | female underwear & bikinis | Elvaerwyn, JALdMIC, Mindfront, punkduck |
| Underwear 03 | underwear & swimwear | Elvaerwyn, Joel Palmius, Mindfront, punkduck |

## Per-character provenance
Log which packs each shipped figure was built from, so the CC-BY credits trace to
specific icons (the catalog above is the master credit list).

| Figure | MPFB packs used | CC-BY to credit |
|---|---|---|
| _(e.g. Guitarist — Female)_ | system, Hair 02, Shirts 01 | Hair 02 (Elvaerwyn) |

## BlenderKit (Royalty-Free) instruments
Instrument icons may be captured from **BlenderKit** (paid subscription) models via
the same Blender stage, for visual consistency with the figures. Per BlenderKit's
licensing FAQ, **Royalty-Free** assets allow commercial use, no attribution
required, and explicitly permit **2D renders/derivatives** (even sold on stock
sites) — the only bar is redistributing the asset **in 3D form** or in a way
**users can easily extract**. A line-art SVG is non-extractable 2D, so it ships
cleanly (and may be CC-BY-4.0 — downstream only ever gets the 2D).

**Rules:** ship only the SVG; **never commit the BlenderKit `.blend`/model** (it
stays in gitignored `blendsrc/`). These icons are *terminal* — distributable, but
not regenerable by others without their own BlenderKit subscription. RF needs no
attribution, but we credit authors here as a courtesy (BlenderKit's FAQ asks the
same).

| Instrument icon | BlenderKit asset | Author | License |
|---|---|---|---|
| _(e.g. Electric Guitar)_ | _(asset name)_ | _(author handle)_ | Royalty Free — 2D derivative only, model not redistributed |

## Other sources (not redistributable as 3D)
- **HumGen / DAZ:** paid EULAs permit distributing your **2D renders** (the SVG,
  no extractable mesh) but **not** the meshes/textures. Avoid DAZ items with the
  **Editorial License**. Icons OK; never commit the `.blend`/mesh.

_Not legal advice — verify current pack/asset terms with each source._