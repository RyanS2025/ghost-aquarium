"""Ghost Aquarium enclosure: parametric code-CAD (manifold3d) → printable STLs + a preview image.

Two parts, both support-free on an FDM printer:
  base.stl  the screen box. Print UPSIDE DOWN (top plate on the bed).
  hood.stl  side walls with 45° grooves for the clear sheet, plus a dark back wall. Print standing up.

How the illusion maps onto the parts (docs/DESIGN.md §3): the screen lies face-up in the base
window. The sheet slides into the hood's grooves, its top edge toward the viewer and its bottom
edge at the back, so the screen's light reflects toward the viewer and the image appears to float
behind the sheet, in front of the black back wall.

MEASURE YOUR PARTS and edit the parameters below; the defaults are typical, not universal.
Run:  make cad   (or: python hardware/ghost_base.py)
Units: millimetres. Coordinates: x = width, y = depth (negative = the viewer's side), z = up.
"""

from __future__ import annotations

import math
import pathlib

import manifold3d as m3d
import numpy as np
import trimesh

# ------------------------------------------------------------------ parameters (mm)
# 2.8" ILI9341 SPI module (the common red PCB). Check yours with calipers.
DISP_PCB = (86.0, 50.0)          # PCB outline (x, y)
DISP_VIEW = (58.0, 44.0)         # visible screen area (x, y): the window
DISP_VIEW_OFFSET_X = 0.0         # visible-area centre offset from the PCB centre (x)
DISP_HOLE_INSET = 3.0            # mounting-hole centres from the PCB edges
DISP_GLASS_ABOVE_PCB = 3.6       # PCB top surface → glass top surface (sets the boss length)
WINDOW_CLEARANCE = 0.8           # extra around the visible area

# Base box (outer).
BASE = (110.0, 90.0, 32.0)       # (x, y, z): room for header pins, jumper wires, the ESP32-S3 board
WALL = 2.4
TOP = 2.4                        # top-plate thickness
USB_NOTCH = (14.0, 10.0)         # back-wall cutout for the ESP32's USB-C cable (width, height from the desk)
BOSS_D = 6.0                     # display bosses (M3 self-tapping, pilot hole below)
PILOT_D = 2.6

# Hood (reflector holder).
SHEET_T = 1.0                    # clear sheet thickness (PETG / acrylic)
SLOT_CLEARANCE = 0.4
GROOVE_DEPTH = 1.6               # how far the sheet sits into each side wall
CHEEK_T = 4.0                    # side-wall thickness
HOOD_MARGIN = 6.0                # inner clearance beyond the visible area, each side (x)
HOOD_DEPTH_MARGIN = 4.0          # sheet coverage beyond the visible area, front and back (y)
SCREW_CLEARANCE_D = 3.4          # M3 through the top plate, from inside the base, into the hood

OUT = pathlib.Path(__file__).parent / "out"


# ------------------------------------------------------------------ helpers
def box(x0, y0, z0, x1, y1, z1) -> m3d.Manifold:
    return m3d.Manifold.cube([x1 - x0, y1 - y0, z1 - z0]).translate([x0, y0, z0])


def cylinder(x, y, z0, z1, d) -> m3d.Manifold:
    return m3d.Manifold.cylinder(z1 - z0, d / 2, d / 2, 48).translate([x, y, z0])


# ------------------------------------------------------------------ derived geometry
view_x = DISP_VIEW[0] + 2 * WINDOW_CLEARANCE
view_y = DISP_VIEW[1] + 2 * WINDOW_CLEARANCE
hood_inner_w = DISP_VIEW[0] + 2 * HOOD_MARGIN          # between the side walls
hood_depth = DISP_VIEW[1] + 2 * HOOD_DEPTH_MARGIN      # the sheet spans this in y, at 45°
hood_h = hood_depth + 2.0                              # 45°: height == depth (+ a little lip)
cheek_x = hood_inner_w / 2 + CHEEK_T / 2               # side-wall centre lines (±)
slot_w = SHEET_T + SLOT_CLEARANCE
sheet_w = hood_inner_w + 2 * (GROOVE_DEPTH - 0.3)      # sits into both grooves with 0.3 mm play
sheet_l = hood_depth * math.sqrt(2)


