import hou
import json
import os

name = "Blood8_optimized"
node_path = "/obj/geo1/OUT_POINTS"
scatter_node_path = "/obj/geo1/POINT_SCATTER"
output_directory = r"C:/Hell2025/Hell2025/Hell2025/res/PointAnimations"
out_path = os.path.join(output_directory, "{}.json".format(name))

frame_end = 60
point_count_frame = 8
rounding = 6

node = hou.node(node_path)
if node is None:
    raise RuntimeError("Could not find point animation node: {}".format(node_path))

scatter_node = hou.node(scatter_node_path)
if scatter_node is None:
    raise RuntimeError("Could not find point scatter node: {}".format(scatter_node_path))

original_frame = hou.frame()
try:
    hou.setFrame(point_count_frame)
    scatter_geo = scatter_node.geometry()
    point_count = len(scatter_geo.points())
finally:
    hou.setFrame(original_frame)

if point_count == 0:
    raise RuntimeError(
        "Point scatter node {} has no points at frame {}".format(
            scatter_node_path, point_count_frame
        )
    )

print(
    "Using {} points from {} at frame {}".format(
        point_count, scatter_node_path, point_count_frame
    )
)

data = {
    "type": "PointAnimation",
    "fps": float(hou.fps()),
    "frameCount": frame_end + 1,
    "pointCount": point_count,
    "frames": []
}

for frame in range(0, frame_end + 1):
    hou.setFrame(frame)

    geo = node.geometry()
    points = list(geo.points())

    if len(points) == 0:
        data["frames"].append([])
        continue

    id_attrib = geo.findPointAttrib("id")
    if id_attrib is None:
        raise RuntimeError("Point animation export requires an integer point attribute named 'id'")

    if len(points) != point_count:
        raise RuntimeError(
            "Frame {} has {} points, expected {}".format(frame, len(points), point_count)
        )

    points.sort(key=lambda p: p.intAttribValue(id_attrib))

    frame_points = []
    expected_id = 0

    for p in points:
        point_id = p.intAttribValue(id_attrib)
        if point_id != expected_id:
            raise RuntimeError(
                "Frame {} has point id {}, expected {}".format(frame, point_id, expected_id)
            )

        pos = p.position()
        frame_points.append([
            round(pos.x(), rounding),
            round(pos.y(), rounding),
            round(pos.z(), rounding)
        ])
        expected_id += 1

    data["frames"].append(frame_points)

os.makedirs(os.path.dirname(out_path), exist_ok=True)

with open(out_path, "w") as f:
    json.dump(data, f, indent=2)

print("Exported point animation:", out_path)
