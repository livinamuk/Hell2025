import bpy
import os
from bpy_extras import anim_utils

# folder for exported animations
export_path = "C:/Animations/"
if not os.path.exists(export_path):
    os.makedirs(export_path)

armature = bpy.context.active_object

if armature and armature.type == 'ARMATURE':
    # backup current action to restore later
    original_action = armature.animation_data.action
    
    for action in bpy.data.actions:
        # switch to current action
        armature.animation_data.action = action
        
        # 5.0 specific: get curves via the active slot's channelbag
        anim_data = armature.animation_data
        channelbag = anim_utils.action_get_channelbag_for_slot(action, anim_data.action_slot)
        
        if channelbag and channelbag.fcurves:
            # determine length based on keyframes
            frames = [p.co[0] for fcu in channelbag.fcurves for p in fcu.keyframe_points]
            start_frame = int(min(frames))
            end_frame = int(max(frames))
        else:
            start_frame, end_frame = 0, 1
        
        # update scene frames for the exporter
        bpy.context.scene.frame_start = start_frame
        bpy.context.scene.frame_end = end_frame
        
        # define file name based on animation name
        file_name = f"{action.name}.fbx"
        full_path = os.path.join(export_path, file_name)
        
        # execute export with your screenshot settings
        bpy.ops.export_scene.fbx(
            filepath=full_path,
            use_selection=True,
            object_types={'ARMATURE'},
            # transform settings
            global_scale=1.0,
            apply_scale_options='FBX_SCALE_ALL',
            axis_forward='-Z',
            axis_up='Y',
            use_space_transform=True,
            bake_space_transform=True, 
            # armature settings
            primary_bone_axis='Y',
            secondary_bone_axis='X',
            armature_nodetype='NULL',
            use_armature_deform_only=True, 
            add_leaf_bones=False, 
            # animation settings
            bake_anim=True,
            bake_anim_use_all_bones=True, 
            bake_anim_use_nla_strips=False,
            bake_anim_use_all_actions=False,
            bake_anim_force_startend_keying=True, 
            bake_anim_step=1.0,
            bake_anim_simplify_factor=0.0
        )
        
        print(f"Exported: {file_name} ({start_frame} to {end_frame})")

    # restore original state
    armature.animation_data.action = original_action
else:
    print("Please select your armature first.")
