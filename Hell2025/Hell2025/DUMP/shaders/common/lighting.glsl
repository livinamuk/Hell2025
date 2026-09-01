#include "../common/pbr_functions.glsl"
#include "../common/types.glsl"

#if defined(VULKAN)
float ApplyIESProfilePrecomputed(vec3 lightToPointDirection, float lightDistance, Light light, texture2D iesTexture, sampler iesSampler) {
#else
float ApplyIESProfilePrecomputed(vec3 lightToPointDirection, float lightDistance, Light light, sampler2D iesSampler) {
#endif
    vec3 forward = light.forward_iesMaxIntensity.rgb;
    vec3 right = light.right_iesExposure.rgb;
    vec3 up = light.up.rgb;
    float lightRadius = light.radius;
    float vScale = light.iesVScale;
    float vBias = light.iesVBias;
    float hScale = light.iesHScale;
    float hBias = light.iesHBias;
    float maxIntensity = light.forward_iesMaxIntensity.w;
    float exposure = light.right_iesExposure.w;
    const float globalDampener = 0.005;

    if (lightDistance > lightRadius) return 0.0;

    // Project into local space
    float dotF = dot(lightToPointDirection, forward);
    float dotR = dot(lightToPointDirection, right);
    float dotU = dot(lightToPointDirection, up);

    // U
    float theta = acos(clamp(dotF, -1.0, 1.0)) * 57.29578;
    float u = theta * vScale + vBias;

    // V
    float phi = atan(dotU, dotR) * 57.29578;
    float v = abs(phi) * hScale + hBias;

    // Compute mask
#if defined(VULKAN)
    float mask = texture(sampler2D(iesTexture, iesSampler), vec2(u, v)).r;
#else
    float mask = texture(iesSampler, vec2(u, v)).r;
#endif
    float atten = pow(clamp(1.0 - pow(lightDistance / lightRadius, 4.0), 0.0, 1.0), 2.0) / (lightDistance * lightDistance + 1.0);
    return mask * maxIntensity * atten * exposure * globalDampener;
}

#if defined(VULKAN)
float ApplyIESProfile(vec3 worldPos, Light light, texture2D iesTexture, sampler iesSampler) {
    vec3 lightToPoint = worldPos - vec3(light.posX, light.posY, light.posZ);
    float lightDistance = length(lightToPoint);
    return ApplyIESProfilePrecomputed(lightToPoint / max(lightDistance, 0.000001), lightDistance, light, iesTexture, iesSampler);
}
#else
float ApplyIESProfile(vec3 worldPos, Light light, sampler2D iesSampler) {
    vec3 lightToPoint = worldPos - vec3(light.posX, light.posY, light.posZ);
    float lightDistance = length(lightToPoint);
    return ApplyIESProfilePrecomputed(lightToPoint / max(lightDistance, 0.000001), lightDistance, light, iesSampler);
}
#endif

vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 toLight = lightPos - WorldPos;
    float dist = length(toLight);
    vec3 lightDir = toLight / dist;
    vec3 viewDir = normalize(viewPos - WorldPos);
    float att = smoothstep(radius, 0.0, dist) * strength;
    float ndl = max(dot(Normal, lightDir), 0.0);

    // Hack to lesson nDotL blowouts from IES profile
    float wrap = 0.125;
    ndl = clamp((ndl + wrap) / (1.0 + wrap), 0.0, 1.0);

    vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, 1.0, roughness);
    return brdf * ndl * att * clamp(lightColor, 0.0, 1.0);
}

vec3 GetDirectLightingSpecularOnlyOLD(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 toLight = lightPos - WorldPos;
    float dist = length(toLight);
    vec3 lightDir = toLight / dist;
    vec3 viewDir = normalize(viewPos - WorldPos);
    float att = smoothstep(radius, 0.0, dist) * strength;
    float ndl = max(dot(Normal, lightDir), 0.0) * att;
    vec3 brdf = microfacetBRDFSpecularOnly(lightDir, viewDir, Normal, baseColor, metallic, 1.0, roughness);
    return brdf * ndl * clamp(lightColor, 0.0, 1.0);
}

vec3 GetDirectLightingSpecularOnly(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 toLight = lightPos - WorldPos;
    float dist = length(toLight);
    vec3 lightDir = toLight / dist;
    vec3 viewDir = normalize(viewPos - WorldPos);

    // falloff and light intensity
    float att = smoothstep(radius, 0.0, dist) * strength;
    float ndl = max(dot(Normal, lightDir), 0.0) * att;

    // calculate surface reflection only
    vec3 brdf = microfacetBRDFSpecularOnly(lightDir, viewDir, Normal, baseColor, metallic, 1.0, roughness);

    // return the specular highlight for this light
    return brdf * ndl * clamp(lightColor, 0.0, 1.0);
}

vec3 GetDirectionalLighting(vec3 lightDir, vec3 lightColor, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 viewDir = normalize(viewPos - WorldPos);
    float ndl = max(dot(Normal, lightDir), 0.0) * strength;
    vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, 1.0, roughness);
    return brdf * ndl * clamp(lightColor, 0.0, 1.0);
}


