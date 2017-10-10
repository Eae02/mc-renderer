#version 440 core
#extension GL_GOOGLE_include_directive : enable

#define WIND_NOISE_LAYOUT set=0, binding=3

#include "inc/rendersettings.glh"
#include "inc/wind.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(location=0) in vec3 position_in;
layout(location=1) in vec4 normalAndBendiness_in;
layout(location=2) in vec3 textureCoord_in;

layout(location=0) out vec3 textureCoord_out;
layout(location=1) out vec3 worldPos_out;

void main()
{
	textureCoord_out = textureCoord_in;
	worldPos_out = position_in + getWindDisplacement(position_in, normalAndBendiness_in.xyz, renderSettings.time) * normalAndBendiness_in.w;
}
