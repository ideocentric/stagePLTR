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
  capture  — iterate prepared subjects, isolate each, attach its instrument, and
             export one raw Grease-Pencil SVG + a small metadata sidecar per
             subject. All SVG processing (recolour, crop, size, mirror) is done
             later by `figures/generate.py --ingest`, in tested stdlib Python —
             not here — so the fragile bits stay out of Blender.

WHY NOT SCRIPT MPFB ITSELF: character creation in MPFB is slider-driven and its
operators are version-fragile, so characters are made interactively and saved
(as MPFB human presets or as named collections). This script automates the
stable, repetitive part: staging, attaching, and capturing.

VERSION-SENSITIVE CALLS (Grease Pencil v3): ensure_lineart_object(), the optional
bake_lineart(), and export_svg() touch GP operators whose names changed across
versions. Each is wrapped and logged; if one fails, the console prints the
manual-menu fallback. Verified against Blender 5.1 (create uses
type='LINEART_SCENE'; live export needs no bake).
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
RES           = 1024         # px, square; this full frame = ORTHO_SCALE (2 m) = 200 units
OUT_DIR       = "//svg_out/" # // = relative to this .blend
STROKE_PX     = 6            # Line Art stroke thickness
CREASE_ANGLE  = math.radians(60)

# Output contract (spec.md §1, §11): the camera's ORTHO_SCALE metres fill the
# RES-px render frame, so that frame is the 2 m / 200-unit spec frame. capture()
# only EXPORTS the raw Grease-Pencil SVG (filled #000000 line art, RES-px viewBox)
# plus a small metadata sidecar; figures/generate.py --ingest does all the SVG
# work (recolour to spec ink, crop to the content bbox, derive defaultSize, mirror
# for left-handed). Keeping the fragile SVG math in tested Python, not in Blender.

ANCHOR_BONE   = "spine03"    # torso bone instruments attach to — VERIFY in your rig
CLEAN_DEFAULTS = True        # on build, remove Blender's startup Cube/Camera/Light
DEFAULT_OBJECTS = ("Cube", "Camera", "Light")  # Blender's startup object names
LINEART_NAME  = "StageLineArt"
COL = dict(figure="FIGURE", hair="HAIR", instrument="INSTRUMENT",
           proxy="PROXY", lineart="LINEART")

