#version 440 core
#extension GL_GOOGLE_include_directive : enable

#include "inc/rendersettings.glh"

layout(location=0) in vec4 positionAndDepth_in;

layout(location=0) out vec3 position_out;
layout(location=1) out float depth_out;
layout(location=2) out vec3 scatteringColor_out;

layout(set=0, binding=0) uniform RenderSettingsUB
{
	RenderSettings renderSettings;
};

#include "inc/atmosphere.glh"

layout(constant_id=0) const bool underwater = false;

void main()
{
	vec3 worldPos = positionAndDepth_in.xyz;
	
	depth_out = positionAndDepth_in.w;
	position_out = worldPos;
	
	if (!underwater)
	{
		vec3 eyeVector = worldPos - renderSettings.cameraPos;
		float distToCamera = length(eyeVector);
		scatteringColor_out = getScatteringColor(blockDepthToAtmosphereDepth(distToCamera), eyeVector / distToCamera);
	}
}
