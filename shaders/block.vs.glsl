#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(location=0) in vec3 position_in;
layout(location=1) in vec3 normal_in;
layout(location=2) in vec3 textureCoord_in;

layout(location=0) out vec3 textureCoord_out;
layout(location=1) out vec3 normal_out;
layout(location=2) out vec3 worldPos_out;

void main()
{
	normal_out = normal_in;
	textureCoord_out = textureCoord_in;
	worldPos_out = position_in;
	gl_Position = renderSettings.viewProj * vec4(position_in, 1.0);
}
