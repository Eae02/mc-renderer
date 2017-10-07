#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"
#include "inc/light.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(set=0, binding=1) uniform sampler2DArray blocksTexture;

layout(location=0) in vec3 textureCoord_in;
layout(location=1) in vec3 normal_in;
layout(location=2) in vec3 worldPos_in;

layout(location=0) out vec3 color_out;

void main()
{
	vec4 texColor = texture(blocksTexture, textureCoord_in);
	if (texColor.a < 0.5)
		discard;
	
	vec3 toEye = normalize(renderSettings.cameraPos - worldPos_in);
	
	MaterialData materialData;
	materialData.albedo = texColor.rgb;
	materialData.roughness = 1;
	materialData.metallic = 0;
	materialData.normal = normalize(normal_in);
	
	vec3 F = calcFresnel(materialData, toEye);
	
	color_out = vec3(0);
	
	color_out += calcDirLightReflectance(renderSettings.sun, toEye, F, materialData);
	color_out += calcDirLightReflectance(renderSettings.moon, toEye, F, materialData);
	
	color_out += vec3(0.1) * materialData.albedo;
}
