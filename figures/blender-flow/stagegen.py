"""
stagegen.py — Stage-Plot Figure toolkit for Blender + MPFB2
===========================================================
Run inside Blender (Scripting tab > Open > Run Script). Set MODE at the bottom.

Three jobs, one file:
  build    — construct the standardized "stage": metric units, white world, the
             locked orthographic camera at your tilt, render collections, and a
             Grease Pencil Line Art object. Run once per .blend (or ship the
             saved .blend; this script regenerates it reproducibly).
  attach   — rigidly bind an instrument object to a character's rig bone so the
             instrument follows the figure when it is reposed (no deformation).
  capture  — iterate prepared subjects, toggle visibility, bake Line Art, and
             export one SVG per subject with deterministic names.

WHY NOT SCRIPT MPFB ITSELF: character creation in MPFB is slider-driven and its
operators are version-fragile, so characters are made interactively and saved
(as MPFB human presets or as named collections). This script automates the
stable, repetitive part: staging, attaching, and capturing.

VERSION-SENSITIVE CALLS (Grease Pencil v3, Blender 4.3+): the three functions
ensure_lineart_object(), bake_lineart(), and export_svg() touch GP operators
whose names changed across versions. Each is wrapped and logged; if one fails,
the console prints the manual-menu fallback. Validate them once against your
Blender build (the menu items reveal the exact operator names).
"""
import math
import logging
import bpy

log = logging.getLogger("stagegen")
logging.basicConfig(level=logging.INFO, format="%(levelname)s %(message)s")

# =========================================================================== #
# CONFIG — edit these
# =========================================================================== #
TILT_DEG      = 8.0          # camera tilt from vertical (0 = true plan)
ORTHO_SCALE   = 2.0          # metres across the frame = 200 spec units (spec.md §1)
CAM_HEIGHT    = 5.0          # metres above pivot (orthographic, so distance is cosmetic)
PIVOT_Z       = 0.95         # tilt about ~hip height so the figure stays framed
RES           = 1024         # px, square
OUT_DIR       = "//svg_out/" # // = relative to this .blend
STROKE_PX     = 6            # Line Art stroke thickness
CREASE_ANGLE  = math.radians(60)

# Output contract (spec.md §1, §11): the captured SVG is normalized into a square
# 0 0 FRAME_UNITS frame where 1 unit = UNIT_MM mm. ORTHO_SCALE metres map to
# FRAME_UNITS, i.e. 2.0 m -> 200 units -> 1 unit = 10 mm.
FRAME_UNITS   = 200          # spec frame width/height in units
UNIT_MM       = 10           # millimetres per unit (ORTHO_SCALE*1000 / FRAME_UNITS)
# Must match manifest.json "style_def" so the generator's injected <style> drives
# the look; parts/objects carry classes, not inline stroke.
STYLE_DEF = (".ln{fill:none;stroke:#111;stroke-width:1.6;stroke-linejoin:round;"
             "stroke-linecap:round}"
             ".lnf{fill:#fff;stroke:#111;stroke-width:1.6;stroke-linejoin:round;"
             "stroke-linecap:round}")

ANCHOR_BONE   = "spine03"    # torso bone instruments attach to — VERIFY in your rig
FLIP_Y        = True         # GP export is Y-up; SVG is Y-down. VALIDATE per build.
LINEART_NAME  = "StageLineArt"
COL = dict(figure="FIGURE", hair="HAIR", instrument="INSTRUMENT",
           proxy="PROXY", lineart="LINEART")

# For capture mode: each subject is a prepared character. Put each character's
# body+hair+instrument in its own collection (or sub-collections) and list them.
SUBJECTS = [
    # dict(name="guitar_fem_hairlong_R", collection="CHAR_guitar_fem_hairlong",
    #      instrument="guitar", armature="Human", bone=ANCHOR_BONE),
]

# =========================================================================== #
# helpers
# =========================================================================== #
def get_or_create_collection(name):
    c = bpy.data.collections.get(name)
    if c is None:
        c = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(c)
    return c

def find_layer_collection(name, root=None):
    root = root or bpy.context.view_layer.layer_collection
    if root.name == name:
        return root
    for ch in root.children:
        r = find_layer_collection(name, ch)
        if r:
            return r
    return None

def set_collection_excluded(name, excluded):
    lc = find_layer_collection(name)
    if lc:
        lc.exclude = excluded

