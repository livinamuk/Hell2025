#version 450

uniform mat4 u_projectionView;
uniform mat4 u_view;
uniform float u_time;
uniform vec3 u_particlePosition;
uniform float u_particleRotation;
uniform float u_particleScale;

//const vec3 ORIGIN = vec3(36.25, 32.5, 37.0);
//const float SCALE = 0.1;

//const vec3 ORIGIN = vec3(26.0, 28.5, 37.0);
//const float SCALE = 0.025;

out vec2 v_uv;
out vec2 v_uvQuad;
out vec3 v_worldPos;

const int u_rowCount = 10;
const int u_columnCount = 10;
const float u_animSpeed = 200.0;

void main() {
    // Vertex positions
    const vec2 positions[6] = vec2[6](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0, -1.0),
        vec2( 1.0,  1.0)
    );

    // isolate the current vertex within the current quad
    vec2 quadPos = positions[gl_VertexID % 6];

    // Frame index
    int frameCount = u_columnCount * u_rowCount;
    int u_frameIndex = int(mod(u_time * u_animSpeed, float(frameCount)));
    //u_frameIndex = 79;

    // Quad UV
    vec2 quadUV = quadPos * 0.5 + 0.5;
    quadUV.y = 1.0 - quadUV.y;
    v_uvQuad = quadUV;

    // Frame UV
    float frameWidth = 1.0 / u_columnCount;
    float frameHeight = 1.0 / u_rowCount;
    int frameX = u_frameIndex % u_columnCount;
    int frameY = (u_frameIndex - (u_frameIndex % u_columnCount)) / u_columnCount;
    vec2 frameOffset = vec2(frameX * frameWidth, frameY * frameHeight);
    v_uv = frameOffset + quadUV * vec2(frameWidth, frameHeight);

    // 2D rotation matrix
    float angle = u_particleRotation * 0.01;
    float c = cos(angle);
    float s = sin(angle);
    mat2 rotation = mat2(c, s, -s, c);

    // Apply rotation to local quad coords
    vec2 rotatedPos = rotation * quadPos;

    float scale = u_particleScale * 1.75;

    // Make quad face the camera in 3D space
    vec3 cameraRight = vec3(u_view[0][0], u_view[1][0], u_view[2][0]);
    vec3 cameraUp = vec3(u_view[0][1], u_view[1][1], u_view[2][1]);
    v_worldPos = u_particlePosition + cameraRight * rotatedPos.x * scale + cameraUp * rotatedPos.y * scale;

    gl_Position = u_projectionView * vec4(v_worldPos, 1.0);
}