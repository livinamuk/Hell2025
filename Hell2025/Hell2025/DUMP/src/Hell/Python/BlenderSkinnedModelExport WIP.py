import bpy
import mathutils
import math
from io_scene_fbx import axis_conversion

def get_bone_accumulated_matrix(bone):
    """Accumulate bone.matrix_local through its parent chain (in armature-local space)."""
    m = bone.matrix_local.copy()
    parent = bone.parent
    while parent:
        m = parent.matrix_local @ m
        parent = parent.parent
    return m

def export_dm_hand_inverse_bind_unit_adjusted():
    # Ensure an Armature is selected.
    arm_obj = bpy.context.active_object
    if not arm_obj or arm_obj.type != 'ARMATURE':
        print("Please select an Armature object in Object Mode.")
        return

    # --- Decompose the Armature's World Matrix ---
    # (Your armature container, e.g. Dm-master, has a scale of 0.01.)
    arm_world = arm_obj.matrix_world.copy()
    loc, rot, scl = arm_world.decompose()
    # Compute a unit conversion factor to cancel the armature's scale.
    # For a uniform scale of 0.01, factor = 1/0.01 = 100.
    unit_factor = 1.0 / scl.x  # assuming uniform scale (scl.x == scl.y == scl.z)
    unit_conv = mathutils.Matrix.Scale(unit_factor, 4)
    
    # Build an adjusted armature matrix with scale canceled:
    arm_world_unit = arm_obj.matrix_world @ unit_conv

    # --- Global Axis Conversion ---
    # Convert from Blender's default (-Y forward, Z up) to FBX (-Z forward, Y up)
    global_conv = axis_conversion(
        from_forward='-Y',
        from_up='Z',
        to_forward='-Z',
        to_up='Y'
    ).to_4x4()

    # --- Process the "Dm-Hand" Bone ---
    bone = None
    bone_list = list(arm_obj.data.bones)
    for b in bone_list:
        if b.name == "Dm-Hand":
            bone = b
            break
    if bone is None:
        print("Bone 'Dm-Hand' not found.")
        return

    parent_index = bone_list.index(bone.parent) if bone.parent else -1

    # Get the bone's accumulated local transform (in armature space).
    bone_local = get_bone_accumulated_matrix(bone)

    # Compute the bone's global transform using the adjusted armature matrix.
    bone_global = arm_world_unit @ bone_local

    # (Optionally, you could apply extra per-bone reorientation here if needed.
    # For now, we'll assume no extra per-bone rotation is needed.)
    
    # Apply the global FBX axis conversion.
    bone_global_fbx = global_conv @ bone_global

    # Invert to get the inverse bind matrix.
    inv_bind = bone_global_fbx.inverted()

    # Print the result (C++-style) only for Dm-Hand.
    print("\n// Inverse Bind Matrix for 'Dm-Hand' with unit conversion")
    print("Node node = {")
    print(f'    "{bone.name}",')
    print(f"    {parent_index},")
    print("    glm::mat4(")
    for i in range(4):
        row_vals = [f"{inv_bind[i][j]:.6f}f" for j in range(4)]
        print("        " + ", ".join(row_vals) + ("," if i < 3 else ""))
    print("    )")
    print("};")

export_dm_hand_inverse_bind_unit_adjusted()
