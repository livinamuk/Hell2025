#define PI 3.14159265359
#define HEIGHTMAP_SCALE_Y 40.0
#define HEIGHTMAP_SCALE_XZ 0.25
#define TERRAIN_DISPLACEMENT_ENABLED 1
#define TERRAIN_DISPLACEMENT_TESSELLATION_LEVEL 4
#define TERRAIN_DISPLACEMENT_MESH_SIZE 48.0
#define TERRAIN_DISPLACEMENT_SCALE 1.0
#define TERRAIN_VERTICAL_PROJECTION_ENABLED 1
#define TERRAIN_VERTICAL_PROJECTION_MATERIAL_MASK (1u << 2u) // Terrain material 2: RockFace
#define TILE_SIZE 24

#define SPOT_LIGHT_FLAG_CAST_SHADOWS       (1u << 0)
#define SPOT_LIGHT_FLAG_SKIP_OWNER_SHADOW   (1u << 1)
#define SPOT_LIGHT_FLAG_VIEW_DISTANCE_SCALE (1u << 2)

// Blood decal projector dimensions in local space. The model matrix supplies
// the uniform world scale; aspectScale stretches the two surface axes.
const float BLOOD_DECAL_DEPTH_SCALE = 0.2;
const float BLOOD_DECAL_MIN_NORMAL_ALIGNMENT = 0.0871557; // cos(85 degrees)

#define BLENDING_MODE_ALPHA_DISCARD    0u
#define BLENDING_MODE_BLENDED          1u
#define BLENDING_MODE_DEFAULT          2u
#define BLENDING_MODE_HAIR_UNDER_LAYER 3u
#define BLENDING_MODE_HAIR             4u
#define BLENDING_MODE_TOILET_WATER     5u
#define BLENDING_MODE_MIRROR           6u
#define BLENDING_MODE_GLASS            7u
#define BLENDING_MODE_PLASTIC          8u
#define BLENDING_MODE_DO_NOT_RENDER    9u
#define BLENDING_MODE_STAINED_GLASS    10u
#define BLENDING_MODE_UNDEFINED        11u

const vec3 UNDER_WATER_TINT = mix(vec3(0.4, 0.8, 0.6) * 1.75, vec3(0.01, 0.03, 0.04), 0.25);


vec3 FOG_COLOR = vec3(0.222, 0.233, 0.27);

// GI
//#define PROBE_NORMAL_BIAS 0.02

