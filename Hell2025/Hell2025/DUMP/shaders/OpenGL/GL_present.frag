#version 460 core

layout(binding = 0) uniform sampler2D u_texture;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 outColor;

const float BAYER_4X4[16] = float[](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
);

void main() {
    vec4 color = texture(u_texture, v_uv);

    ivec2 bayerPixel = ivec2(gl_FragCoord.xy) & 3;
    float threshold = (BAYER_4X4[bayerPixel.y * 4 + bayerPixel.x] + 0.5) / 16.0;
    float dither = (threshold - 0.5) / 255.0;

    outColor = vec4(clamp(color.rgb + dither, 0.0, 1.0), color.a);
}
