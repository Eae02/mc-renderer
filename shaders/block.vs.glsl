#version 440 core
#extension GL_GOOGLE_include_directive : enable

#define WIND_NOISE_LAYOUT set=0, binding=2

#include "inc/rendersettings.glh"
#include "inc/wind.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(location=0) in vec4 positionAndRoughness_in;
layout(location=1) in vec4 normalAndBendiness_in;
layout(location=2) in vec3 tangent_in;
layout(location=3) in vec4 textureCoord_in;

layout(location=0) out vec4 worldPosAndRoughness_out;
layout(location=1) out vec4 textureCoord_out;
layout(location=2) out mat3 tbnMatrix_out;

void main()
{
	vec3 normal = normalAndBendiness_in.xyz;
	float bendiness = normalAndBendiness_in.w;
	
	worldPosAndRoughness_out = positionAndRoughness_in;
	worldPosAndRoughness_out.xyz += getWindDisplacement(worldPosAndRoughness_out.xyz, normal, renderSettings.time) * normalAndBendiness_in.w;
	
	tbnMatrix_out = mat3(tangent_in, cross(tangent_in, normal), normal);
	textureCoord_out = textureCoord_in;
	
	gl_Position = renderSettings.viewProj * vec4(worldPosAndRoughness_out.xyz, 1.0);
}
