import ast
import bpy
import json
import math
import os
import re
from bpy_extras import anim_utils


# Curve-only output. This script writes no FBX and no mesh geometry.
export_path = "C:/Animations/"


def safe_filename(name):
    name = re.sub(r'[<>:"/\\|?*]', "_", name).strip(" .")
    return name or "UnnamedShapeAnimation"


def shape_target_from_data_path(data_path):
    match = re.fullmatch(r"key_blocks\[(.+)\]\.value", data_path)
    if match is None:
        return None

    try:
        return ast.literal_eval(match.group(1))
    except (SyntaxError, ValueError):
        return None


def mesh_names_using(shape_keys):
    return sorted({
        obj.name
        for obj in bpy.context.scene.objects
        if obj.type == 'MESH' and obj.data.shape_keys == shape_keys
    })


def find_pose_slot(action):
    for slot in action.slots:
        if slot.target_id_type not in {'OBJECT', 'UNSPECIFIED'}:
            continue

        channelbag = anim_utils.action_get_channelbag_for_slot(action, slot)
        if channelbag is None:
            continue

        if any(
            fcurve.data_path.startswith("pose.bones[")
            for fcurve in channelbag.fcurves
        ):
            return slot

    return None


def round_sample(value):
    value = float(value)
    if abs(value) < 1.0e-7:
        return 0.0
    if abs(value - 1.0) < 1.0e-7:
        return 1.0
    return round(value, 7)


context = bpy.context
scene = context.scene
armature = context.active_object

if armature is None or armature.type != 'ARMATURE':
    raise RuntimeError("Select the animated armature and make it active.")


# Locate shape-key values driven by this armature. RatKing's Eye_Blink_L/R
# values are drivers, not Key data-block F-curves.
driven_channels = {}

for shape_keys in bpy.data.shape_keys:
    animation_data = shape_keys.animation_data
    if animation_data is None:
        continue

    mesh_names = mesh_names_using(shape_keys)
    if not mesh_names:
        continue

    for driver_fcurve in animation_data.drivers:
        target_name = shape_target_from_data_path(driver_fcurve.data_path)
        if target_name is None:
            continue
        key_block = shape_keys.key_blocks.get(target_name)
        if key_block is None:
            continue

        for mesh_name in mesh_names:
            channel_key = (mesh_name, target_name)
            sources = driven_channels.setdefault(channel_key, [])
            if all(source.as_pointer() != key_block.as_pointer() for source in sources):
                sources.append(key_block)


if not driven_channels:
    raise RuntimeError(
        f"No shape-key values driven by armature '{armature.name}' were found."
    )


pose_actions = []
for action in bpy.data.actions:
    slot = find_pose_slot(action)
    if slot is not None:
        pose_actions.append((action, slot))

if not pose_actions:
    raise RuntimeError(f"No pose Actions were found for armature '{armature.name}'.")


animation_data = armature.animation_data_create()
original_action = animation_data.action
original_slot = animation_data.action_slot
original_frame = scene.frame_current
original_subframe = scene.frame_subframe

fps = scene.render.fps / scene.render.fps_base
os.makedirs(export_path, exist_ok=True)
exported_count = 0
skipped_count = 0


try:
    for action, slot in sorted(pose_actions, key=lambda item: item[0].name.casefold()):
        animation_data.action = action
        animation_data.action_slot = slot

        start_frame = math.floor(action.frame_range[0])
        end_frame = math.ceil(action.frame_range[1])
        frames = list(range(start_frame, end_frame + 1))

        samples_by_channel = {
            channel_key: []
            for channel_key in driven_channels
        }

        for frame in frames:
            scene.frame_set(frame)
            context.view_layer.update()

            for channel_key, source_blocks in driven_channels.items():
                values = [round_sample(block.value) for block in source_blocks]
                value = values[0]

                if any(abs(other - value) > 1.0e-5 for other in values[1:]):
                    mesh_name, target_name = channel_key
                    raise RuntimeError(
                        f"Conflicting driven values for {mesh_name}:{target_name} "
                        f"at frame {frame}: {values}"
                    )

                samples_by_channel[channel_key].append(value)

        # Merge meshes whose identically named target has identical samples.
        merged_channels = {}
        for (mesh_name, target_name), samples in samples_by_channel.items():
            # A missing channel means weight zero, so omit channels that remain zero.
            if not any(abs(value) > 1.0e-7 for value in samples):
                continue

            merge_key = (target_name, tuple(samples))
            merged_channels.setdefault(merge_key, []).append(mesh_name)

        channels = [
            {
                "target": target_name,
                "mesh_objects": sorted(mesh_names),
                "samples": list(samples),
            }
            for (target_name, samples), mesh_names in merged_channels.items()
        ]
        channels.sort(key=lambda channel: (channel["target"], channel["mesh_objects"]))

        file_name = safe_filename(action.name) + ".shapeanim.json"
        full_path = os.path.join(export_path, file_name)

        if not channels:
            skipped_count += 1
            if os.path.isfile(full_path):
                os.remove(full_path)
                print(f"Skipped: {action.name} (removed stale empty export)")
            else:
                print(f"Skipped: {action.name} (no animated shape-key data)")
            continue

        output = {
            "format": "HellShapeAnimation",
            "version": 2,
            "clip": action.name,
            "frames_per_second": fps,
            "frame_start": start_frame,
            "frame_end": end_frame,
            "sample_step_frames": 1,
            "duration_seconds": (
                (end_frame - start_frame) / fps
                if fps > 0.0
                else 0.0
            ),
            "channels": channels,
        }

        with open(full_path, "w", encoding="utf-8", newline="\n") as output_file:
            json.dump(output, output_file, ensure_ascii=False, separators=(",", ":"))
            output_file.write("\n")

        exported_count += 1
        print(
            f"Exported: {file_name} "
            f"({len(channels)} driven shape channel groups)"
        )

finally:
    animation_data.action = original_action
    if original_action is not None and original_slot is not None:
        animation_data.action_slot = original_slot
    scene.frame_set(original_frame, subframe=original_subframe)


print(
    f"Finished exporting {exported_count} shape animation file(s); "
    f"skipped {skipped_count} action(s) with no shape-key data."
)
