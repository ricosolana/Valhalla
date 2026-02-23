# This script exports Blender cubes into a C++ Code test format for BoxBoxOverlap testing
#    ASSERT_TRUE(VUtils::Physics::BoxBoxOverlap(
#        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0),
#        Vector3f(0, 0, 0), Vector3f(1, 1, 1), Quaternion::euler(0, 0, 0)
#    ));
# Automatically detects whether the boxes collides, and creates test cases
# Usage:
#   Add a Blender Cube to a new scene
#   If the object measures 2m * 2m * 2m, you must scale it by 0.5
#       Then Apply -> Scale
#   Duplicate the Cube, and arrange as desired for an intended test case
#   For multiple tests, you must arrange along the X-Axis,
#       So, duplicate the cube(s), translate along X+10, then transform as desired
# To perform the Test Case export:
#   Open Blender Scripting tab
#   Paste this code into a new File (you shouldnt need to save a file)
#   Click Run
#   Ensure that the output directory makes sense for your use case (~/exported_objs/*).


import bpy
import math
import os
import time
import bmesh
from mathutils.bvhtree import BVHTree

# ---------------------------
# Formatting helpers
# ---------------------------

def format_float(value):
    if abs(value) < 1e-6:
        return "0"

    rounded = round(value, 3)

    if abs(rounded - int(rounded)) < 1e-6:
        return str(int(rounded))

    formatted = f"{rounded:.3f}".rstrip('0').rstrip('.')
    return f"{formatted}f"

def vec_to_cpp(v):
    return f"Vector3f({format_float(v.x)}, {format_float(v.y)}, {format_float(v.z)})"

def rot_to_cpp(euler):
    deg = [math.degrees(a) for a in euler]
    return f"Quaternion::euler({format_float(deg[0])}, {format_float(deg[1])}, {format_float(deg[2])})"
#def rot_to_cpp(obj):
#    # Get world quaternion (order independent)
#    quat = obj.matrix_world.to_quaternion()
#
#    # Convert to ZXY Euler order (Unity-style)
#    euler_zxy = quat.to_euler('ZXY')
#
#    deg = [math.degrees(a) for a in euler_zxy]
#
#    return f"Quaternion::euler({format_float(deg[0])}, {format_float(deg[1])}, {format_float(deg[2])})"


# ---------------------------
# Collision test using BVH
# ---------------------------

def build_bvh(obj, depsgraph):
    eval_obj = obj.evaluated_get(depsgraph)
    mesh = eval_obj.to_mesh()

    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.transform(obj.matrix_world)
    bm.normal_update()

    bvh = BVHTree.FromBMesh(bm)

    bm.free()
    eval_obj.to_mesh_clear()

    return bvh


def objects_collide(obj1, obj2, depsgraph):
    bvh1 = build_bvh(obj1, depsgraph)
    bvh2 = build_bvh(obj2, depsgraph)

    overlap = bvh1.overlap(bvh2)
    return len(overlap) > 0


# ---------------------------
# Export
# ---------------------------

def export_box_pairs_to_file():
    depsgraph = bpy.context.evaluated_depsgraph_get()

    objects = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
    objects.sort(key=lambda o: o.matrix_world.translation.x)

    if len(objects) < 2:
        print("Not enough objects to export.")
        return

    home_dir = os.path.expanduser("~")
    export_dir = os.path.join(home_dir, "exported_objs")
    os.makedirs(export_dir, exist_ok=True)

    timestamp = int(time.time())
    filename = f"objs_exported_{timestamp}.cpp"
    filepath = os.path.join(export_dir, filename)

    with open(filepath, "w") as f:
        for i in range(0, len(objects) - 1, 2):
            obj1 = objects[i]
            obj2 = objects[i + 1]

            collide = objects_collide(obj1, obj2, depsgraph)

            pos1 = obj1.matrix_world.translation
            pos2 = obj2.matrix_world.translation

            size1 = obj1.dimensions
            size2 = obj2.dimensions

            rot1 = obj1.matrix_world.to_euler('ZXY')
            rot2 = obj2.matrix_world.to_euler('ZXY')

            assertion = "ASSERT_TRUE" if collide else "ASSERT_FALSE"

            f.write(f"{assertion}(VUtils::Physics::BoxBoxOverlap(\n")
            f.write(f"    {vec_to_cpp(pos1)}, {vec_to_cpp(size1)}, {rot_to_cpp(rot1)},\n")
            f.write(f"    {vec_to_cpp(pos2)}, {vec_to_cpp(size2)}, {rot_to_cpp(rot2)}\n")
            f.write("));\n\n")

    print(f"Export complete: {filepath}")


export_box_pairs_to_file()