vec3 GetSpotlightLighting(vec3 lightPos, vec3 lightDir, vec3 lightColor, float radius, float strength, float innerAngle, float outerAngle, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos, mat4 LightViewProj, float distanceFalloffExponent) {
    vec3 d = lightPos - WorldPos;
    float dist = length(d);
    vec3 toLight = d / dist;
    vec3 viewDir = normalize(viewPos - WorldPos);

    // distance fall-off + strength
    float lightAttenuation = smoothstep(radius, 0.0, dist) * strength;

    // cone fall-off
    float spotFactor = smoothstep(outerAngle, innerAngle, dot(toLight, -lightDir));

    // extra smooth fade by distance
    float distanceFactor = clamp(1.0 - dist / radius, 0.0, 1.0);
    spotFactor *= pow(distanceFactor, max(distanceFalloffExponent, 0.001));

    // lambert
    float irradiance = max(dot(toLight, Normal), 0.0) * lightAttenuation * spotFactor;

    vec3 brdf = microfacetBRDF(toLight, viewDir, Normal, baseColor, metallic, 1.0, roughness);
    return brdf * irradiance * clamp(lightColor, 0.0, 1.0);
}

vec3 GetSpotlightLighting(vec3 lightPos, vec3 lightDir, vec3 lightColor, float radius, float strength, float innerAngle, float outerAngle, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos, mat4 LightViewProj) {
    return GetSpotlightLighting(lightPos, lightDir, lightColor, radius, strength, innerAngle, outerAngle, Normal, WorldPos, baseColor, roughness, metallic, viewPos, LightViewProj, 2.0);
}

// Three.js IESSpotLightNode replaces the regular cone attenuation with the IES
// attenuation. Keep Hell's BRDF and distance behavior, but do not multiply the
// measured profile by the ordinary spotlight smoothstep as well.
vec3 GetFlashlightIESLighting(vec3 lightPos, vec3 lightColor, float attenuation, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
    vec3 d = lightPos - WorldPos;
    float dist = max(length(d), 0.000001);
    vec3 toLight = d / dist;
    vec3 viewDir = normalize(viewPos - WorldPos);

    float irradiance = max(dot(toLight, Normal), 0.0) * attenuation;

    vec3 brdf = microfacetBRDF(toLight, viewDir, Normal, baseColor, metallic, 1.0, roughness);
    return brdf * irradiance * clamp(lightColor, 0.0, 1.0);
}

float SpotlightShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, vec3 fragWorldPos, vec3 lightPos, vec3 viewPos, sampler2DArray shadowMapArray, int layerIndex) {
    // Project and bias
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;

    // Outside the flashlight's shadow camera means "not represented by this
    // shadow map", not "sample its clamped border". The latter creates a
    // visible square around wide IES profiles.
    if (fragPosLightSpace.w <= 0.0 || any(lessThan(projCoords, vec3(0.0))) || any(greaterThan(projCoords, vec3(1.0)))) {
        return 0.0;
    }

    float currentDepth = projCoords.z;

    // Fold slope bias and constant bias into one
    float dist = length(lightPos - fragWorldPos);
    float bias = 0.0001 + 0.028/(dist + 0.001);

    // Precompute texel size
    ivec2 size = textureSize(shadowMapArray, 0).xy;
    vec2 texelSize = 1.0/vec2(size);

    // PCF over 55 kernel
    float shadow = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float d = texture(shadowMapArray, vec3(projCoords.xy + vec2(x, y)*texelSize, layerIndex)).r;
            shadow += (currentDepth - bias > d) ? 1.0 : 0.0;
        }
    }

    // Average via multiply
    return shadow * (1.0 / 25.0);
}

float GetSpotlightVisibilitySingleSample(vec4 fragPosLightSpace, float lightDistance, sampler2DArray shadowMapArray, int layerIndex) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;

    if (fragPosLightSpace.w <= 0.0 || any(lessThan(projCoords, vec3(0.0))) || any(greaterThan(projCoords, vec3(1.0)))) {
        return 1.0;
    }

    float bias = 0.0001 + 0.028 / (lightDistance + 0.001);
    float closestDepth = texture(shadowMapArray, vec3(projCoords.xy, layerIndex)).r;

    return projCoords.z - bias <= closestDepth ? 1.0 : 0.0;
}