# =========================================================================== #
# build
# =========================================================================== #
def set_scene_basics():
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0          # 1 Blender unit = 1 metre
    scene.render.resolution_x = RES
    scene.render.resolution_y = RES
    scene.render.film_transparent = False
    # white world
    world = bpy.data.worlds.get("StageWorld") or bpy.data.worlds.new("StageWorld")
    world.use_nodes = False
    world.color = (1, 1, 1)
    scene.world = world
    log.info("scene: metric, 1 unit = 1 m, %dx%d", RES, RES)

def build_camera():
    scene = bpy.context.scene
    pivot = bpy.data.objects.get("TiltPivot")
    if pivot is None:
        pivot = bpy.data.objects.new("TiltPivot", None)
        scene.collection.objects.link(pivot)
    pivot.location = (0.0, 0.0, PIVOT_Z)
    pivot.rotation_euler = (math.radians(TILT_DEG), 0.0, 0.0)  # tilt the whole rig

    cdata = bpy.data.cameras.get("StageCam") or bpy.data.cameras.new("StageCam")
    cdata.type = "ORTHO"
    cdata.ortho_scale = ORTHO_SCALE
    cam = bpy.data.objects.get("StageCamObj")
    if cam is None:
        cam = bpy.data.objects.new("StageCamObj", cdata)
        scene.collection.objects.link(cam)
    cam.data = cdata
    cam.parent = pivot
    cam.location = (0.0, 0.0, CAM_HEIGHT)   # default camera looks down -Z
    cam.rotation_euler = (0.0, 0.0, 0.0)
    scene.camera = cam
    log.info("camera: ortho_scale=%.2f, tilt=%.1f deg from vertical", ORTHO_SCALE, TILT_DEG)

def build_collections():
    for key, name in COL.items():
        get_or_create_collection(name)
    log.info("collections: %s", ", ".join(COL.values()))

def _find_lineart_modifier(obj):
    # GP v3 (4.3+) puts modifiers on obj.modifiers; older GP on grease_pencil_modifiers
    for stack in (getattr(obj, "modifiers", []),
                  getattr(obj, "grease_pencil_modifiers", [])):
        for m in stack:
            if "LINE" in getattr(m, "type", ""):
                return m
    return None

def ensure_lineart_object():
    """Find or create the Line Art Grease Pencil object and configure it."""
    obj = bpy.data.objects.get(LINEART_NAME)
    if obj is None:
        # VERSION-SENSITIVE: GP-with-LineArt add operator differs by build.
        created = False
        for attempt in ("object.grease_pencil_add", "object.gpencil_add"):
            op = bpy.ops
            try:
                getattr(getattr(op, attempt.split('.')[0]), attempt.split('.')[1])(type="LINEART")
                created = True
                break
            except Exception as e:  # noqa
                log.warning("could not create via %s: %s", attempt, e)
        if not created:
            log.error("Create the Line Art object manually: Add > Grease Pencil > "
                      "Scene Line Art, rename it to %r, then re-run build.", LINEART_NAME)
            return None
        obj = bpy.context.active_object
        obj.name = LINEART_NAME

    mod = _find_lineart_modifier(obj)
    if mod:
        # configure the edge types we want for clean overhead line art
        for attr, val in (("use_contour", True), ("use_crease", True),
                          ("use_material", True), ("use_intersection", True),
                          ("crease_threshold", CREASE_ANGLE)):
            if hasattr(mod, attr):
                setattr(mod, attr, val)
        # source: whole scene so FIGURE + HAIR + INSTRUMENT are all captured,
        # while PROXY objects are excluded per-object (see mark_proxy()).
        if hasattr(mod, "source_type"):
            mod.source_type = "SCENE"
        log.info("Line Art modifier configured on %r", obj.name)
    else:
        log.warning("No Line Art modifier found on %r — add one in the modifier "
                    "stack and set Source = Scene.", obj.name)
    # move it into the LINEART collection
    for c in obj.users_collection:
        c.objects.unlink(obj)
    get_or_create_collection(COL["lineart"]).objects.link(obj)
    return obj

def mark_proxy(obj):
    """Exclude an object from rendering + Line Art (alignment guides, etc.)."""
    obj.hide_render = True
    if hasattr(obj, "lineart"):
        obj.lineart.usage = "EXCLUDE"

