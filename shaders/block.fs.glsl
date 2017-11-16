#version 440 core
#extension GL_GOOGLE_include_directive : enable

#define DLS_SET_INDEX 1

#include "inc/rendersettings.glh"
#include "inc/light.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

#include "inc/dlshadow.glh"

layout(set=0, binding=1) uniform sampler2DArray blocksTexture;

layout(location=0) in vec4 worldPosAndRoughness_in;
layout(location=1) in vec4 textureCoord_in;
layout(location=2) in mat3 tbnMatrix_in;

layout(location=0) out vec4 color_out;

void main()
{
	vec4 texColor = texture(blocksTexture, textureCoord_in.xyz);
	if (texColor.a < 0.5)
		discard;
	
	vec3 worldPos = worldPosAndRoughness_in.xyz;
	vec3 toEye = normalize(renderSettings.cameraPos - worldPos);
	
	MaterialData materialData;
	materialData.albedo = texColor.rgb;
	materialData.roughness = worldPosAndRoughness_in.w;
	materialData.metallic = 0;
	materialData.specularIntensity = 1;
	
	if (textureCoord_in.w < 0)
	{
		materialData.normal = tbnMatrix_in[2];
	}
	else
	{
		vec3 nmNormal = (texture(blocksTexture, textureCoord_in.xyw).rgb * (255.0 / 128.0)) - vec3(1.0);
		materialData.normal = normalize(tbnMatrix_in * nmNormal);
	}
	
	vec3 F = calcFresnel(materialData, toEye);
	
	color_out = vec4(0, 0, 0, 1);
	
	float shadowFactor = getShadowFactor(worldPos);
	
	if (shadowFactor > 0)
	{
		color_out.rgb += calcDirLightReflectance(renderSettings.sun, toEye, F, materialData);
		color_out.rgb += calcDirLightReflectance(renderSettings.moon, toEye, F, materialData);
		color_out.rgb *= shadowFactor;
	}
	
	color_out.rgb += getAmbientReflectance(materialData) * (renderSettings.sun.radiance + renderSettings.moon.radiance);
}
