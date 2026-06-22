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

# A Blender-GP-style capture: a 1024-px frame of FILLED black paths
# (fill=#000000, stroke=none), one rectangle whose content bbox is
# x=300 y=400 w=400 h=400.
CAPTURE_SVG = (
    "<?xml version='1.0' encoding='UTF-8'?>\n"
    '<svg xmlns="http://www.w3.org/2000/svg" width="1024px" height="1024px" '
    'viewBox="0 0 1024 1024">'
    '<g id="blender_frame.0"><g id="blender_object.StageLineArt">'
    '<path d="M300,400L700,400L700,800L300,800z" fill="#000000" stroke="none"/>'
    "</g></g></svg>"
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
        self.assertEqual(
            generate.crop_for({"instrument": "guitar", "handedness": "left"}, self.manifest),
            (44, 42, 112, 116))

    def test_missing_footprint_is_full_canvas(self):
        self.assertEqual(generate.crop_for({"instrument": "drums"}, self.manifest),
                         (0, 0, 200, 200))


class CaptureHelperTests(unittest.TestCase):
    def test_content_bbox_from_paths_only(self):
        # The id="blender_frame.0" etc. must NOT pollute the geometry bbox.
        root = ET.fromstring(CAPTURE_SVG)
        self.assertEqual(generate._content_bbox(root), (300.0, 400.0, 400.0, 400.0))

    def test_src_frame_from_viewbox(self):
        self.assertEqual(generate._src_frame(ET.fromstring(CAPTURE_SVG)), 1024.0)

    def test_recolor_fills_to_ink(self):
        root = ET.fromstring(CAPTURE_SVG)
        generate._recolor(root)
        out = ET.tostring(root, encoding="unicode")
        self.assertIn(generate.INK, out)
        self.assertNotIn("#000000", out)

    def test_object_svg_crops_in_source_space(self):
        root = ET.fromstring(CAPTURE_SVG)
        out = generate._object_svg(root, (300, 400, 400, 400), mirror=False, frame=1024)
        self.assertEqual(out.get("viewBox"), "300 400 400 400")
        self.assertEqual((out.get("width"), out.get("height")), ("400", "400"))

    def test_object_svg_mirror_flips_crop_and_wraps(self):
        root = ET.fromstring(CAPTURE_SVG)
        out = generate._object_svg(root, (300, 400, 400, 400), mirror=True, frame=1024)
        self.assertEqual(out.get("viewBox"), "324 400 400 400")  # 1024-300-400
        wrap = [c for c in out if c.tag.endswith("}g")
                and "translate(1024,0) scale(-1,1)" == c.get("transform")]
        self.assertEqual(len(wrap), 1)


class IngestTests(unittest.TestCase):
    def _capture_dir(self, tmp, mirror=True, sidecar=True):
        d = Path(tmp)
        (d / "female_guitarist.svg").write_text(CAPTURE_SVG, encoding="utf-8")
        if sidecar:
            (d / "female_guitarist.json").write_text(json.dumps({
                "name": "Guitarist — Female", "category": "People",
                "mirror_for_left": mirror,
            }), encoding="utf-8")
        return d

    def test_ingest_real_format(self):
        with tempfile.TemporaryDirectory() as cap, tempfile.TemporaryDirectory() as dist:
            self._capture_dir(cap)
            n = generate.ingest_objects(cap, Path(dist), "My Figures")
            self.assertEqual(n, 2)  # right + left
            pack = Path(dist) / "packs" / "my-figures"
            obj = json.loads((pack / "objects.json").read_text())
            self.assertEqual(obj["name"], "My Figures")
            self.assertTrue(obj["id"])
            ids = sorted(d["id"] for d in obj["devices"])
            self.assertEqual(ids, ["fig-female-guitarist", "fig-female-guitarist-left"])
            for d in obj["devices"]:
                # 400 px in a 1024 frame -> round(400 * 200/1024) = 78 units
                self.assertEqual(d["defaultSize"], [78, 78])
                self.assertTrue((pack / d["icon"]).exists())
            # the shipped object SVG is recoloured + cropped
            svg = (pack / "fig-female-guitarist.svg").read_text()
            self.assertIn('viewBox="300 400 400 400"', svg)
            self.assertIn(generate.INK, svg)
            self.assertNotIn("#000000", svg)

    def test_no_sidecar_uses_filename(self):
        with tempfile.TemporaryDirectory() as cap, tempfile.TemporaryDirectory() as dist:
            self._capture_dir(cap, mirror=False, sidecar=False)
            n = generate.ingest_objects(cap, Path(dist), "Pack")
            self.assertEqual(n, 1)  # no sidecar -> no mirror
            obj = json.loads((Path(dist) / "packs/pack/objects.json").read_text())
            self.assertEqual(obj["devices"][0]["id"], "fig-female-guitarist")
            self.assertEqual(obj["devices"][0]["name"], "Female Guitarist")
            self.assertEqual(obj["devices"][0]["category"], "People")

    def test_stable_guid_across_runs(self):
        with tempfile.TemporaryDirectory() as cap, \
             tempfile.TemporaryDirectory() as d1, tempfile.TemporaryDirectory() as d2:
            self._capture_dir(cap, mirror=False)
            generate.ingest_objects(cap, Path(d1), "My Figures")
            generate.ingest_objects(cap, Path(d2), "My Figures")
            id1 = json.loads((Path(d1) / "packs/my-figures/objects.json").read_text())["id"]
            id2 = json.loads((Path(d2) / "packs/my-figures/objects.json").read_text())["id"]
            self.assertEqual(id1, id2)


if __name__ == "__main__":
    unittest.main()