#if !defined(VULKAN)
float ApplyFlashlightIESProfile(vec3 worldPos, vec3 lightPos, vec3 lightDir, RendererData flashlightSettings, sampler2D iesTexture) {
    vec3 lightVector = lightPos - worldPos;
    float lightDistance = length(lightVector);
    if (lightDistance <= 0.000001) return 1.0;

    // Matches IESSpotLightNode: acos(angleCosine) drives a rotationally
    // symmetric one-dimensional IES lookup. Keep the complete measured lobe at
    // 80% of the enclosing spotlight/shadow cone so its rings are preserved and
    // its tail cannot hit the shadow boundary.
    const float iesProfileConeInset = 0.8;
    float angleCosine = dot(lightVector / lightDistance, -normalize(lightDir));
    float worldAngle = degrees(acos(clamp(angleCosine, -1.0, 1.0)));
    float safeConeScale = clamp(flashlightSettings.flashlightIESConeScale, 0.001, 1.2);

    // Recover the IES texture's authored upper angle from its scale/bias. A
    // flashlight only uses the forward hemisphere, even if the file also has
    // rear-facing photometric data.
    float authoredMaxAngle = flashlightSettings.flashlightIESVerticalScale > 0.000001
        ? (1.0 - flashlightSettings.flashlightIESVerticalBias) / flashlightSettings.flashlightIESVerticalScale
        : 90.0;
    float authoredForwardAngle = clamp(authoredMaxAngle, 0.001, 90.0);

    // With no override, preserve the IES-authored angular size. With an outer
    // angle, fit the entire authored pattern inside that cone instead of merely
    // chopping off everything (including rings) beyond the new angle.
    float enclosingConeAngle = flashlightSettings.flashlightIESOuterAngle > 0.0 ? flashlightSettings.flashlightIESOuterAngle : authoredForwardAngle;
    float iesWorldConeAngle = enclosingConeAngle * safeConeScale * iesProfileConeInset;
    float profileT = worldAngle / max(iesWorldConeAngle, 0.001);
    if (profileT > 1.0) return 0.0;

    float profileAngle = profileT * authoredForwardAngle;
    float textureU = profileAngle * flashlightSettings.flashlightIESVerticalScale + flashlightSettings.flashlightIESVerticalBias;

    // The IES file contains no authored data beyond this angular interval.
    // Returning zero also prevents the final texel interval from extrapolating
    // a non-zero tail indefinitely.
    if (textureU < 0.0 || textureU > 1.0) return 0.0;

    // Three.js squares candela values before normalizing and interpolating its
    // 180-sample spotlight texture. Reproduce that interpolation from our native
    // grid so the shared texture and point-light IES path remain untouched.
    ivec2 sampleCount = textureSize(iesTexture, 0);

    float verticalPosition = textureU * float(max(sampleCount.x - 1, 0));
    int vertical0 = 0;
    int vertical1 = 0;
    float verticalInterpolation = 0.0;
    if (sampleCount.x > 1) {
        if (verticalPosition <= 0.0) {
            vertical1 = 1;
            verticalInterpolation = verticalPosition;
        }
        else if (verticalPosition >= float(sampleCount.x - 1)) {
            vertical0 = sampleCount.x - 2;
            vertical1 = sampleCount.x - 1;
            verticalInterpolation = verticalPosition - float(vertical0);
        }
        else {
            vertical0 = int(floor(verticalPosition));
            vertical1 = vertical0 + 1;
            verticalInterpolation = fract(verticalPosition);
        }
    }

    // Three.js reduces the IES data to its theta-zero slice. Some source files
    // begin at C90, so preserve its first-interval extrapolation in that case.
    float horizontalPosition = flashlightSettings.flashlightIESHorizontalBias * float(max(sampleCount.y - 1, 0));
    int horizontal0 = 0;
    int horizontal1 = 0;
    float horizontalInterpolation = 0.0;
    if (sampleCount.y > 1) {
        if (horizontalPosition <= 0.0) {
            horizontal1 = 1;
            horizontalInterpolation = horizontalPosition;
        }
        else if (horizontalPosition >= float(sampleCount.y - 1)) {
            horizontal0 = sampleCount.y - 2;
            horizontal1 = sampleCount.y - 1;
            horizontalInterpolation = horizontalPosition - float(horizontal0);
        }
        else {
            horizontal0 = int(floor(horizontalPosition));
            horizontal1 = horizontal0 + 1;
            horizontalInterpolation = fract(horizontalPosition);
        }
    }

    float value00 = texelFetch(iesTexture, ivec2(vertical0, horizontal0), 0).r;
    float value01 = texelFetch(iesTexture, ivec2(vertical0, horizontal1), 0).r;
    float value10 = texelFetch(iesTexture, ivec2(vertical1, horizontal0), 0).r;
    float value11 = texelFetch(iesTexture, ivec2(vertical1, horizontal1), 0).r;
    float value0 = mix(value00 * value00, value01 * value01, horizontalInterpolation);
    float value1 = mix(value10 * value10, value11 * value11, horizontalInterpolation);
    float attenuation = pow(max(mix(value0, value1, verticalInterpolation), 0.0), max(flashlightSettings.flashlightIESContrast, 0.001));

    // Zero disables the optional artistic mask. This makes the IES data alone
    // define the cone at the defaults. Inner/outer are degrees at scale 1 and
    // share the same user cone scale as the IES profile and shadow projection.
    if (flashlightSettings.flashlightIESOuterAngle > 0.0) {
        float scaledOuterAngle = flashlightSettings.flashlightIESOuterAngle * safeConeScale;
        float scaledInnerAngle = clamp(flashlightSettings.flashlightIESInnerAngle, 0.0, flashlightSettings.flashlightIESOuterAngle) * safeConeScale;
        float featherWidth = scaledOuterAngle - scaledInnerAngle;
        float coneMask = featherWidth > 0.0001
            ? 1.0 - smoothstep(scaledInnerAngle, scaledOuterAngle, worldAngle)
            : (worldAngle <= scaledOuterAngle ? 1.0 : 0.0);
        attenuation *= coneMask;
    }

    return attenuation;
}

