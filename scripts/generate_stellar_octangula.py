import numpy as np


def normalize(v):
    return v / np.linalg.norm(v)


def rotation_matrix(axis, angle):
    """
    Rodrigues rotation formula
    """
    axis = normalize(axis)

    x, y, z = axis
    c = np.cos(angle)
    s = np.sin(angle)
    C = 1 - c

    return np.array([
        [c + x*x*C,     x*y*C - z*s, x*z*C + y*s],
        [y*x*C + z*s,   c + y*y*C,   y*z*C - x*s],
        [z*x*C - y*s,   z*y*C + x*s, c + z*z*C]
    ])


# --------------------------------------------------
# Step 1
# 标准 Stella Octangula
# --------------------------------------------------

vertices = np.array([
    [ 1,  1,  1],   # 1
    [ 1, -1, -1],   # 2
    [-1,  1, -1],   # 3
    [-1, -1,  1],   # 4

    [-1, -1, -1],   # 5
    [-1,  1,  1],   # 6
    [ 1, -1,  1],   # 7
    [ 1,  1, -1],   # 8
], dtype=float)

# --------------------------------------------------
# Step 2
# 让第一个四面体顶点1朝上
# --------------------------------------------------

target = np.array([0, 0, 1.0])

v = normalize(vertices[0])

axis = np.cross(v, target)
angle = np.arccos(np.clip(np.dot(v, target), -1.0, 1.0))

R1 = rotation_matrix(axis, angle)

vertices = (R1 @ vertices.T).T

# --------------------------------------------------
# Step 3
# 调整绕Z轴旋转
# 使底面水平且边方向对称
# --------------------------------------------------

base_center = (
    vertices[1] +
    vertices[2] +
    vertices[3]
) / 3

edge_dir = vertices[1] - base_center

phi = np.arctan2(edge_dir[1], edge_dir[0])

Rz = np.array([
    [np.cos(-phi), -np.sin(-phi), 0],
    [np.sin(-phi),  np.cos(-phi), 0],
    [0, 0, 1]
])

vertices = (Rz @ vertices.T).T

# --------------------------------------------------
# 面定义
# --------------------------------------------------

tetra_a = [
    (1,2,3),
    (1,4,2),
    (1,3,4),
    (2,4,3),
]

tetra_b = [
    (5,6,7),
    (5,8,6),
    (5,7,8),
    (6,8,7),
]

# --------------------------------------------------
# 输出OBJ
# --------------------------------------------------

with open("stella_octangula.obj", "w") as f:

    f.write("mtllib stella_octangula.mtl\n\n")

    for x, y, z in vertices:
        f.write(f"v {x:.8f} {y:.8f} {z:.8f}\n")

    f.write("\nusemtl StellaMaterial\n\n")

    for face in tetra_a:
        f.write(f"f {face[0]} {face[1]} {face[2]}\n")

    for face in tetra_b:
        f.write(f"f {face[0]} {face[1]} {face[2]}\n")

print("OBJ generated.")

