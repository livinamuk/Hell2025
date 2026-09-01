#version 460 core
layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;
layout (location = 6) out vec4 EmssiveMask;

in vec3 Normal;
uniform int playerIndex;

void main() {

    BaseColorOut.rgb = vec3(0.2);
	BaseColorOut.a = 1.0;
    
    NormalsOut.rgb = vec3(Normal);
	NormalsOut.a = playerIndex;

    RMAOut.rgb = vec3(0.5, 0.5, 1);
	RMAOut.a = 1.0;

    EmssiveMask.rgb = vec3(0.0);
	EmssiveMask.a = 1.0;

}
