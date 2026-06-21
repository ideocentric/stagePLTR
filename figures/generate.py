#!/usr/bin/env python3
"""
stageplot-figures generator
============================
Composes a layered overhead line-art SVG figure for every combination of the
variation axes defined in manifest.json. Output is deterministic, version-
controllable, and dependency-free (Python 3 stdlib only).

Usage:
  python3 generate.py                      # build the full matrix into out/
  python3 generate.py --dry-run            # list combinations, write nothing
  python3 generate.py --filter style=punk  # only build matching combos
  python3 generate.py --filter style=punk --filter gender=female
  python3 generate.py --limit 24           # cap the number of files (preview)
  python3 generate.py --no-index           # skip out/index.html
  python3 generate.py --emit builtins      # curated built-ins -> dist/builtins/
  python3 generate.py --emit packs         # full matrix as one importable pack
  python3 generate.py --emit packs --pack-by style   # one pack per genre
  python3 generate.py --emit all --pack-by style     # built-ins + per-genre packs

Filenames are deterministic: <instrument>_<gender>_<ethnicity>_<hand>_<style>.svg
Swap manifest.json to JSON-with-comments or YAML at will; only load_manifest()
needs to change.
"""
from __future__ import annotations

import argparse
import itertools
import json
import logging
import sys
import xml.etree.ElementTree as ET
from copy import deepcopy
from pathlib import Path

SVG_NS = "http://www.w3.org/2000/svg"
ET.register_namespace("", SVG_NS)  # emit clean <svg>/<path>, not <ns0:path>

ROOT = Path(__file__).resolve().parent
PARTS_DIR = ROOT / "parts"
OUT_DIR = ROOT / "out"

log = logging.getLogger("stageplot")


# --------------------------------------------------------------------------- #
# Manifest
# --------------------------------------------------------------------------- #
def load_manifest(path: Path) -> dict:
    with path.open(encoding="utf-8") as fh:
        return json.load(fh)


def resolve_part(rules: list[dict], traits: dict) -> str | None:
    """First rule whose `when` is a subset of traits wins. {} matches anything."""
    for rule in rules:
        when = rule.get("when", {})
        if all(traits.get(k) == v for k, v in when.items()):
            return rule.get("use")
    return None


# --------------------------------------------------------------------------- #
# Part loading (cached) + composition
# --------------------------------------------------------------------------- #
_part_cache: dict[str, list[ET.Element]] = {}


def load_part_children(rel_path: str) -> list[ET.Element]:
    """Return the drawing elements (children of <svg>) for a part file."""
    if rel_path in _part_cache:
        return _part_cache[rel_path]
    fp = PARTS_DIR / rel_path
    if not fp.exists():
        raise FileNotFoundError(f"part not found: {fp}")
    tree = ET.parse(fp)
    children = list(tree.getroot())
    _part_cache[rel_path] = children
    return children


def crop_for(traits: dict, manifest: dict) -> tuple[int, int, int, int]:
    """The object's footprint rect (x, y, w, h) in the 200-unit frame.

    Per spec §6 each object declares `output.instruments.<instrument>.
    footprint_units`; the output viewBox is set to it (tight crop) and
    `defaultSize` = its [w, h] (1 unit -> 1 px). Falls back to the full canvas.
    Left-handed mirrors the figure about the canvas centre, so the crop's x is
    mirrored too — keeps an asymmetric footprint aligned to its content.
    """
    cw = manifest["canvas"]["width"]
    ch = manifest["canvas"]["height"]
    fp = (manifest.get("output", {}).get("instruments", {})
          .get(traits.get("instrument"), {}).get("footprint_units"))
    if not (isinstance(fp, list) and len(fp) == 4):
        return (0, 0, cw, ch)
    x, y, w, h = (int(v) for v in fp)
    if traits.get("handedness") == "left":
        x = cw - x - w
    return (x, y, w, h)


def compose(traits: dict, manifest: dict) -> ET.Element:
    cw = manifest["canvas"]["width"]
    mirror_layers = set(manifest.get("mirror_layers", []))
    flip = traits.get("handedness") == "left"

    x, y, w, h = crop_for(traits, manifest)
    svg = ET.Element(f"{{{SVG_NS}}}svg", {
        "viewBox": f"{x} {y} {w} {h}",
        "width": str(w),
        "height": str(h),
    })
    # Metadata so generated files are self-describing.
    for k, v in traits.items():
        svg.set(f"data-{k}", str(v))

    style = ET.SubElement(svg, f"{{{SVG_NS}}}style")
    style.text = manifest["style_def"]

    for layer in manifest["layer_order"]:
        rules = manifest["layers"].get(layer, [])
        part = resolve_part(rules, traits)
        if not part:
            continue
        g = ET.SubElement(svg, f"{{{SVG_NS}}}g", {"id": layer})
        # Mirror about the full-canvas centre (not the crop) so left-handed
        # parts still register to the right-handed source geometry.
        if flip and layer in mirror_layers:
            g.set("transform", f"translate({cw},0) scale(-1,1)")
        for child in load_part_children(part):
            g.append(deepcopy(child))
    return svg


