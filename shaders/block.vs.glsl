#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(location=0) in vec4 positionAndRoughness_in;
layout(location=1) in vec3 normal_in;
layout(location=2) in vec3 tangent_in;
layout(location=3) in vec4 textureCoord_in;

layout(location=0) out vec4 worldPosAndRoughness_out;
layout(location=1) out vec4 textureCoord_out;
layout(location=2) out mat3 tbnMatrix_out;

void main()
{
	worldPosAndRoughness_out = positionAndRoughness_in;
	tbnMatrix_out = mat3(tangent_in, cross(tangent_in, normal_in), normal_in);
	textureCoord_out = textureCoord_in;
	gl_Position = renderSettings.viewProj * vec4(positionAndRoughness_in.xyz, 1.0);
}