float GetFlashlightCenterSpotAttenuation(vec3 worldPos, vec3 lightPos, vec3 lightDir, RendererData flashlightSettings) {
    if (flashlightSettings.flashlightCenterSpotEnabled == 0u || flashlightSettings.flashlightCenterSpotBrightness <= 0.0) return 0.0;

    // The center spot shares the main flashlight shadow map, so keep its range
    // inside the shadow camera's far plane.
    float radius = min(
        max(flashlightSettings.flashlightCenterSpotRange, 0.001),
        max(flashlightSettings.flashlightRange, 0.001)
    );
    vec3 lightVector = lightPos - worldPos;
    float lightDistance = length(lightVector);
    if (lightDistance >= radius) return 0.0;

    float worldAngle = lightDistance > 0.000001
        ? degrees(acos(clamp(dot(lightVector / lightDistance, -normalize(lightDir)), -1.0, 1.0)))
        : 0.0;
    float outerAngle = clamp(flashlightSettings.flashlightCenterSpotOuterAngle, 0.0, 89.0);
    float innerAngle = clamp(flashlightSettings.flashlightCenterSpotInnerAngle, 0.0, outerAngle);
    if (outerAngle <= 0.0 || worldAngle >= outerAngle) return 0.0;

    float featherWidth = outerAngle - innerAngle;
    float coneAttenuation = featherWidth > 0.0001
        ? 1.0 - smoothstep(innerAngle, outerAngle, worldAngle)
        : 1.0;

    float distanceFactor = clamp(1.0 - lightDistance / radius, 0.0, 1.0);
    float distanceAttenuation = smoothstep(radius, 0.0, lightDistance);
    distanceAttenuation *= pow(distanceFactor, max(flashlightSettings.flashlightCenterSpotFalloffExponent, 0.001));

    float strength = 4.5 * max(flashlightSettings.flashlightCenterSpotBrightness, 0.0);
    return distanceAttenuation * coneAttenuation * strength;
}

float GetFlashlightIESAttenuation(vec3 worldPos, vec3 lightPos, vec3 lightDir, RendererData flashlightSettings, sampler2D iesTexture) {
    float radius = max(flashlightSettings.flashlightRange, 0.001);
    float lightDistance = distance(lightPos, worldPos);
    float iesLighting = 0.0;

    if (flashlightSettings.flashlightIESEnabled != 0u && lightDistance < radius) {
        float distanceFactor = clamp(1.0 - lightDistance / radius, 0.0, 1.0);
        float distanceAttenuation = smoothstep(radius, 0.0, lightDistance);
        distanceAttenuation *= pow(distanceFactor, max(flashlightSettings.flashlightFalloffExponent, 0.001));

        float iesAttenuation = ApplyFlashlightIESProfile(worldPos, lightPos, lightDir, flashlightSettings, iesTexture);
        float strength = 4.5 * max(flashlightSettings.flashlightBrightness, 0.0);
        iesLighting = distanceAttenuation * iesAttenuation * strength;
    }

    return iesLighting + GetFlashlightCenterSpotAttenuation(worldPos, lightPos, lightDir, flashlightSettings);
}

float GetFlashlightViewDistanceScale(float fragDistance) {
    if (fragDistance <= 1.0) return 0.5;
    if (fragDistance >= 10.0) return 1.0;
    float t = (fragDistance - 1.0) / 9.0;
    return min(mix(0.5, 5.0, t * t), 1.0);
}

vec3 GetFlashlightContribution(int flashlightIndex, uint viewportIndex, float flashlightModifer, mat4 flashlightProjectionView, vec3 flashlightDir, vec3 flashlightPosition, vec3 flashlightViewPos, bool flashlightIsInShop, RendererData flashlightSettings, vec3 normal, vec3 worldPos, vec3 baseColor, float roughness, float metallic, float fragDistance, float oceanHeight, sampler2D flashlightIESTexture, sampler2DArray flashlightShadowMapArrayTexture) {
    if (flashlightModifer <= 0.05) return vec3(0.0);

    int layerIndex = flashlightIndex;
    vec3 spotLightPos = flashlightPosition;
    vec3 spotLightDir = normalize(flashlightDir);
    vec3 spotLightColor = flashlightSettings.flashlightColor.rgb;

    // Prevent flashlight being drawn on the back of your head when viewed by another player
    if (flashlightIndex != int(viewportIndex)) {
        spotLightPos += spotLightDir * 0.2;

        // and weaken it for other players
        spotLightColor *= 0.825;
    }

    mat4 lightProjectionView = flashlightProjectionView;
    float attenuation = GetFlashlightIESAttenuation(worldPos, spotLightPos, spotLightDir, flashlightSettings, flashlightIESTexture);
    if (worldPos.y < oceanHeight - 0.1) {
        attenuation *= 2.0;
    }
    vec3 spotLighting = GetFlashlightIESLighting(spotLightPos, spotLightColor, attenuation, normal, worldPos, baseColor, roughness, metallic, flashlightViewPos);

    vec4 FragPosLightSpace = lightProjectionView * vec4(worldPos, 1.0);
    float shadow = 0;

    // If this flashlight is in the shop AND this flashlight belongs to the current viewport
    if (flashlightIndex == int(viewportIndex) && flashlightIsInShop) {
        // do nothing
    }
    else {
        shadow = SpotlightShadowCalculation(FragPosLightSpace, normal, spotLightDir, worldPos, spotLightPos, flashlightViewPos, flashlightShadowMapArrayTexture, layerIndex);
    }

    spotLighting *= GetFlashlightViewDistanceScale(fragDistance);

    spotLighting *= vec3(1 - shadow);

    return vec3(spotLighting) * flashlightModifer;
}

