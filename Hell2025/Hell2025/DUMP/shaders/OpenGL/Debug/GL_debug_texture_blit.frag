#version 450
layout(location=0) out vec4 outColor;

layout(binding=0) uniform sampler2D u_texture;
uniform float u_scale;

void main() {
    vec2 drawSize = vec2(textureSize(u_texture, 0)) * u_scale;
    vec2 uv = gl_FragCoord.xy / drawSize;

    if (gl_FragCoord.x > drawSize.x || gl_FragCoord.y > drawSize.y || any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))))
        discard;

    outColor = texture(u_texture, uv);
}
