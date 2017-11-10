#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

layout(location=0) in vec3 position_in;

layout(location=0) out vec3 position_out;

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

void main()
{
	position_out = position_in;
	
	gl_Position = renderSettings.viewProj * vec4(position_in.xyz, 1.0);
}