vec3 GetFlashlightContributionSingleSample(int flashlightIndex, uint viewportIndex, float flashlightModifer, mat4 flashlightProjectionView, vec3 flashlightDir, vec3 flashlightPosition, vec3 flashlightViewPos, bool flashlightIsInShop, RendererData flashlightSettings, vec3 normal, vec3 worldPos, vec3 baseColor, float roughness, float metallic, float fragDistance, float oceanHeight, sampler2D flashlightIESTexture, sampler2DArray flashlightShadowMapArrayTexture) {
    if (flashlightModifer <= 0.05) return vec3(0.0);

    vec3 spotLightPos = flashlightPosition;
    vec3 spotLightDir = normalize(flashlightDir);
    vec3 spotLightColor = flashlightSettings.flashlightColor.rgb;

    if (flashlightIndex != int(viewportIndex)) {
        spotLightPos += spotLightDir * 0.2;
        spotLightColor *= 0.825;
    }

    vec3 toLight = spotLightPos - worldPos;
    float lightDistance = length(toLight);
    vec3 lightDirection = toLight / max(lightDistance, 0.000001);
    float nDotL = dot(normal, lightDirection);
    if (nDotL <= 0.0) return vec3(0.0);

    float attenuation = GetFlashlightIESAttenuation(worldPos, spotLightPos, spotLightDir, flashlightSettings, flashlightIESTexture);
    if (worldPos.y < oceanHeight - 0.1) {
        attenuation *= 2.0;
    }
    if (attenuation <= 0.0) return vec3(0.0);

    float visibility = 1.0;
    if (flashlightIndex != int(viewportIndex) || !flashlightIsInShop) {
        vec4 fragPosLightSpace = flashlightProjectionView * vec4(worldPos, 1.0);
        visibility = GetSpotlightVisibilitySingleSample(fragPosLightSpace, lightDistance, flashlightShadowMapArrayTexture, flashlightIndex);
        if (visibility <= 0.0) return vec3(0.0);
    }

    vec3 viewDirection = normalize(flashlightViewPos - worldPos);
    vec3 brdf = microfacetBRDFSpecularOnly(lightDirection, viewDirection, normal, baseColor, metallic, 1.0, roughness);
    vec3 spotLighting = brdf * (nDotL * attenuation) * clamp(spotLightColor, 0.0, 1.0);
    spotLighting *= GetFlashlightViewDistanceScale(fragDistance);

    return spotLighting * visibility * flashlightModifer;
}

void GetSpotLightShadingInputs(SpotLight light, RendererData settings, uint viewportIndex, out vec3 lightPosition, out vec3 lightDirection, out vec3 lightColor) {
    lightPosition = light.positionModifier.xyz;
    lightDirection = normalize(light.direction.xyz);
    lightColor = settings.flashlightColor.rgb;

    int ownerViewportIndex = light.metadata.y;
    if (ownerViewportIndex >= 0 && ownerViewportIndex != int(viewportIndex)) {
        // Keep the player-owned light out of the back of its owner's head when
        // viewed by somebody else. World/enemy spotlights have no such offset.
        lightPosition += lightDirection * 0.2;
        lightColor *= 0.825;
    }
}

float GetSpotLightAttenuation(SpotLight light, RendererData settings, vec3 worldPos, sampler2D iesTexture) {
    float modifier = light.positionModifier.w;
    if (modifier <= 0.05) return 0.0;

    vec3 lightPosition = light.positionModifier.xyz;
    if (distance(lightPosition, worldPos) >= settings.flashlightRange) return 0.0;

    float attenuation = GetFlashlightIESAttenuation(worldPos, lightPosition, normalize(light.direction.xyz), settings, iesTexture);
    return attenuation * modifier;
}