def build():
    set_scene_basics()
    build_collections()
    build_camera()
    ensure_lineart_object()
    log.info("STAGE READY. Calibrate scale once: render a known-length object and "
             "confirm 1 SVG unit = 1 mm in export settings.")

# =========================================================================== #
# attach — rigid instrument binding (follows posing, never deforms)
# =========================================================================== #
def attach_instrument(instrument_name, armature_name, bone_name=ANCHOR_BONE):
    inst = bpy.data.objects.get(instrument_name)
    arm = bpy.data.objects.get(armature_name)
    if not inst or not arm:
        log.error("attach: missing instrument %r or armature %r", instrument_name, armature_name)
        return
    if bone_name not in arm.pose.bones:
        log.error("attach: bone %r not in %r. Bones include: %s ...",
                  bone_name, armature_name,
                  ", ".join(b.name for b in list(arm.pose.bones)[:8]))
        return
    world = inst.matrix_world.copy()      # preserve current placement
    inst.parent = arm
    inst.parent_type = "BONE"
    inst.parent_bone = bone_name
    inst.matrix_world = world             # re-anchor so it doesn't jump
    log.info("attached %r to %r bone %r (rigid, follows pose)",
             instrument_name, armature_name, bone_name)

# =========================================================================== #
# normalize + footprint — make a raw GP export obey spec.md
# =========================================================================== #
def collection_mesh_objects(collection_name):
    col = bpy.data.collections.get(collection_name)
    return [o for o in col.all_objects if o.type == "MESH"] if col else []


def compute_footprint(objs):
    """Project the subject's world bounding box through the camera into the
    spec frame and return `footprint_units` [x, y, w, h] (ints). Reliable: it
    measures Blender geometry, not the exported SVG, so no SVG geometry engine is
    needed. world_to_camera_view gives 0..1 with origin bottom-left; SVG y is
    flipped (down)."""
    try:
        from bpy_extras.object_utils import world_to_camera_view
        from mathutils import Vector
    except Exception as e:  # noqa
        log.warning("footprint: bpy_extras/mathutils unavailable: %s", e)
        return None
    scene = bpy.context.scene
    cam = scene.camera
    if not cam or not objs:
        log.warning("footprint: no camera or no subject meshes")
        return None
    xs, ys = [], []
    for o in objs:
        for corner in o.bound_box:
            ndc = world_to_camera_view(scene, cam, o.matrix_world @ Vector(corner[:]))
            xs.append(ndc.x)
            ys.append(ndc.y)
    x0, x1 = max(0.0, min(xs)), min(1.0, max(xs))
    y0, y1 = max(0.0, min(ys)), min(1.0, max(ys))
    return [round(x0 * FRAME_UNITS), round((1.0 - y1) * FRAME_UNITS),
            round((x1 - x0) * FRAME_UNITS), round((y1 - y0) * FRAME_UNITS)]


def normalize_svg(filepath):
    """Rewrite a raw Grease-Pencil SVG to obey spec.md §1/§2/§11: a `0 0 200 200`
    frame, `.ln`/`.lnf` classes instead of inline stroke, and north-up. Scale is
    assumed already calibrated to 1 unit = 10 mm at export (REQUIREMENTS §3), so
    this sets the frame + styling rather than rescaling. The tight crop comes
    later from the footprint sidecar, so a full-frame viewBox here is fine.

    VALIDATE the axis flip (FLIP_Y) and that exported coordinates land in 0..200
    against your Blender build's first real export, then we lock it."""
    import xml.etree.ElementTree as ET
    svg_ns = "http://www.w3.org/2000/svg"
    ET.register_namespace("", svg_ns)
    try:
        tree = ET.parse(filepath)
        root = tree.getroot()
    except Exception as e:  # noqa
        log.error("normalize: cannot parse %s: %s", filepath, e)
        return False

    root.set("viewBox", f"0 0 {FRAME_UNITS} {FRAME_UNITS}")
    root.set("width", str(FRAME_UNITS))
    root.set("height", str(FRAME_UNITS))

    if not any(c.tag.endswith("style") for c in root):
        style = ET.Element(f"{{{svg_ns}}}style")
        style.text = STYLE_DEF
        root.insert(0, style)

    # Strip inline stroke and class stroked elements. GP export rarely fills, so
    # default to .ln (open line work); flip a shape to .lnf by hand where it must
    # occlude what is beneath it.
    for el in root.iter():
        tag = el.tag.split("}")[-1]
        if tag in ("path", "polyline", "line", "polygon"):
            filled = el.get("fill") not in (None, "none") or tag == "polygon"
            for attr in ("stroke", "stroke-width", "stroke-linecap",
                         "stroke-linejoin", "style", "fill"):
                el.attrib.pop(attr, None)
            el.set("class", "lnf" if filled else "ln")

    if FLIP_Y:
        wrap = ET.Element(f"{{{svg_ns}}}g")
        wrap.set("transform", f"translate(0,{FRAME_UNITS}) scale(1,-1)")
        for child in list(root):
            if not child.tag.endswith("style"):
                root.remove(child)
                wrap.append(child)
        root.append(wrap)

    tree.write(filepath, encoding="utf-8", xml_declaration=True)
    log.info("normalized %s -> spec frame", filepath)
    return True