def write_svg(elem: ET.Element, dest: Path) -> None:
    dest.write_bytes(
        b'<?xml version="1.0" encoding="UTF-8"?>\n'
        + ET.tostring(elem, encoding="utf-8")
    )


# --------------------------------------------------------------------------- #
# Matrix
# --------------------------------------------------------------------------- #
def iter_combinations(manifest: dict, filters: dict[str, str]):
    axes = manifest["axes"]
    names = list(axes.keys())
    for values in itertools.product(*(axes[n] for n in names)):
        combo = dict(zip(names, values))
        if all(combo.get(k) == v for k, v in filters.items()):
            yield combo


def filename(combo: dict) -> str:
    return (f"{combo['instrument']}_{combo['gender']}_{combo['ethnicity']}"
            f"_{combo['handedness']}_{combo['style']}.svg")


def build_index(files: list[str], dest: Path) -> None:
    cells = "\n".join(
        f'<figure><img src="{f}" width="120" height="120" loading="lazy">'
        f'<figcaption>{f[:-4]}</figcaption></figure>'
        for f in sorted(files)
    )
    dest.write_text(f"""<!doctype html><meta charset="utf-8">
<title>stageplot-figures preview</title>
<style>
 body{{font:13px/1.4 ui-monospace,monospace;background:#fafafa;color:#111;margin:24px}}
 h1{{font-size:15px}}
 .grid{{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:12px}}
 figure{{margin:0;background:#fff;border:1px solid #ddd;border-radius:6px;padding:8px;text-align:center}}
 img{{display:block;margin:0 auto;background:#fff}}
 figcaption{{font-size:10px;color:#555;margin-top:6px;word-break:break-all}}
</style>
<h1>stageplot-figures &middot; {len(files)} variants</h1>
<div class="grid">
{cells}
</div>
""", encoding="utf-8")


# --------------------------------------------------------------------------- #
# Catalog / pack emission (step b): footprint -> defaultSize, app-shaped JSON
# --------------------------------------------------------------------------- #
# Display strings for traits that read better than their raw axis values.
_ETHNICITY_LABEL = {
    "default": None, "black": "Black", "eastasian": "East Asian",
    "southasian": "South Asian", "latino": "Latino",
}


def _instrument_meta(combo: dict, manifest: dict) -> dict:
    return (manifest.get("output", {}).get("instruments", {})
            .get(combo["instrument"], {}))


def device_id(combo: dict) -> str:
    """Stable, namespaced id (distinct from hand-authored built-in ids)."""
    return ("fig-" + filename(combo)[:-4]).replace("_", "-")


def device_name(combo: dict, manifest: dict) -> str:
    base = _instrument_meta(combo, manifest).get("name") or combo["instrument"].title()
    bits = [combo["gender"].title()]
    eth = _ETHNICITY_LABEL.get(combo["ethnicity"], combo["ethnicity"].title())
    if eth:
        bits.append(eth)
    bits.append(combo["style"].title())
    if combo["handedness"] == "left":
        bits.append("Left-handed")
    return f"{base} — {', '.join(bits)}"


def device_entry(combo: dict, manifest: dict, icon: str) -> dict:
    """One catalog/pack device record, shaped for the app's parseDeviceType."""
    _, _, w, h = crop_for(combo, manifest)
    return {
        "id": device_id(combo),
        "name": device_name(combo, manifest),
        "category": _instrument_meta(combo, manifest).get("category", "Other"),
        "icon": icon,
        "defaultSize": [w, h],
        "ports": [],  # figures are physical placements, not signal sources
    }


def _write_svgs(combos: list[dict], manifest: dict, dest: Path) -> list[str]:
    dest.mkdir(parents=True, exist_ok=True)
    names = []
    for combo in combos:
        try:
            svg = compose(combo, manifest)
        except FileNotFoundError as e:
            log.error("skip %s: %s", filename(combo), e)
            continue
        name = filename(combo)
        write_svg(svg, dest / name)
        names.append(name)
    return names