def make_base() -> m3d.Manifold:
    bx, by, bz = BASE
    shell = box(-bx / 2, -by / 2, 0, bx / 2, by / 2, bz) - box(
        -bx / 2 + WALL, -by / 2 + WALL, -1, bx / 2 - WALL, by / 2 - WALL, bz - TOP)
    # screen window, centred on the visible area
    window = box(DISP_VIEW_OFFSET_X - view_x / 2, -view_y / 2, bz - TOP - 1,
                 DISP_VIEW_OFFSET_X + view_x / 2, view_y / 2, bz + 1)
    # USB-C notch in the back wall, open at the desk
    notch = box(-USB_NOTCH[0] / 2, by / 2 - WALL - 1, -1, USB_NOTCH[0] / 2, by / 2 + 1, USB_NOTCH[1])
    # hood screws, through the top plate under each side wall
    screws = cylinder(-cheek_x, 0, bz - TOP - 1, bz + 1, SCREW_CLEARANCE_D) + cylinder(
        cheek_x, 0, bz - TOP - 1, bz + 1, SCREW_CLEARANCE_D)
    # display bosses hanging from the top plate: glass presses up against the plate
    hx, hy = DISP_PCB[0] / 2 - DISP_HOLE_INSET, DISP_PCB[1] / 2 - DISP_HOLE_INSET
    boss_bottom = bz - TOP - DISP_GLASS_ABOVE_PCB
    bosses, pilots = m3d.Manifold(), m3d.Manifold()
    for sx in (-1, 1):
        for sy in (-1, 1):
            bosses += cylinder(sx * hx, sy * hy, boss_bottom, bz - TOP + 0.01, BOSS_D)
            pilots += cylinder(sx * hx, sy * hy, boss_bottom - 1, bz - TOP - 0.6, PILOT_D)
    return shell + bosses - window - notch - screws - pilots


def make_hood() -> m3d.Manifold:
    """Hood in its own frame: bottom at z=0, front (viewer side) at y=-hood_depth/2."""
    y0, y1 = -hood_depth / 2, hood_depth / 2
    left = box(-cheek_x - CHEEK_T / 2, y0, 0, -cheek_x + CHEEK_T / 2, y1 + WALL, hood_h)
    right = box(cheek_x - CHEEK_T / 2, y0, 0, cheek_x + CHEEK_T / 2, y1 + WALL, hood_h)
    back = box(-cheek_x - CHEEK_T / 2, y1, 0, cheek_x + CHEEK_T / 2, y1 + WALL, hood_h)
    hood = left + right + back
    # 45° grooves on the inner faces: the sheet's centre line runs from (y0, z=hood_depth) to (y1, 0)
    slab_len = sheet_l + 20
    for side in (-1, 1):
        inner_face = side * hood_inner_w / 2
        gx0, gx1 = sorted((inner_face, inner_face + side * GROOVE_DEPTH))
        groove = box(gx0, -slab_len / 2, -slot_w / 2, gx1, slab_len / 2, slot_w / 2)
        groove = groove.rotate([-45, 0, 0]).translate([0, 0, hood_depth / 2])  # through (0, depth/2)
        hood -= groove
    # pilot holes for the M3 screws coming up through the base
    hood -= cylinder(-cheek_x, 0, -1, 9, PILOT_D) + cylinder(cheek_x, 0, -1, 9, PILOT_D)
    return hood


def to_trimesh(part: m3d.Manifold) -> trimesh.Trimesh:
    mesh = part.to_mesh()
    return trimesh.Trimesh(vertices=np.asarray(mesh.vert_properties)[:, :3], faces=np.asarray(mesh.tri_verts))


