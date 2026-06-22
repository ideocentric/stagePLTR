#!/usr/bin/env python3
"""Tests for generate.py (stdlib unittest, no dependencies).

    cd figures && python3 -m unittest test_generate -v
"""
import json
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

import generate

# A complete, normalized capture as stagegen.py would emit it: 200-frame, a
# <style> block, .ln/.lnf classes, and the north-flip wrapper.
CAPTURE_SVG = (
    "<?xml version='1.0' encoding='utf-8'?>\n"
    '<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200" '
    'viewBox="0 0 200 200">'
    "<style>.ln{fill:none;stroke:#111}.lnf{fill:#fff;stroke:#111}</style>"
    '<g transform="translate(0,200) scale(1,-1)">'
    '<path d="M70 80 L130 120" class="ln"/></g></svg>'
)


class CropForTests(unittest.TestCase):
    def setUp(self):
        self.manifest = {
            "canvas": {"width": 200, "height": 200},
            "output": {"instruments": {"guitar": {"footprint_units": [44, 42, 112, 116]}}},
        }

    def test_declared_footprint(self):
        self.assertEqual(generate.crop_for({"instrument": "guitar"}, self.manifest),
                         (44, 42, 112, 116))

    def test_left_handed_mirrors_x(self):
        # x' = 200 - 44 - 112 = 44 (the curated footprint is centred → symmetric)
        self.assertEqual(
            generate.crop_for({"instrument": "guitar", "handedness": "left"}, self.manifest),
            (44, 42, 112, 116))

    def test_missing_footprint_is_full_canvas(self):
        self.assertEqual(generate.crop_for({"instrument": "drums"}, self.manifest),
                         (0, 0, 200, 200))


class CroppedCaptureTests(unittest.TestCase):
    def test_mirror_flips_crop_and_wraps(self):
        root = ET.fromstring(CAPTURE_SVG)
        out = generate._cropped_capture(root, (60, 62, 80, 76), mirror=True, frame=200.0)
        self.assertEqual(out.get("viewBox"), "60 62 80 76")  # 200-60-80 = 60
        wrap = [c for c in out if c.tag.endswith("}g")
                and "scale(-1,1)" in (c.get("transform") or "")]
        self.assertEqual(len(wrap), 1)
        # the <style> stays at the top level, not inside the mirror wrapper
        self.assertTrue(any(c.tag.endswith("style") for c in out))

    def test_right_is_cropped_not_wrapped(self):
        root = ET.fromstring(CAPTURE_SVG)
        out = generate._cropped_capture(root, (60, 62, 80, 76), mirror=False, frame=200.0)
        self.assertEqual(out.get("viewBox"), "60 62 80 76")
        self.assertEqual((out.get("width"), out.get("height")), ("80", "76"))


class IngestTests(unittest.TestCase):
    def _capture_dir(self, tmp, mirror=True):
        d = Path(tmp)
        (d / "guitar_fem_punk.svg").write_text(CAPTURE_SVG, encoding="utf-8")
        (d / "guitar_fem_punk.footprint.json").write_text(json.dumps({
            "name": "Guitarist — Female, Punk", "category": "People",
            "footprint_units": [60, 62, 80, 76], "mirror_for_left": mirror,
        }), encoding="utf-8")
        return d

    def test_ingest_emits_pack_with_mirror(self):
        with tempfile.TemporaryDirectory() as cap, tempfile.TemporaryDirectory() as dist:
            self._capture_dir(cap)
            n = generate.ingest_objects(cap, Path(dist), "My Figures")
            self.assertEqual(n, 2)  # right + left
            pack = Path(dist) / "packs" / "my-figures"
            obj = json.loads((pack / "objects.json").read_text())
            self.assertEqual(obj["name"], "My Figures")
            self.assertTrue(obj["id"])  # stable GUID present
            ids = sorted(d["id"] for d in obj["devices"])
            self.assertEqual(ids, ["fig-guitar-fem-punk", "fig-guitar-fem-punk-left"])
            for d in obj["devices"]:
                self.assertEqual(d["defaultSize"], [80, 76])
                self.assertEqual(d["category"], "People")
                self.assertTrue((pack / d["icon"]).exists())

    def test_stable_guid_across_runs(self):
        with tempfile.TemporaryDirectory() as cap, \
             tempfile.TemporaryDirectory() as d1, tempfile.TemporaryDirectory() as d2:
            self._capture_dir(cap, mirror=False)
            generate.ingest_objects(cap, Path(d1), "My Figures")
            generate.ingest_objects(cap, Path(d2), "My Figures")
            id1 = json.loads((Path(d1) / "packs/my-figures/objects.json").read_text())["id"]
            id2 = json.loads((Path(d2) / "packs/my-figures/objects.json").read_text())["id"]
            self.assertEqual(id1, id2)

    def test_no_sidecar_falls_back_to_full_frame(self):
        with tempfile.TemporaryDirectory() as cap, tempfile.TemporaryDirectory() as dist:
            Path(cap, "lonely.svg").write_text(CAPTURE_SVG, encoding="utf-8")
            n = generate.ingest_objects(cap, Path(dist), "Pack")
            self.assertEqual(n, 1)
            obj = json.loads((Path(dist) / "packs/pack/objects.json").read_text())
            self.assertEqual(obj["devices"][0]["defaultSize"], [200, 200])


if __name__ == "__main__":
    unittest.main()