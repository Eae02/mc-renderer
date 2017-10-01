#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

layout(location=0) in vec3 position_in;

void main()
{
	gl_Position = renderSettings.viewProj * vec4(position_in, 1.0);
}
