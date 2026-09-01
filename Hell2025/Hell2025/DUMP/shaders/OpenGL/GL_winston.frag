// By Tokyospliff �
//
// All rights reserved 2025 till forever

#version 450 core
layout (location = 0) out vec4 FinalLightingOut;

layout (binding = 0) uniform sampler2D DepthTexture;

uniform vec2 screensize;
uniform float near;
uniform float far;
uniform float u_time;

in vec3 normal;
in vec3 viewDir;

float LinearizeDepth01(float depth01) {
    float zNdc = 2.0 * depth01 - 1.0;
    float zEye = (2.0 * far * near) / ((far + near) - zNdc * (far - near));
    return (zEye - near) / (far - near); // normalized 0..1
}

float AAWIDTH(float v) {
    return max(fwidth(v), 1e-6);
}

void main() {
    vec3 N = normalize(normal);
    vec3 V = normalize(-viewDir);

    // Rim lighting (AA)
    float ndotv = clamp(abs(dot(N, V)), 0.0, 1.0);
    float rimStrength = 1.0 - ndotv; // 0 in center, 1 at grazing
    float rimW = AAWIDTH(rimStrength);

    // Rim band in rimStrength space
    float rimStart = 0.0; // when rim begins
    float rimEnd   = 0.9; // when rim is full

    float rimFactor = smoothstep(rimStart - rimW, rimEnd + rimW, rimStrength);
    rimFactor = pow(rimFactor, 2.0); 

    vec3 rimColor = vec3(0.5, 0.9, 1.0);
    rimColor = vec3(1.0);

    float rimIntensity = 0.125;
    vec4 rim = vec4(rimColor * (rimFactor * rimIntensity), rimFactor * 0.015);

    // Depth intersection line (AA)
    vec2 depthUV = gl_FragCoord.xy / screensize;

    float sceneDepth01  = texture(DepthTexture, depthUV).r;
    float bubbleDepth01 = gl_FragCoord.z;

    float sceneDepthLin  = LinearizeDepth01(sceneDepth01);
    float bubbleDepthLin = LinearizeDepth01(bubbleDepth01);

    float dist = abs(bubbleDepthLin - sceneDepthLin);

    float threshold = 0.2;
    float t = dist / threshold; // 0..1 around the intersection
    float tw = AAWIDTH(t);

    float intersectionFactor = 1.0 - smoothstep(0.0, 1.0 + tw, t);

    vec3 intersectionColor = vec3(0.5, 0.9, 1.0);
    intersectionColor = vec3(0.05);

    float intersectionIntensity = 0.025;
    vec4 intersection = vec4(intersectionColor * (intersectionFactor * intersectionIntensity), intersectionFactor * 0.20);

    // Base glow
    vec3 baseColor = vec3(0.5, 0.9, 1.0);
    baseColor = vec3(0.9, 0.9, 0.9);

    float baseIntensity = 0.00;
    float baseAlpha = 0.0125;
    vec4 bubbleBase = vec4(baseColor * baseIntensity, baseAlpha);


    // Pulse
    float speed = 15.0;
    float pulse01 = 0.5 + 0.5 * sin(u_time * speed);    
    float pulse = pow(pulse01, 1.2); // Make it linger near bright
    float basePulse = mix(0.85, 1.25, pulse);
    float rimPulse = mix(0.70, 1.40, pulse);
    float intersectionPulse = mix(0.60, 1.60, pulse);
    bubbleBase.rgb *= basePulse;
    bubbleBase.a *= basePulse;
    rim.rgb *= rimPulse;
    rim.a *= rimPulse;
    intersection.rgb *= intersectionPulse;
    intersection.a *= intersectionPulse;

    vec4 outColor = bubbleBase + intersection + rim;
    
    outColor.rgb *= 0.275;

    // Increaseeeeee
    //outColor.rgb = vec3(rimFactor = pow(outColor.r, 1.15));

    // Prevent blowout
    FinalLightingOut = clamp(outColor, 0.0, 1.0);
}