def emit_builtins(manifest: dict, dist_dir: Path) -> int:
    """Curated built-ins: SVGs + a drop-in `catalog.json` fragment.

    `output.builtin` lists the combos that ship with the app. The fragment's
    `devices` merge into assets/plot/catalog.json and the SVGs copy alongside
    it (flat `icon` filenames -> no path edits). Categories listed for ordering.
    """
    combos = manifest.get("output", {}).get("builtin", [])
    if not combos:
        log.warning("no output.builtin combos in manifest; nothing to emit")
        return 0
    dest = dist_dir / "builtins"
    names = _write_svgs(combos, manifest, dest)
    devices = sorted((device_entry(c, manifest, filename(c)) for c in combos),
                     key=lambda d: d["id"])
    categories = list(dict.fromkeys(d["category"] for d in devices))
    fragment = {
        "version": 1,
        "_comment": ("Curated figure built-ins (generated by figures/generate.py). "
                     "Merge 'devices' into assets/plot/catalog.json and copy these "
                     "SVGs into assets/plot/ (then add them to resources.qrc)."),
        "categories": categories,
        "devices": devices,
    }
    (dest / "catalog.json").write_text(
        json.dumps(fragment, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    log.info("emitted %d built-in(s) + catalog.json to %s", len(names), dest)
    return len(names)


def _slug(value: str) -> str:
    return "".join(c if c.isalnum() else "-" for c in value.lower()).strip("-")


def emit_packs(manifest: dict, dist_dir: Path, combos: list[dict],
               pack_by: str | None) -> int:
    """Object packs: per group, a dir of SVGs + `objects.json` (library shape).

    Each pack is `{version, name, devices:[...]}` with flat `icon` filenames —
    the same shape stagePLTR's user library writes, so "Import Object Pack…"
    loads it directly. `pack_by` splits the matrix along one axis (e.g. style);
    omitted -> a single pack of the whole (filtered) matrix.
    """
    groups: dict[str, list[dict]] = {}
    for combo in combos:
        key = combo[pack_by] if pack_by else "all"
        groups.setdefault(key, []).append(combo)

    instr_names = sorted({_instrument_meta(c, manifest).get("name")
                          or c["instrument"].title() for c in combos})
    label = ", ".join(instr_names) if instr_names else "Figures"

    total = 0
    for key, group in sorted(groups.items()):
        pack_name = f"{label} — {key.title()}" if pack_by else f"{label} (all)"
        dest = dist_dir / "packs" / (f"{_slug(label)}-{_slug(key)}" if pack_by
                                     else f"{_slug(label)}-all")
        names = _write_svgs(group, manifest, dest)
        devices = sorted((device_entry(c, manifest, filename(c)) for c in group),
                         key=lambda d: d["id"])
        objects = {
            "version": 1,
            "name": pack_name,
            "_comment": "stagePLTR object pack — Import Object Pack… loads these "
                        "into your user library.",
            "devices": devices,
        }
        (dest / "objects.json").write_text(
            json.dumps(objects, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        log.info("emitted pack '%s' (%d object(s)) to %s", pack_name, len(names), dest)
        total += len(names)
    return total


# --------------------------------------------------------------------------- #
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="Generate overhead line-art musician figures.")
    ap.add_argument("--manifest", default=str(ROOT / "manifest.json"))
    ap.add_argument("--out", default=str(OUT_DIR))
    ap.add_argument("--filter", action="append", default=[], metavar="AXIS=VALUE",
                    help="restrict matrix, e.g. --filter style=punk (repeatable)")
    ap.add_argument("--limit", type=int, default=0, help="cap number of files (0 = no cap)")
    ap.add_argument("--dry-run", action="store_true", help="list combos, write nothing")
    ap.add_argument("--no-index", action="store_true", help="do not write out/index.html")
    ap.add_argument("--emit", choices=["builtins", "packs", "all"],
                    help="emit app artifacts instead of the out/ preview: a "
                         "catalog.json fragment for curated built-ins, and/or "
                         "Import-Object-Pack object packs")
    ap.add_argument("--dist", default=str(ROOT / "dist"),
                    help="base dir for --emit artifacts (default figures/dist)")
    ap.add_argument("--pack-by", metavar="AXIS",
                    help="split packs along an axis, e.g. --pack-by style "
                         "(default: one pack for the whole matrix)")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)s %(message)s",
    )

    filters = {}
    for f in args.filter:
        if "=" not in f:
            log.error("bad --filter %r (expected AXIS=VALUE)", f)
            return 2
        k, v = f.split("=", 1)
        filters[k] = v

    manifest = load_manifest(Path(args.manifest))
    out_dir = Path(args.out)
    combos = list(iter_combinations(manifest, filters))
    if args.limit:
        combos = combos[: args.limit]

    log.info("matrix size: %d combination(s)%s", len(combos),
             f"  filters={filters}" if filters else "")

    if args.dry_run:
        for c in combos:
            log.info("  %s", filename(c))
        return 0

    if args.emit:
        if args.pack_by and args.pack_by not in manifest["axes"]:
            log.error("--pack-by %r is not an axis (choose from: %s)",
                      args.pack_by, ", ".join(manifest["axes"]))
            return 2
        dist_dir = Path(args.dist)
        n = 0
        if args.emit in ("builtins", "all"):
            n += emit_builtins(manifest, dist_dir)
        if args.emit in ("packs", "all"):
            n += emit_packs(manifest, dist_dir, combos, args.pack_by)
        log.info("emit complete: %d SVG(s) under %s", n, dist_dir)
        return 0

    out_dir.mkdir(parents=True, exist_ok=True)
    written: list[str] = []
    for combo in combos:
        try:
            svg = compose(combo, manifest)
        except FileNotFoundError as e:
            log.error("skip %s: %s", filename(combo), e)
            continue
        name = filename(combo)
        write_svg(svg, out_dir / name)
        written.append(name)
        log.debug("wrote %s", name)

    log.info("wrote %d file(s) to %s", len(written), out_dir)

    if not args.no_index and written:
        build_index(written, out_dir / "index.html")
        log.info("wrote %s", out_dir / "index.html")

    return 0


if __name__ == "__main__":
    sys.exit(main())
