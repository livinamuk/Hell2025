#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) out vec4 FragOut;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec3 BiTangent;
in vec3 WorldPos;

uniform mat4 inverseView;
uniform vec3 viewPos;
uniform int settings;
uniform float viewportWidth;
uniform float viewportHeight;
uniform float time;

uniform int baseColorIndex;
uniform int normalMapIndex;
uniform int rmaIndex;

struct PlayerData {
    int flashlightOn;
    int padding0;
    int padding1;
    int padding2;
};

layout(std430, binding = 21) buffer PlayerDataBuffer {
    PlayerData playerData[];
};

readonly restrict layout(std430, binding = 0) buffer textureSamplerers {
	uvec2 textureSamplers[];
};


/////////////////////////
//                     //
//   Direct Lighting   //


const float PI = 3.14159265359;

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
  vec3 H = normalize(V + L); // half vector
  // all required dot products
  float NoV = clamp(dot(N, V), 0.0, 1.0);
  float NoL = clamp(dot(N, L), 0.0, 1.0);
  float NoH = clamp(dot(N, H), 0.0, 1.0);
  float VoH = clamp(dot(V, H), 0.0, 1.0);
  // F0 for dielectics in range [0.0, 0.16]
  // default FO is (0.16 * 0.5^2) = 0.04
  vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect));
  // f0 = vec3(0.125);
  // in case of metals, baseColor contains F0
  f0 = mix(f0, baseColor, metallicness);
  // specular microfacet (cook-torrance) BRDF
  vec3 F = fresnelSchlick(VoH, f0);
  float D = D_GGX(NoH, roughness);
  float G = G_Smith(NoV, NoL, roughness);
  vec3 spec = (D * G * F) / max(4.0 * NoV * NoL, 0.001);

  // diffuse
  vec3 notSpec = vec3(1.0) - F; // if not specular, use as diffuse
  notSpec *= 1.0 - metallicness; // no diffuse for metals
  vec3 diff = notSpec * baseColor / PI;
  spec *= 1.05;
  vec3 result = diff + spec;

  return result;
}

vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
	
    
    float fresnelReflect = 1.0; // 0.5 is what they used for box, 1.0 for demon

	vec3 viewDir = normalize(viewPos - WorldPos);
	float lightRadiance = strength;
	vec3 lightDir = normalize(lightPos - WorldPos);
	float lightAttenuation = smoothstep(radius, 0, length(lightPos - WorldPos));
	// lightAttenuation = clamp(lightAttenuation, 0.0, 0.9); // THIS IS WRONG, but does stop super bright region around light source and doesn't seem to affect anything else...
	float irradiance = max(dot(lightDir, Normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance;
	vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness, WorldPos);
	return brdf * irradiance * clamp(lightColor, 0, 1);
}

