#version 460
layout (location = 0) out float DepthOut;
in vec4 v_viewSpacePosition;

void main() {
    DepthOut = v_viewSpacePosition.z;
}
