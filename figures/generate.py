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


def compose(traits: dict, manifest: dict) -> ET.Element:
    w = manifest["canvas"]["width"]
    h = manifest["canvas"]["height"]
    mirror_layers = set(manifest.get("mirror_layers", []))
    flip = traits.get("handedness") == "left"

    svg = ET.Element(f"{{{SVG_NS}}}svg", {
        "viewBox": f"0 0 {w} {h}",
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
        if flip and layer in mirror_layers:
            g.set("transform", f"translate({w},0) scale(-1,1)")
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
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="Generate overhead line-art musician figures.")
    ap.add_argument("--manifest", default=str(ROOT / "manifest.json"))
    ap.add_argument("--out", default=str(OUT_DIR))
    ap.add_argument("--filter", action="append", default=[], metavar="AXIS=VALUE",
                    help="restrict matrix, e.g. --filter style=punk (repeatable)")
    ap.add_argument("--limit", type=int, default=0, help="cap number of files (0 = no cap)")
    ap.add_argument("--dry-run", action="store_true", help="list combos, write nothing")
    ap.add_argument("--no-index", action="store_true", help="do not write out/index.html")
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