vec3 GetSpotLightContribution(SpotLight light, RendererData settings, uint viewportIndex, vec3 viewPos, vec3 normal, vec3 worldPos, vec3 baseColor, float roughness, float metallic, float fragDistance, float oceanHeight, sampler2D iesTexture, sampler2DArray shadowMapArray) {
    float modifier = light.positionModifier.w;
    if (modifier <= 0.05) return vec3(0.0);

    vec3 lightPosition;
    vec3 lightDirection;
    vec3 lightColor;
    GetSpotLightShadingInputs(light, settings, viewportIndex, lightPosition, lightDirection, lightColor);

    vec3 toLight = lightPosition - worldPos;
    float lightDistance = length(toLight);
    if (lightDistance >= settings.flashlightRange) return vec3(0.0);

    vec3 surfaceToLight = toLight / max(lightDistance, 0.000001);
    if (dot(normal, surfaceToLight) <= 0.0) return vec3(0.0);

    float attenuation = GetFlashlightIESAttenuation(worldPos, lightPosition, lightDirection, settings, iesTexture);
    if (worldPos.y < oceanHeight - 0.1) attenuation *= 2.0;
    if (attenuation <= 0.0) return vec3(0.0);

    vec3 lighting = GetFlashlightIESLighting(lightPosition, lightColor, attenuation, normal, worldPos, baseColor, roughness, metallic, viewPos);

    const uint castShadowsFlag = 1u << 0;
    const uint skipOwnerShadowFlag = 1u << 1;
    uint flags = uint(light.metadata.z);
    int shadowLayer = light.metadata.x;
    int ownerViewportIndex = light.metadata.y;
    bool skipOwnerShadow = ownerViewportIndex == int(viewportIndex) && (flags & skipOwnerShadowFlag) != 0u;
    if (shadowLayer >= 0 && (flags & castShadowsFlag) != 0u && !skipOwnerShadow) {
        vec4 fragPosLightSpace = light.projectionView * vec4(worldPos, 1.0);
        float shadow = SpotlightShadowCalculation(fragPosLightSpace, normal, lightDirection, worldPos, lightPosition, viewPos, shadowMapArray, shadowLayer);
        lighting *= 1.0 - shadow;
    }

    if ((flags & (1u << 2)) != 0u) lighting *= GetFlashlightViewDistanceScale(fragDistance);
    return lighting * modifier;
}

vec3 GetSpotLightContributionSingleSample(SpotLight light, RendererData settings, uint viewportIndex, vec3 viewPos, vec3 normal, vec3 worldPos, vec3 baseColor, float roughness, float metallic, float fragDistance, float oceanHeight, sampler2D iesTexture, sampler2DArray shadowMapArray) {
    float modifier = light.positionModifier.w;
    if (modifier <= 0.05) return vec3(0.0);

    vec3 lightPosition;
    vec3 lightDirection;
    vec3 lightColor;
    GetSpotLightShadingInputs(light, settings, viewportIndex, lightPosition, lightDirection, lightColor);

    vec3 toLight = lightPosition - worldPos;
    float lightDistance = length(toLight);
    if (lightDistance >= settings.flashlightRange) return vec3(0.0);

    vec3 surfaceToLight = toLight / max(lightDistance, 0.000001);
    float nDotL = dot(normal, surfaceToLight);
    if (nDotL <= 0.0) return vec3(0.0);

    float attenuation = GetFlashlightIESAttenuation(worldPos, lightPosition, lightDirection, settings, iesTexture);
    if (worldPos.y < oceanHeight - 0.1) attenuation *= 2.0;
    if (attenuation <= 0.0) return vec3(0.0);

    const uint castShadowsFlag = 1u << 0;
    const uint skipOwnerShadowFlag = 1u << 1;
    uint flags = uint(light.metadata.z);
    int shadowLayer = light.metadata.x;
    int ownerViewportIndex = light.metadata.y;
    bool skipOwnerShadow = ownerViewportIndex == int(viewportIndex) && (flags & skipOwnerShadowFlag) != 0u;
    float visibility = 1.0;
    if (shadowLayer >= 0 && (flags & castShadowsFlag) != 0u && !skipOwnerShadow) {
        vec4 fragPosLightSpace = light.projectionView * vec4(worldPos, 1.0);
        visibility = GetSpotlightVisibilitySingleSample(fragPosLightSpace, lightDistance, shadowMapArray, shadowLayer);
        if (visibility <= 0.0) return vec3(0.0);
    }

    vec3 viewDirection = normalize(viewPos - worldPos);
    vec3 brdf = microfacetBRDFSpecularOnly(surfaceToLight, viewDirection, normal, baseColor, metallic, 1.0, roughness);
    vec3 lighting = brdf * (nDotL * attenuation) * clamp(lightColor, 0.0, 1.0);
    if ((flags & (1u << 2)) != 0u) lighting *= GetFlashlightViewDistanceScale(fragDistance);
    return lighting * visibility * modifier;
}
#endif



