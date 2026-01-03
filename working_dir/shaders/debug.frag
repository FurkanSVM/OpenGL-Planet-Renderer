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
			// Specular rengi yüzey renginden etkilensin (daha renkli)
			vec3 specularTint = albedo * 0.7 + vec3(0.3); // %70 yüzey rengi, %30 beyaz katkı
			vec3 specularColor = specularTint * 0.4 * spec * shadow;
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
			vec3 N = normalize(fNormal);
			
			// Texture sampling
			vec3 albedo = texture(uAlbedoMap, fUV).rgb;
			float specMap = texture(uSpecularMap, fUV).r; // 1 = water (high spec), 0 = ground (low spec)
			vec3 nightLights = texture(uNightMap, fUV).rgb; // RGB for colored city lights
			
			// Specular power interpolation: water has high specularity, ground has low
			float waterSpec = 60.0;
			float groundSpec = 25.0;
			float specPower = mix(groundSpec, waterSpec, specMap);
			
			// Diffuse term
			float NdotL = dot(N, lightDir);
			float diff = max(NdotL, 0.0);
			
			// Specular term (Blinn-Phong)
			vec3 halfDir = normalize(lightDir + viewDir);
			float spec = pow(max(dot(N, halfDir), 0.0), specPower) * specMap; // Water gets more specular
			
			// Apply shadow to diffuse and specular
			float shadowedDiff = diff * shadow;
			float shadowedSpec = spec * shadow;
			
			// Night lights visibility: smooth transition based on how dark the surface is
			// NdotL ranges from -1 (full night) to 1 (full day)
			// We want night lights visible when NdotL < some threshold
			float nightTerminator = 0.1; // Where day/night transition happens
			float nightSoftness = 0.3;   // How soft the transition is
			float nightFactor = 1.0 - smoothstep(-nightSoftness, nightTerminator, NdotL);
			
			// In shadow (eclipse), also show night lights
			if (shadow < 0.5 && diff > 0.1) {
				nightFactor = max(nightFactor, 0.8); // Show night lights in eclipse shadow
			}
			
			// Ambient term (very subtle, so night side isn't pure black)
			float ambient = 0.02;
			
			// Day color: ambient + diffuse + specular
			// Specular rengi yüzey renginden etkilensin (su için biraz daha beyaz, toprak için daha renkli)
			vec3 specularTint = albedo * (0.8 - specMap * 0.3) + vec3(0.2 + specMap * 0.3); // Toprak=%80 renkli, Su=%50 renkli
			vec3 dayColor = albedo * ambient + albedo * shadowedDiff + specularTint * shadowedSpec;
			
			// Night color: city lights
			vec3 nightColor = nightLights * 1.2; // Boost night lights slightly
			
			// Blend day and night
			vec3 color = mix(dayColor, dayColor + nightColor, nightFactor);
			
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