# For capture mode: each subject is a prepared character. Put each character's
# body+hair+instrument in its own collection (or sub-collections) and list them.
# Optional metadata travels to the pack via a sidecar: display (shown name),
# category (palette section), mirror_for_left (emit the free 2-D left mirror).
SUBJECTS = [
    # dict(name="guitar_fem_punk", collection="CHAR_guitar_fem_punk",
    #      instrument="guitar", armature="Human", bone=ANCHOR_BONE,
    #      display="Guitarist — Female, Punk", category="People", mirror_for_left=True),
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
def clean_default_scene():
    """Remove Blender's startup Cube/Camera/Light so a fresh file becomes a clean
    stage in one click — no manual deletion, no confusion about what's needed.
    Name-targeted and idempotent (skips anything absent); it never matches our
    stage objects (StageCamObj, StageLineArt, TiltPivot) or an imported rig."""
    removed = []
    for name in DEFAULT_OBJECTS:
        obj = bpy.data.objects.get(name)
        if obj is not None:
            bpy.data.objects.remove(obj, do_unlink=True)
            removed.append(name)
    # Drop the now-orphaned mesh/camera/light datablocks too, so the file is tidy.
    if removed and hasattr(bpy.ops.outliner, "orphans_purge"):
        try:
            bpy.ops.outliner.orphans_purge(do_recursive=True)
        except Exception as e:  # noqa
            log.warning("orphan purge skipped: %s", e)
    log.info("cleaned default objects: %s", ", ".join(removed) if removed else "(none)")


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
        # VERSION-SENSITIVE: the "Scene Line Art" GP add operator + its type enum
        # differ by build. Blender 5.x uses type='LINEART_SCENE'; older GPv3 used
        # 'LINEART'; legacy GPv2 used object.gpencil_add.
        attempts = (
            ("grease_pencil_add", "LINEART_SCENE"),  # Blender 5.x (verified 5.1)
            ("grease_pencil_add", "LINEART"),        # earlier GPv3
            ("gpencil_add", "LINEART"),              # legacy GPv2
        )
        created = False
        for op_name, gp_type in attempts:
            op = getattr(bpy.ops.object, op_name, None)
            if op is None:
                continue
            try:
                op(type=gp_type)
                created = True
                log.info("created Line Art object via object.%s(type=%r)", op_name, gp_type)
                break
            except Exception as e:  # noqa
                log.warning("create via object.%s(type=%r) failed: %s", op_name, gp_type, e)
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
    if CLEAN_DEFAULTS:
        clean_default_scene()
    set_scene_basics()
    build_collections()
    build_camera()
    ensure_lineart_object()
    log.info("STAGE READY. ORTHO_SCALE=%.1f m fills the %d-px frame, so capture +"
             " generate.py --ingest scale everything automatically (no manual"
             " calibration).", ORTHO_SCALE, RES)

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
# capture — export the raw GP SVG + a metadata sidecar per subject
# =========================================================================== #
# Blender 5.x exports Line Art live (no bake needed — verified on 5.1); set True
# only if a build yields empty SVGs without baking first.
BAKE_FIRST = False


def bake_lineart(obj):
    """VERSION-SENSITIVE: bake Line Art strokes (used only when BAKE_FIRST)."""
    if obj:
        bpy.context.view_layer.objects.active = obj
    for op in ("object.lineart_bake_strokes", "gpencil.lineart_bake_strokes"):
        try:
            getattr(getattr(bpy.ops, op.split('.')[0]), op.split('.')[1])()
            return True
        except Exception as e:  # noqa
            log.warning("bake via %s failed: %s", op, e)
    log.warning("Could not bake Line Art automatically; the live modifier usually "
                "still exports. If output is empty, set BAKE_FIRST or bake manually.")
    return False

def export_svg(filepath, obj):
    """VERSION-SENSITIVE: GP SVG export.

    use_clip_camera=True is REQUIRED — it clips the export to the camera frame
    (the RES-px render frame). Without it (the operator default) GP exports in raw
    object space: huge coordinates and a content-sized viewBox, not the 0 0 RES
    RES frame the ingest expects. Verified against Blender 5.1
    (wm.grease_pencil_export_svg)."""
    if obj:
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
    kwargs = dict(filepath=filepath, selected_object_type="ACTIVE",
                  use_clip_camera=True)
    tried = []
    for op_name in ("grease_pencil_export_svg", "gpencil_export_svg"):
        op = getattr(bpy.ops.wm, op_name, None)
        if op is None:
            continue
        try:
            op(**kwargs)
            log.info("exported %s", filepath)
            return True
        except Exception as e:  # noqa
            tried.append(f"wm.{op_name}: {e}")
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
        if BAKE_FIRST:
            bake_lineart(la)
        svg_path = os.path.join(out, f"{sub['name']}.svg")
        if not export_svg(svg_path, la):
            continue
        # Metadata sidecar for `generate.py --ingest`, which does all the SVG work
        # (recolour, crop to content, size, mirror). Raw export stays untouched.
        meta = {
            "name": sub.get("display", sub["name"]),
            "category": sub.get("category", "People"),
            "mirror_for_left": bool(sub.get("mirror_for_left", False)),
        }
        with open(svg_path[:-4] + ".json", "w", encoding="utf-8") as fh:
            json.dump(meta, fh, indent=2)
        log.info("captured %s (+ metadata)", sub["name"])
    log.info("capture done -> %s  (next: generate.py --ingest)", out)

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
