import bpy, struct, time, os, mathutils, numpy as np

# AABB class using NumPy for min/max calculations.
class AABB:
    def __init__(self):
        self.min = np.array([float('inf'), float('inf'), float('inf')])
        self.max = np.array([float('-inf'), float('-inf'), float('-inf')])
    def update_np(self, point):
        self.min = np.minimum(self.min, point)
        self.max = np.maximum(self.max, point)
    def to_tuple(self):
        return tuple(self.min), tuple(self.max)

class ModelExporter:
    def __init__(self, filepath, rounding=6):
        self.filepath = filepath
        # Export only visible mesh objects.
        self.meshes = [
            obj for obj in bpy.context.scene.objects
            if obj.type == 'MESH' and obj.visible_get()
        ]
        # Map mesh name -> its index in self.meshes, for parentIndex lookup
        self.mesh_index_map = {obj.name: i for i, obj in enumerate(self.meshes)}
        self.mesh_aabbs = {}
        self.rounding = rounding
        # Conversion matrix from Blender's Z-up to Y-up.
        self.conv_mat = mathutils.Matrix((
            (1, 0, 0),
            (0, 0, 1),
            (0, -1, 0)
        ))

    def triangulate_mesh(self, obj):
        depsgraph = bpy.context.evaluated_depsgraph_get()
        mesh_eval = obj.evaluated_get(depsgraph).to_mesh()
        mesh_eval.calc_loop_triangles()  # Ensure triangulation.
        return mesh_eval

    def compute_aabb(self, mesh, obj):
        aabb = AABB()
        world_mat = obj.matrix_world
        conv_mat = self.conv_mat
        for vertex in mesh.vertices:
            world_v = world_mat @ vertex.co
            conv_v = conv_mat @ world_v
            aabb.update_np(np.array([conv_v.x, conv_v.y, conv_v.z]))
        return aabb

    def export(self):
        start = time.time()

        # First, compute per-mesh AABBs
        for obj in self.meshes:
            mesh_eval = self.triangulate_mesh(obj)
            self.mesh_aabbs[obj.name] = self.compute_aabb(mesh_eval, obj)
            obj.to_mesh_clear()

        # Compute overall scene AABB
        scene_aabb = AABB()
        for aabb in self.mesh_aabbs.values():
            scene_aabb.update_np(aabb.min)
            scene_aabb.update_np(aabb.max)

        with open(self.filepath, 'wb') as file:
            # signature[32]
            model_sig = b"HELL_MODEL"
            file.write(model_sig.ljust(32, b'\0'))
            # uint32_t version
            file.write(struct.pack('<I', 2))
            # uint32_t meshCount
            file.write(struct.pack('<I', len(self.meshes)))
            # uint64_t timestamp
            timestamp = int(time.time())
            file.write(struct.pack('<Q', timestamp))
            # glm::vec3 aabbMin, aabbMax
            min_t, max_t = scene_aabb.to_tuple()
            file.write(struct.pack('<3f', *min_t))
            file.write(struct.pack('<3f', *max_t))

            # --- Each MeshHeader + vertex/index data ---
            for obj in self.meshes:
                mesh_eval = self.triangulate_mesh(obj)
                if mesh_eval.uv_layers.active:
                    mesh_eval.calc_tangents()

                # prepare unique_vertices & indices
                vertex_map = {}
                unique_vertices = []
                indices = []
                world_mat = obj.matrix_world
                world_mat3 = world_mat.to_3x3()
                conv_mat = self.conv_mat

                for tri in mesh_eval.loop_triangles:
                    for loop_index in tri.loops:
                        loop = mesh_eval.loops[loop_index]
                        v_idx = loop.vertex_index
                        vert = mesh_eval.vertices[v_idx]

                        world_v = world_mat @ vert.co
                        conv_v = conv_mat @ world_v

                        world_n = (world_mat3 @ loop.normal).normalized()
                        conv_n = (conv_mat @ world_n).normalized()

                        if mesh_eval.uv_layers.active:
                            uv_data = mesh_eval.uv_layers.active.data[loop_index].uv
                            uv = (uv_data.x, 1.0 - uv_data.y)
                            world_t = (world_mat3 @ loop.tangent).normalized()
                            conv_t = (conv_mat @ world_t).normalized()
                        else:
                            uv = (0.0, 0.0)
                            conv_t = mathutils.Vector((0.0, 0.0, 0.0))

                        key = (
                            round(conv_v.x, self.rounding),
                            round(conv_v.y, self.rounding),
                            round(conv_v.z, self.rounding),
                            round(uv[0], self.rounding),
                            round(uv[1], self.rounding)
                        )
                        found = False
                        if key in vertex_map:
                            for i, (e_idx, s_norm, s_tan, cnt) in enumerate(vertex_map[key]):
                                avg_norm = s_norm / cnt
                                if avg_norm.dot(conv_n) >= 0.95:
                                    vertex_map[key][i] = (
                                        e_idx,
                                        s_norm + conv_n,
                                        s_tan + conv_t,
                                        cnt + 1
                                    )
                                    indices.append(e_idx)
                                    found = True
                                    break

                        if not found:
                            idx = len(unique_vertices)
                            vertex_map.setdefault(key, []).append(
                                (idx, conv_n.copy(), conv_t.copy(), 1)
                            )
                            unique_vertices.append([
                                conv_v.x, conv_v.y, conv_v.z,
                                conv_n.x, conv_n.y, conv_n.z,
                                uv[0], uv[1],
                                conv_t.x, conv_t.y, conv_t.z
                            ])
                            indices.append(idx)

                # finalize normals & tangents
                for bucket in vertex_map.values():
                    for (idx, s_norm, s_tan, cnt) in bucket:
                        avg_n = (s_norm / cnt).normalized()
                        avg_t = (s_tan / cnt).normalized()
                        unique_vertices[idx][3:6] = [avg_n.x, avg_n.y, avg_n.z]
                        unique_vertices[idx][8:11] = [avg_t.x, avg_t.y, avg_t.z]

                vertex_count = len(unique_vertices)
                index_count = len(indices)
                mesh_aabb = self.mesh_aabbs[obj.name]
                min_m, max_m = mesh_aabb.to_tuple()

                # --- MeshHeader ---
                # char signature[32]
                mesh_sig = b"HELL_MESH"
                file.write(mesh_sig.ljust(32, b'\0'))
                # char name[256]
                name_bytes = obj.name.encode('utf-8')
                name_bytes = name_bytes[:256]
                file.write(name_bytes.ljust(256, b'\0'))
                # uint32_t vertexCount, uint32_t indexCount
                file.write(struct.pack('<I', vertex_count))
                file.write(struct.pack('<I', index_count))
                # int32_t parentIndex
                parent = obj.parent
                parent_idx = (
                    self.mesh_index_map[parent.name]
                    if parent and parent.name in self.mesh_index_map
                    else -1
                )
                file.write(struct.pack('<i', parent_idx))
                # glm::vec3 aabbMin, aabbMax
                file.write(struct.pack('<3f', *min_m))
                file.write(struct.pack('<3f', *max_m))

                # Write identity matrix for localTransform and inverseBindTransform
                identity_matrix = [1.0, 0.0, 0.0, 0.0,
                                   0.0, 1.0, 0.0, 0.0,
                                   0.0, 0.0, 1.0, 0.0,
                                   0.0, 0.0, 0.0, 1.0]
                file.write(struct.pack('<16f', *identity_matrix))  # localTransform
                file.write(struct.pack('<16f', *identity_matrix))  # inverseBindTransform

                # --- vertex & index data ---
                for v in unique_vertices:
                    file.write(struct.pack('<3f3f2f3f', *v))
                for i in indices:
                    file.write(struct.pack('<I', i))

                obj.to_mesh_clear()

        end = time.time()
        print("Export took", end - start, "seconds")

# Example usage:
exporter = ModelExporter(
    'C:/Hell2025/Hell2025/Hell2025/res/models/Piano.model',
    rounding=6
)
exporter.export()
