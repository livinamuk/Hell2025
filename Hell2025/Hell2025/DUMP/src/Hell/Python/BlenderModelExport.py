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
        self.meshes = [obj for obj in bpy.context.scene.objects 
                       if obj.type == 'MESH' and obj.visible_get()]
        self.mesh_aabbs = {}
        self.rounding = rounding
        # Conversion matrix from Blender's Z-up to Y-up.
        self.conv_mat = mathutils.Matrix(((1, 0, 0),
                                          (0, 0, 1),
                                          (0, -1, 0)))
    
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
        with open(self.filepath, 'wb') as file:
            # Write header signature (10 bytes).
            signature = b"HELL_MODEL"
            file.write(signature)
            
            # Prepare header fields.
            version = 1
            mesh_count = len(self.meshes)
            base_name = os.path.splitext(os.path.basename(self.filepath))[0]
            model_name = base_name.encode('utf-8')
            name_length = len(model_name)
            timestamp = int(time.time())
            
            # Compute per-mesh AABBs.
            for obj in self.meshes:
                mesh_eval = self.triangulate_mesh(obj)
                self.mesh_aabbs[obj.name] = self.compute_aabb(mesh_eval, obj)
                obj.to_mesh_clear()
            
            # Compute overall scene AABB.
            scene_aabb = AABB()
            for aabb in self.mesh_aabbs.values():
                scene_aabb.update_np(aabb.min)
                scene_aabb.update_np(aabb.max)
            
            # Write fixed header fields.
            file.write(struct.pack('<I', version))
            file.write(struct.pack('<I', mesh_count))
            file.write(struct.pack('<I', name_length))
            file.write(struct.pack('<Q', timestamp))
            min_tuple, max_tuple = scene_aabb.to_tuple()
            file.write(struct.pack('<3f', *min_tuple))
            file.write(struct.pack('<3f', *max_tuple))
            # Write the model name immediately after the header.
            file.write(model_name)
            
            # Process and export each mesh.
            for obj in self.meshes:
                mesh_eval = self.triangulate_mesh(obj)
                if mesh_eval.uv_layers.active:
                    mesh_eval.calc_tangents()
                    
                mesh_name = obj.name.encode('utf-8')
                mesh_name_length = len(mesh_name)
                mesh_aabb = self.mesh_aabbs[obj.name]
                
                world_mat = obj.matrix_world
                world_mat3 = world_mat.to_3x3()
                conv_mat = self.conv_mat
                
                # Build unique vertices and index buffer.
                vertex_map = {}  # key -> (index, sum_normal, sum_tangent, count)
                unique_vertices = []  # Each vertex: [pos.x, pos.y, pos.z, norm.x, norm.y, norm.z, uv.x, uv.y, tan.x, tan.y, tan.z]
                indices = []
                
                for tri in mesh_eval.loop_triangles:
                    for loop_index in tri.loops:
                        loop = mesh_eval.loops[loop_index]
                        vert_idx = loop.vertex_index
                        vertex = mesh_eval.vertices[vert_idx]
                        
                        # Transform vertex position.
                        world_v = world_mat @ vertex.co
                        conv_v = conv_mat @ world_v
                        
                        # Transform and normalize normal.
                        world_normal = (world_mat3 @ loop.normal).normalized()
                        conv_normal = (conv_mat @ world_normal).normalized()
                        
                        # Process UV and tangent data.
                        if mesh_eval.uv_layers.active:
                            uv_data = mesh_eval.uv_layers.active.data[loop_index].uv
                            uv = (uv_data.x, 1.0 - uv_data.y)
                            world_tangent = (world_mat3 @ loop.tangent).normalized()
                            conv_tangent = (conv_mat @ world_tangent).normalized()
                        else:
                            uv = (0.0, 0.0)
                            conv_tangent = (0.0, 0.0, 0.0)
                        
                        # Deduplication key based on rounded position and UV.
                        key = (round(conv_v.x, self.rounding), round(conv_v.y, self.rounding), round(conv_v.z, self.rounding),
                               round(uv[0], self.rounding), round(uv[1], self.rounding))
                        
                        found = False
                        if key in vertex_map:
                            for i, (existing_idx, sum_norm, sum_tan, count) in enumerate(vertex_map[key]):
                                avg_norm = sum_norm / count
                                if avg_norm.dot(conv_normal) >= 0.95:
                                    vertex_map[key][i] = (existing_idx, sum_norm + conv_normal, sum_tan + conv_tangent, count + 1)
                                    indices.append(existing_idx)
                                    found = True
                                    break
                        if not found:
                            idx = len(unique_vertices)
                            vertex_map.setdefault(key, []).append((idx, conv_normal.copy(), conv_tangent.copy(), 1))
                            unique_vertices.append([conv_v.x, conv_v.y, conv_v.z,
                                                     conv_normal.x, conv_normal.y, conv_normal.z,
                                                     uv[0], uv[1],
                                                     conv_tangent.x, conv_tangent.y, conv_tangent.z])
                            indices.append(idx)
                
                # Average normals and tangents for merged vertices.
                for bucket in vertex_map.values():
                    for (idx, sum_norm, sum_tan, count) in bucket:
                        avg_norm = sum_norm / count
                        avg_norm.normalize()
                        avg_tan = sum_tan / count
                        avg_tan.normalize()
                        unique_vertices[idx][3:6] = [avg_norm.x, avg_norm.y, avg_norm.z]
                        unique_vertices[idx][8:11] = [avg_tan.x, avg_tan.y, avg_tan.z]
                
                vertex_count = len(unique_vertices)
                index_count = len(indices)
                
                # Write mesh header.
                file.write(struct.pack('<I', mesh_name_length))
                file.write(struct.pack('<I', vertex_count))
                file.write(struct.pack('<I', index_count))
                file.write(mesh_name)
                min_tuple, max_tuple = mesh_aabb.to_tuple()
                file.write(struct.pack('<3f', *min_tuple))
                file.write(struct.pack('<3f', *max_tuple))
                
                # Write vertex and index data.
                for vert in unique_vertices:
                    file.write(struct.pack('<3f3f2f3f', *vert))
                for idx in indices:
                    file.write(struct.pack('<I', idx))
                
                obj.to_mesh_clear()
        end = time.time()
        print("Export took", end - start, "seconds")

# Example usage:
exporter = ModelExporter('C:/Hell2025/Hell2025/Hell2025/res/models/Boards.model', rounding=6)
exporter.export()
