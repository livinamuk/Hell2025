#pragma once

#define WORLD_UP glm::vec3(0.0f, 1.0f, 0.0f)

#define PROBE_MAX_DISTANCE_COOLDOWN 20
#define PROBE_MAX_IRRADIANCE_COOLDOWN 35

#define MAX_CHRISTMAS_LIGHTS 1024
#define MAX_SCREEN_SPACE_BLOOD_DECAL_COUNT 1024
#define MAX_BLOOD_DECAL_INDICES 1024

#define DOOR_WIDTH 0.8f
#define DOOR_HEIGHT 2.0f
#define DOOR_DEPTH 0.034f

#define AUDIO_SELECT "UI_Select.wav"

#define EDITOR_LEFT_PANEL_WIDTH 500.0f

#define HEIGHT_MAP_CHUNK_PIXEL_SIZE 32
#define HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE 8
#define HEIGHT_MAP_SIZE 256
#define HEIGHTMAP_SCALE_Y 40.0f
#define HEIGHTMAP_SCALE_XZ 0.25f
#define VERTICES_PER_CHUNK 1089
#define INDICES_PER_CHUNK 6144
#define CHUNK_COUNT_PER_MAP_CELL 8
#define MAP_CELL_WORLDSPACE_SIZE 64.0f

#define PLAYER_CAPSULE_HEIGHT 0.4f
#define PLAYER_CAPSULE_RADIUS 0.15f

#define MAX_MAP_WIDTH 10
#define MAX_MAP_DEPTH 10

#define SECTOR_SIZE_WORLD_SPACE (float(HEIGHT_MAP_SIZE) * float(HEIGHTMAP_SCALE_XZ))

#define WOUND_MASK_TEXTURE_ARRAY_SIZE 25
#define WOUND_MASK_TEXTURE_SIZE 256
#define KANGAROO_MAX_HEALTH 400

#define MAX_INVENTORY_X_SIZE 8
#define MAX_INVENTORY_Y_SIZE 4

#define DEFAULT_MATERIAL_NAME "CheckerBoard"
#define TIME_WRAP 10000.0f
#define NO_ID 0

#define PINK                glm::vec4(0.98f, 0.06f, 0.75f, 1.0f)
#define ORANGE              glm::vec4(1.00f, 0.65f, 0.00f, 1.0f)
#define BLACK               glm::vec4(0.00f, 0.00f, 0.00f, 1.0f)
#define WHITE               glm::vec4(1.00f, 1.00f, 1.00f, 1.0f)
#define RED                 glm::vec4(1.00f, 0.00f, 0.00f, 1.0f)
#define GREEN               glm::vec4(0.00f, 1.00f, 0.00f, 1.0f)
#define BLUE                glm::vec4(0.00f, 0.00f, 1.00f, 1.0f)
#define YELLOW              glm::vec4(1.00f, 1.00f, 0.00f, 1.0f)
#define PURPLE              glm::vec4(1.00f, 0.00f, 1.00f, 1.0f)
#define GREY                glm::vec4(0.25f, 0.25f, 0.25f, 1.0f)
#define LIGHT_BLUE          glm::vec4(0.00f, 1.00f, 1.00f, 1.0f)
#define LIGHT_GREEN         glm::vec4(0.16f, 0.78f, 0.23f, 1.0f)
#define LIGHT_RED           glm::vec4(0.80f, 0.05f, 0.05f, 1.0f)
#define TRANSPARENT         glm::vec4(0.00f, 0.00f, 0.00f, 0.0f)
#define GRID_COLOR          glm::vec4(0.50f, 0.50f, 0.60f, 1.0f)
#define OUTLINE_COLOR       glm::vec4(1.00f, 0.50f, 0.00f, 0.0f)
#define DEFAULT_LIGHT_COLOR glm::vec4(1.00f, 0.7799999713897705f, 0.5289999842643738f, 1.0f)

#define MAX_VIEWPORT_COUNT 4

#define HELL_CURSOR_ARROW           0x00036001
#define HELL_CURSOR_IBEAM           0x00036002
#define HELL_CURSOR_CROSSHAIR       0x00036003
#define HELL_CURSOR_HAND            0x00036004
#define HELL_CURSOR_HRESIZE         0x00036005
#define HELL_CURSOR_VRESIZE         0x00036006
