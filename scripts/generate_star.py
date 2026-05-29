#!/usr/bin/env python3
"""Generate 'Star of Bethlehem' boss model: cube core + 6 pyramidal spikes."""

import os

HALF = 1       # cube half-extent
SPIKE = 5.0      # spike length from face to tip
OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "models", "bosses")

verts = [
    # 1-8: cube corners
    ( HALF,  HALF,  HALF),   #  1   +X +Y +Z
    ( HALF,  HALF, -HALF),   #  2   +X +Y -Z
    ( HALF, -HALF,  HALF),   #  3   +X -Y +Z
    ( HALF, -HALF, -HALF),   #  4   +X -Y -Z
    (-HALF,  HALF,  HALF),   #  5   -X +Y +Z
    (-HALF,  HALF, -HALF),   #  6   -X +Y -Z
    (-HALF, -HALF,  HALF),   #  7   -X -Y +Z
    (-HALF, -HALF, -HALF),   #  8   -X -Y -Z
    # 9-14: spike tips
    ( HALF + SPIKE,  0.0,  0.0),   #  9  +X
    (-HALF - SPIKE,  0.0,  0.0),   # 10  -X
    ( 0.0,  HALF + SPIKE,  0.0),   # 11  +Y
    ( 0.0, -HALF - SPIKE,  0.0),   # 12  -Y
    ( 0.0,  0.0,  HALF + SPIKE),   # 13  +Z
    ( 0.0,  0.0, -HALF - SPIKE),   # 14  -Z
]

def cross(a, b):
    return (a[1]*b[2] - a[2]*b[1],
            a[2]*b[0] - a[0]*b[2],
            a[0]*b[1] - a[1]*b[0])

def sub(a, b):
    return (a[0]-b[0], a[1]-b[1], a[2]-b[2])

def dot(a, b):
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]

def add(a, b):
    return (a[0]+b[0], a[1]+b[1], a[2]+b[2])

def scale(a, s):
    return (a[0]*s, a[1]*s, a[2]*s)

def centroid(vi):
    p0 = verts[vi[0]-1]; p1 = verts[vi[1]-1]; p2 = verts[vi[2]-1]
    return scale(add(add(p0, p1), p2), 1.0/3.0)

def face_normal(vi):
    p0 = verts[vi[0]-1]; p1 = verts[vi[1]-1]; p2 = verts[vi[2]-1]
    return cross(sub(p1, p0), sub(p2, p0))

# All faces CCW looking from the outside.
groups = [
    # --- Cube core ---
    # +X: looking from +X, right=+Z, up=+Y, CCW: 1→3→4→2
    ("core_+X", [(1, 3, 4), (1, 4, 2)]),
    # -X: looking from -X, right=-Z, up=+Y, CCW: 6→8→7→5
    ("core_-X", [(6, 8, 7), (6, 7, 5)]),
    # +Y: looking from +Y, right=+X, up=-Z, CCW: 2→6→5→1
    ("core_+Y", [(2, 6, 5), (2, 5, 1)]),
    # -Y: looking from -Y, right=+X, up=+Z, CCW: 3→7→8→4
    ("core_-Y", [(3, 7, 8), (3, 8, 4)]),
    # +Z: looking from +Z, right=+X, up=+Y, CCW: 1→5→7→3
    ("core_+Z", [(1, 5, 7), (1, 7, 3)]),
    # -Z: looking from -Z, right=-X, up=+Y, CCW: 6→2→4→8
    ("core_-Z", [(6, 2, 4), (6, 4, 8)]),

    # --- Spikes ---
    ("spike_+X", [(1, 3, 9), (3, 4, 9), (4, 2, 9), (2, 1, 9)]),
    ("spike_-X", [(6, 8, 10), (8, 7, 10), (7, 5, 10), (5, 6, 10)]),
    ("spike_+Y", [(2, 6, 11), (6, 5, 11), (5, 1, 11), (1, 2, 11)]),
    ("spike_-Y", [(3, 7, 12), (7, 8, 12), (8, 4, 12), (4, 3, 12)]),
    ("spike_+Z", [(1, 5, 13), (5, 7, 13), (7, 3, 13), (3, 1, 13)]),
    ("spike_-Z", [(6, 2, 14), (2, 4, 14), (4, 8, 14), (8, 6, 14)]),
]

errors = 0
for name, tris in groups:
    for vi in tris:
        n = face_normal(vi)
        c = centroid(vi)
        if dot(n, c) < 0:
            print(f"  WARNING: {name} face {vi} normal points inward")
            errors += 1

if errors == 0:
    print("All face normals validated (outward-facing).")

os.makedirs(OUT, exist_ok=True)

obj_path = os.path.join(OUT, "star_of_bethlehem.obj")
with open(obj_path, "w") as f:
    f.write("# Star of Bethlehem - Boss Model\n")
    f.write("# Cube core with 6 pyramidal spikes, one per face\n")
    f.write("mtllib star_of_bethlehem.mtl\n")
    f.write("o star_of_bethlehem\n\n")
    for v in verts:
        f.write(f"v {v[0]:.3f} {v[1]:.3f} {v[2]:.3f}\n")
    f.write("\n")
    for name, tris in groups:
        f.write(f"g {name}\n")
        f.write("usemtl boss_material\n")
        f.write("s 1\n")
        for vi in tris:
            f.write(f"f {vi[0]} {vi[1]} {vi[2]}\n")
        f.write("\n")

print(f"Wrote {obj_path}")
print(f"  Vertices: {len(verts)}")
print(f"  Triangles: {sum(len(tris) for _, tris in groups)}")

mtl_path = os.path.join(OUT, "star_of_bethlehem.mtl")
with open(mtl_path, "w") as f:
    f.write("# Star of Bethlehem material\n")
    f.write("newmtl boss_material\n")
    f.write("Ka 0.85 0.78 0.55\n")
    f.write("Kd 0.90 0.82 0.58\n")
    f.write("Ks 0.60 0.55 0.40\n")
    f.write("Ns 96.0\n")
    f.write("d 1.0\n")
    f.write("illum 2\n")

print(f"Wrote {mtl_path}")
