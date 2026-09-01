import bpy, struct, time, mathutils, numpy as np, bmesh

def make_conversion_matrices():
    conv3 = mathutils.Matrix(((1,0,0), (0,0,1), (0,-1,0)))
    conv4 = mathutils.Matrix(((1,0,0,0), (0,0,1,0), (0,-1,0,0), (0,0,0,1)))
    return conv3, conv4, conv4.inverted()

class AABB:
    def __init__(self):
        self.min = np.array([np.inf, np.inf, np.inf])
        self.max = np.array([-np.inf, -np.inf, -np.inf])
    def update(self, p):
        self.min = np.minimum(self.min, p)
        self.max = np.maximum(self.max, p)
    def to_tuple(self):
        return tuple(self.min), tuple(self.max)

class ModelExporter:
    def __init__(self, filepath, rounding=6, selected_only=False, scale=1.0, include_non_deforming=True, export_armature=False):
        self.filepath = filepath
        self.rounding = int(rounding)
        self.selected_only = bool(selected_only)
        self.scale = float(scale)
        self.include_non_deforming = bool(include_non_deforming)
        self.export_armature = bool(export_armature)

        if self.selected_only:
            sel = bpy.context.selected_objects
            self.all_meshes = [o for o in sel if o.type == 'MESH' and o.visible_get()]
            self.all_armatures = [o for o in sel if o.type == 'ARMATURE' and o.visible_get()]
        else:
            objs = bpy.context.scene.objects
            self.all_meshes = [o for o in objs if o.type == 'MESH' and o.visible_get()]
            self.all_armatures = [o for o in objs if o.type == 'ARMATURE' and o.visible_get()]

        self.sorted_meshes = self._sort_meshes_by_parent()
        self.mesh_index_map = {o.name: i for i, o in enumerate(self.sorted_meshes)}
        self.conv3, self.conv4, self.conv4_inv = make_conversion_matrices()

    def _sort_meshes_by_parent(self):
        parent_map = {o.name: o.parent for o in self.all_meshes}
        out, seen = [], set()
        def add(o):
            if o.name in seen: return
            p = parent_map.get(o.name)
            if p and p.type == 'MESH':
                add(p)
            out.append(o); seen.add(o.name)
        for o in self.all_meshes: add(o)
        return out

    def triangulate(self, obj):
        deps = bpy.context.evaluated_depsgraph_get()
        mesh = obj.evaluated_get(deps).to_mesh()
        bm = bmesh.new(); bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(mesh); bm.free()
        mesh.calc_loop_triangles()
        return mesh

    def compute_aabbs(self):
        self.aabbs = {}
        for obj in self.sorted_meshes:
            mesh = self.triangulate(obj)
            aabb = AABB()
            for v in mesh.vertices:
                p = (self.conv3 @ v.co) * self.scale
                self.aabbs.setdefault(obj.name, aabb)
                aabb.update(np.array([p.x, p.y, p.z]))
            self.aabbs[obj.name] = aabb
            obj.to_mesh_clear()

    def _flatten_col_major(self, m):
        return [m[r][c] for c in range(4) for r in range(4)]

    def _scale_translation_inplace(self, m):
        m[0][3] *= self.scale; m[1][3] *= self.scale; m[2][3] *= self.scale

    def _collect_bones_topo(self, arm_obj):
        bones = list(arm_obj.data.bones)
        if not self.include_non_deforming:
            bones = [b for b in bones if b.use_deform]
        included = set(bones)
        roots = [b for b in bones if b.parent is None or b.parent not in included]
        order, seen = [], set()
        def dfs(b):
            if b in seen: return
            seen.add(b)
            for c in b.children:
                if c in included: dfs(c)
            order.append(b)
        for r in roots: dfs(r)
        return order, included

    def _compute_global_rest_conv_all(self, arm_obj):
        """
        Build each bone's GLOBAL rest matrix in ENGINE space:
            G_world_blender = ArmWorld * (accumulated matrix_local)
            G_engine        = conv4 * G_world_blender * conv4_inv
        Then scale the translation once.
        """
        # Accumulate in Blender armature space first
        data_bones = list(arm_obj.data.bones)
        global_arm = {}
        roots = [b for b in data_bones if b.parent is None]
        def dfs(b):
            g = b.matrix_local.copy() if b.parent is None else (global_arm[b.parent.name] @ b.matrix_local)
            global_arm[b.name] = g
            for c in b.children: dfs(c)
        for r in roots: dfs(r)

        ArmW = arm_obj.matrix_world
        global_conv_all = {}
        for b in data_bones:
            G_world = ArmW @ global_arm[b.name]                      # Blender world
            g_conv  = self.conv4 @ G_world @ self.conv4_inv          # Engine basis
            g_conv  = g_conv.copy()
            self._scale_translation_inplace(g_conv)                  # uniform scale once
            global_conv_all[b.name] = g_conv
        return global_conv_all

    def _export_one_armature(self, f, arm_obj):
        f.write(b"HELL_ARMATURE".ljust(32, b'\0'))
        f.write(arm_obj.name.encode('utf-8')[:256].ljust(256, b'\0'))

        bones_order, included = self._collect_bones_topo(arm_obj)
        f.write(struct.pack('<I', len(bones_order)))

        global_conv_all = self._compute_global_rest_conv_all(arm_obj)
        index_of = {b.name: i for i, b in enumerate(bones_order)}

        for b in bones_order:
            g_child = global_conv_all[b.name]
            if b.parent and (b.parent in included):
                g_parent = global_conv_all[b.parent.name]
                local_conv = g_parent.inverted() @ g_child
                parent_index = index_of.get(b.parent.name, -1)
            else:
                local_conv = g_child
                parent_index = -1
            inv_bind = g_child.inverted()

            f.write(b.name.encode('utf-8')[:64].ljust(64, b'\0'))
            f.write(struct.pack('<16f', *self._flatten_col_major(local_conv)))
            f.write(struct.pack('<16f', *self._flatten_col_major(inv_bind)))
            f.write(struct.pack('<i', parent_index))
            f.write(struct.pack('<i', 1 if b.use_deform else 0))

    def export(self):
        start = time.time()
        self.compute_aabbs()

        scene_bb = AABB()
        for bb in self.aabbs.values():
            scene_bb.update(bb.min); scene_bb.update(bb.max)

        mesh_count = len(self.sorted_meshes)
        armature_count = len(self.all_armatures)

        with open(self.filepath, 'wb') as f:
            f.write(b"HELL_MODEL".ljust(32, b'\0'))
            f.write(struct.pack('<I', 3))
            f.write(struct.pack('<I', mesh_count))
            f.write(struct.pack('<I', armature_count))
            f.write(struct.pack('<Q', int(time.time())))
            mn, mx = scene_bb.to_tuple()
            f.write(struct.pack('<3f', *mn))
            f.write(struct.pack('<3f', *mx))

            for obj in self.sorted_meshes:
                mesh = self.triangulate(obj)
                if mesh.uv_layers.active:
                    try:
                        mesh.calc_tangents()
                    except RuntimeError:
                        pass

                vert_map, verts, indices = {}, [], []

                for tri in mesh.loop_triangles:
                    for li in tri.loops:
                        loop = mesh.loops[li]
                        v = mesh.vertices[loop.vertex_index]

                        p3 = (self.conv3 @ v.co) * self.scale
                        n3 = (self.conv3 @ loop.normal).normalized()
                        t3 = (self.conv3 @ loop.tangent).normalized() if mesh.uv_layers.active else mathutils.Vector((0,0,0))

                        if mesh.uv_layers.active:
                            uv = mesh.uv_layers.active.data[li].uv
                            uv2 = (uv.x, 1.0 - uv.y)
                        else:
                            uv = (0.0, 0.0)
                            uv2 = (0.0, 0.0)

                        key = (round(p3.x, self.rounding), round(p3.y, self.rounding), round(p3.z, self.rounding),
                               round(uv2[0], self.rounding), round(uv2[1], self.rounding))
                        bucket = vert_map.setdefault(key, [])
                        found = False
                        for idx, sum_n, sum_t, cnt in bucket:
                            if (sum_n / cnt).dot(n3) >= 0.95:
                                bucket[bucket.index((idx, sum_n, sum_t, cnt))] = (idx, sum_n + n3, sum_t + t3, cnt + 1)
                                indices.append(idx); found = True; break
                        if not found:
                            idx = len(verts)
                            bucket.append((idx, n3.copy(), t3.copy(), 1))
                            verts.append([p3.x, p3.y, p3.z, n3.x, n3.y, n3.z, uv2[0], uv2[1], t3.x, t3.y, t3.z])
                            indices.append(idx)

                for bucket in vert_map.values():
                    for idx, sum_n, sum_t, cnt in bucket:
                        avg_n = (sum_n / cnt).normalized()
                        avg_t = (sum_t / cnt).normalized() if (sum_t.length > 0.0) else mathutils.Vector((0,0,0))
                        verts[idx][3:6] = [avg_n.x, avg_n.y, avg_n.z]
                        verts[idx][8:11] = [avg_t.x, avg_t.y, avg_t.z]

                bb = self.aabbs[obj.name]; mn, mx = bb.to_tuple()
                f.write(b"HELL_MESH".ljust(32, b'\0'))
                f.write(obj.name.encode('utf-8')[:256].ljust(256, b'\0'))
                f.write(struct.pack('<I', len(verts)))
                f.write(struct.pack('<I', len(indices)))
                parent = obj.parent.name if (obj.parent and obj.parent.type == 'MESH') else None
                pi = self.mesh_index_map.get(parent, -1)
                f.write(struct.pack('<i', pi))
                f.write(struct.pack('<3f', *mn))
                f.write(struct.pack('<3f', *mx))

                W = self.conv4 @ obj.matrix_world @ self.conv4_inv
                if obj.parent and obj.parent.type == 'MESH':
                    P = self.conv4 @ obj.parent.matrix_world @ self.conv4_inv
                    local4 = P.inverted() @ W
                else:
                    local4 = W
                local4 = local4.copy()
                local4[0][3] *= self.scale; local4[1][3] *= self.scale; local4[2][3] *= self.scale
                inv4 = local4.inverted()

                flat_l = [local4[r][c] for c in range(4) for r in range(4)]
                flat_i = [inv4[r][c] for c in range(4) for r in range(4)]
                f.write(struct.pack('<16f', *flat_l))
                f.write(struct.pack('<16f', *flat_i))

                for vtx in verts: f.write(struct.pack('<3f3f2f3f', *vtx))
                for i in indices: f.write(struct.pack('<I', i))

                obj.to_mesh_clear()

            if (self.export_armature):
                for arm in self.all_armatures:
                    self._export_one_armature(f, arm)

        print(f"Export took {time.time()-start:.2f}s")

# Usage
exporter = ModelExporter(
    'C:/Hell2025/Hell2025/Hell2025/res/models/Name.model',
    rounding=6,
    selected_only=True,
    scale=1.0,
    include_non_deforming=True,
    export_armature=False
)
exporter.export()