vec3 gridSamplingDisk[20] = vec3[](
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float ShadowCalculationOLD(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal, samplerCubeArray shadowCubeMapArray) {
    vec3 lightToFrag = fragPos - lightPos;
    vec3 L = normalize(-lightToFrag);
    float currentDepth = length(lightToFrag);
    float far_plane = lightRadius;
    float shadow = 0.0;

    // Bias
    float cosTheta = clamp(dot(Normal, L), 0.0, 1.0);
    float bias = max(0.05 * (1.0 - cosTheta), 0.005);

    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;

    for (int i = 0; i < samples; ++i) {
        // Sample with offset
        float closestDepth = texture(shadowCubeMapArray, vec4(lightToFrag + gridSamplingDisk[i] * diskRadius, lightIndex)).r;
        closestDepth *= far_plane;

        if (currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }

    shadow /= float(samples);
    return 1.0 - shadow;
}

float SamplePointShadowReceiverPlane(int lightIndex, vec3 lightToFrag, vec3 receiverNormal, float currentDepth, float lightRadius, float bias, vec3 sampleDir, float maxPlaneDepthDelta, samplerCubeArrayShadow shadowCubeMapArray) {
    vec3 sampleRay = normalize(sampleDir);

    float receiverDepth = currentDepth;
    float denominator = dot(receiverNormal, sampleRay);

    if (denominator < -0.03) {
        receiverDepth = dot(receiverNormal, lightToFrag) / denominator;
        receiverDepth = clamp(receiverDepth, currentDepth - maxPlaneDepthDelta, currentDepth + maxPlaneDepthDelta);
    }

    float compareDepth = clamp((receiverDepth - bias) / lightRadius, 0.0, 1.0);
    return texture(shadowCubeMapArray, vec4(sampleDir, float(lightIndex)), compareDepth);
}


float ShadowCalculationSkin(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal, samplerCubeArrayShadow shadowCubeMapArray) {
vec3 lightToFrag = fragPos - lightPos;
    vec3 L = normalize(-lightToFrag);
    float currentDepth = length(lightToFrag);
    float far_plane = lightRadius;

    float cosTheta = clamp(dot(Normal, L), 0.0, 1.0);
    float bias = max(0.05 * (1.0 - cosTheta), 0.005);

    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;
    float compareDepth = clamp((currentDepth - bias) / far_plane, 0.0, 1.0);

    float visibility = 0.0;

    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = lightToFrag + gridSamplingDisk[i] * diskRadius;
        visibility += texture(shadowCubeMapArray, vec4(sampleDir, float(lightIndex)), compareDepth);
    }

    return visibility / float(samples);
}

float ShadowCalculationNEW(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal, samplerCubeArrayShadow shadowCubeMapArray) {

    vec3 lightToFrag = fragPos - lightPos;
   float currentDepth = length(lightToFrag);
   vec3 rayDir = lightToFrag / currentDepth;
   vec3 L = -rayDir;

   vec3 receiverNormal = normalize(Normal);
   if (dot(receiverNormal, L) < 0.0) {
       receiverNormal = -receiverNormal;
   }

   float cosTheta = clamp(dot(receiverNormal, L), 0.0, 1.0);
   float bias = max(0.05 * (1.0 - cosTheta), 0.005);

   float shadowMapSize = float(textureSize(shadowCubeMapArray, 0).x);
   float texelWorldSize = currentDepth * 2.0 / shadowMapSize;

   float softness = 2.25;
   float grazingScale = smoothstep(0.08, 0.45, cosTheta);
   float diskRadius = texelWorldSize * mix(0.75, softness, grazingScale);
   float maxPlaneDepthDelta = max(diskRadius * 8.0, 0.02);

   vec3 basisSeed = abs(rayDir.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
   vec3 right = normalize(cross(basisSeed, rayDir));
   vec3 up = cross(rayDir, right);

   vec2 poissonDisk[8] = vec2[](
       vec2( 0.527,  0.085),
       vec2(-0.406,  0.331),
       vec2( 0.226, -0.543),
       vec2(-0.589, -0.205),
       vec2( 0.703, -0.391),
       vec2(-0.168,  0.743),
       vec2(-0.812,  0.125),
       vec2( 0.311,  0.379)
   );

   float visibility = 0.0;

   visibility += SamplePointShadowReceiverPlane(lightIndex, lightToFrag, receiverNormal, currentDepth, lightRadius, bias, lightToFrag, maxPlaneDepthDelta, shadowCubeMapArray) * 2.0;

   for (int i = 0; i < 8; ++i) {
       vec2 disk = poissonDisk[i] * diskRadius;
       vec3 sampleDir = lightToFrag + right * disk.x + up * disk.y;
       visibility += SamplePointShadowReceiverPlane(lightIndex, lightToFrag, receiverNormal, currentDepth, lightRadius, bias, sampleDir, maxPlaneDepthDelta, shadowCubeMapArray);
   }

   return visibility * 0.1;

  // vec3 lightToFrag = fragPos - lightPos;
  // vec3 L = normalize(-lightToFrag);
  // float currentDepth = length(lightToFrag);
  // float far_plane = lightRadius;
  // float shadow = 0.0;
  //
  // // Bias
  // float cosTheta = clamp(dot(Normal, L), 0.0, 1.0);
  // float bias = max(0.05 * (1.0 - cosTheta), 0.005);
  //
  // int samples = 20;
  // float viewDistance = length(viewPos - fragPos);
  // float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;
  //
  // for (int i = 0; i < samples; ++i) {
  //     vec3 sampleDir = lightToFrag + gridSamplingDisk[i] * diskRadius;
  //     float compareDepth = (currentDepth - bias) / far_plane;
  //
  //     float visibility = texture(shadowCubeMapArray, vec4(sampleDir, float(lightIndex)), compareDepth);
  //
  //     shadow += 1.0 - visibility;
  // }
  //
  // shadow /= float(samples);
  // return 1.0 - shadow;
}


float ShadowCalculationFastOLD(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal, samplerCubeArray shadowCubeMapArray) {
    vec3 lightDir = fragPos - lightPos;
    float currentDepth = length(lightDir);
    float far_plane = lightRadius;
    float shadow = 0.0;
    float bias = max(0.0125 * (1.0 - dot(Normal, normalize(lightDir))), 0.00125);  // Added normalize to lightDir
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;

    // Sample the cubemap array for shadows (single sample)
    float closestDepth = texture(shadowCubeMapArray, vec4(lightDir + gridSamplingDisk[0] * diskRadius, lightIndex)).r;
    closestDepth *= far_plane;  // Undo mapping [0;1]

    // Apply bias and check if the fragment is in shadow
    if (currentDepth - bias > closestDepth) {
        shadow = 1.0;
    }

    // Return the final shadow factor (1 means fully lit, 0 means fully in shadow)
    return 1.0 - shadow;
}


float ShadowCalculationMedium(int lightIndex, vec3 lightPos, float lightRadius, vec3 fragPos, vec3 viewPos, vec3 Normal, samplerCubeArrayShadow shadowCubeMapArray) {
    vec3 lightToFrag = fragPos - lightPos;
    vec3 L = normalize(-lightToFrag);
    float currentDepth = length(lightToFrag);
    float far_plane = lightRadius;
    float shadow = 0.0;

    // Bias
    float cosTheta = clamp(dot(Normal, L), 0.0, 1.0);
    float bias = max(0.05 * (1.0 - cosTheta), 0.005);

    int samples = 8;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;

    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = lightToFrag + gridSamplingDisk[i] * diskRadius;
        float compareDepth = (currentDepth - bias) / far_plane;

        float visibility = texture(shadowCubeMapArray, vec4(sampleDir, float(lightIndex)), compareDepth);

        shadow += 1.0 - visibility;
    }

    shadow /= float(samples);
    return 1.0 - shadow;
}










//vec3 GetDirectionalLighting(vec3 WorldPos, vec3 Normal, vec3 baseColor, float roughness, float metallic, vec3 viewPos, vec3 lightDir, vec3 lightColor, float strength, float fresnelReflect) {
//	vec3 viewDir = normalize(viewPos - WorldPos);
//	float irradiance = max(dot(lightDir, Normal), 0.0) * strength;
//	vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness);
//    return brdf * irradiance * clamp(lightColor, 0, 1);
//}














// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro
// These are duplicates of what is also in the pbr common shader, only the ocean frag shader uses these so maybe organise better bro

float D_GGX(float NoH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float b = (NoH2 * (alpha2 - 1.0) + 1.0);
    return alpha2 / (PI * b * b);
}

float G1_GGX_Schlick(float NdotV, float roughness) {
  //float r = roughness; // original
  float r = 0.5 + 0.5 * roughness; // Disney remapping
  float k = (r * r) / 2.0;
  float denom = NdotV * (1.0 - k) + k;
  return NdotV / denom;
}

float G_Smith(float NoV, float NoL, float roughness) {
  float g1_l = G1_GGX_Schlick(NoL, roughness);
  float g1_v = G1_GGX_Schlick(NoV, roughness);
  return g1_l * g1_v;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 microfacetBRDF(in vec3 L, in vec3 V, in vec3 N, in vec3 baseColor, in float metallicness, in float fresnelReflect, in float roughness, in vec3 WorldPos) {
  // Half vector
  vec3 H = normalize(V + L);

  // Dot products
  float NoV = clamp(dot(N, V), 0.0, 1.0);
  float NoL = clamp(dot(N, L), 0.0, 1.0);
  float NoH = clamp(dot(N, H), 0.0, 1.0);
  float VoH = clamp(dot(V, H), 0.0, 1.0);

  // Base reflectance (F0)
  vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect));
  f0 = mix(f0, baseColor, metallicness);

  // Fresnel term
  vec3 F = fresnelSchlick(VoH, f0);

  // Specular microfacet BRDF
  float D = D_GGX(NoH, roughness);
  float G = G_Smith(NoV, NoL, roughness);
  vec3 specular = (D * G * F) / max(4.0 * NoV * NoL, 0.001);

  // Energy-conserving diffuse
  vec3 notSpecular = (1.0 - F) * (1.0 - metallicness);
  vec3 diffuse = notSpecular * baseColor / PI;

  return diffuse + specular;
}

vec3 microfacetSpecular(in vec3 L, in vec3 V, in vec3 N, in vec3 F0, in float roughness) {
    vec3 H = normalize(L + V);

    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);

    float D = D_GGX(NoH, roughness);
    float G = G_Smith(NoV, NoL, roughness);
    vec3  F = fresnelSchlick(VoH, F0);

    return (D * G * F) / max(4.0 * NoV * NoL, 0.001);
}