# =========================================================================== #
# capture — bake Line Art + export SVG per subject
# =========================================================================== #
def bake_lineart(obj):
    """VERSION-SENSITIVE: bakes Line Art strokes so they can be exported."""
    if obj:
        bpy.context.view_layer.objects.active = obj
    for op in ("object.lineart_bake_strokes", "gpencil.lineart_bake_strokes"):
        try:
            getattr(getattr(bpy.ops, op.split('.')[0]), op.split('.')[1])()
            return True
        except Exception as e:  # noqa
            log.warning("bake via %s failed: %s", op, e)
    log.warning("Could not bake Line Art automatically; the live modifier may "
                "still export. If output is empty, bake manually first.")
    return False

def export_svg(filepath, obj):
    """VERSION-SENSITIVE: GP SVG export. Verify the operator via File > Export."""
    if obj:
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
    tried = []
    for op in ("wm.grease_pencil_export_svg", "wm.gpencil_export_svg"):
        try:
            getattr(getattr(bpy.ops, op.split('.')[0]), op.split('.')[1])(
                filepath=filepath, selected_object_type="ACTIVE")
            log.info("exported %s", filepath)
            return True
        except Exception as e:  # noqa
            tried.append(f"{op}: {e}")
    log.error("SVG export failed (%s). In your build, use File > Export > "
              "Grease Pencil as SVG once to find the exact operator name.",
              " | ".join(tried))
    return False

def show_only_subject(sub):
    """Make exactly this subject's collection visible to the render/Line Art."""
    for s in SUBJECTS:
        set_collection_excluded(s["collection"], s["collection"] != sub["collection"])

def capture():
    import os
    out = bpy.path.abspath(OUT_DIR)
    os.makedirs(out, exist_ok=True)
    la = bpy.data.objects.get(LINEART_NAME)
    if not la:
        log.error("No %r object. Run build first.", LINEART_NAME)
        return
    if not SUBJECTS:
        log.error("SUBJECTS is empty — list your prepared characters in CONFIG.")
        return
    import json
    for sub in SUBJECTS:
        show_only_subject(sub)
        if sub.get("instrument") and sub.get("armature"):
            attach_instrument(sub["instrument"], sub["armature"],
                              sub.get("bone", ANCHOR_BONE))
        bpy.context.view_layer.update()
        bake_lineart(la)
        svg_path = os.path.join(out, f"{sub['name']}.svg")
        if not export_svg(svg_path, la):
            continue
        # Normalize the raw export to the spec frame, and record the object's
        # footprint (from Blender geometry) so generate.py can crop + size it.
        normalize_svg(svg_path)
        fp = compute_footprint(collection_mesh_objects(sub["collection"]))
        if fp:
            sidecar = svg_path[:-4] + ".footprint.json"
            with open(sidecar, "w", encoding="utf-8") as fh:
                json.dump({"name": sub["name"], "footprint_units": fp}, fh, indent=2)
            log.info("footprint %s -> %s", sub["name"], fp)
    log.info("capture done -> %s", out)

# =========================================================================== #
# dispatch — set MODE and Run Script
# =========================================================================== #
MODE = "build"      # "build" | "capture"
# attach is normally called per subject inside capture(), but you can also call
# attach_instrument("guitar", "Human", "spine03") directly while authoring.

if MODE == "build":
    build()
elif MODE == "capture":
    capture()
else:
    log.error("Unknown MODE %r", MODE)