vec3 Tonemap_ACES(const vec3 x) { // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}


vec3 GetSpotLighting(vec3 fragPos, vec3 normal, vec3 viewPos, vec3 spotLightDirection) {
    vec3 position = viewPos;
    vec3 direction = spotLightDirection;
	float cutoff = cos(radians(10.5));
    float outerCutoff = cos(radians(47.5));
    float constant = 0.25;
    float linear = 0.09;
    float quadratic = 0.042;
    vec3 ambient = vec3(0.5);
    vec3 diffuse = vec3(0.99);
    vec3 specular = vec3(0.5);
    constant = 0.25;
    linear = 0.2;
    quadratic = 0.042;
    vec3 ambientLight = ambient;
    vec3 lightDir = normalize(position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuseLight = diffuse * diff;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // Shininess factor
    vec3 specularLight = specular * spec;
    float theta = dot(lightDir, -direction);
    float epsilon = cutoff - outerCutoff;
    float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);
    float distance = length(position - fragPos);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
    vec3 result = (ambientLight + intensity * (diffuseLight + specularLight)) * attenuation;
    return result;
}


void main() {

    vec4 baseColor = texture(sampler2D(textureSamplers[baseColorIndex]), TexCoord);
    vec3 normalMap = texture(sampler2D(textureSamplers[normalMapIndex]), TexCoord).rgb;       
	baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
    mat3 tbn = mat3(normalize(Tangent), normalize(BiTangent), normalize(Normal));
    normalMap = normalMap.rgb * 2.0 - 1.0;
    normalMap = normalize(normalMap);
    vec3 normal = normalize(tbn * (normalMap));
    vec4 rma = texture(sampler2D(textureSamplers[rmaIndex]), TexCoord);    

    float roughness = rma.r;
    float metallic = rma.g;

    // Direct lighting
    vec3 directLighting = vec3(0);    
    vec3 lightPosition = vec3(12.95, 4.46, -1.6f);
    vec3 lightColor = vec3(1, 0.78, 0.52);
    float lightRadius = 5.3;
    float lightStrength = 1;
    directLighting += GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal, WorldPos.xyz, baseColor.rgb, roughness, metallic, viewPos);
    float ambientIntensity = 0.05;
    vec3 ambientColor = baseColor.rgb * lightColor;
    vec3 ambientLighting = ambientColor * ambientIntensity;
    float finalAlpha = 1.0;
    finalAlpha = baseColor.a;    
    vec3 finalColor = directLighting.rgb;// + ambientLighting;

    // Player flashlights
    int playerIndex = 0;    // WARNING! YOU HARDCODED PLAYER INDEX!!!!!!!!!!!!!!!!!!!
    for (int i = 0; i < 2; i++) {	
		vec3 forward = -normalize(vec3(inverseView[2].xyz));
		vec3 camPos = inverseView[3].xyz;				
		vec3 spotLightPos = camPos;
		if (playerIndex != i) {
			spotLightPos += (forward * 0.125);
		}		
		spotLightPos -= vec3(0, 0.0, 0);
		vec3 dir = normalize(spotLightPos - (camPos - forward));
		vec3 spotLightingFactor = GetSpotLighting(WorldPos.xyz, normalize(normal), spotLightPos, dir);	        
		vec3 spotLightColor = vec3(1, 0.8799999713897705, 0.6289999842643738);
        float fresnelReflect = 0.9;
        float spotLightRadius = 20.0;
        float spotLightStregth = 0.5;
		vec3 spotLighting = GetDirectLighting(spotLightPos, spotLightColor, spotLightRadius, spotLightStregth, normal, WorldPos.xyz, baseColor.rgb, roughness, metallic, viewPos);
		spotLighting = max(spotLighting, vec3(0));
		spotLighting *= spotLightingFactor;	
		if (playerData[i].flashlightOn == 1) {
			finalColor += spotLighting;
		}		
	}
	







    
    // Hair transluceny    
	vec3 viewDir = normalize(viewPos - WorldPos);
    vec3 lightDir = normalize(lightPosition - WorldPos.xyz);
    vec3 halfVector = normalize(lightDir + viewDir);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(normal, halfVector), 0.0), 32.0);
    float backlight = max(dot(-normal, lightDir), 0.0);
   // vec3 lightColor = vec3(1, 0.98, 0.94);
    float translucencyFactor = 0.01;
    vec3 translucency = backlight * lightColor * translucencyFactor;
    //finalColor.rgb += translucency * baseColor.rgb; // multiplying with baseColor is a hack but looks 100x better
    
    // Hair frensel
    float frenselFactor = 0.025;
    float fresnel = pow(1.0 - dot(normal, viewDir), 2.0);        
    //finalColor.rgb += vec3(fresnel * frenselFactor) * baseColor.rgb; // multiplying with baseColor is a hack but looks 100x better
 

    // Tone mapping
	finalColor = mix(finalColor, Tonemap_ACES(finalColor), 1.0);
	finalColor = pow(finalColor, vec3(1.0/2.2));
	finalColor = mix(finalColor, Tonemap_ACES(finalColor), 0.235);

    
    finalAlpha = baseColor.a * 2.0;//1.1;
    finalAlpha = clamp(finalAlpha, 0, 1);
  

    finalColor.rgb = finalColor.rgb * finalAlpha;
    FragOut = vec4(finalColor, finalAlpha);
    //FragOut = vec4(baseColor.rgb, finalAlpha);

    
    //FragOut = vec4((WorldPos.rgb * 0.2) - baseColor.rgb, finalAlpha);
    //FragOut = vec4(baseColor.rgb, finalAlpha);
  
}