def preview(base: m3d.Manifold, hood: m3d.Manifold, path: pathlib.Path) -> None:
    """True cross-sections of the assembled parts (side view, y–z), with the sheet and a light ray."""
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    def outline(ax, part, x, color, label):
        # rotate 90° about y (new z = −x, new x = old z), then slice horizontally at −x
        for poly in part.rotate([0, 90, 0]).slice(-x).to_polygons():
            pts = np.asarray(poly)
            ax.fill(pts[:, 1], pts[:, 0], color=color, alpha=0.85, lw=0)  # (y, z)
        ax.plot([], [], color=color, lw=6, label=label)

    z_top, y0, y1 = BASE[2], -hood_depth / 2, hood_depth / 2
    fig, axes = plt.subplots(1, 2, figsize=(11, 5), dpi=110)
    groove_x = hood_inner_w / 2 + GROOVE_DEPTH / 2
    for ax, x, title in ((axes[0], 0.0, "Section through the screen centre (x = 0)"),
                         (axes[1], groove_x, f"Section through a side-wall groove (x = {groove_x:.1f})")):
        outline(ax, base, x, "#3a4a63", "base")
        outline(ax, hood, x, "#22262e", "hood")
        ax.plot([-DISP_VIEW[1] / 2, DISP_VIEW[1] / 2], [z_top - TOP, z_top - TOP], color="#ff4fd8", lw=4, label="screen (face up)")
        ax.plot([y0, y1], [z_top + hood_depth, z_top], color="#3fb8ff", lw=2, label="clear sheet, 45°")
        # a ray: up from the screen centre, reflected horizontally toward the viewer (−y)
        hit_z = z_top + hood_depth / 2
        ax.annotate("", xy=(0, hit_z), xytext=(0, z_top - TOP), arrowprops=dict(arrowstyle="->", color="#ffb347", lw=1.5))
        ax.annotate("", xy=(-BASE[1] / 2 - 12, hit_z), xytext=(0, hit_z), arrowprops=dict(arrowstyle="->", color="#ffb347", lw=1.5))
        ax.text(-BASE[1] / 2 - 12, hit_z + 3, "to viewer", color="#b06a00", fontsize=8)
        ax.set_title(title, fontsize=9)
        ax.set_aspect("equal")
        ax.set_xlabel("y (mm), viewer on the left")
        ax.set_ylabel("z (mm)")
        ax.set_xlim(-BASE[1] / 2 - 18, BASE[1] / 2 + 6)
        ax.set_ylim(-4, z_top + hood_h + 6)
        ax.grid(alpha=0.2)
    axes[0].legend(fontsize=7, loc="upper right")
    fig.suptitle("Ghost Aquarium enclosure: assembled cross-sections", fontsize=11)
    fig.tight_layout()
    fig.savefig(path, facecolor="white")


def main() -> None:
    OUT.mkdir(exist_ok=True)
    base, hood = make_base(), make_hood()
    for name, part in (("base", base), ("hood", hood)):
        if part.status() != m3d.Error.NoError:
            raise SystemExit(f"{name}: invalid geometry ({part.status()})")
    base_mesh, hood_mesh = to_trimesh(base), to_trimesh(hood)
    for name, mesh in (("base", base_mesh), ("hood", hood_mesh)):
        assert mesh.is_watertight, f"{name} is not watertight"
        mesh.export(OUT / f"{name}.stl")
    preview(base, hood.translate([0, 0, BASE[2]]), OUT / "preview.png")
    print(f"base.stl  {base_mesh.extents.round(1)} mm, {base_mesh.volume / 1000:.0f} cm³ (print top-plate down)")
    print(f"hood.stl  {hood_mesh.extents.round(1)} mm, {hood_mesh.volume / 1000:.0f} cm³ (print standing)")
    print(f"sheet     cut {sheet_w:.0f} × {sheet_l:.0f} mm from {SHEET_T} mm clear PETG/acrylic")
    print(f"preview   {OUT / 'preview.png'}")


if __name__ == "__main__":
    main()
