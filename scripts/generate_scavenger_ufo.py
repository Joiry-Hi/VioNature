#!/usr/bin/env python3
"""Generate the Scavenger UFO OBJ model.

The model is deliberately procedural so it stays stable:
- all faces are triangles
- visible faces are emitted double-sided to avoid OBJ backface surprises
- vertices/normals are deterministic
- no transparent materials
- no large flat disk cap that can read as a giant billboard in-game
"""

from __future__ import annotations

from dataclasses import dataclass
from math import cos, pi, sin
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OBJ_PATH = ROOT / "assets/models/bosses/scavenger_ufo.obj"
MTL_PATH = ROOT / "assets/models/bosses/scavenger_ufo.mtl"

SEGMENTS = 24


@dataclass
class Mesh:
    vertices: list[tuple[float, float, float]]
    normals: list[tuple[float, float, float]]
    faces: list[tuple[str, tuple[int, int, int], tuple[int, int, int]]]


def normalize(v: tuple[float, float, float]) -> tuple[float, float, float]:
    x, y, z = v
    length = (x * x + y * y + z * z) ** 0.5
    if length <= 1e-8:
        return (0.0, 1.0, 0.0)
    return (x / length, y / length, z / length)


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


class Builder:
    def __init__(self) -> None:
        self.vertices: list[tuple[float, float, float]] = []
        self.normals: list[tuple[float, float, float]] = []
        self.faces: list[tuple[str, tuple[int, int, int], tuple[int, int, int]]] = []

    def vertex(self, x: float, y: float, z: float) -> int:
        self.vertices.append((x, y, z))
        return len(self.vertices)

    def normal(self, n: tuple[float, float, float]) -> int:
        self.normals.append(normalize(n))
        return len(self.normals)

    def face(self, material: str, a: int, b: int, c: int) -> None:
        va, vb, vc = self.vertices[a - 1], self.vertices[b - 1], self.vertices[c - 1]
        normal = normalize(cross(sub(vb, va), sub(vc, va)))
        front = self.normal(normal)
        back = self.normal((-normal[0], -normal[1], -normal[2]))
        self.faces.append((material, (a, b, c), (front, front, front)))
        self.faces.append((material, (c, b, a), (back, back, back)))

    def ring(self, radius_x: float, y: float, radius_z: float | None = None, phase: float = 0.0) -> list[int]:
        if radius_z is None:
            radius_z = radius_x
        ids = []
        for i in range(SEGMENTS):
            t = phase + 2.0 * pi * i / SEGMENTS
            ids.append(self.vertex(radius_x * cos(t), y, radius_z * sin(t)))
        return ids

    def connect_rings(self, material: str, upper: list[int], lower: list[int]) -> None:
        count = len(upper)
        for i in range(count):
            j = (i + 1) % count
            # Consistent winding for outward-facing side facets.
            self.face(material, upper[i], lower[i], upper[j])
            self.face(material, upper[j], lower[i], lower[j])

    def fan_to_point(self, material: str, ring: list[int], point: int, reverse: bool = False) -> None:
        count = len(ring)
        for i in range(count):
            j = (i + 1) % count
            if reverse:
                self.face(material, ring[j], ring[i], point)
            else:
                self.face(material, ring[i], ring[j], point)

    def box(self, material: str, cx: float, cy: float, cz: float, sx: float, sy: float, sz: float) -> None:
        x0, x1 = cx - sx * 0.5, cx + sx * 0.5
        y0, y1 = cy - sy * 0.5, cy + sy * 0.5
        z0, z1 = cz - sz * 0.5, cz + sz * 0.5
        v = [
            self.vertex(x0, y0, z0), self.vertex(x1, y0, z0),
            self.vertex(x1, y0, z1), self.vertex(x0, y0, z1),
            self.vertex(x0, y1, z0), self.vertex(x1, y1, z0),
            self.vertex(x1, y1, z1), self.vertex(x0, y1, z1),
        ]
        quads = [
            (0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1),
            (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0),
        ]
        for a, b, c, d in quads:
            self.face(material, v[a], v[b], v[c])
            self.face(material, v[a], v[c], v[d])

    def build(self) -> Mesh:
        return Mesh(self.vertices, self.normals, self.faces)


