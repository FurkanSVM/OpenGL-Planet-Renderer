#version 430
/*
	File Name	: color.vert
	Author		: Bora Yalciner
	Description	:

		Basic fragment shader that just outputs
		color to the FBO
*/

// Definitions
// These locations must match between vertex/fragment shaders
#define IN_UV		layout(location = 0)
#define IN_NORMAL	layout(location = 1)
#define IN_COLOR	layout(location = 2)

// This output must match to the COLOR_ATTACHMENTi (where 'i' is this location)
#define OUT_FBO		layout(location = 0)

// This must match GL_TEXTUREi (where 'i' is this binding)
#define T_ALBEDO	layout(binding = 0)

// This must match the first parameter of glUniform...() calls
#define U_MODE		layout(location = 0)

// Input
in IN_UV	 vec2 fUV;
in IN_NORMAL vec3 fNormal;
in vec3 fWorldPos; // World space position for shadow mapping

// Output
// This parameter goes to the framebuffer
out OUT_FBO vec4 fboColor;


// Uniforms
U_MODE uniform uint uMode;
uniform sampler2D uAlbedoMap;
uniform sampler2D uSpecularMap;
uniform sampler2D uNightMap;
uniform sampler2D uCloudMap;
uniform float uCloudPhase;
uniform vec3 uLightDir; // world space
uniform vec3 uViewDir;  // world space

// Shadow mapping uniforms
uniform sampler2D uShadowMap;
uniform mat4 uLightVP; // Light's view-projection matrix
uniform float uShadowBias;

// Textures
uniform T_ALBEDO sampler2D tAlbedo;

// Shadow calculation function
float calculateShadow() {
    // Transform world position to light's clip space
    vec4 lightClipPos = uLightVP * vec4(fWorldPos, 1.0);
    
    // Perspective divide (for orthographic, w=1)
    vec3 lightNDC = lightClipPos.xyz / lightClipPos.w;
    
    // Convert from NDC [-1,1] to texture coords [0,1]
    vec2 shadowUV = lightNDC.xy * 0.5 + 0.5;
    float currentDepth = lightNDC.z * 0.5 + 0.5;
    
    // Check if outside shadow map
    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0)
        return 1.0; // Not in shadow
    
    // Sample shadow map
    float shadowDepth = texture(uShadowMap, shadowUV).r;
    
    // Compare depths with bias
    float shadow = (currentDepth - uShadowBias > shadowDepth) ? 0.0 : 1.0;
    
    return shadow;
}

void main(void)
{
	uint mode = uMode;
	switch(mode)
	{
		// Pure Red
		case 0: fboColor = vec4(1, 0, 0, 1); break;
		// Vertex Normals. Normal axes by definition is between [-1, 1])
		// Color is in between [0, 1]) so we adjust here for that
		case 1: fboColor = vec4((fNormal + 1) * 0.5, 1); break;
		// UV
		case 2: fboColor = vec4(fUV, 0, 1); break;
		// Texture Mapping without shading.
		case 3: fboColor = texture2D(tAlbedo, fUV); break;
		// Texture with diffuse + specular + shadow (for Moon and other planets)
		case 4: {
			float shadow = calculateShadow();
			vec3 albedo = texture(tAlbedo, fUV).rgb;
			vec3 N = normalize(fNormal);
			vec3 lightDir = normalize(uLightDir);
			vec3 viewDir = normalize(uViewDir);
			
			// Diffuse
			float diff = max(dot(N, lightDir), 0.0);
			
			// Specular (Blinn-Phong)
			vec3 halfDir = normalize(lightDir + viewDir);
			float spec = pow(max(dot(N, halfDir), 0.0), 32.0);
			
			// Ambient + Diffuse + Specular with shadow
			float ambient = 0.05;
			vec3 diffuseColor = albedo * diff * shadow;
			vec3 specularColor = vec3(0.3) * spec * shadow;
			vec3 ambientColor = albedo * ambient;
			
			vec3 color = ambientColor + diffuseColor + specularColor;
			fboColor = vec4(color, 1.0);
			break;
		}
		// Earth Shading Effects
		case 10: {
			float shadow = calculateShadow();
			vec3 lightDir = normalize(uLightDir);
			vec3 viewDir = normalize(uViewDir);
			vec3 albedo = texture(uAlbedoMap, fUV).rgb;
			float specMap = texture(uSpecularMap, fUV).r;
			float night = texture(uNightMap, fUV).r;
			float waterSpec = 64.0;
			float groundSpec = 8.0;
			float specPower = mix(groundSpec, waterSpec, specMap);
			vec3 N = normalize(fNormal);
			float diff = max(dot(N, lightDir), 0.0);
			vec3 reflectDir = reflect(-lightDir, fNormal);
			float spec = pow(max(dot(viewDir, reflectDir), 0.0), specPower);
			
			// Apply shadow to diffuse and specular
			float shadowedDiff = diff * shadow;
			float shadowedSpec = spec * shadow;
			
			// Night map katkısını sadece tam geceye yakın bölgede uygula
			float nightStart = 0.08;
			float nightEnd = 0.45;
			// Shadow'da iken de night map görünsün
			float effectiveDiff = shadow < 0.5 ? 0.0 : shadowedDiff;
			float nightFactor = 1.0 - smoothstep(nightStart, nightEnd, effectiveDiff);
			nightFactor = clamp(nightFactor, 0.0, 1.0);
			vec3 dayColor = albedo * shadowedDiff + shadowedSpec * vec3(1.0);
			vec3 color = mix(dayColor, night * vec3(1.0), nightFactor);
			// Tamamen gündüzde ve gölge dışında night katkısı olmasın
			if (effectiveDiff > nightEnd + 0.05) color = dayColor;
			fboColor = vec4(color, 1.0);
			break;
		}
		// Sun rendering (saf sarı ve parlak)
		case 12: {
			float intensity = pow(max(dot(normalize(fNormal), vec3(0,0,1)), 0.0), 32.0);
			vec3 sunColor = vec3(1.0, 0.95, 0.6) * (0.7 + 0.6 * intensity);
			fboColor = vec4(sunColor, 1.0);
			break;
		}
		// Cloud Map Rendering
		case 11: {
			// Bulut texture'ını doğrudan UV ile örnekle (dönüş model matrisinde yapılıyor)
			vec4 cloud = texture(tAlbedo, fUV);
			// Alpha blending için cloud alpha değerini kullan
			fboColor = vec4(cloud.rgb, cloud.a * 0.5);
			break;
		}
		// If mode is wrong, put pure white.
		default: fboColor = vec4(1); break;
	}
}