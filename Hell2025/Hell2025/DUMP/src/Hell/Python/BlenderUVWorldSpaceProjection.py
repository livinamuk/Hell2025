import bpy, bmesh
from mathutils import Vector

TEX_SCALE = 1.0 / 2.4
TEX_OFFSET_X = 0.0
TEX_OFFSET_Y = 0.0

def project_selected_objects():
    meshes = [o for o in bpy.context.selected_objects if o.type == 'MESH']
    if not meshes:
        raise RuntimeError("Select mesh object(s).")

    if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')

    for obj in meshes:
        if obj.data.users > 1:
            obj.data = obj.data.copy()

        me = obj.data
        bm = bmesh.new()
        bm.from_mesh(me)
        uv_layer = bm.loops.layers.uv.verify()

        M = obj.matrix_world
        N = obj.matrix_world.to_3x3()

        for f in bm.faces:
            n_world_bl = (N @ f.normal).normalized()
            # blender -> engine (x, z, y)
            n_eng = Vector((n_world_bl.x, n_world_bl.z, n_world_bl.y))
            abs_n = Vector((abs(n_eng.x), abs(n_eng.y), abs(n_eng.z)))

            for loop in f.loops:
                p_world_bl = M @ loop.vert.co
                # blender -> engine (x, z, y)
                p_eng = Vector((p_world_bl.x, p_world_bl.z, p_world_bl.y))

                if abs_n.x > abs_n.y and abs_n.x > abs_n.z:
                    uv_y = p_eng.y / abs_n.x
                    uv_x = p_eng.z / abs_n.x
                    uv_y = 1.0 - uv_y
                    if n_eng.x > 0.0:
                        uv_x = 1.0 - uv_x
                elif abs_n.y > abs_n.x and abs_n.y > abs_n.z:
                    uv_x = p_eng.x / abs_n.y
                    uv_y = p_eng.z / abs_n.y
                    uv_y = 1.0 - uv_y
                    if n_eng.y < 0.0:
                        uv_x = 1.0 - uv_x
                else:
                    uv_x = p_eng.x / abs_n.z
                    uv_y = p_eng.y / abs_n.z
                    uv_y = 1.0 - uv_y
                    if n_eng.z < 0.0:
                        uv_x = 1.0 - uv_x
                        
                        
                uv_y = 1.0 - uv_y
                uv_x = 1.0 - uv_x

                uv_x = uv_x * TEX_SCALE + TEX_OFFSET_X
                uv_y = uv_y * TEX_SCALE + TEX_OFFSET_Y

                loop[uv_layer].uv = (uv_x, uv_y)

        bm.to_mesh(me)
        bm.free()
        me.update()

project_selected_objects()