def generate() -> Mesh:
    b = Builder()

    # Stacked rings create a thick saucer silhouette without a single huge cap.
    rim = b.ring(5.25, 0.00, 5.25)
    shoulder = b.ring(4.55, 0.58, 4.55)
    deck = b.ring(2.35, 1.10, 2.35)
    lower = b.ring(4.20, -0.92, 4.20)
    collar = b.ring(2.10, -1.42, 2.10)
    belly = b.ring(0.95, -1.85, 0.95)
    belly_tip = b.vertex(0.0, -2.18, 0.0)

    b.connect_rings("ufo_body", rim, shoulder)
    b.connect_rings("ufo_body", shoulder, deck)
    b.connect_rings("ufo_body", rim, lower)
    b.connect_rings("ufo_glow", lower, collar)
    b.connect_rings("ufo_glow", collar, belly)
    b.fan_to_point("ufo_glow", belly, belly_tip, reverse=True)

    cockpit_base = b.ring(1.72, 1.20, 1.72)
    cockpit_mid = b.ring(1.10, 2.08, 1.10)
    cockpit_tip = b.vertex(0.0, 2.72, 0.0)
    b.connect_rings("ufo_dome", deck, cockpit_base)
    b.connect_rings("ufo_dome", cockpit_base, cockpit_mid)
    b.fan_to_point("ufo_dome", cockpit_mid, cockpit_tip)

    # Four chunky underside pods with visible height.
    b.box("ufo_dark", 3.55, -1.20, 0.0, 1.20, 0.95, 0.90)
    b.box("ufo_dark", -3.55, -1.20, 0.0, 1.20, 0.95, 0.90)
    b.box("ufo_dark", 0.0, -1.20, 3.55, 0.90, 0.95, 1.20)
    b.box("ufo_dark", 0.0, -1.20, -3.55, 0.90, 0.95, 1.20)

    # Small bright front marker: gives a readable orientation but keeps the model simple.
    b.box("ufo_glow", 0.0, 0.12, 5.05, 1.20, 0.28, 0.18)

    return b.build()


def write_mtl() -> None:
    MTL_PATH.write_text(
        """newmtl ufo_body
Ka 0.36 0.39 0.40
Kd 0.58 0.64 0.66
Ks 0.22 0.26 0.28
Ns 28.0

newmtl ufo_dome
Ka 0.10 0.55 0.72
Kd 0.22 0.80 0.95
Ks 0.48 0.82 1.00
Ns 64.0

newmtl ufo_glow
Ka 0.02 0.62 0.82
Kd 0.08 0.88 1.00
Ks 0.35 0.90 1.00
Ns 52.0

newmtl ufo_dark
Ka 0.06 0.07 0.08
Kd 0.15 0.18 0.20
Ks 0.08 0.10 0.12
Ns 16.0
""",
        encoding="utf-8",
    )


def write_obj(mesh: Mesh) -> None:
    lines: list[str] = [
        "# Scavenger UFO - generated by scripts/generate_scavenger_ufo.py",
        "mtllib scavenger_ufo.mtl",
        "o scavenger_ufo",
        "",
    ]
    for x, y, z in mesh.vertices:
        lines.append(f"v {x:.6f} {y:.6f} {z:.6f}")
    lines.append("")
    for x, y, z in mesh.normals:
        lines.append(f"vn {x:.6f} {y:.6f} {z:.6f}")
    lines.append("")

    current_material = None
    for material, verts, norms in mesh.faces:
        if material != current_material:
            lines.append(f"g {material}")
            lines.append(f"usemtl {material}")
            lines.append("s 1")
            current_material = material
        a, b, c = verts
        na, nb, nc = norms
        lines.append(f"f {a}//{na} {b}//{nb} {c}//{nc}")

    OBJ_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    mesh = generate()
    write_mtl()
    write_obj(mesh)
    xs = [v[0] for v in mesh.vertices]
    ys = [v[1] for v in mesh.vertices]
    zs = [v[2] for v in mesh.vertices]
    print(f"Wrote {OBJ_PATH}")
    print(f"Wrote {MTL_PATH}")
    print(
        "bbox "
        f"x={min(xs):.2f}..{max(xs):.2f} "
        f"y={min(ys):.2f}..{max(ys):.2f} "
        f"z={min(zs):.2f}..{max(zs):.2f}; "
        f"verts={len(mesh.vertices)} normals={len(mesh.normals)} faces={len(mesh.faces)}"
    )


if __name__ == "__main__":
    main()
