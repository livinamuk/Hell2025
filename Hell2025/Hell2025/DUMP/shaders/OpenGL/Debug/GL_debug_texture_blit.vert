#version 450

void main() {
    const vec2 position[3] = vec2[3](vec2(-1.0,-1.0), 
                                     vec2(3.0,-1.0), 
                                     vec2(-1.0,3.0)
    );

    gl_Position = vec4(position[gl_VertexID], 0.0, 1.0);
}
